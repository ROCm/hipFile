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
#include "hipfile-warnings.h"
#include "io.h"
#include "rocfile-private.h"
#include "state.h"
#include "sys.h"

#include <cerrno>
#include <memory>
#include <stdexcept>
#include <vector>
#include <system_error>

using namespace hipFile;
using namespace std;

const char *
rocFileOpStatusError(rocFileOpError_t status)
{
    switch (status) {
        case rocFileSuccess:
            return "rocFile success";
        case rocFileDriverNotInitialized:
            return "GPU IO driver is not loaded";
        case rocFileDriverInvalidProps:
            return "Invalid GPU IO driver property provided";
        case rocFileDriverUnsupportedLimit:
            return "GPU IO Driver property value is unsupported";
        case rocFileDriverVersionMismatch:
            return "rocFile version does not match GPU IO driver version";
        case rocFileDriverVersionReadError:
            return "Unable to read the GPU IO driver version";
        case rocFileDriverClosing:
            return "GPU IO driver is closing and not accepting new requests";
        case rocFilePlatformNotSupported:
            return "rocFile is not supported on the current platform";
        case rocFileIONotSupported:
            return "rocFile is not supported on the selected file";
        case rocFileDeviceNotSupported:
            return "The selected GPU does not support rocFile";
        case rocFileDriverError:
            return "GPU IO driver error";
        case rocFileHipDriverError:
            return "GPU driver error: Inspect hip_drv_err for additional information";
        case rocFileHipPointerInvalid:
            return "Invalid GPU pointer";
        case rocFileHipMemoryTypeInvalid:
            return "Memory type backing pointer is incompatible with rocFile";
        case rocFileHipPointerRangeError:
            return "Pointer range exceeds allocated memory region";
        case rocFileHipContextMismatch:
            return "GPU driver context mismatch";
        case rocFileInvalidMappingSize:
            return "Accessing memory beyond pinned memory buffer";
        case rocFileInvalidMappingRange:
            return "Accessing memory beyond mapped memory region";
        case rocFileInvalidFileType:
            return "Unsupported file type";
        case rocFileInvalidFileOpenFlag:
            return "Unsupported file open flags";
        case rocFileDIONotSet:
            return "O_DIRECT flag not set";
        case rocFileInvalidValue:
            return "One or more arguments have an invalid value";
        case rocFileMemoryAlreadyRegistered:
            return "Device pointer is already registered";
        case rocFileMemoryNotRegistered:
            return "Device pointer is not registered";
        case rocFilePermissionDenied:
            return "Permission error on device or file access";
        case rocFileDriverAlreadyOpen:
            return "GPU IO driver is already open";
        case rocFileHandleNotRegistered:
            return "File handle for GPU IO is not registered";
        case rocFileHandleAlreadyRegistered:
            return "File handle for GPU IO is already registered";
        case rocFileDeviceNotFound:
            return "Selected device not found";
        case rocFileInternalError:
            return "Internal GPU IO library error";
        case rocFileGetNewFDFailed:
            return "Unable to obtain a new file descriptor";
        case rocFileDriverSetupError:
            return "GPU IO Driver initialization error";
        case rocFileIODisabled:
            return "GPU IO config file prohibits GPU IO on specified file";
        case rocFileBatchSubmitFailed:
            return "Failed to submit request for batch operation";
        case rocFileGPUMemoryPinningFailed:
            return "Failed to allocated pinned device memory";
        case rocFileBatchFull:
            return "Batch operation queue is full";
        case rocFileAsyncNotSupported:
            return "rocFile async IO is not supported";
        case rocFileIOMaxError:
            return "Internal flag to mark largest rocFile error code";
        default:
            return "Unknown rocFile error";
    }
}

/// Catch C++ exceptions from the rocFile code and convert
/// them into error values that can be returned from public
/// C API calls.
static inline rocFileError_t
handle_exception() noexcept
try {
    throw;
}
catch (rocFileError_t e) {
    return e;
}
catch (const Hip::RuntimeError &e) {
    return {rocFileHipDriverError, e.error};
}
catch (...) {
    return {rocFileInternalError, hipSuccess};
}

rocFileError_t
rocFileHandleRegister(rocFileHandle_t *fh, rocFileDescr_t *descr)
try {
    if (fh == nullptr || descr == nullptr) {
        return {rocFileInvalidValue, hipSuccess};
    }

    switch (descr->type) {
        case rocFileHandleTypeOpaqueFD: {
            UnregisteredFile uf{descr->handle.fd};
            *fh = Context<DriverState>::get()->registerFile(uf);
            return {rocFileSuccess, hipSuccess};
        }
        case rocFileHandleTypeOpaqueWin32:
        case rocFileHandleTypeUserspaceFS:
        default:
            return {rocFileIONotSupported, hipSuccess};
    }
}
catch (const FileAlreadyRegistered &) {
    return {rocFileHandleAlreadyRegistered, hipSuccess};
}
catch (...) {
    return handle_exception();
}

rocFileError_t
rocFileHandleDeregister(rocFileHandle_t fh)
try {
    if (fh == nullptr) {
        return {rocFileInvalidValue, hipSuccess};
    }

    Context<DriverState>::get()->deregisterFile(fh);
    return {rocFileSuccess, hipSuccess};
}
catch (const DriverNotInitialized &) {
    return {rocFileDriverNotInitialized, hipSuccess};
}
catch (const FileOperationsOutstanding &) {
    return {rocFileInternalError, hipSuccess};
}
catch (const FileNotRegistered &) {
    return {rocFileHandleNotRegistered, hipSuccess};
}
catch (...) {
    return handle_exception();
}

rocFileError_t
rocFileBufRegister(const void *buffer_base, size_t length, int flags)
try {
    Context<DriverState>::get()->registerBuffer(buffer_base, length, flags);
    return {rocFileSuccess, hipSuccess};
}
catch (const BufferAlreadyRegistered &) {
    return {rocFileMemoryAlreadyRegistered, hipSuccess};
}
catch (const InvalidMemoryType &) {
    return {rocFileHipMemoryTypeInvalid, hipSuccess};
}
catch (const InvalidPointerRange &) {
    return {rocFileHipPointerRangeError, hipSuccess};
}
catch (const Hip::RuntimeError &e) {
    if (e.error == hipErrorInvalidValue) {
        return {rocFileInvalidValue, hipSuccess};
    }
    return handle_exception();
}
catch (const std::invalid_argument &) {
    return {rocFileInvalidValue, hipSuccess};
}
catch (...) {
    return handle_exception();
}

rocFileError_t
rocFileBufDeregister(const void *buffer_base)
try {
    Context<DriverState>::get()->deregisterBuffer(buffer_base);
    return {rocFileSuccess, hipSuccess};
}
catch (const DriverNotInitialized &) {
    return {rocFileDriverNotInitialized, hipSuccess};
}
catch (const BufferNotRegistered &) {
    return {rocFileMemoryNotRegistered, hipSuccess};
}
catch (const BufferOperationsOutstanding &) {
    return {rocFileInternalError, hipSuccess};
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
rocFileIo(IoType type, rocFileHandle_t fh, const void *buffer_base, size_t size, hoff_t file_offset,
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
    return -rocFileDriverNotInitialized;
}
catch (rocFileError_t e) {
    return -e.err;
}
catch (const InvalidMemoryType &) {
    return -rocFileHipMemoryTypeInvalid;
}
catch (const std::invalid_argument &) {
    return -rocFileInvalidValue;
}
catch (const FileNotRegistered &) {
    return -rocFileHandleNotRegistered;
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
    return -rocFileInternalError;
}

ssize_t
rocFileRead(rocFileHandle_t fh, void *buffer_base, size_t size, hoff_t file_offset, hoff_t buffer_offset)
{
    return rocFileIo(IoType::Read, fh, buffer_base, size, file_offset, buffer_offset, getCachedBackends());
}

ssize_t
rocFileWrite(rocFileHandle_t fh, const void *buffer_base, size_t size, hoff_t file_offset,
             hoff_t buffer_offset)
{
    return rocFileIo(IoType::Write, fh, buffer_base, size, file_offset, buffer_offset, getCachedBackends());
}

rocFileError_t
rocFileDriverOpen()
try {
    Context<DriverState>::get()->incrRefCount();

    return {rocFileSuccess, hipSuccess};
}
catch (...) {
    return handle_exception();
}

rocFileError_t
rocFileDriverClose()
try {
    if (Context<DriverState>::get()->getRefCount() > 0) {
        Context<DriverState>::get()->decrRefCount();
        return {rocFileSuccess, hipSuccess};
    }
    else {
        return {rocFileDriverNotInitialized, hipSuccess};
    }
}
catch (...) {
    return handle_exception();
}

int64_t
rocFileUseCount()
try {
    return Context<DriverState>::get()->getRefCount();
}
catch (...) {
    return -1;
}

rocFileError_t
rocFileDriverGetProperties(rocFileDriverProps_t *props)
try {
    (void)props;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

rocFileError_t
rocFileDriverSetPollMode(bool poll, size_t poll_threshold_size)
try {
    (void)poll;
    (void)poll_threshold_size;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

rocFileError_t
rocFileDriverSetMaxDirectIOSize(size_t max_direct_io_size)
try {
    (void)max_direct_io_size;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

rocFileError_t
rocFileDriverSetMaxCacheSize(size_t max_cache_size)
try {
    (void)max_cache_size;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

rocFileError_t
rocFileDriverSetMaxPinnedMemSize(size_t max_pinned_size)
try {
    (void)max_pinned_size;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

rocFileError_t
rocFileBatchIOSetUp(rocFileBatchHandle_t *batch_idp, unsigned max_nr)
try {
    if (batch_idp == nullptr) {
        return {rocFileInvalidValue, hipSuccess};
    }

    *batch_idp = Context<DriverState>::get()->createBatchContext(max_nr);

    return {rocFileSuccess, hipSuccess};
}
catch (const std::invalid_argument &) {
    return {rocFileInvalidValue, hipSuccess};
}
catch (...) {
    return handle_exception();
}

rocFileError_t
rocFileBatchIOSubmit(rocFileBatchHandle_t batch_idp, unsigned nr, rocFileIOParams_t *iocbp, unsigned flags)
try {
    (void)flags; // Unused at this time.

    std::shared_ptr<IBatchContext> batch_context = Context<DriverState>::get()->getBatchContext(batch_idp);
    batch_context->submit_operations(iocbp, nr);

    return {rocFileSuccess, hipSuccess};
}
catch (const std::invalid_argument &) {
    return {rocFileInvalidValue, hipSuccess};
}
catch (...) {
    return handle_exception();
}

rocFileError_t
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

rocFileError_t
rocFileBatchIOCancel(rocFileBatchHandle_t batch_idp)
try {
    (void)batch_idp;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

rocFileError_t
rocFileBatchIODestroy(rocFileBatchHandle_t batch_idp)
try {
    (void)batch_idp;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

rocFileError_t
rocFileReadAsync(rocFileHandle_t fh, void *buffer_base, size_t *size_p, hoff_t *file_offset_p,
                 hoff_t *buffer_offset_p, ssize_t *bytes_read_p, hipStream_t stream)
try {
    if (Context<DriverState>::get()->getRefCount() == 0) {
        return {rocFileDriverNotInitialized, hipSuccess};
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

rocFileError_t
rocFileWriteAsync(rocFileHandle_t fh, void *buffer_base, size_t *size_p, hoff_t *file_offset_p,
                  hoff_t *buffer_offset_p, ssize_t *bytes_written_p, hipStream_t stream)
try {
    if (Context<DriverState>::get()->getRefCount() == 0) {
        return {rocFileDriverNotInitialized, hipSuccess};
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

rocFileError_t
rocFileStreamRegister(hipStream_t stream, unsigned flags)
try {
    Context<DriverState>::get()->registerStream(stream, flags);
    return {rocFileSuccess, hipSuccess};
}
catch (std::invalid_argument &) {
    return {rocFileInvalidValue, hipSuccess};
}
catch (...) {
    return handle_exception();
}

rocFileError_t
rocFileStreamDeregister(hipStream_t stream)
try {
    Context<DriverState>::get()->deregisterStream(stream);
    return {rocFileSuccess, hipSuccess};
}
catch (std::invalid_argument &) {
    return {rocFileInvalidValue, hipSuccess};
}
catch (...) {
    return handle_exception();
}

rocFileError_t
rocFileGetVersion(unsigned *major, unsigned *minor, unsigned *patch)
try {
    // NULL parameters are ignored
    if (major != nullptr) {
        *major = ROCFILE_VERSION_MAJOR;
    }
    if (minor != nullptr) {
        *minor = ROCFILE_VERSION_MINOR;
    }
    if (patch != nullptr) {
        *patch = ROCFILE_VERSION_PATCH;
    }

    return {rocFileSuccess, hipSuccess};
}
catch (...) {
    return handle_exception();
}

rocFileError_t
rocFileGetParameterSizeT(rocFileSizeTConfigParameter_t param, size_t *value)
try {
    (void)param;
    (void)value;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

rocFileError_t
rocFileGetParameterBool(rocFileBoolConfigParameter_t param, bool *value)
try {
    (void)param;
    (void)value;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

rocFileError_t
rocFileGetParameterString(rocFileStringConfigParameter_t param, char *desc_str, int len)
try {
    (void)param;
    (void)desc_str;
    (void)len;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

rocFileError_t
rocFileSetParameterSizeT(rocFileSizeTConfigParameter_t param, size_t value)
try {
    (void)param;
    (void)value;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

rocFileError_t
rocFileSetParameterBool(rocFileBoolConfigParameter_t param, bool value)
try {
    (void)param;
    (void)value;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}

rocFileError_t
rocFileSetParameterString(rocFileStringConfigParameter_t param, const char *desc_str)
try {
    (void)param;
    (void)desc_str;

    throw std::runtime_error("Not Implemented");
}
catch (...) {
    return handle_exception();
}
