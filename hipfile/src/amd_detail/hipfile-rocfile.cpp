/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "hipfile-rocfile.h"

#include <stdexcept>

hipFileOpError_t
toHipFileOpError(rocFileOpError_t status)
{
    switch (status) {
        case rocFileSuccess:
            return hipFileSuccess;
        case rocFileDriverNotInitialized:
            return hipFileDriverNotInitialized;
        case rocFileDriverInvalidProps:
            return hipFileDriverInvalidProps;
        case rocFileDriverUnsupportedLimit:
            return hipFileDriverUnsupportedLimit;
        case rocFileDriverVersionMismatch:
            return hipFileDriverVersionMismatch;
        case rocFileDriverVersionReadError:
            return hipFileDriverVersionReadError;
        case rocFileDriverClosing:
            return hipFileDriverClosing;
        case rocFilePlatformNotSupported:
            return hipFilePlatformNotSupported;
        case rocFileIONotSupported:
            return hipFileIONotSupported;
        case rocFileDeviceNotSupported:
            return hipFileDeviceNotSupported;
        case rocFileDriverError:
            return hipFileDriverError;
        case rocFileHipDriverError:
            return hipFileHipDriverError;
        case rocFileHipPointerInvalid:
            return hipFileHipPointerInvalid;
        case rocFileHipMemoryTypeInvalid:
            return hipFileHipMemoryTypeInvalid;
        case rocFileHipPointerRangeError:
            return hipFileHipPointerRangeError;
        case rocFileHipContextMismatch:
            return hipFileHipContextMismatch;
        case rocFileInvalidMappingSize:
            return hipFileInvalidMappingSize;
        case rocFileInvalidMappingRange:
            return hipFileInvalidMappingRange;
        case rocFileInvalidFileType:
            return hipFileInvalidFileType;
        case rocFileInvalidFileOpenFlag:
            return hipFileInvalidFileOpenFlag;
        case rocFileDIONotSet:
            return hipFileDIONotSet;
        case rocFileInvalidValue:
            return hipFileInvalidValue;
        case rocFileMemoryAlreadyRegistered:
            return hipFileMemoryAlreadyRegistered;
        case rocFileMemoryNotRegistered:
            return hipFileMemoryNotRegistered;
        case rocFilePermissionDenied:
            return hipFilePermissionDenied;
        case rocFileDriverAlreadyOpen:
            return hipFileDriverAlreadyOpen;
        case rocFileHandleNotRegistered:
            return hipFileHandleNotRegistered;
        case rocFileHandleAlreadyRegistered:
            return hipFileHandleAlreadyRegistered;
        case rocFileDeviceNotFound:
            return hipFileDeviceNotFound;
        case rocFileInternalError:
            return hipFileInternalError;
        case rocFileGetNewFDFailed:
            return hipFileGetNewFDFailed;
        case rocFileDriverSetupError:
            return hipFileDriverSetupError;
        case rocFileIODisabled:
            return hipFileIODisabled;
        case rocFileBatchSubmitFailed:
            return hipFileBatchSubmitFailed;
        case rocFileGPUMemoryPinningFailed:
            return hipFileGPUMemoryPinningFailed;
        case rocFileBatchFull:
            return hipFileBatchFull;
        case rocFileAsyncNotSupported:
            return hipFileAsyncNotSupported;
        case rocFileIOMaxError:
            return hipFileIOMaxError;
        default:
            throw std::invalid_argument("Invalid rocFileOpError_t value");
    }
}

hipFileError_t
toHipFileError(const rocFileError_t &status)
{
    return {toHipFileOpError(status.err), status.hip_drv_err};
}

rocFileFileHandleType_t
toRocFileFileHandleType(hipFileFileHandleType_t hf_type)
{
    switch (hf_type) {
        case hipFileHandleTypeOpaqueFD:
            return rocFileHandleTypeOpaqueFD;
        case hipFileHandleTypeOpaqueWin32:
            return rocFileHandleTypeOpaqueWin32;
        case hipFileHandleTypeUserspaceFS:
            return rocFileHandleTypeUserspaceFS;
        default:
            throw std::invalid_argument("Invalid hipFileFileHandleType_t value");
    }
}

hipFileDriverProps_t
toHipFileDriverProps(const rocFileDriverProps_t &rf_props)
{
    hipFileDriverProps_t hf_props;

    hf_props.nvfs.major_version         = rf_props.nvfs.major_version;
    hf_props.nvfs.minor_version         = rf_props.nvfs.minor_version;
    hf_props.nvfs.poll_thresh_size      = rf_props.nvfs.poll_thresh_size;
    hf_props.nvfs.max_direct_io_size    = rf_props.nvfs.max_direct_io_size;
    hf_props.nvfs.driver_status_flags   = rf_props.nvfs.driver_status_flags;
    hf_props.nvfs.driver_control_flags  = rf_props.nvfs.driver_control_flags;
    hf_props.feature_flags              = rf_props.feature_flags;
    hf_props.max_device_cache_size      = rf_props.max_device_cache_size;
    hf_props.per_buffer_cache_size      = rf_props.per_buffer_cache_size;
    hf_props.max_device_pinned_mem_size = rf_props.max_device_pinned_mem_size;
    hf_props.max_batch_io_count         = rf_props.max_batch_io_count;
    hf_props.max_batch_io_timeout_msecs = rf_props.max_batch_io_timeout_msecs;

    return hf_props;
}

rocFileDescr_t
toRocFileDescr(const hipFileDescr_t &fd)
{
    rocFileDescr_t rfd;

    rfd.fs_ops = reinterpret_cast<const rocFileFSOps_t *>(fd.fs_ops);

    switch (fd.type) {
        case hipFileHandleTypeOpaqueFD:
            rfd.type      = rocFileHandleTypeOpaqueFD;
            rfd.handle.fd = fd.handle.fd;
            break;
        case hipFileHandleTypeOpaqueWin32:
            rfd.type         = rocFileHandleTypeOpaqueWin32;
            rfd.handle.hFile = fd.handle.hFile;
            break;
        case hipFileHandleTypeUserspaceFS:
            rfd.type      = rocFileHandleTypeUserspaceFS;
            rfd.handle.fd = fd.handle.fd;
            break;
        default:
            throw std::invalid_argument("Invalid hipFileFileHandleType_t value");
    }

    return rfd;
}

rocFileBatchMode
toRocFileBatchMode(hipFileBatchMode_t hf_batch_mode)
{
    switch (hf_batch_mode) {
        case hipFileBatch:
            return rocFileBatch;
        default:
            throw std::invalid_argument("Invalid hipFileBatchMode_t value");
    }
}

rocFileOpcode_t
toRocFileOpcode(hipFileOpcode_t hf_opcode)
{
    switch (hf_opcode) {
        case hipFileBatchRead:
            return rocFileBatchRead;
        case hipFileBatchWrite:
            return rocFileBatchWrite;
        default:
            throw std::invalid_argument("Invalid hipFileOpcode_t value");
    }
}

rocFileIOParams_t
toRocFileIOParams(const hipFileIOParams_t &hf_io_params)
{
    rocFileIOParams_t rf_io_params;

    rf_io_params.mode                  = toRocFileBatchMode(hf_io_params.mode);
    rf_io_params.u.batch.devPtr_base   = hf_io_params.u.batch.devPtr_base;
    rf_io_params.u.batch.file_offset   = hf_io_params.u.batch.file_offset;
    rf_io_params.u.batch.devPtr_offset = hf_io_params.u.batch.devPtr_offset;
    rf_io_params.u.batch.size          = hf_io_params.u.batch.size;
    rf_io_params.fh                    = hf_io_params.fh;
    rf_io_params.opcode                = toRocFileOpcode(hf_io_params.opcode);
    rf_io_params.cookie                = hf_io_params.cookie;

    return rf_io_params;
}

hipFileStatus_t
toHipFileStatus(rocFileStatus_t rf_status)
{
    switch (rf_status) {
        case rocFileWaiting:
            return hipFileWaiting;
        case rocFilePending:
            return hipFilePending;
        case rocFileInvalid:
            return hipFileInvalid;
        case rocFileCanceled:
            return hipFileCanceled;
        case rocFileComplete:
            return hipFileComplete;
        case rocFileTimeout:
            return hipFileTimeout;
        case rocFileFailed:
            return hipFileFailed;
        default:
            throw std::invalid_argument("Invalid rocFileStatus_t value");
    }
}

hipFileIOEvents_t
toHipFileIOEvents(const rocFileIOEvents_t &rf_io_event)
{
    hipFileIOEvents_t hf_io_event;

    hf_io_event.cookie = rf_io_event.cookie;
    hf_io_event.status = toHipFileStatus(rf_io_event.status);
    hf_io_event.ret    = rf_io_event.ret;

    return hf_io_event;
}
