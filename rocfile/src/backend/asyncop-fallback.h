/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "async.h"

namespace rocFile::backend {

struct AsyncOpFallback : async::AsyncOp {
    ssize_t     bytes_transferred_internal;
    void *const gpu_buffer;
    void       *bounce_buffer_dev_ptr;

private:
    std::unique_ptr<void, void (*)(void *)> bounce_buffer;

public:
    AsyncOpFallback(io::IoType ioType, std::shared_ptr<file::IFile> file,
                    std::shared_ptr<buffer::IBuffer> buffer, std::shared_ptr<stream::IStream> stream,
                    size_t *size, off_t *fileOffset, off_t *bufferOffset, ssize_t *bytesTransferred);

    void *bounceBufferHostPtr();
    void *devPtr();

    virtual ~AsyncOpFallback() override;
    void  operator delete(void *ptr) noexcept;
    void *operator new(size_t size);
};

}
