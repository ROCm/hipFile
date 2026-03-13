/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "buffer.h"
#include "file.h"
#include "hip.h"
#include "io.h"
#include "sys.h"

#include <cstddef>
#include <memory>
#include <sys/types.h>
#include <unistd.h>

namespace hipFile {

// The maximum number of bytes that can be transferred in a single read() or
// write() system call. Mirrors kernel's MAX_RW_COUNT
static const size_t MAX_RW_COUNT = 0x7ffff000;

struct Backend {
    virtual ~Backend() = default;

    /// @brief Indicate this backend's eagerness to service the IO request.
    ///
    /// Indicates the willingness of the backend to perform service an IO
    /// request. The higher the number the more eager the backend is to service
    /// the request. If a negative number is returned, the backend is refusing to
    /// perform the operation. If the operation is submitted to the backend, the
    /// operation would likely fail.
    ///
    /// @param file           The IO request's file
    /// @param buffer         The IO request's buffer
    /// @param size           The IO request's size
    /// @param file_offset    Offset from the start of the file
    /// @param buffer_offset  Offset from the start of the buffer
    /// @return The eagerness "score"
    virtual int score(std::shared_ptr<IFile> file, std::shared_ptr<IBuffer> buffer, size_t size,
                      hoff_t file_offset, hoff_t buffer_offset) const = 0;

    /// @brief Perform a read or write operation
    ///
    /// @param type          IO type (read/write)
    /// @param file          File to read from or write to
    /// @param buffer        Buffer to write to or read from
    /// @param size          Number of bytes to transfer
    /// @param file_offset   Offset from the start of the file
    /// @param buffer_offset Offset from the start of the buffer
    ///
    /// @return Number of bytes transferred, negative on error
    ///
    /// @throws Hip::RuntimeError Sys::RuntimeError
    virtual ssize_t io(IoType type, std::shared_ptr<IFile> file, std::shared_ptr<IBuffer> buffer, size_t size,
                       hoff_t file_offset, hoff_t buffer_offset) = 0;
};

// This kind of Backend allows for an IO to be retried automatically in the
// event of an error. The RetryableBackend implements the logic to determine
// which errors are retryable, and which are not, in Backend::io().
struct RetryableBackend : public Backend {
    ssize_t io(IoType type, std::shared_ptr<IFile> file, std::shared_ptr<IBuffer> buffer, size_t size,
               hoff_t file_offset, hoff_t buffer_offset) override final;

    /// @brief Check if a failed IO operation is retryable.
    ///
    /// @param e_ptr         Pointer to the thrown Exception by the failed IO
    /// @param nbytes        Return value from `retryable_io`, or 0 if an exception was thrown.
    /// @param file          File to read from or write to
    /// @param buffer        Buffer to write to or read from
    /// @param size          Number of bytes to transfer
    /// @param file_offset   Offset from the start of the file
    /// @param buffer_offset Offset from the start of the buffer
    ///
    /// @note By default, RetryableBackend just checks if a Backend has been
    ///       registered for retrying an IO, and that backend supports the
    ///       the request.
    /// @note The parameters from the original IO request are passed to this function.
    ///
    /// @return True if this RetryableBackend can retry the IO, else False.
    virtual bool is_retryable(std::exception_ptr e_ptr, ssize_t nbytes, std::shared_ptr<IFile> file,
                              std::shared_ptr<IBuffer> buffer, size_t size, hoff_t file_offset,
                              hoff_t buffer_offset) const;

    /// @brief Register a Backend to call into to retry a failed IO operation.
    ///
    /// @param backend Backend to retry a failed IO operation.
    void register_retry_backend(std::shared_ptr<Backend> backend) noexcept;

    /// @brief Process an IO Request that can be resubmitted to another Backend if the IO fails.
    ///
    /// @param type          IO type (read/write)
    /// @param file          File to read from or write to
    /// @param buffer        Buffer to write to or read from
    /// @param size          Number of bytes to transfer
    /// @param file_offset   Offset from the start of the file
    /// @param buffer_offset Offset from the start of the buffer
    ///
    /// @return Number of bytes transferred, negative on error
    ///
    /// @throws Hip::RuntimeError Sys::RuntimeError
    virtual ssize_t retryable_io(IoType type, std::shared_ptr<IFile> file, std::shared_ptr<IBuffer> buffer,
                                 size_t size, hoff_t file_offset, hoff_t buffer_offset) = 0;

    // This maybe should be moved to Backend
    /// @brief Update the read stats for this RetryableBackend
    virtual void update_read_stats(ssize_t nbytes) = 0;

    // This maybe should be moved to Backend
    /// @brief Update the write stats for this RetryableBackend
    virtual void update_write_stats(ssize_t nbytes) = 0;

protected:
    std::shared_ptr<Backend> retry_backend;
};

}
