/***
    This file is part of snapcast
    Copyright (C) 2014-2020  Johannes Pohl

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
***/

#ifndef STREAM_H
#define STREAM_H

#include "common/queue.h"
#include "common/sample_format.hpp"
#include "double_buffer.hpp"
#include "message/message.hpp"
#include "message/pcm_chunk.hpp"
#include <deque>
#include <memory>
#ifdef HAS_SOXR
#include <soxr.h>
#endif

/// Time synchronized audio stream
/**
 * Queue with PCM data.
 * Returns "online" server-time-synchronized PCM data
 */
class Stream
{
public:
    Stream(const SampleFormat& in_format, const SampleFormat& out_format);
    virtual ~Stream();

    /// Adds PCM data to the queue
    void addChunk(std::unique_ptr<msg::PcmChunk> chunk);
    void clearChunks();

    /// Get PCM data, which will be played out in "outputBufferDacTime" time
    /// frame = (num_channels) * (1 sample in bytes) = (2 channels) * (2 bytes (16 bits) per sample) = 4 bytes (32 bits)
    bool getPlayerChunk(void* outputBuffer, const chronos::usec& outputBufferDacTime, uint32_t frames);

    /// "Server buffer": playout latency, e.g. 1000ms
    void setBufferLen(size_t bufferLenMs);

    const SampleFormat& getFormat() const
    {
        return format_;
    }

    bool waitForChunk(const std::chrono::milliseconds& timeout) const;

private:
    /// Request an audio chunk from the front of the stream.
    /// @param outputBuffer will be filled with the chunk
    /// @param frames the number of requested frames
    /// @return the timepoint when this chunk should be audible
    chronos::time_point_clk getNextPlayerChunk(void* outputBuffer, uint32_t frames);

    /// Request an audio chunk from the front of the stream with a tempo adaption
    /// @param outputBuffer will be filled with the chunk
    /// @param frames the number of requested frames
    /// @param framesCorrection number of frames that should be added or removed.
    ///        The function will allways return "frames" frames, but will fit "frames + framesCorrection" frames into "frames"
    ///        so if frames is 100 and framesCorrection is 2, 102 frames will be read from the stream and 2 frames will be removed.
    ///        This makes us "fast-forward" by 2 frames, or if framesCorrection is -3, 97 frames will be read from the stream and
    ///        filled with 3 frames (simply by dublication), this makes us effectively slower
    /// @return the timepoint when this chunk should be audible
    chronos::time_point_clk getNextPlayerChunk(void* outputBuffer, uint32_t frames, int32_t framesCorrection);

    /// Request a silent audio chunk
    /// @param outputBuffer will be filled with the chunk
    /// @param frames the number of requested frames
    void getSilentPlayerChunk(void* outputBuffer, uint32_t frames) const;

    void updateBuffers(chronos::usec::rep age);
    void resetBuffers();
    void setRealSampleRate(double sampleRate);

    SampleFormat format_;
    SampleFormat in_format_;

    Queue<std::shared_ptr<msg::PcmChunk>> chunks_;
    DoubleBuffer<chronos::usec::rep> miniBuffer_;
    DoubleBuffer<chronos::usec::rep> shortBuffer_;
    DoubleBuffer<chronos::usec::rep> buffer_;
    std::shared_ptr<msg::PcmChunk> chunk_;

    chronos::usec::rep median_;
    chronos::usec::rep shortMedian_;
    time_t lastUpdate_;
    uint32_t playedFrames_;
    int32_t correctAfterXFrames_;
    chronos::msec bufferMs_;

#ifdef HAS_SOXR
    soxr_t soxr_;
#endif
    std::vector<char> resample_buffer_;
    std::vector<char> read_buffer_;
    int frame_delta_;
    // int64_t next_us_;

    bool hard_sync_;
};



#endif
