/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "hip.h"
#include "io.h"
#include "context.h"

#include <cstdlib>
#include <system_error>

using namespace rocFile;
using namespace rocFile::io;
using namespace rocFile::context;

static void *
hipGetProcAddressHelper(const char *symbol)
try {
    auto hip_version{Context<Hip>::get()->hipRuntimeGetVersion()};
    return Context<Hip>::get()->hipGetProcAddress(symbol, hip_version, 0, nullptr);
}
catch (...) {
    return nullptr;
}

hipAmdFileRead_t
rocFile::getHipAmdFileReadPtr()
{
    static hipAmdFileRead_t hipAmdFileReadPtr{
        reinterpret_cast<hipAmdFileRead_t>(hipGetProcAddressHelper("hipAmdFileRead"))};
    return hipAmdFileReadPtr;
}

hipAmdFileWrite_t
rocFile::getHipAmdFileWritePtr()
{
    static hipAmdFileWrite_t hipAmdFileWritePtr{
        reinterpret_cast<hipAmdFileRead_t>(hipGetProcAddressHelper("hipAmdFileWrite"))};
    return hipAmdFileWritePtr;
}

/// Throws Hip::RuntimeError(error) if error != hipSuccess
template <typename ExceptionType>
static inline hipError_t
throwOnHipError(hipError_t error)
{
    if (hipSuccess != error) {
        throw ExceptionType(error);
    }
    return error;
}

hipPointerAttribute_t
Hip::hipPointerGetAttributes(const void *ptr) const
{
    hipPointerAttribute_t attributes;
    (void)throwOnHipError<Hip::RuntimeError>(::hipPointerGetAttributes(&attributes, ptr));
    return attributes;
}

void
Hip::hipMemcpy(void *dst, const void *src, size_t sizeBytes, hipMemcpyKind kind) const
{
    (void)throwOnHipError<Hip::RuntimeError>(::hipMemcpy(dst, src, sizeBytes, kind));
}

void
Hip::hipStreamSynchronize(hipStream_t stream) const
{
    (void)throwOnHipError<Hip::RuntimeError>(::hipStreamSynchronize(stream));
}

void *
Hip::hipHostMalloc(size_t size, unsigned int flags) const
{
    void *host_ptr;
    (void)throwOnHipError<Hip::RuntimeError>(::hipHostMalloc(&host_ptr, size, flags));
    return host_ptr;
}

void
Hip::hipHostFree(void *ptr) const
{
    (void)throwOnHipError<Hip::RuntimeError>(::hipHostFree(ptr));
}

void *
Hip::hipHostGetDevicePointer(void *hstPtr, unsigned int flags) const
{
    void *dev_ptr;
    (void)throwOnHipError<Hip::RuntimeError>(::hipHostGetDevicePointer(&dev_ptr, hstPtr, flags));
    return dev_ptr;
}

HipMemAddressRange
Hip::hipMemGetAddressRange(hipDeviceptr_t dptr) const
{
    HipMemAddressRange range;
    (void)throwOnHipError<Hip::RuntimeError>(::hipMemGetAddressRange(&range.base, &range.size, dptr));
    return range;
}

int
Hip::hipRuntimeGetVersion() const
{
    int version;
    (void)throwOnHipError<Hip::RuntimeError>(::hipRuntimeGetVersion(&version));
    return version;
}

void *
Hip::hipGetProcAddress(const char *symbol, int hipVersion, uint64_t flags,
                       hipDriverProcAddressQueryResult *symbolStatus) const
{
    void *ptr;
    (void)throwOnHipError<Hip::RuntimeError>(
        ::hipGetProcAddress(symbol, &ptr, hipVersion, flags, symbolStatus));
    return ptr;
}

uint64_t
Hip::hipAmdFileRead(hipAmdFileHandle_t handle, void *devicePtr, uint64_t size, int64_t file_offset) const
{
    static const hipAmdFileRead_t hipAmdFileReadPtr{getHipAmdFileReadPtr()};
    uint64_t                      bytes_read;
    int                           status;

    if (!hipAmdFileReadPtr) {
        throw Hip::SymbolNotFound("Could not find hipAmdFileRead()");
    }

    (void)throwOnHipError<Hip::RuntimeError>(
        (*hipAmdFileReadPtr)(handle, devicePtr, size, file_offset, &bytes_read, &status));

    if (status) {
        throw std::system_error(status, std::generic_category());
    }

    return bytes_read;
}

uint64_t
Hip::hipAmdFileWrite(hipAmdFileHandle_t handle, void *devicePtr, uint64_t size, int64_t file_offset) const
{
    static const hipAmdFileWrite_t hipAmdFileWritePtr{getHipAmdFileWritePtr()};
    uint64_t                       bytes_written;
    int32_t                        status;

    if (!hipAmdFileWritePtr) {
        throw Hip::SymbolNotFound("Could not find hipAmdFileWrite()");
    }

    (void)throwOnHipError<Hip::RuntimeError>(
        (*hipAmdFileWritePtr)(handle, devicePtr, size, file_offset, &bytes_written, &status));

    if (status) {
        throw std::system_error(status, std::generic_category());
    }

    return bytes_written;
}
