/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "backend.h"
#include "batch/batch.h"
#include "buffer.h"
#include "context.h"
#include "file.h"
#include "hip.h"
#include "hipfile.h"
#include "hipfile-private.h"
#include "hipfile-warnings.h"
#include "io.h"
#include "state.h"
#include "sys.h"

#include <cerrno>
#include <memory>
#include <stdexcept>
#include <vector>
#include <system_error>

using namespace hipFile;
using namespace std;

/// Catch C++ exceptions from the hipFile code and convert
/// them into error values that can be returned from public
/// C API calls.
static inline hipFileError_t
handle_exception() noexcept
try {
    throw;
}
catch (hipFileError_t e) {
    return e;
}
catch (const Hip::RuntimeError &e) {
    return {hipFileHipDriverError, e.error};
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
rocFileHandleRegister(rocFileHandle_t *fh, rocFileDescr_t *descr)
try {
    if (fh == nullptr || descr == nullptr) {
        return {hipFileInvalidValue, hipSuccess};
    }

    switch (descr->type) {
        case rocFileHandleTypeOpaqueFD: {
            UnregisteredFile uf{descr->handle.fd};
            *fh = Context<DriverState>::get()->registerFile(uf);
            return {hipFileSuccess, hipSuccess};
        }
        case rocFileHandleTypeOpaqueWin32:
        case rocFileHandleTypeUserspaceFS:
        default:
            return {hipFileIONotSupported, hipSuccess};
    }
}
catch (const FileAlreadyRegistered &) {
    return {hipFileHandleAlreadyRegistered, hipSuccess};
}
catch (...) {
    return handle_exception();
}

hipFileError_t
rocFileHandleDeregister(rocFileHandle_t fh)
try {
    if (fh == nullptr) {
        return {hipFileInvalidValue, hipSuccess};
    }

    Context<DriverState>::get()->deregisterFile(fh);
    return {hipFileSuccess, hipSuccess};
}
catch (const DriverNotInitialized &) {
    return {hipFileDriverNotInitialized, hipSuccess};
}
catch (const FileOperationsOutstanding &) {
    return {hipFileInternalError, hipSuccess};
}
catch (const FileNotRegistered &) {
    return {hipFileHandleNotRegistered, hipSuccess};
}
catch (...) {
    return handle_exception();
}

hipFileError_t
rocFileBufRegister(const void *buffer_base, size_t length, int flags)
try {
    Context<DriverState>::get()->registerBuffer(buffer_base, length, flags);
    return {hipFileSuccess, hipSuccess};
}
catch (const BufferAlreadyRegistered &) {
    return {hipFileMemoryAlreadyRegistered, hipSuccess};
}
catch (const InvalidMemoryType &) {
    return {hipFileHipMemoryTypeInvalid, hipSuccess};
}
catch (const InvalidPointerRange &) {
    return {hipFileHipPointerRangeError, hipSuccess};
}
catch (const Hip::RuntimeError &e) {
    if (e.error == hipErrorInvalidValue) {
        return {hipFileInvalidValue, hipSuccess};
    }
    return handle_exception();
}
catch (const std::invalid_argument &) {
    return {hipFileInvalidValue, hipSuccess};
}
catch (...) {
    return handle_exception();
}

hipFileError_t
rocFileBufDeregister(const void *buffer_base)
try {
    Context<DriverState>::get()->deregisterBuffer(buffer_base);
    return {hipFileSuccess, hipSuccess};
}
catch (const DriverNotInitialized &) {
    return {hipFileDriverNotInitialized, hipSuccess};
}
catch (const BufferNotRegistered &) {
    return {hipFileMemoryNotRegistered, hipSuccess};
}
catch (const BufferOperationsOutstanding &) {
    return {hipFileInternalError, hipSuccess};
}
catch (...) {
    return handle_exception();
}

/// @brief Get the cached list of backends obtained from DriverState
static const vector<shared_ptr<Backend>> &
getCachedBackends()
{
    HIPFILE_WARN_NO_EXIT_DTOR_OFF
    static const auto backends{Context<DriverState>::get()->getBackends()};
    HIPFILE_WARN_NO_EXIT_DTOR_ON
    return backends;
}

ssize_t
hipFileIo(IoType type, rocFileHandle_t fh, const void *buffer_base, size_t size, hoff_t file_offset,
          hoff_t buffer_offset, const vector<shared_ptr<Backend>> &backends)
try {
    auto [file, buffer] = Context<DriverState>::get()->getFileAndBuffer(fh, buffer_base, size, 0);
    int                      score{-1};
    std::shared_ptr<Backend> backend{};

    for (const auto &_backend : backends) {
        auto _score = _backend->score(file, buffer, size, file_offset, buffer_offset);
        if (score < _score) {
            score   = _score;
            backend = _backend;
        }
    }

    if (!backend) {
        if (backends.size() == 0) {
            throw std::runtime_error("No available backends for IO request");
        }
        else {
            throw std::runtime_error("All backends refused IO request");
        }
    }

    return backend->io(type, file, buffer, size, file_offset, buffer_offset);
}
catch (const DriverNotInitialized &) {
    return -hipFileDriverNotInitialized;
}
catch (hipFileError_t e) {
    return -e.err;
}
catch (const InvalidMemoryType &) {
    return -hipFileHipMemoryTypeInvalid;
}
catch (const std::invalid_argument &) {
    return -hipFileInvalidValue;
}
catch (const FileNotRegistered &) {
    return -hipFileHandleNotRegistered;
}
catch (const Hip::RuntimeError &e) {
    return -e.error;
}
catch (const Sys::RuntimeError &e) {
    errno = e.error;
    return -1;
}
catch (const std::system_error &e) {
    errno = e.code().value();
    return -1;
}
catch (...) {
    return -hipFileInternalError;
}

ssize_t
rocFileRead(rocFileHandle_t fh, void *buffer_base, size_t size, hoff_t file_offset, hoff_t buffer_offset)
{
    return hipFileIo(IoType::Read, fh, buffer_base, size, file_offset, buffer_offset, getCachedBackends());
}

ssize_t
rocFileWrite(rocFileHandle_t fh, const void *buffer_base, size_t size, hoff_t file_offset,
             hoff_t buffer_offset)
{
    return hipFileIo(IoType::Write, fh, buffer_base, size, file_offset, buffer_offset, getCachedBackends());
}

hipFileError_t
hipFileDriverOpen()
try {
    Context<DriverState>::get()->incrRefCount();

    return {hipFileSuccess, hipSuccess};
}
catch (...) {
    return handle_exception();
}

hipFileError_t
hipFileDriverClose()
try {
    if (Context<DriverState>::get()->getRefCount() > 0) {
        Context<DriverState>::get()->decrRefCount();
        return {hipFileSuccess, hipSuccess};
    }
    else {
        return {hipFileDriverNotInitialized, hipSuccess};
    }
}
catch (...) {
    return handle_exception();
}

int64_t
hipFileUseCount()
try {
    return Context<DriverState>::get()->getRefCount();
}
catch (...) {
    return -1;
}

hipFileError_t
hipFileDriverGetProperties(hipFileDriverProps_t *props)
try {
    (void)props;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

hipFileError_t
hipFileDriverSetPollMode(bool poll, size_t poll_threshold_size)
try {
    (void)poll;
    (void)poll_threshold_size;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

hipFileError_t
hipFileDriverSetMaxDirectIOSize(size_t max_direct_io_size)
try {
    (void)max_direct_io_size;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

hipFileError_t
hipFileDriverSetMaxCacheSize(size_t max_cache_size)
try {
    (void)max_cache_size;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

hipFileError_t
hipFileDriverSetMaxPinnedMemSize(size_t max_pinned_size)
try {
    (void)max_pinned_size;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

hipFileError_t
rocFileBatchIOSetUp(rocFileBatchHandle_t *batch_idp, unsigned max_nr)
try {
    if (batch_idp == nullptr) {
        return {hipFileInvalidValue, hipSuccess};
    }

    *batch_idp = Context<DriverState>::get()->createBatchContext(max_nr);

    return {hipFileSuccess, hipSuccess};
}
catch (const std::invalid_argument &) {
    return {hipFileInvalidValue, hipSuccess};
}
catch (...) {
    return handle_exception();
}

hipFileError_t
rocFileBatchIOSubmit(rocFileBatchHandle_t batch_idp, unsigned nr, rocFileIOParams_t *iocbp, unsigned flags)
try {
    (void)flags; // Unused at this time.

    std::shared_ptr<IBatchContext> batch_context = Context<DriverState>::get()->getBatchContext(batch_idp);
    batch_context->submit_operations(iocbp, nr);

    return {hipFileSuccess, hipSuccess};
}
catch (const std::invalid_argument &) {
    return {hipFileInvalidValue, hipSuccess};
}
catch (...) {
    return handle_exception();
}

hipFileError_t
rocFileBatchIOGetStatus(rocFileBatchHandle_t batch_idp, unsigned min_nr, unsigned *nr,
                        rocFileIOEvents_t *iocbp, struct timespec *timeout)
try {
    (void)batch_idp;
    (void)min_nr;
    (void)nr;
    (void)iocbp;
    (void)timeout;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

hipFileError_t
rocFileBatchIOCancel(rocFileBatchHandle_t batch_idp)
try {
    (void)batch_idp;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

hipFileError_t
rocFileBatchIODestroy(rocFileBatchHandle_t batch_idp)
try {
    (void)batch_idp;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

hipFileError_t
rocFileReadAsync(rocFileHandle_t fh, void *buffer_base, size_t *size_p, hoff_t *file_offset_p,
                 hoff_t *buffer_offset_p, ssize_t *bytes_read_p, hipStream_t stream)
try {
    if (Context<DriverState>::get()->getRefCount() == 0) {
        return {hipFileDriverNotInitialized, hipSuccess};
    }

    (void)fh;
    (void)buffer_base;
    (void)size_p;
    (void)file_offset_p;
    (void)buffer_offset_p;
    (void)bytes_read_p;
    (void)stream;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

hipFileError_t
rocFileWriteAsync(rocFileHandle_t fh, void *buffer_base, size_t *size_p, hoff_t *file_offset_p,
                  hoff_t *buffer_offset_p, ssize_t *bytes_written_p, hipStream_t stream)
try {
    if (Context<DriverState>::get()->getRefCount() == 0) {
        return {hipFileDriverNotInitialized, hipSuccess};
    }

    (void)fh;
    (void)buffer_base;
    (void)size_p;
    (void)file_offset_p;
    (void)buffer_offset_p;
    (void)bytes_written_p;
    (void)stream;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

hipFileError_t
rocFileStreamRegister(hipStream_t stream, unsigned flags)
try {
    Context<DriverState>::get()->registerStream(stream, flags);
    return {hipFileSuccess, hipSuccess};
}
catch (std::invalid_argument &) {
    return {hipFileInvalidValue, hipSuccess};
}
catch (...) {
    return handle_exception();
}

hipFileError_t
rocFileStreamDeregister(hipStream_t stream)
try {
    Context<DriverState>::get()->deregisterStream(stream);
    return {hipFileSuccess, hipSuccess};
}
catch (std::invalid_argument &) {
    return {hipFileInvalidValue, hipSuccess};
}
catch (...) {
    return handle_exception();
}

// ***********************************************************************
//  PROPERTIES API
// ***********************************************************************

hipFileError_t
hipFileGetParameterSizeT(hipFileSizeTConfigParameter_t param, size_t *value)
try {
    (void)param;
    (void)value;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

hipFileError_t
hipFileGetParameterBool(hipFileBoolConfigParameter_t param, bool *value)
try {
    (void)param;
    (void)value;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

hipFileError_t
hipFileGetParameterString(hipFileStringConfigParameter_t param, char *desc_str, int len)
try {
    (void)param;
    (void)desc_str;
    (void)len;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

hipFileError_t
hipFileSetParameterSizeT(hipFileSizeTConfigParameter_t param, size_t value)
try {
    (void)param;
    (void)value;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

hipFileError_t
hipFileSetParameterBool(hipFileBoolConfigParameter_t param, bool value)
try {
    (void)param;
    (void)value;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

hipFileError_t
hipFileSetParameterString(hipFileStringConfigParameter_t param, const char *desc_str)
try {
    (void)param;
    (void)desc_str;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}
