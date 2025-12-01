/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "buffer.h"
#include "context.h"
#include "hip.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <hip/hip_runtime_api.h>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>

using std::shared_ptr;
using std::transform;
using std::vector;

namespace hipFile {

static bool
isValidBufferRegion(void *ptr, size_t length)
{
    uintptr_t uptr = reinterpret_cast<uintptr_t>(ptr);

    try {
        HipMemAddressRange buffer_range = Context<Hip>::get()->hipMemGetAddressRange(ptr);
        if (uptr + length < uptr ||
            uptr + length > reinterpret_cast<uintptr_t>(buffer_range.base) + buffer_range.size) {
            return false;
        }
    }
    catch (Hip::RuntimeError &e) {
        if (e.error == hipErrorNotFound) {
            throw std::invalid_argument("Buffer pointer not found.");
        }
        throw;
    }

    return true;
}

Buffer::Buffer(const void *_buffer, size_t _length, int _flags)
    : buffer{const_cast<void *>(_buffer)}, length{_length}, flags{_flags}
{
    if (!buffer) {
        throw std::invalid_argument("Buffer pointer cannot be null.");
    }

    hipPointerAttribute_t _attrs = Context<Hip>::get()->hipPointerGetAttributes(buffer);
    if (_attrs.type != hipMemoryTypeDevice) {
        throw InvalidMemoryType();
    }
    type   = _attrs.type;
    gpu_id = _attrs.device;

    if (!isValidBufferRegion(buffer, length)) {
        throw InvalidPointerRange();
    }
}

void *
Buffer::getBuffer() const
{
    return buffer;
}

size_t
Buffer::getLength() const
{
    return length;
}

int
Buffer::getFlags() const
{
    return flags;
}

hipMemoryType
Buffer::getType() const
{
    return type;
}

int
Buffer::getGpuId() const
{
    return gpu_id;
}

void
BufferMap::registerBuffer(const void *buf, size_t length, int flags)
{
    if (from_ptr.end() != from_ptr.find(buf)) {
        throw BufferAlreadyRegistered();
    }

    auto buffer   = std::shared_ptr<IBuffer>(new Buffer(buf, length, flags));
    from_ptr[buf] = buffer;
}

void
BufferMap::deregisterBuffer(const void *buf)
{
    auto itr = from_ptr.find(buf);
    if (from_ptr.end() == itr) {
        throw BufferNotRegistered();
    }

    if (1 < itr->second.use_count()) {
        throw BufferOperationsOutstanding();
    }

    from_ptr.erase(buf);
}

shared_ptr<IBuffer>
BufferMap::getBuffer(const void *buf)
{
    auto itr = from_ptr.find(buf);
    if (from_ptr.end() == itr) {
        throw BufferNotRegistered();
    }

    return itr->second;
}

shared_ptr<IBuffer>
BufferMap::getBuffer(const void *buf, size_t length, int flags)
{
    auto itr = from_ptr.find(buf);

    if (from_ptr.end() == itr) {
        // If the buffer hasn't been registered, use an unregistered
        // temporary Buffer object
        return std::shared_ptr<IBuffer>(new Buffer(buf, length, flags));
    }
    else {
        // If we found a registered buffer, it's an error if the
        // length parameter doesn't match what we found
        if (itr->second->getLength() < length) {
            throw std::invalid_argument("bad length parameter");
        }
        return itr->second;
    }
}

void
BufferMap::clear()
{
    from_ptr.clear();
}

}
