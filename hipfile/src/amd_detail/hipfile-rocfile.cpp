/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "hipfile-rocfile.h"

#include <stdexcept>

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
