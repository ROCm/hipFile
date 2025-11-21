/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "buffer.h"
#include "context.h"
#include "hip.h"
#include "hipfile-warnings.h"
#include "mhip.h"
#include "rocfile.h"
#include "rocfile-test.h"
#include "state.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hip/hip_runtime_api.h>
#include <memory>
#include <stdexcept>

using namespace rocFile;

using ::testing::StrictMock;

// Put tests inside the macros to suppress the global constructor
// warnings
HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

void
expect_buffer_registration(MHip &mhip, hipMemoryType memory_type)
{
    hipPointerAttribute_t attrs;
    attrs.type = memory_type;
    HipMemAddressRange range{reinterpret_cast<void *>(0x1), UINT64_MAX - 1};
    EXPECT_CALL(mhip, hipPointerGetAttributes).WillOnce(testing::Return(attrs));
    EXPECT_CALL(mhip, hipMemGetAddressRange).WillOnce(testing::Return(range));
}

struct RocFileBuffer : public RocFileOpened {
    RocFileBuffer() : nonnull_ptr(reinterpret_cast<void *>(0x1))
    {
    }
    void *nonnull_ptr;
};

TEST_F(RocFileBuffer, register_internal_supported_hip_memory)
{
    for (const auto memoryType : SupportedHipMemoryTypes) {
        StrictMock<MHip> mhip;
        expect_buffer_registration(mhip, memoryType);
        Context<DriverState>::get()->registerBuffer(nonnull_ptr, 0, 0);
    }
}

TEST_F(RocFileBuffer, register_supported_hip_memory)
{
    for (const auto memoryType : SupportedHipMemoryTypes) {
        StrictMock<MHip> mhip;
        expect_buffer_registration(mhip, memoryType);
        ASSERT_EQ(rocFileBufRegister(nonnull_ptr, 0, 0), ROCFILE_SUCCESS);
    }
}

TEST_F(RocFileBuffer, register_internal_unsupported_hip_memory)
{
    for (const auto memoryType : UnsupportedHipMemoryTypes) {
        StrictMock<MHip>      mhip;
        hipPointerAttribute_t attrs;
        attrs.type = memoryType;
        EXPECT_CALL(mhip, hipPointerGetAttributes).WillOnce(testing::Return(attrs));
        ASSERT_THROW(Context<DriverState>::get()->registerBuffer(nonnull_ptr, 0, 0), InvalidMemoryType);
    }
}

TEST_F(RocFileBuffer, register_unsupported_hip_memory)
{
    for (const auto memoryType : UnsupportedHipMemoryTypes) {
        StrictMock<MHip>      mhip;
        hipPointerAttribute_t attrs;
        attrs.type = memoryType;
        EXPECT_CALL(mhip, hipPointerGetAttributes).WillOnce(testing::Return(attrs));
        ASSERT_EQ(rocFileBufRegister(nonnull_ptr, 0, 0), RocFileOpError(rocFileHipMemoryTypeInvalid));
    }
}

TEST_F(RocFileBuffer, register_internal_hip_pointer_get_attributes_error)
{
    StrictMock<MHip> mhip;
    EXPECT_CALL(mhip, hipPointerGetAttributes).WillOnce(testing::Throw(Hip::RuntimeError(hipErrorUnknown)));
    ASSERT_THROW(Context<DriverState>::get()->registerBuffer(nonnull_ptr, 0, 0), Hip::RuntimeError);
}

TEST_F(RocFileBuffer, register_hip_pointer_get_attributes_error)
{
    StrictMock<MHip> mhip;
    EXPECT_CALL(mhip, hipPointerGetAttributes).WillOnce(testing::Throw(Hip::RuntimeError(hipErrorUnknown)));
    ASSERT_EQ(rocFileBufRegister(nonnull_ptr, 0, 0), RocFileHipError(hipErrorUnknown));

    // hipErrorInvalidValue is handled differently to match the behaviour of cufile
    EXPECT_CALL(mhip, hipPointerGetAttributes)
        .WillOnce(testing::Throw(Hip::RuntimeError(hipErrorInvalidValue)));
    ASSERT_EQ(rocFileBufRegister(nonnull_ptr, 0, 0), RocFileOpError(rocFileInvalidValue));
}

TEST_F(RocFileBuffer, register_internal_already_registered)
{
    StrictMock<MHip> mhip;
    expect_buffer_registration(mhip, hipMemoryTypeDevice);
    ASSERT_EQ(rocFileBufRegister(nonnull_ptr, 0, 0), ROCFILE_SUCCESS);
    ASSERT_THROW(Context<DriverState>::get()->registerBuffer(nonnull_ptr, 0, 0), BufferAlreadyRegistered);
}

TEST_F(RocFileBuffer, register_already_registered)
{
    StrictMock<MHip> mhip;
    expect_buffer_registration(mhip, hipMemoryTypeDevice);
    ASSERT_EQ(rocFileBufRegister(nonnull_ptr, 0, 0), ROCFILE_SUCCESS);
    ASSERT_EQ(rocFileBufRegister(nonnull_ptr, 0, 0), RocFileOpError(rocFileMemoryAlreadyRegistered));
}

TEST_F(RocFileBuffer, registerNullPointerReturnsError)
{
    ASSERT_EQ(rocFileBufRegister(nullptr, 0x10000, 0), RocFileOpError(rocFileInvalidValue));
}

TEST_F(RocFileBuffer, registerOversizeRangeReturnsError)
{
    StrictMock<MHip>   mhip;
    HipMemAddressRange range{nonnull_ptr, 100};
    EXPECT_CALL(mhip, hipMemGetAddressRange).WillOnce(testing::Return(range));
    hipPointerAttribute_t attrs;
    attrs.type = hipMemoryTypeDevice;
    EXPECT_CALL(mhip, hipPointerGetAttributes).WillOnce(testing::Return(attrs));
    ASSERT_EQ(rocFileBufRegister(reinterpret_cast<void *>(0x1), 101, 0),
              RocFileOpError(rocFileHipPointerRangeError));
}

TEST_F(RocFileBuffer, registerHipMemGetAddressRangeThrowReturnsError)
{
    StrictMock<MHip> mhip;
    EXPECT_CALL(mhip, hipMemGetAddressRange).WillOnce(testing::Throw(Hip::RuntimeError(hipErrorNotFound)));
    hipPointerAttribute_t attrs;
    attrs.type = hipMemoryTypeDevice;
    EXPECT_CALL(mhip, hipPointerGetAttributes).WillOnce(testing::Return(attrs));
    ASSERT_EQ(rocFileBufRegister(reinterpret_cast<void *>(0x1), 100, 0), RocFileOpError(rocFileInvalidValue));
}

TEST_F(RocFileBuffer, registerOverflowingRangeReturnsError)
{
    StrictMock<MHip>   mhip;
    HipMemAddressRange range{nonnull_ptr, 100};
    EXPECT_CALL(mhip, hipMemGetAddressRange).WillOnce(testing::Return(range));
    hipPointerAttribute_t attrs;
    attrs.type = hipMemoryTypeDevice;
    EXPECT_CALL(mhip, hipPointerGetAttributes).WillOnce(testing::Return(attrs));
    ASSERT_EQ(rocFileBufRegister(reinterpret_cast<void *>(0xFFFF0000), 0x10000, 0),
              RocFileOpError(rocFileHipPointerRangeError));
}

TEST_F(RocFileBuffer, deregister_internal_not_registered)
{
    ASSERT_THROW(Context<DriverState>::get()->deregisterBuffer(nonnull_ptr), BufferNotRegistered);
}

TEST_F(RocFileBuffer, deregister_not_registered)
{
    ASSERT_EQ(rocFileBufDeregister(nonnull_ptr), RocFileOpError(rocFileMemoryNotRegistered));
}

TEST_F(RocFileBuffer, deregister_internal)
{
    StrictMock<MHip> mhip;
    expect_buffer_registration(mhip, hipMemoryTypeDevice);
    Context<DriverState>::get()->registerBuffer(nonnull_ptr, 0, 0);
    Context<DriverState>::get()->deregisterBuffer(nonnull_ptr);
}

TEST_F(RocFileBuffer, deregister)
{
    StrictMock<MHip> mhip;
    expect_buffer_registration(mhip, hipMemoryTypeDevice);
    ASSERT_EQ(rocFileBufRegister(nonnull_ptr, 0, 0), ROCFILE_SUCCESS);
    ASSERT_EQ(rocFileBufDeregister(nonnull_ptr), ROCFILE_SUCCESS);
}

TEST_F(RocFileBuffer, deregister_internal_duplicate_deregister)
{
    StrictMock<MHip> mhip;
    expect_buffer_registration(mhip, hipMemoryTypeDevice);
    Context<DriverState>::get()->registerBuffer(nonnull_ptr, 0, 0);
    Context<DriverState>::get()->deregisterBuffer(nonnull_ptr);
    ASSERT_THROW(Context<DriverState>::get()->deregisterBuffer(nonnull_ptr), BufferNotRegistered);
}

TEST_F(RocFileBuffer, deregister_duplicate_deregister)
{
    StrictMock<MHip> mhip;
    expect_buffer_registration(mhip, hipMemoryTypeDevice);
    ASSERT_EQ(rocFileBufRegister(nonnull_ptr, 0, 0), ROCFILE_SUCCESS);
    ASSERT_EQ(rocFileBufDeregister(nonnull_ptr), ROCFILE_SUCCESS);
    ASSERT_EQ(rocFileBufDeregister(nonnull_ptr), RocFileOpError(rocFileMemoryNotRegistered));
}

TEST_F(RocFileBuffer, deregister_internal_get_prevents_deregister)
{
    StrictMock<MHip> mhip;
    expect_buffer_registration(mhip, hipMemoryTypeDevice);
    Context<DriverState>::get()->registerBuffer(nonnull_ptr, 0, 0);
    {
        auto buffer = Context<DriverState>::get()->getBuffer(nonnull_ptr);
        ASSERT_THROW(Context<DriverState>::get()->deregisterBuffer(nonnull_ptr), BufferOperationsOutstanding);
    }
    Context<DriverState>::get()->deregisterBuffer(nonnull_ptr);
}

TEST_F(RocFileBuffer, deregister_get_prevents_deregister)
{
    StrictMock<MHip> mhip;
    expect_buffer_registration(mhip, hipMemoryTypeDevice);
    ASSERT_EQ(rocFileBufRegister(nonnull_ptr, 0, 0), ROCFILE_SUCCESS);
    {
        auto buffer = Context<DriverState>::get()->getBuffer(nonnull_ptr);
        ASSERT_EQ(rocFileBufDeregister(nonnull_ptr), RocFileOpError(rocFileInternalError));
    }
    ASSERT_EQ(rocFileBufDeregister(nonnull_ptr), ROCFILE_SUCCESS);
}

TEST_F(RocFileBuffer, get_not_registered)
{
    ASSERT_THROW(Context<DriverState>::get()->getBuffer(nonnull_ptr), BufferNotRegistered);
}

TEST_F(RocFileBuffer, get_internal_after_register)
{
    StrictMock<MHip> mhip;
    expect_buffer_registration(mhip, hipMemoryTypeDevice);
    Context<DriverState>::get()->registerBuffer(nonnull_ptr, 0, 0);
    auto buffer = Context<DriverState>::get()->getBuffer(nonnull_ptr);
}

TEST_F(RocFileBuffer, get_after_register)
{
    StrictMock<MHip> mhip;
    expect_buffer_registration(mhip, hipMemoryTypeDevice);
    ASSERT_EQ(rocFileBufRegister(nonnull_ptr, 0, 0), ROCFILE_SUCCESS);
    auto buffer = Context<DriverState>::get()->getBuffer(nonnull_ptr);
}

TEST_F(RocFileBuffer, get_internal_after_deregister)
{
    StrictMock<MHip> mhip;
    expect_buffer_registration(mhip, hipMemoryTypeDevice);
    Context<DriverState>::get()->registerBuffer(nonnull_ptr, 0, 0);
    Context<DriverState>::get()->deregisterBuffer(nonnull_ptr);
    ASSERT_THROW(Context<DriverState>::get()->getBuffer(nonnull_ptr), BufferNotRegistered);
}

TEST_F(RocFileBuffer, get_after_deregister)
{
    StrictMock<MHip> mhip;
    expect_buffer_registration(mhip, hipMemoryTypeDevice);
    ASSERT_EQ(rocFileBufRegister(nonnull_ptr, 0, 0), ROCFILE_SUCCESS);
    ASSERT_EQ(rocFileBufDeregister(nonnull_ptr), ROCFILE_SUCCESS);
    ASSERT_THROW(Context<DriverState>::get()->getBuffer(nonnull_ptr), BufferNotRegistered);
}

TEST_F(RocFileBuffer, get_buffer_makes_temporary_buffer)
{
    StrictMock<MHip> mhip;
    expect_buffer_registration(mhip, hipMemoryTypeDevice);
    auto buffer = Context<DriverState>::get()->getBuffer(nonnull_ptr, 0, 0);
    ASSERT_EQ(buffer.use_count(), 1);
}

TEST_F(RocFileBuffer, get_buffer_returns_registered_buffer)
{
    StrictMock<MHip> mhip;
    expect_buffer_registration(mhip, hipMemoryTypeDevice);
    Context<DriverState>::get()->registerBuffer(nonnull_ptr, 0, 0);
    ASSERT_EQ(Context<DriverState>::get()->getBuffer(nonnull_ptr, 0, 0),
              Context<DriverState>::get()->getBuffer(nonnull_ptr));
}

TEST_F(RocFileBuffer, get_buffer_throws_if_length_larger_than_registered_length)
{
    StrictMock<MHip> mhip;
    size_t           buffer_length = 0;
    expect_buffer_registration(mhip, hipMemoryTypeDevice);
    Context<DriverState>::get()->registerBuffer(nonnull_ptr, buffer_length, 0);
    ASSERT_THROW(Context<DriverState>::get()->getBuffer(nonnull_ptr, buffer_length + 1, 0),
                 std::invalid_argument);
}

TEST_F(RocFileBuffer, get_buffer_throws_on_getPointerAttributes_error)
{
    StrictMock<MHip> mhip;
    EXPECT_CALL(mhip, hipPointerGetAttributes).WillOnce(testing::Throw(Hip::RuntimeError(hipErrorUnknown)));
    ASSERT_THROW(Context<DriverState>::get()->getBuffer(nonnull_ptr, 1, 0), Hip::RuntimeError);
}

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
