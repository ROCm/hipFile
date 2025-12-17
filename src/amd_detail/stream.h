/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <cstdint>
#include <hip/hip_runtime_api.h>
#include <memory>
#include <unordered_map>

template <typename T> class PassKey;

namespace hipFile {

class IStream {
public:
    virtual ~IStream() = default;

    virtual hipStream_t getHipStream() const      = 0;
    virtual bool        fixedBufferOffset() const = 0;
    virtual bool        fixedFileOffset() const   = 0;
    virtual bool        fixedIOSize() const       = 0;
    virtual bool        pageAligned() const       = 0;
};

class StreamMap;

class Stream : public IStream {
public:
    virtual ~Stream() override = default;

    virtual hipStream_t getHipStream() const override;
    virtual bool        fixedBufferOffset() const override;
    virtual bool        fixedFileOffset() const override;
    virtual bool        fixedIOSize() const override;
    virtual bool        pageAligned() const override;

    Stream(const hipStream_t hip_stream, uint32_t flags, const PassKey<StreamMap> &k);

private:
    Stream(const Stream &)             = delete;
    Stream &operator=(const Stream &)  = delete;
    Stream(const Stream &&)            = delete;
    Stream &operator=(const Stream &&) = delete;

    hipStream_t hip_stream;
    bool        fixed_buf_offset;
    bool        fixed_file_offset;
    bool        fixed_io_size;
    bool        page_aligned;
};

class StreamMap {
public:
    ~StreamMap();

    /// @brief Registers a stream to use for async operations
    /// @param hip_stream A HIP stream
    /// @param flags Stream flags
    void registerStream(const hipStream_t hip_stream, uint32_t flags);

    /// @brief Deregisters a stream from use
    /// @param hip_stream A HIP stream
    void deregisterStream(const hipStream_t hip_stream);

    /// @brief Lookup a stream using the hipStream
    /// @param hip_stream A HIP stream
    std::shared_ptr<IStream> getStream(hipStream_t hip_stream);

private:
    std::unordered_map<hipStream_t, std::shared_ptr<IStream>> from_ptr;
};

}
