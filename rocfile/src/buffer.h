/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "hip.h"

#include <cstddef>
#include <memory>
#include <shared_mutex>
#include <stdexcept>
#include <unordered_map>

namespace rocFile::buffer {

/// @brief Buffer is currently registered
struct AlreadyRegistered : public std::runtime_error {
    AlreadyRegistered() : std::runtime_error("Already registered")
    {
    }
};

/// @brief Buffer is not currently registered
struct NotRegistered : public std::runtime_error {
    NotRegistered() : std::runtime_error("Not registered")
    {
    }
};

/// @brief Memory type is not supported
struct InvalidMemoryType : public std::runtime_error {
    InvalidMemoryType() : std::runtime_error("Invalid memory type")
    {
    }
};

/// @brief Range of memory pointer is invalid
struct InvalidPointerRange : public std::runtime_error {
    InvalidPointerRange() : std::runtime_error("Invalid pointer range")
    {
    }
};

/// @brief Buffer has outstanding operations
struct OperationsOutstanding : public std::runtime_error {
    OperationsOutstanding() : std::runtime_error("Operations outstanding")
    {
    }
};

class IBuffer {
public:
    virtual ~IBuffer() = default;

    virtual void         *getBuffer() const = 0;
    virtual size_t        getLength() const = 0;
    virtual int           getFlags() const  = 0;
    virtual hipMemoryType getType() const   = 0;
};

class Buffer : public IBuffer {

public:
    virtual ~Buffer() override = default;

    // Don't allow copying
    Buffer(const Buffer &)            = delete;
    Buffer &operator=(const Buffer &) = delete;

    // Don't allow moving
    Buffer(Buffer &&)            = delete;
    Buffer &operator=(Buffer &&) = delete;

    virtual void         *getBuffer() const override;
    virtual size_t        getLength() const override;
    virtual int           getFlags() const override;
    virtual hipMemoryType getType() const override;

private:
    /// @brief Creates a buffer.
    /// @param buf Buffer pointer
    /// @param length Buffer length
    /// @param flags Buffer flags (unused)
    Buffer(const void *buf, size_t length, int flags);

    /// @brief Pointer to a hip allocated buffer
    void *buffer;

    /// @brief Buffer length
    size_t length;

    /// @brief Buffer flags (unused?)
    int flags;

    /// @brief Memory type
    hipMemoryType type;

    friend class BufferMap;
};

class BufferMap {
public:
    virtual ~BufferMap();

    /// @brief Creates and registers a buffer
    /// @attention A unique_lock on RocFileMutex must be held
    /// @param buf Buffer pointer
    /// @param length Buffer length
    /// @param flags Buffer flags (unused)
    virtual void registerBuffer(const void *buf, size_t length, int flags);

    /// @brief Deregisters and destroys a buffer
    /// @attention A unique_lock on RocFileMutex must be held
    /// @param buf Buffer pointer
    virtual void deregisterBuffer(const void *buf);

    /// @brief Look up a registered buffer using the buffer pointer
    /// @attention A shared_lock on RocFileMutex must be held
    /// @param buf Buffer pointer
    /// @return A registered buffer
    virtual std::shared_ptr<IBuffer> getBuffer(const void *buf);

    /// @brief Look up a registered buffer. Returns a temporary unregistered
    ///        buffer (of size length, using flags) if no matching buffer is found.
    /// @attention A shared_lock on RocFileMutex must be held
    /// @param buf Buffer pointer
    /// @param length Buffer length
    /// @return A registered or temporary unregistered buffer
    virtual std::shared_ptr<IBuffer> getBuffer(const void *buf, size_t length, int flags);

    virtual void clear();

private:
    /// Buffer lookup table. Protected by rocfile.cpp's StateMutex
    std::unordered_map<const void *, std::shared_ptr<IBuffer>> from_ptr;
};

}
