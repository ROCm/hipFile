/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "async.h"
#include "asyncop-fallback.h"
#include "backend.h"
#include "buffer.h"
#include "context.h"
#include "hip.h"
#include "stream.h"
#include "sys.h"

#include <memory>
#include <new>
#include <syslog.h>

namespace hipFile {
class IFile;
}
namespace hipFile {
class IStream;
}
namespace hipFile {
enum class IoType;
}

using namespace hipFile;

AsyncOpFallback::AsyncOpFallback(IoType _io_type, std::shared_ptr<IFile> _file,
                                 std::shared_ptr<IBuffer> _buffer, std::shared_ptr<IStream> _stream,
                                 size_t *_size, hoff_t *_file_offset, hoff_t *_buffer_offset,
                                 ssize_t *_bytes_transferred)
    : AsyncOp{_io_type, _file, _buffer, _stream, _size, _file_offset, _buffer_offset, _bytes_transferred},
      submitted_size{std::min(*_size, hipFile::MAX_RW_COUNT)}, bytes_transferred_internal{0},
      gpu_buffer{buffer->getBuffer()}, bounce_buffer_dev_ptr{nullptr},
      bounce_buffer{new(std::align_val_t{64}) uint8_t[submitted_size]}
{
    HipSetDevice hsd{stream->getHipDevice()};
    Context<Hip>::get()->hipHostRegister(bounce_buffer.get(), submitted_size, hipHostRegisterDefault);
    Context<Hip>::get()->hipHostRegister(this, sizeof(AsyncOpFallback), hipHostRegisterDefault);
    bounce_buffer_dev_ptr = Context<Hip>::get()->hipHostGetDevicePointer(bounce_buffer.get(), 0);
}

void *
AsyncOpFallback::bounceBufferHostPtr()
{
    return bounce_buffer.get();
}

void *
AsyncOpFallback::devPtr()
{
    HipSetDevice hsd{stream->getHipDevice()};
    return Context<Hip>::get()->hipHostGetDevicePointer(this, 0);
}

AsyncOpFallback::~AsyncOpFallback()
{
    try {
        Context<Hip>::get()->hipHostUnregister(bounce_buffer.get());
    }
    catch (Hip::RuntimeError &e) {
        Context<Sys>::get()->syslog(LOG_CRIT, "Error unregistering bounce buffer.");
    }

    try {
        Context<Hip>::get()->hipHostUnregister(this);
    }
    catch (Hip::RuntimeError &e) {
        Context<Sys>::get()->syslog(LOG_CRIT, "Error unregistering AsyncOpFallback pointer.");
    }
}
