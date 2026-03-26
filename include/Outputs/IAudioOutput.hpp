#pragma once

#include <string>
#include <cstdint>

class IAudioOutput {
public:
    virtual ~IAudioOutput() = default;

    /**
     * 初始化音频的输出
     * @param rate 采样率 (e.g., 44100, 48000)
     * @param channels 声道数 (e.g., 1, 2)
     * @return true if successful, false otherwise.
     */
    virtual bool init(int rate, int channels) = 0;

    /**
     * Write audio data to the output.
     * @param data Pointer to the audio data.
     * @param size Size of the data in bytes.
     */
    virtual void write(const uint8_t* data, int size) = 0;

    /**
     * Get the number of bytes currently queued in the output buffer.
     * Used for clock synchronization.
     * @return Number of bytes queued.
     */
    virtual int getQueuedSize() const = 0;

    /**
     * Pause the audio output.
     */
    virtual void pause() = 0;

    /**
     * Resume the audio output.
     */
    virtual void resume() = 0;

    /**
     * Set the playback speed.
     * @param speed Speed factor (e.g., 1.0 for normal speed).
     */
    virtual void setSpeed(double speed) = 0;
    virtual void flush() = 0;

    /**
     * Close the audio output and release resources.
     */
    virtual void close() = 0;

    /**
     * Get the last error message.
     * @return Error message string.
     */
    virtual const std::string& lastError() const = 0;
};
