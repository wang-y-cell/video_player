#include "AudioDecoder.hpp"

AudioDecoder::~AudioDecoder() {
    close();
}

bool AudioDecoder::init(AVFormatContext* fmt, int stream_index, AVRational time_base, Clock* clock) {
    close();
    clock_ = clock;
    time_base_ = time_base;

    if (!fmt || stream_index < 0 || stream_index >= static_cast<int>(fmt->nb_streams)) {
        last_error_ = "invalid audio stream";
        return false;
    }

    AVStream* st = fmt->streams[stream_index];
    const AVCodec* codec = avcodec_find_decoder(st->codecpar->codec_id);
    if (!codec) {
        last_error_ = "audio decoder not found";
        return false;
    }

    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) {
        last_error_ = "avcodec_alloc_context3 failed";
        return false;
    }

    int ret = avcodec_parameters_to_context(codec_ctx_, st->codecpar);
    if (ret < 0) {
        last_error_ = ff::errStr(ret);
        return false;
    }

    ret = avcodec_open2(codec_ctx_, codec, nullptr);
    if (ret < 0) {
        last_error_ = ff::errStr(ret);
        return false;
    }

    out_rate_ = codec_ctx_->sample_rate > 0 ? codec_ctx_->sample_rate : 48000;
    out_channels_ = codec_ctx_->ch_layout.nb_channels > 0 ? codec_ctx_->ch_layout.nb_channels : 2;
    out_sample_fmt_ = AV_SAMPLE_FMT_S16;

    av_channel_layout_default(&out_ch_layout_, out_channels_);

    int in_rate = codec_ctx_->sample_rate;
    if (in_rate <= 0) {
        in_rate = st->codecpar->sample_rate;
    }
    if (in_rate <= 0) {
        in_rate = out_rate_;
    }

    av_channel_layout_uninit(&in_ch_layout_);
    if (codec_ctx_->ch_layout.nb_channels > 0) {
        av_channel_layout_copy(&in_ch_layout_, &codec_ctx_->ch_layout);
    } else if (st->codecpar->ch_layout.nb_channels > 0) {
        av_channel_layout_copy(&in_ch_layout_, &st->codecpar->ch_layout);
    } else {
        av_channel_layout_default(&in_ch_layout_, out_channels_);
    }

    ret = swr_alloc_set_opts2(&swr_ctx_, &out_ch_layout_, out_sample_fmt_, out_rate_, &in_ch_layout_, codec_ctx_->sample_fmt, in_rate, 0, nullptr);
    if (ret < 0) {
        last_error_ = ff::errStr(ret);
        return false;
    }

    ret = swr_init(swr_ctx_);
    if (ret < 0) {
        last_error_ = ff::errStr(ret);
        return false;
    }

    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_S16;
    spec.channels = out_channels_;
    spec.freq = out_rate_;
    sdl_stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (!sdl_stream_) {
        last_error_ = SDL_GetError();
        return false;
    }
    if (!SDL_ResumeAudioStreamDevice(sdl_stream_)) {
        last_error_ = SDL_GetError();
        return false;
    }

    bytes_per_second_ = out_rate_ * out_channels_ * av_get_bytes_per_sample(out_sample_fmt_);
    if (mediator_) {
        mediator_->Notify(this, "AudioReady");
    }
    return true;
}

void AudioDecoder::setPaused(bool paused) {
    if (!sdl_stream_) {
        return;
    }
    if (paused) {
        SDL_PauseAudioStreamDevice(sdl_stream_);
    } else {
        SDL_ResumeAudioStreamDevice(sdl_stream_);
    }
}

void AudioDecoder::close() {
    if (sdl_stream_) {
        SDL_DestroyAudioStream(sdl_stream_);
        sdl_stream_ = nullptr;
    }
    if (swr_ctx_) {
        swr_free(&swr_ctx_);
    }
    if (codec_ctx_) {
        avcodec_free_context(&codec_ctx_);
    }
    av_channel_layout_uninit(&out_ch_layout_);
    av_channel_layout_uninit(&in_ch_layout_);
    clock_ = nullptr;
    time_base_ = AVRational{0, 1};
    out_rate_ = 0;
    out_channels_ = 0;
    bytes_per_second_ = 0;
    last_error_.clear();
}

void AudioDecoder::decodeLoop(std::atomic<bool>& abort_flag, SafeQueue<ff::PacketPtr>& packets) {
    ff::FramePtr frame = ff::makeFrame();
    int64_t fallback_samples = 0;

    while (!abort_flag.load()) {
        ff::PacketPtr pkt;
        if (!packets.pop(pkt)) {
            break;
        }
        if (!pkt) {
            break;
        }

        int ret = avcodec_send_packet(codec_ctx_, pkt.get());
        if (ret < 0) {
            continue;
        }

        while (!abort_flag.load()) {
            ret = avcodec_receive_frame(codec_ctx_, frame.get());
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            if (ret < 0) {
                break;
            }

            const double pts_seconds = (frame->best_effort_timestamp != AV_NOPTS_VALUE)
                                           ? static_cast<double>(frame->best_effort_timestamp) * av_q2d(time_base_)
                                           : static_cast<double>(fallback_samples) / static_cast<double>(out_rate_);

            const int max_out_samples = swr_get_out_samples(swr_ctx_, frame->nb_samples);
            uint8_t* out_data = nullptr;
            int out_linesize = 0;
            const int alloc_ret = av_samples_alloc(&out_data, &out_linesize, out_channels_, max_out_samples, out_sample_fmt_, 0);
            if (alloc_ret < 0) {
                av_frame_unref(frame.get());
                continue;
            }

            const int out_samples = swr_convert(swr_ctx_, &out_data, max_out_samples, (const uint8_t**)frame->extended_data, frame->nb_samples);
            if (out_samples > 0) {
                const int out_size = out_samples * out_channels_ * av_get_bytes_per_sample(out_sample_fmt_);
                while (!abort_flag.load() && bytes_per_second_ > 0 &&
                       SDL_GetAudioStreamAvailable(sdl_stream_) > (bytes_per_second_ * 2)) {
                    SDL_Delay(10);
                }
                SDL_PutAudioStreamData(sdl_stream_, out_data, out_size);

                if (clock_) {
                    const int buffered_bytes = SDL_GetAudioStreamAvailable(sdl_stream_);
                    const double buffered_seconds =
                        bytes_per_second_ > 0 ? static_cast<double>(buffered_bytes) / static_cast<double>(bytes_per_second_) : 0.0;
                    clock_->setAudioClock(std::max(0.0, pts_seconds - buffered_seconds));
                }
                fallback_samples += out_samples;
            }

            av_freep(&out_data);
            av_frame_unref(frame.get());
        }
    }
}

const std::string& AudioDecoder::lastError() const {
    return last_error_;
}
