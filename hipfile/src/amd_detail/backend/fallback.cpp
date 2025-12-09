/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "backend.h"
#include "buffer.h"
#include "context.h"
#include "fallback.h"
#include "file.h"
#include "hip.h"
#include "io.h"
#include "sys.h"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <hip/hip_runtime_api.h>
#include <hip/driver_types.h>
#include <stdexcept>
#include <sys/mman.h>

using namespace hipFile;

using std::min;
using std::shared_ptr;
using std::unique_ptr;

static const size_t DefaultChunkSize = 16 * 1024 * 1024;

int
Fallback::score(std::shared_ptr<IFile> file, std::shared_ptr<IBuffer> buffer, size_t size, hoff_t file_offset,
                hoff_t buffer_offset) const
{
    (void)buffer_offset;
    (void)file;
    (void)file_offset;
    (void)size;
    return buffer->getType() == hipMemoryTypeDevice ? 0 : -1;
}

ssize_t
Fallback::io(IoType type, std::shared_ptr<IFile> file, std::shared_ptr<IBuffer> buffer, size_t size,
             hoff_t file_offset, hoff_t buffer_offset)
{
    return io(type, file, buffer, size, file_offset, buffer_offset, DefaultChunkSize);
}

ssize_t
Fallback::io(IoType io_type, shared_ptr<IFile> file, shared_ptr<IBuffer> buffer, size_t size,
             hoff_t file_offset, hoff_t buffer_offset, size_t chunk_size)
{
    size_t buflen{buffer->getLength()};

    size = min(size, hipFile::MAX_RW_COUNT);

    if (file_offset < 0) {
        throw std::invalid_argument("Negative file offset");
    }

    if (buffer_offset < 0) {
        throw std::invalid_argument("Negative buffer offset");
    }

    if (buflen <= static_cast<size_t>(buffer_offset)) {
        throw std::invalid_argument("Buffer offset larger than buffer length");
    }

    if (buflen - static_cast<size_t>(buffer_offset) < size) {
        throw std::invalid_argument("IO could overflow buffer");
    }

    auto ptr     = Context<Sys>::get()->mmap(nullptr, chunk_size, PROT_READ | PROT_WRITE,
                                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    auto deleter = [&](void *addr) { Context<Sys>::get()->munmap(addr, chunk_size); };
    unique_ptr<void, decltype(deleter)> bounce_buffer{ptr, deleter};

    ssize_t total_io_bytes = 0;
    do {
        auto    count                  = min(chunk_size, size - static_cast<size_t>(total_io_bytes));
        auto    offset                 = file_offset + total_io_bytes;
        ssize_t io_bytes               = 0;
        auto    device_buffer_position = reinterpret_cast<void *>(
            reinterpret_cast<uintptr_t>(buffer->getBuffer()) + static_cast<size_t>(buffer_offset) +
            static_cast<size_t>(total_io_bytes));
        try {
            switch (io_type) {
                case IoType::Read:
                    io_bytes = Context<Sys>::get()->pread(file->getFdUnbuffered(), bounce_buffer.get(), count,
                                                          offset);
                    if (io_bytes > 0) {
                        Context<Hip>::get()->hipMemcpy(device_buffer_position, bounce_buffer.get(),
                                                       static_cast<size_t>(io_bytes), hipMemcpyHostToDevice);
                    }
                    break;
                case IoType::Write:
                    Context<Hip>::get()->hipMemcpy(bounce_buffer.get(), device_buffer_position, count,
                                                   hipMemcpyDeviceToHost);
                    Context<Hip>::get()->hipStreamSynchronize(nullptr);
                    io_bytes = Context<Sys>::get()->pwrite(file->getFdUnbuffered(), bounce_buffer.get(),
                                                           count, offset);
                    break;
                default:
                    throw std::runtime_error("Invalid IO type");
            }
        }
        catch (const Sys::RuntimeError &e) {
            if (e.error == EINTR) {
                continue;
            }
            throw;
        }

        total_io_bytes += io_bytes;
        if (io_bytes == 0) {
            break;
        }
    } while (static_cast<size_t>(total_io_bytes) < size);

    return total_io_bytes;
}
