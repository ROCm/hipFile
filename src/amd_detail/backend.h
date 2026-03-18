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

/// @brief Backend is not enabled
struct BackendDisabled : public std::runtime_error {
    BackendDisabled() : std::runtime_error("Backend is disabled")
    {
    }
};

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

}
