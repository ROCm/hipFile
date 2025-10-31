/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "hipfile-rocfile.h"
#include "hipfile.h"

#include "rocfile.h"

#include <hip/hip_runtime_api.h>

#include <sys/types.h>

#include <cerrno>
#include <cstdlib>
#include <limits>
#include <vector>

using namespace std;

void rocFileEnsureDriverInitPrivate();

hipFileError_t
hipFileHandleRegister(hipFileHandle_t *fh, hipFileDescr_t *descr)
try {
    rocFileHandle_t *rocfile_fh = fh;
    rocFileError_t   status;

    if (descr) {
        auto rocfile_descr = toRocFileDescr(*descr);
        status             = rocFileHandleRegister(rocfile_fh, &rocfile_descr);
        descr->fs_ops      = reinterpret_cast<const hipFileFSOps_t *>(rocfile_descr.fs_ops);
    }
    else {
        status = rocFileHandleRegister(rocfile_fh, nullptr);
    }

    return toHipFileError(status);
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

void
hipFileHandleDeregister(hipFileHandle_t fh)
{
    (void)rocFileHandleDeregister(fh);
}

hipFileError_t
hipFileBufRegister(const void *buffer_base, size_t length, int flags)
try {
    return toHipFileError(rocFileBufRegister(buffer_base, length, flags));
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileBufDeregister(const void *buffer_base)
try {
    auto error = toHipFileError(rocFileBufDeregister(buffer_base));
    if (error.err == hipFileDriverNotInitialized) {
        error.err = hipFileDriverClosing;
    }
    return error;
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

ssize_t
hipFileRead(hipFileHandle_t fh, void *buffer_base, size_t size, hoff_t file_offset, hoff_t buffer_offset)
{
    auto result = rocFileRead(fh, buffer_base, size, file_offset, buffer_offset);
    if (result == -rocFileDriverNotInitialized) {
        // Match cuFile behaviour
        errno  = EINVAL;
        result = -1;
    }
    return result;
}

ssize_t
hipFileWrite(hipFileHandle_t fh, const void *buffer_base, size_t size, hoff_t file_offset,
             hoff_t buffer_offset)
{
    auto result = rocFileWrite(fh, buffer_base, size, file_offset, buffer_offset);
    if (result == -rocFileDriverNotInitialized) {
        // Match cuFile behaviour
        errno  = EINVAL;
        result = -1;
    }
    return result;
}

hipFileError_t
hipFileDriverOpen()
try {
    return toHipFileError(rocFileDriverOpen());
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileDriverClose()
try {
    return toHipFileError(rocFileDriverClose());
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

int64_t
hipFileUseCount()
{
    return rocFileUseCount();
}

hipFileError_t
hipFileDriverGetProperties(hipFileDriverProps_t *props)
try {
    rocFileError_t status;

    if (props) {
        rocFileDriverProps_t roc_props;
        status = rocFileDriverGetProperties(&roc_props);
        if (status.err == rocFileSuccess) {
            *props = toHipFileDriverProps(roc_props);
        }
    }
    else {
        status = rocFileDriverGetProperties(nullptr);
    }

    return toHipFileError(status);
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileDriverSetPollMode(bool poll, size_t poll_threshold_size)
try {
    return toHipFileError(rocFileDriverSetPollMode(poll, poll_threshold_size));
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileDriverSetMaxDirectIOSize(size_t max_direct_io_size)
try {
    return toHipFileError(rocFileDriverSetMaxDirectIOSize(max_direct_io_size));
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileDriverSetMaxCacheSize(size_t max_cache_size)
try {
    return toHipFileError(rocFileDriverSetMaxCacheSize(max_cache_size));
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileDriverSetMaxPinnedMemSize(size_t max_pinned_size)
try {
    return toHipFileError(rocFileDriverSetMaxPinnedMemSize(max_pinned_size));
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileBatchIOSetUp(hipFileBatchHandle_t *batch_idp, unsigned max_nr)
try {
    return toHipFileError(rocFileBatchIOSetUp(batch_idp, max_nr));
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileBatchIOSubmit(hipFileBatchHandle_t batch_idp, unsigned nr, hipFileIOParams_t *iocbp, unsigned flags)
try {
    auto           rf_batch_idp = batch_idp;
    rocFileError_t status;

    if (iocbp) {
        vector<rocFileIOParams_t> io_params(nr);

        for (unsigned i = 0; i < nr; i++) {
            io_params[i] = toRocFileIOParams(iocbp[i]);
        }

        status = rocFileBatchIOSubmit(rf_batch_idp, nr, io_params.data(), flags);
    }
    else {
        status = rocFileBatchIOSubmit(rf_batch_idp, nr, nullptr, flags);
    }

    return toHipFileError(status);
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileBatchIOGetStatus(hipFileBatchHandle_t batch_idp, unsigned min_nr, unsigned *nr,
                        hipFileIOEvents_t *iocbp, struct timespec *timeout)
try {
    auto           rf_batch_idp = batch_idp;
    rocFileError_t status;

    if (iocbp) {
        vector<rocFileIOEvents_t> io_events(*nr);

        status = rocFileBatchIOGetStatus(rf_batch_idp, min_nr, nr, io_events.data(), timeout);

        if (status.err == rocFileSuccess) {
            for (unsigned i = 0; i < *nr; i++) {
                iocbp[i] = toHipFileIOEvents(io_events[i]);
            }
        }
    }
    else {
        status = rocFileBatchIOGetStatus(rf_batch_idp, min_nr, nr, nullptr, timeout);
    }

    return toHipFileError(status);
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileBatchIOCancel(hipFileBatchHandle_t batch_idp)
try {
    return toHipFileError(rocFileBatchIOCancel(batch_idp));
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

void
hipFileBatchIODestroy(hipFileBatchHandle_t batch_idp)
{
    (void)rocFileBatchIODestroy(batch_idp);
}

hipFileError_t
hipFileReadAsync(hipFileHandle_t fh, void *buffer_base, size_t *size_p, hoff_t *file_offset_p,
                 hoff_t *buffer_offset_p, ssize_t *bytes_read_p, hipStream_t stream)
try {
    auto result = toHipFileError(
        rocFileReadAsync(fh, buffer_base, size_p, file_offset_p, buffer_offset_p, bytes_read_p, stream));
    if (result.err == hipFileDriverNotInitialized) {
        // Match cuFile behaviour
        rocFileEnsureDriverInitPrivate();
        result.err = hipFileInvalidValue;
    }
    return result;
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileWriteAsync(hipFileHandle_t fh, void *buffer_base, size_t *size_p, hoff_t *file_offset_p,
                  hoff_t *buffer_offset_p, ssize_t *bytes_written_p, hipStream_t stream)
try {
    auto result = toHipFileError(
        rocFileWriteAsync(fh, buffer_base, size_p, file_offset_p, buffer_offset_p, bytes_written_p, stream));
    if (result.err == hipFileDriverNotInitialized) {
        // Match cuFile behaviour
        rocFileEnsureDriverInitPrivate();
        result.err = hipFileInvalidValue;
    }
    return result;
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileStreamRegister(hipStream_t stream, unsigned flags)
try {
    return toHipFileError(rocFileStreamRegister(stream, flags));
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileStreamDeregister(hipStream_t stream)
try {
    return toHipFileError(rocFileStreamDeregister(stream));
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileGetBackendVersion(int *version)
try {
    unsigned major = UINT_MAX;
    unsigned minor = UINT_MAX;

    if (version == nullptr) {
        return {hipFileInvalidValue, hipSuccess};
    }

    auto result = rocFileGetVersion(&major, &minor, nullptr);
    if (result.err != rocFileSuccess) {
        return toHipFileError(result);
    }

    // This is the same algorithm as NVIDIA's
    *version = static_cast<int>((major * 1000) + (minor * 10));

    return {hipFileSuccess, hipSuccess};
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileGetParameterSizeT(hipFileSizeTConfigParameter_t param, size_t *value)
try {
    return toHipFileError(rocFileGetParameterSizeT(toRocFileSizeTConfigParameter(param), value));
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileGetParameterBool(hipFileBoolConfigParameter_t param, bool *value)
try {
    return toHipFileError(rocFileGetParameterBool(toRocFileBoolConfigParameter(param), value));
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileGetParameterString(hipFileStringConfigParameter_t param, char *desc_str, int len)
try {
    return toHipFileError(rocFileGetParameterString(toRocFileStringConfigParameter(param), desc_str, len));
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileSetParameterSizeT(hipFileSizeTConfigParameter_t param, size_t value)
try {
    return toHipFileError(rocFileSetParameterSizeT(toRocFileSizeTConfigParameter(param), value));
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileSetParameterBool(hipFileBoolConfigParameter_t param, bool value)
try {
    return toHipFileError(rocFileSetParameterBool(toRocFileBoolConfigParameter(param), value));
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileSetParameterString(hipFileStringConfigParameter_t param, const char *desc_str)
try {
    return toHipFileError(rocFileSetParameterString(toRocFileStringConfigParameter(param), desc_str));
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}
