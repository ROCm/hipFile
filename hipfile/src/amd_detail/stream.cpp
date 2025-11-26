/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include "context.h"
#include "rocfile.h"
#include "stream.h"
#include "sys.h"

#include <hip/hip_runtime_api.h>
#include <memory>
#include <syslog.h>
#include <stdexcept>
#include <utility>

namespace rocFile {

Stream::Stream(const hipStream_t _hip_stream, uint32_t flags)
    : hip_stream{_hip_stream}, fixed_buf_offset{(flags & ROCFILE_STREAM_FIXED_BUF_OFFSET) != 0},
      fixed_file_offset{(flags & ROCFILE_STREAM_FIXED_FILE_OFFSET) != 0},
      fixed_io_size{(flags & ROCFILE_STREAM_FIXED_FILE_SIZE) != 0},
      page_aligned{(flags & ROCFILE_STREAM_PAGE_ALIGNED_INPUTS) != 0}

{
    if ((flags & ROCFILE_STREAM_FLAGS_MASK) != flags) {
        throw std::invalid_argument("Invalid flags for stream");
    }
}

hipStream_t
Stream::getHipStream() const
{
    return hip_stream;
}
bool
Stream::fixedBufferOffset() const
{
    return fixed_buf_offset;
}
bool
Stream::fixedFileOffset() const
{
    return fixed_file_offset;
}
bool
Stream::fixedIOSize() const
{
    return fixed_io_size;
}
bool
Stream::pageAligned() const
{
    return page_aligned;
}

void
StreamMap::registerStream(hipStream_t hip_stream, uint32_t flags)
{
    if (StreamMap::from_ptr.end() != StreamMap::from_ptr.find(hip_stream)) {
        throw std::invalid_argument("Stream is already registered");
    }

    auto stream                     = std::shared_ptr<Stream>(new Stream(hip_stream, flags));
    StreamMap::from_ptr[hip_stream] = stream;
}

void
StreamMap::deregisterStream(hipStream_t hip_stream)
{
    auto itr = StreamMap::from_ptr.find(hip_stream);
    if (StreamMap::from_ptr.end() == itr) {
        throw std::invalid_argument("Stream is not registered");
    }

    StreamMap::from_ptr.erase(hip_stream);
}

std::shared_ptr<IStream>
StreamMap::getStream(hipStream_t hip_stream)
{
    auto itr = StreamMap::from_ptr.find(hip_stream);
    if (StreamMap::from_ptr.end() == itr) {
        return std::shared_ptr<Stream>(new Stream(hip_stream, 0));
    }

    return itr->second;
}

StreamMap::~StreamMap()
{
    for (const auto &[hip_stream, stream] : from_ptr) {
        (void)hip_stream;
        if (stream.use_count() > 1) {
            Context<Sys>::get()->syslog(LOG_CRIT,
                                        "Stream state is being destructed while operations are outstanding.");
            return;
        }
    }
}

}
