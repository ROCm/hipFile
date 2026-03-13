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
#include <unistd.h>

using namespace hipFile;

ssize_t
RetryableBackend::io(IoType type, std::shared_ptr<IFile> file, std::shared_ptr<IBuffer> buffer, size_t size,
                     hoff_t file_offset, hoff_t buffer_offset)
{
    ssize_t nbytes{0};
    try {
        nbytes = retryable_io(type, file, buffer, size, file_offset, buffer_offset);
    }
    catch (...) {
        std::exception_ptr e_ptr = std::current_exception();
        if (is_retryable(e_ptr, nbytes, file, buffer, size, file_offset, buffer_offset)) {
            nbytes = retry_backend->io(type, file, buffer, size, file_offset, buffer_offset);
        }
        else {
            throw;
        }
        // For now, assume retry_backend will handle its own stats (which is true at this moment).
        return nbytes;
    }
    switch (type) {
        case (IoType::Read):
            update_read_stats(nbytes);
            break;
        case (IoType::Write):
            update_write_stats(nbytes);
            break;
        default:
            break;
    }
    return nbytes;
}

bool
RetryableBackend::is_retryable(std::exception_ptr e_ptr, ssize_t nbytes, std::shared_ptr<IFile> file,
                               std::shared_ptr<IBuffer> buffer, size_t size, hoff_t file_offset,
                               hoff_t buffer_offset) const
{
    (void)e_ptr;
    (void)nbytes;
    return static_cast<bool>(retry_backend) &&
           retry_backend->score(file, buffer, size, file_offset, buffer_offset) >= 0;
}

void
RetryableBackend::register_retry_backend(std::shared_ptr<Backend> backend) noexcept
{
    retry_backend = backend;
}
