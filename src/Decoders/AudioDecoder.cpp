#include "AudioDecoder.hpp"
#include <thread>
#include <chrono>
#include <algorithm>

AudioDecoder::~AudioDecoder() {
    close();
}

void AudioDecoder::setOutput(std::unique_ptr<IAudioOutput> output) {
    output_ = std::move(output);
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
    
    //采样率和声道数基本不变,但是采样格式需要改变

    //设置输出采样率
    out_rate_ = codec_ctx_->sample_rate > 0 ? codec_ctx_->sample_rate : 48000;
    //设置输出声道数
    out_channels_ = codec_ctx_->ch_layout.nb_channels > 0 ? codec_ctx_->ch_layout.nb_channels : 2;
    //设置样本格式
    out_sample_fmt_ = AV_SAMPLE_FMT_S16;

    //根据之前获得的声道数,初始化声道out_ch_layout_结构体
    av_channel_layout_default(&out_ch_layout_, out_channels_);

    //填写输入的采样率
    int in_rate = codec_ctx_->sample_rate;
    if (in_rate <= 0) {
        in_rate = st->codecpar->sample_rate;
    }
    if (in_rate <= 0) {
        in_rate = out_rate_;
    }

    //释放 AVChannelLayout 结构体中动态分配的资源,并将其重置为未初始化状态。
    av_channel_layout_uninit(&in_ch_layout_);
    if (codec_ctx_->ch_layout.nb_channels > 0) {
        av_channel_layout_copy(&in_ch_layout_, &codec_ctx_->ch_layout);
    } else if (st->codecpar->ch_layout.nb_channels > 0) {
        av_channel_layout_copy(&in_ch_layout_, &st->codecpar->ch_layout);
    } else {
        av_channel_layout_default(&in_ch_layout_, out_channels_);
    }

    // 如果输入布局顺序未指定，使用默认布局
    if (in_ch_layout_.order == AV_CHANNEL_ORDER_UNSPEC) {
        av_channel_layout_default(&in_ch_layout_, in_ch_layout_.nb_channels);
    }

    // 检查输入采样格式
    enum AVSampleFormat in_sample_fmt = codec_ctx_->sample_fmt;
    if (in_sample_fmt == AV_SAMPLE_FMT_NONE) {
        in_sample_fmt = static_cast<enum AVSampleFormat>(st->codecpar->format);
    }

    //分配重采样上下文
    ret = swr_alloc_set_opts2(&swr_ctx_, &out_ch_layout_, out_sample_fmt_, out_rate_, &in_ch_layout_, in_sample_fmt, in_rate, 0, nullptr);
    if (ret < 0) {
        last_error_ = ff::errStr(ret);
        return false;
    }

    //初始化重采样上下文
    ret = swr_init(swr_ctx_);
    if (ret < 0) {
        last_error_ = ff::errStr(ret);
        return false;
    }

    //是否打开输出设备
    if (!output_) {
        last_error_ = "audio output not set";
        return false;
    }

    if (!output_->init(out_rate_, out_channels_)) {
        last_error_ = output_->lastError();
        return false;
    }

    //每秒输出的字节数
    bytes_per_second_ = out_rate_ * out_channels_ * av_get_bytes_per_sample(out_sample_fmt_);
    if (mediator_) {
        mediator_->Notify(this, "AudioReady");
    }
    return true;
}

void AudioDecoder::setPaused(bool paused) {
    if (output_) {
        if (paused) {
            output_->pause();
        } else {
            output_->resume();
        }
    }
}

void AudioDecoder::setSpeed(double speed) {
    if (output_) {
        output_->setSpeed(speed);
    }
}

void AudioDecoder::close() {
    if (output_) {
        output_->close();
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
    constexpr double kMaxQueuedSeconds = 0.25;
    constexpr double kMaxClockCompSeconds = 0.25;
    constexpr double kStartupClockCompSeconds = 0.05;

    while (!abort_flag.load()) {
        ff::PacketPtr pkt;
        if (!packets.pop(pkt)) {
            break;
        }
        if (!pkt) {
            break;
        }

        //将pkt数据送到解码器中
        int ret = avcodec_send_packet(codec_ctx_, pkt.get());
        if (ret < 0) {
            continue;
        }

        while (!abort_flag.load()) {
            //将解码后的数据放入frame
            ret = avcodec_receive_frame(codec_ctx_, frame.get());
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            if (ret < 0) {
                break;
            }

            //获取frame中的时间戳
            //AV_NOPTS_VALUE表示没有有效时间戳
            const double pts_seconds = (frame->best_effort_timestamp != AV_NOPTS_VALUE)
                                                //best_effort_timestamp 是尽力而为展示时间戳
                                           ? static_cast<double>(frame->best_effort_timestamp) * av_q2d(time_base_)
                                           : static_cast<double>(fallback_samples) / static_cast<double>(out_rate_);
            //下一次 swr_convert 可能吐出的最大输出样本数”。
            //一般重采样的样本数不等于输入样本数
            //比如:AAC：常见每帧 1024 样本（每声道
            const int max_out_samples = swr_get_out_samples(swr_ctx_, frame->nb_samples);
            //ffmpeg中音频缓冲区是以一字节为基本单位的
            uint8_t* out_data = nullptr;
            //每个平面的字节数
            int out_linesize = 0;
            const int alloc_ret = av_samples_alloc(&out_data, &out_linesize, out_channels_, max_out_samples, out_sample_fmt_, 0);
            if (alloc_ret < 0) {
                av_frame_unref(frame.get());
                continue;
            }

            //负责把“输入音频帧的数据”按你配置的目标采样率、采样格式、声道布局重采样并重排格式，输出到你提供的缓冲区 out_data 中。
            //返回值 out_samples 表示实际输出的样本数。
            const int out_samples = swr_convert(swr_ctx_, &out_data, max_out_samples, (const uint8_t**)frame->extended_data, frame->nb_samples);
            if (out_samples > 0) {
                const int out_size = out_samples * out_channels_ * av_get_bytes_per_sample(out_sample_fmt_);
                
                // Wait if buffer is too full to avoid overfilling
                if (output_) {
                    while (!abort_flag.load() && bytes_per_second_ > 0) {
                        //在音频输出层读取当前设备队列中尚未播放的内容,返回SDL中待播放的字节数
                        const int queued = output_->getQueuedSize();
                        const double queued_seconds = static_cast<double>(queued) / static_cast<double>(bytes_per_second_);
                        if (queued_seconds <= kMaxQueuedSeconds) {
                            break;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    }
                    output_->write(out_data, out_size);

                    if (clock_) {
                        const int buffered_bytes = output_->getQueuedSize();
                        // 1. 算出这些字节在 1.0 倍速下本来要播多久
                        const double base_buffered_seconds =
                            bytes_per_second_ > 0 ? static_cast<double>(buffered_bytes) / static_cast<double>(bytes_per_second_) : 0.0;
                        // 2. 考虑当前的播放倍速，计算实际还需要多久才能播完
                        const double actual_buffered_seconds = clock_->getSpeed() > 0 ? base_buffered_seconds / clock_->getSpeed() : base_buffered_seconds;
                        // 3. 将 PTS 减去“实际缓冲时间”，得到目前喇叭里正在发声的真实时间点
                        const double max_comp = clock_->audioClockSynced() ? kMaxClockCompSeconds : kStartupClockCompSeconds;
                        const double compensated = std::min(actual_buffered_seconds, max_comp);
                        const double target_clock = std::max(0.0, pts_seconds - compensated);
                        if (!clock_->audioClockSynced()) {
                            clock_->setAudioClock(target_clock);
                        } else {
                            const double current_clock = clock_->getAudioClock();
                            clock_->setAudioClock(std::max(current_clock, target_clock));
                        }
                    }
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
