/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "context.h"
#include "hip.h"

#include <gmock/gmock.h>

using ::testing::Invoke;

// ON_CALL is not compatible with StrictMock's
#define MOCK_PASSTHROUGH(base_class, func) \
    ON_CALL(*this, func).WillByDefault( \
        [this](auto&&... args) { \
            return this->base_class::func(std::forward<decltype(args)>(args)...); \
        } \
    )

/* mhipxx (mock hip++)
 *
 * Mock implementations for Hip. Enables unit tests to mock HIP APIs.
 */
namespace hipFile {

struct MHip : Hip {
    ContextOverride<Hip> co;
    MHip() : co{this}
    {
    }
    MOCK_METHOD(hipPointerAttribute_t, hipPointerGetAttributes, (const void *ptr), (const override));
    MOCK_METHOD(void, hipMemcpy, (void *dst, const void *src, size_t sizeBytes, hipMemcpyKind kind),
                (const, override));
    MOCK_METHOD(void, hipStreamSynchronize, (hipStream_t stream), (const, override));
    MOCK_METHOD(void *, hipHostMalloc, (size_t size, unsigned int flags), (const, override));
    MOCK_METHOD(void, hipHostFree, (void *ptr), (const, override));
    MOCK_METHOD(void *, hipHostGetDevicePointer, (void *hstPtr, unsigned int flags), (const, override));
    MOCK_METHOD(int, hipRuntimeGetVersion, (), (const, override));
    MOCK_METHOD(void *, hipGetProcAddress,
                (const char *symbol, int hipVersion, uint64_t flags,
                 hipDriverProcAddressQueryResult *symbolStatus),
                (const, override));
    MOCK_METHOD(uint64_t, hipAmdFileRead,
                (hipAmdFileHandle_t handle, void *devicePtr, uint64_t size, int64_t file_offset),
                (const, override));
    MOCK_METHOD(uint64_t, hipAmdFileWrite,
                (hipAmdFileHandle_t handle, void *devicePtr, uint64_t size, int64_t file_offset),
                (const, override));
    MOCK_METHOD(HipMemAddressRange, hipMemGetAddressRange, (hipDeviceptr_t dptr), (const, override));
    MOCK_METHOD(void, hipLaunchHostFunc, (hipStream_t stream, hipHostFn_t fn, void *user_data),
                (const, override));
    MOCK_METHOD(void, hipLaunchKernel,
                (const void *function_address, dim3 numBlocks, dim3 dimBlocks, void **args,
                 size_t sharedMemBytes, hipStream_t stream),
                (const, override));
    MOCK_METHOD(int, hipDeviceGetAttribute, (hipDeviceAttribute_t attr, int device_id), (const, override));
    MOCK_METHOD(hipDevice_t, hipStreamGetDevice, (hipStream_t stream), (const, override));

    void enable_passthrough()
    {
        MOCK_PASSTHROUGH(Hip, hipPointerGetAttributes);
        MOCK_PASSTHROUGH(Hip, hipMemcpy);
        MOCK_PASSTHROUGH(Hip, hipStreamSynchronize);
        MOCK_PASSTHROUGH(Hip, hipHostMalloc);
        MOCK_PASSTHROUGH(Hip, hipHostFree);
        MOCK_PASSTHROUGH(Hip, hipHostGetDevicePointer);
        MOCK_PASSTHROUGH(Hip, hipRuntimeGetVersion);
        MOCK_PASSTHROUGH(Hip, hipGetProcAddress);
        MOCK_PASSTHROUGH(Hip, hipAmdFileRead);
        MOCK_PASSTHROUGH(Hip, hipAmdFileWrite);
        MOCK_PASSTHROUGH(Hip, hipMemGetAddressRange);
        MOCK_PASSTHROUGH(Hip, hipLaunchHostFunc);
        MOCK_PASSTHROUGH(Hip, hipLaunchKernel);
        MOCK_PASSTHROUGH(Hip, hipDeviceGetAttribute);
        MOCK_PASSTHROUGH(Hip, hipStreamGetDevice);
    }
};

}
