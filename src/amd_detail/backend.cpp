/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "backend.h"
#include "buffer.h"
#include "file.h"
#include "io.h"

#include <cstddef>
#include <exception>
#include <memory>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

using namespace hipFile;

ssize_t
Backend::io(IoType type, std::shared_ptr<IFile> file, std::shared_ptr<IBuffer> buffer, size_t size,
            hoff_t file_offset, hoff_t buffer_offset)
{
    return _io_impl(type, file, buffer, size, file_offset, buffer_offset);
}

ssize_t
BackendWithFallback::io(IoType type, std::shared_ptr<IFile> file, std::shared_ptr<IBuffer> buffer,
                        size_t size, hoff_t file_offset, hoff_t buffer_offset)
{
    ssize_t nbytes{0};
    try {
        nbytes = _io_impl(type, file, buffer, size, file_offset, buffer_offset);
        if (nbytes < 0) {
            // Typically we should not reach this point. But in case we do, throw
            // an exception to use the fallback backend.
            throw std::system_error(-static_cast<int>(nbytes), std::generic_category());
        }
    }
    catch (...) {
        std::exception_ptr e_ptr = std::current_exception();
        if (is_fallback_eligible(e_ptr, nbytes, file, buffer, size, file_offset, buffer_offset)) {
            nbytes = fallback_backend->io(type, file, buffer, size, file_offset, buffer_offset);
        }
        else {
            throw;
        }
        return nbytes;
    }
    return nbytes;
}

bool
BackendWithFallback::is_fallback_eligible(std::exception_ptr e_ptr, ssize_t nbytes,
                                          std::shared_ptr<IFile> file, std::shared_ptr<IBuffer> buffer,
                                          size_t size, hoff_t file_offset, hoff_t buffer_offset) const
{
    (void)e_ptr;
    (void)nbytes;
    return static_cast<bool>(fallback_backend) &&
           fallback_backend->score(file, buffer, size, file_offset, buffer_offset) >= 0;
}

void
BackendWithFallback::register_fallback_backend(std::shared_ptr<Backend> backend) noexcept
{
    fallback_backend = backend;
}
