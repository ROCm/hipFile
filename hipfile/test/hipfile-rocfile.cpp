/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "hipfile.h"
#include "hipfile-rocfile.h"
#include "hipfile-warnings.h"
#include "invalid-enum.h"
#include "rocfile.h"
#include "test-common.h"

#include <gtest/gtest.h>
#include <hip/hip_runtime_api.h>
#include <memory>
#include <stdexcept>

// Put tests inside the macros to suppress the global constructor
// warnings
HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

TEST(ToHipFile, hipFileOpError_t)
{
    ASSERT_THROW(toHipFileOpError(invalidEnum<rocFileOpError_t>(hipFileSuccess - 1)), std::invalid_argument);
    ASSERT_THROW(toHipFileOpError(invalidEnum<rocFileOpError_t>(hipFileSuccess + 1)), std::invalid_argument);

    ASSERT_THROW(toHipFileOpError(invalidEnum<rocFileOpError_t>(HIPFILE_BASE_ERR)), std::invalid_argument);

    ASSERT_THROW(toHipFileOpError(invalidEnum<rocFileOpError_t>(HIPFILE_BASE_ERR + 21)),
                 std::invalid_argument);
    ASSERT_THROW(toHipFileOpError(invalidEnum<rocFileOpError_t>(HIPFILE_BASE_ERR + 32)),
                 std::invalid_argument);

#define r2h(R, H) ASSERT_EQ(toHipFileOpError((R)), (H))
    r2h(rocFileSuccess, hipFileSuccess);
    r2h(rocFileDriverNotInitialized, hipFileDriverNotInitialized);
    r2h(rocFileDriverInvalidProps, hipFileDriverInvalidProps);
    r2h(rocFileDriverUnsupportedLimit, hipFileDriverUnsupportedLimit);
    r2h(rocFileDriverVersionMismatch, hipFileDriverVersionMismatch);
    r2h(rocFileDriverVersionReadError, hipFileDriverVersionReadError);
    r2h(rocFileDriverClosing, hipFileDriverClosing);
    r2h(rocFilePlatformNotSupported, hipFilePlatformNotSupported);
    r2h(rocFileIONotSupported, hipFileIONotSupported);
    r2h(rocFileDeviceNotSupported, hipFileDeviceNotSupported);
    r2h(rocFileDriverError, hipFileDriverError);
    r2h(rocFileHipDriverError, hipFileHipDriverError);
    r2h(rocFileHipPointerInvalid, hipFileHipPointerInvalid);
    r2h(rocFileHipMemoryTypeInvalid, hipFileHipMemoryTypeInvalid);
    r2h(rocFileHipPointerRangeError, hipFileHipPointerRangeError);
    r2h(rocFileHipContextMismatch, hipFileHipContextMismatch);
    r2h(rocFileInvalidMappingSize, hipFileInvalidMappingSize);
    r2h(rocFileInvalidMappingRange, hipFileInvalidMappingRange);
    r2h(rocFileInvalidFileType, hipFileInvalidFileType);
    r2h(rocFileInvalidFileOpenFlag, hipFileInvalidFileOpenFlag);
    r2h(rocFileDIONotSet, hipFileDIONotSet);
    r2h(rocFileInvalidValue, hipFileInvalidValue);
    r2h(rocFileMemoryAlreadyRegistered, hipFileMemoryAlreadyRegistered);
    r2h(rocFileMemoryNotRegistered, hipFileMemoryNotRegistered);
    r2h(rocFilePermissionDenied, hipFilePermissionDenied);
    r2h(rocFileDriverAlreadyOpen, hipFileDriverAlreadyOpen);
    r2h(rocFileHandleNotRegistered, hipFileHandleNotRegistered);
    r2h(rocFileHandleAlreadyRegistered, hipFileHandleAlreadyRegistered);
    r2h(rocFileDeviceNotFound, hipFileDeviceNotFound);
    r2h(rocFileInternalError, hipFileInternalError);
    r2h(rocFileGetNewFDFailed, hipFileGetNewFDFailed);
    r2h(rocFileDriverSetupError, hipFileDriverSetupError);
    r2h(rocFileIODisabled, hipFileIODisabled);
    r2h(rocFileBatchSubmitFailed, hipFileBatchSubmitFailed);
    r2h(rocFileGPUMemoryPinningFailed, hipFileGPUMemoryPinningFailed);
    r2h(rocFileBatchFull, hipFileBatchFull);
    r2h(rocFileAsyncNotSupported, hipFileAsyncNotSupported);
    r2h(rocFileIOMaxError, hipFileIOMaxError);
#undef r2h

    ASSERT_THROW(toHipFileOpError(invalidEnum<rocFileOpError_t>(hipFileIOMaxError + 1)),
                 std::invalid_argument);
}

TEST(ToHipFile, hipFileError_t)
{
    ASSERT_THROW((void)toHipFileError(
                     rocFileError_t{invalidEnum<rocFileOpError_t>(rocFileIOMaxError + 1), hipSuccess}),
                 std::invalid_argument);

    ASSERT_EQ(toHipFileError(rocFileError_t{rocFileSuccess, hipSuccess}), HIPFILE_SUCCESS);
    ASSERT_EQ(toHipFileError(rocFileError_t{rocFileHipDriverError, hipErrorInvalidValue}),
              HipFileDriverError(hipErrorInvalidValue));
    ASSERT_EQ(toHipFileError(rocFileError_t{rocFileDriverNotInitialized, hipSuccess}),
              HipFileOpError(hipFileDriverNotInitialized));
}

TEST(ToHipFile, hipFileStatus_t)
{
    ASSERT_THROW(toHipFileStatus(invalidEnum<rocFileStatus_t>(rocFileWaiting - 1)), std::invalid_argument);

#define r2h(R, H) ASSERT_EQ(toHipFileStatus((R)), (H))
    r2h(rocFileWaiting, hipFileWaiting);
    r2h(rocFilePending, hipFilePending);
    r2h(rocFileInvalid, hipFileInvalid);
    r2h(rocFileCanceled, hipFileCanceled);
    r2h(rocFileComplete, hipFileComplete);
    r2h(rocFileTimeout, hipFileTimeout);
    r2h(rocFileFailed, hipFileFailed);
#undef r2h

    ASSERT_THROW(toHipFileStatus(invalidEnum<rocFileStatus_t>(rocFileFailed + 1)), std::invalid_argument);
}

TEST(ToHipFile, hipFileIOEvents_t)
{
    {
        rocFileIOEvents_t rioe;
        rioe.status = invalidEnum<rocFileStatus_t>(rocFileWaiting - 1);
        ASSERT_THROW(toHipFileIOEvents(rioe), std::invalid_argument);
    }

    {
        rocFileIOEvents_t rioe;
        rioe.status = invalidEnum<rocFileStatus_t>(rocFileFailed + 1);
        ASSERT_THROW(toHipFileIOEvents(rioe), std::invalid_argument);
    }

    {
        rocFileIOEvents_t rioe;
        rfill(static_cast<void *>(&rioe), sizeof(rioe));
        rioe.status = rocFileWaiting;

        auto hioe = toHipFileIOEvents(rioe);

        ASSERT_EQ(hioe.cookie, rioe.cookie);
        ASSERT_EQ(hioe.status, hipFileWaiting);
        ASSERT_EQ(hioe.ret, rioe.ret);
    }
}

TEST(ToRocFile, rocFileDescr_t)
{
    {
        hipFileDescr_t hfd{};
        hfd.type = invalidEnum<hipFileFileHandleType>(hipFileHandleTypeOpaqueFD - 1);
        ASSERT_THROW(toRocFileDescr(hfd), std::invalid_argument);
    }

    {
        hipFileDescr_t hfd{};
        hfd.type      = hipFileHandleTypeOpaqueFD;
        hfd.handle.fd = 0x1234;
        hfd.fs_ops    = reinterpret_cast<const hipFileFSOps_t *>(0xCAFEBABE);

        auto rfd = toRocFileDescr(hfd);
        ASSERT_EQ(rfd.type, rocFileHandleTypeOpaqueFD);
        ASSERT_EQ(rfd.handle.fd, hfd.handle.fd);
        ASSERT_EQ(static_cast<const void *>(rfd.fs_ops), static_cast<const void *>(hfd.fs_ops));
    }

    {
        hipFileDescr_t hfd{};
        hfd.type         = hipFileHandleTypeOpaqueWin32;
        hfd.handle.hFile = reinterpret_cast<void *>(0x5678);
        hfd.fs_ops       = reinterpret_cast<const hipFileFSOps_t *>(0xBADF00D);

        auto rfd = toRocFileDescr(hfd);
        ASSERT_EQ(rfd.type, rocFileHandleTypeOpaqueWin32);
        ASSERT_EQ(rfd.handle.hFile, hfd.handle.hFile);
        ASSERT_EQ(static_cast<const void *>(rfd.fs_ops), static_cast<const void *>(hfd.fs_ops));
    }

    {
        hipFileDescr_t hfd{};
        hfd.type      = hipFileHandleTypeUserspaceFS;
        hfd.handle.fd = 0x90AB;
        hfd.fs_ops    = reinterpret_cast<const hipFileFSOps_t *>(0xFACEFEED);

        auto rfd = toRocFileDescr(hfd);
        ASSERT_EQ(rfd.type, rocFileHandleTypeUserspaceFS);
        ASSERT_EQ(rfd.handle.fd, hfd.handle.fd);
        ASSERT_EQ(static_cast<const void *>(rfd.fs_ops), static_cast<const void *>(hfd.fs_ops));
    }

    {
        hipFileDescr_t hfd{};
        hfd.type = invalidEnum<hipFileFileHandleType>(hipFileHandleTypeUserspaceFS + 1);
        ASSERT_THROW(toRocFileDescr(hfd), std::invalid_argument);
    }
}

TEST(ToRocFile, rocFileFileHandleType)
{
    ASSERT_THROW(toRocFileFileHandleType(invalidEnum<hipFileFileHandleType>(hipFileHandleTypeOpaqueFD - 1)),
                 std::invalid_argument);

#define h2c(H, C) ASSERT_EQ(toRocFileFileHandleType((H)), (C))
    h2c(hipFileHandleTypeOpaqueFD, rocFileHandleTypeOpaqueFD);
    h2c(hipFileHandleTypeOpaqueWin32, rocFileHandleTypeOpaqueWin32);
    h2c(hipFileHandleTypeUserspaceFS, rocFileHandleTypeUserspaceFS);
#undef h2c

    ASSERT_THROW(
        toRocFileFileHandleType(invalidEnum<hipFileFileHandleType>(hipFileHandleTypeUserspaceFS + 1)),
        std::invalid_argument);
}

TEST(ToRocFile, rocFileBatchMode)
{
    ASSERT_THROW(toRocFileBatchMode(invalidEnum<hipFileBatchMode_t>(hipFileBatch - 1)),
                 std::invalid_argument);

    ASSERT_EQ(toRocFileBatchMode(hipFileBatch), rocFileBatch);

    ASSERT_THROW(toRocFileBatchMode(invalidEnum<hipFileBatchMode_t>(hipFileBatch + 1)),
                 std::invalid_argument);
}

TEST(ToRocFile, rocFileOpcode_t)
{
    ASSERT_THROW(toRocFileOpcode(invalidEnum<hipFileOpcode_t>(hipFileBatchRead - 1)), std::invalid_argument);

#define h2c(H, C) ASSERT_EQ(toRocFileOpcode((H)), (C))
    h2c(hipFileBatchRead, rocFileBatchRead);
    h2c(hipFileBatchWrite, rocFileBatchWrite);
#undef h2c

    ASSERT_THROW(toRocFileOpcode(invalidEnum<hipFileOpcode_t>(hipFileBatchWrite + 1)), std::invalid_argument);
}

TEST(ToRocFile, rocFileIOParams_t)
{
    {
        hipFileIOParams_t hiop{};
        hiop.mode   = invalidEnum<hipFileBatchMode_t>(hipFileBatch + 1);
        hiop.opcode = hipFileBatchWrite;

        ASSERT_THROW(toRocFileIOParams(hiop), std::invalid_argument);
    }

    {
        hipFileIOParams_t hiop{};
        hiop.mode   = hipFileBatch;
        hiop.opcode = invalidEnum<hipFileOpcode_t>(hipFileBatchWrite + 1);

        ASSERT_THROW(toRocFileIOParams(hiop), std::invalid_argument);
    }

    {
        hipFileIOParams_t hiop;

        rfill(static_cast<void *>(&hiop), sizeof(hiop));

        hiop.mode   = hipFileBatch;
        hiop.opcode = hipFileBatchWrite;

        auto ciop = toRocFileIOParams(hiop);
        ASSERT_EQ(ciop.mode, rocFileBatch);
        ASSERT_EQ(ciop.u.batch.devPtr_base, hiop.u.batch.devPtr_base);
        ASSERT_EQ(ciop.u.batch.file_offset, hiop.u.batch.file_offset);
        ASSERT_EQ(ciop.u.batch.devPtr_offset, hiop.u.batch.devPtr_offset);
        ASSERT_EQ(ciop.u.batch.size, hiop.u.batch.size);
        ASSERT_EQ(ciop.fh, ciop.fh);
        ASSERT_EQ(ciop.opcode, rocFileBatchWrite);
        ASSERT_EQ(ciop.cookie, hiop.cookie);
    }
}

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
