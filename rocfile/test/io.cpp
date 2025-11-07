/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "backend.h"
#include "backend/fallback.h"
#include "buffer.h"
#include "context.h"
#include "file.h"
#include "hip.h"
#include "hipfile-types.h"
#include "hipfile-warnings.h"
#include "io.h"
#include "mbackend.h"
#include "mbuffer.h"
#include "mfile.h"
#include "mhip.h"
#include "mstate.h"
#include "msys.h"
#include "rocfile.h"
#include "rocfile-test.h"
#include "state.h"
#include "sys.h"

#include <array>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hip/hip_runtime_api.h>
#include <hip/driver_types.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>

using namespace rocFile;
using namespace testing;

using std::shared_ptr;
using ::testing::Return;
using ::testing::StrictMock;

// Put tests inside the macros to suppress the global constructor
// warnings
HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

// Fills vector with random data
static void
rand_fill(std::vector<uint8_t> &v)
{
    // *Quickly* fill the vector with data. Reading from /dev/urandom is
    // faster than C++'s prngs and C's rand.
    auto fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) {
        throw std::runtime_error("Can't open /dev/urandom");
    }
    size_t total_bytes_read = 0;
    while (total_bytes_read < v.size()) {
        auto bytes_read = read(fd, v.data() + total_bytes_read, v.size() - total_bytes_read);
        if (bytes_read == -1) {
            throw std::runtime_error("Can't read from /dev/urandom");
        }
        total_bytes_read += static_cast<size_t>(bytes_read);
    }
    if (close(fd) == -1) {
        throw std::runtime_error("Can't close /dev/urandom");
    }
}

// Checks that count bytes in buffer starting at buffer_offset, match count
// bytes in expected starting at expected offset. Also ensure that all other
// bytes in buffer are zero.
static bool
contains_expected_data(std::vector<uint8_t> &buffer, hoff_t buffer_offset, std::vector<uint8_t> &expected,
                       hoff_t expected_offset, size_t count)
{
    if (buffer_offset < 0 || buffer.size() < static_cast<size_t>(buffer_offset) + count) {
        throw std::invalid_argument("out of bounds: buffer");
    }

    if (expected_offset < 0 || expected.size() < static_cast<size_t>(expected_offset) + count) {
        throw std::invalid_argument("out of bounds: expected");
    }

    for (hoff_t i = 0; i < buffer_offset; i++) {
        if (buffer.data()[i] != 0) {
            return false;
        }
    }

    if (memcmp(buffer.data() + buffer_offset, expected.data() + expected_offset, count)) {
        return false;
    }

    for (size_t i = static_cast<size_t>(buffer_offset) + count; i < buffer.size(); i++) {
        if (buffer.data()[i] != 0) {
            return false;
        }
    }

    return true;
}

struct RocFileIO : public RocFileOpened {

    shared_ptr<IBuffer>  buffer;
    std::vector<uint8_t> buffer_data;
    shared_ptr<IFile>    file;
    std::vector<uint8_t> file_data;

    RocFileIO() : buffer_data(1024 * 1024)
    {
        StrictMock<MHip> mhip;
        StrictMock<MSys> msys;

        expect_buffer_registration(mhip, hipMemoryTypeDevice);
        Context<DriverState>::get()->registerBuffer(buffer_data.data(), buffer_data.size(), 0);
        buffer = Context<DriverState>::get()->getBuffer(buffer_data.data());

        EXPECT_CALL(msys, fstat);
        EXPECT_CALL(msys, fcntl);
        file = Context<DriverState>::get()->getFile(Context<DriverState>::get()->registerFile(0xBADF00D));
    }

    virtual ~RocFileIO() override
    {
        buffer.reset();
        file.reset();
    }

    hipError_t fake_hipMemcpy(void *dst, const void *src, size_t count, hipMemcpyKind kind)
    {
        (void)kind;
        memcpy(dst, src, count);
        return hipSuccess;
    }
};

TEST(RocFileFallbackBackend, FallbackBackendIsBarelyWillingToHandleDeviceMemory)
{
    auto   mfile{std::make_shared<StrictMock<MFile>>()};
    auto   mbuffer{std::make_shared<StrictMock<MBuffer>>()};
    size_t io_size{2048};
    hoff_t file_offset{4096};
    hoff_t buffer_offset{1024};

    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(hipMemoryTypeDevice));

    ASSERT_EQ(Fallback().score(mfile, mbuffer, io_size, file_offset, buffer_offset), 0);
}

TEST(RocFileFallbackBackend, FallbackBackendRejectsUnsupportedHipMemoryTypes)
{
    auto   mfile{std::make_shared<StrictMock<MFile>>()};
    size_t io_size{2048};
    hoff_t file_offset{4096};
    hoff_t buffer_offset{1024};

    for (const auto memoryType : UnsupportedHipMemoryTypes) {
        auto mbuffer = std::make_shared<StrictMock<MBuffer>>();
        EXPECT_CALL(*mbuffer, getType).WillOnce(Return(memoryType));
        ASSERT_EQ(Fallback().score(mfile, mbuffer, io_size, file_offset, buffer_offset), -1);
    }
}

struct RocFileFallbackValidation : ::testing::TestWithParam<IoType> {

    shared_ptr<IBuffer> buffer;
    shared_ptr<IFile>   file;

    RocFileFallbackValidation()
    {
        StrictMock<MHip> mhip;
        StrictMock<MSys> msys;

        assert(rocFileDriverOpen() == ROCFILE_SUCCESS);

        expect_buffer_registration(mhip, hipMemoryTypeDevice);
        void *buf = reinterpret_cast<void *>(0xFEFEFEFE);
        Context<DriverState>::get()->registerBuffer(buf, 4096, 0);
        buffer = Context<DriverState>::get()->getBuffer(buf);

        EXPECT_CALL(msys, fstat);
        EXPECT_CALL(msys, fcntl);
        file = Context<DriverState>::get()->getFile(Context<DriverState>::get()->registerFile(0xBADF00D));
    }

    ~RocFileFallbackValidation() override
    {
        // Drop the references to the file & buffer so that they can be
        // deregistered in rocFileDriverClose()
        file.reset();
        buffer.reset();

        while (rocFileUseCount()) {
            assert(rocFileDriverClose() == ROCFILE_SUCCESS);
        }
    }

protected:
    void SetUp() override
    {
        io_type = GetParam();
    }

    IoType io_type;
};

TEST_P(RocFileFallbackValidation, fallback_io_throws_on_negative_buffer_offset)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;
    ASSERT_THROW(Fallback().io(io_type, file, buffer, 0, 0, -1, 4096), std::invalid_argument);
}

TEST_P(RocFileFallbackValidation, fallback_io_throws_if_buffer_offset_is_out_of_bounds)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;
    hoff_t           buffer_offset = static_cast<hoff_t>(buffer->getLength());
    ASSERT_THROW(Fallback().io(io_type, file, buffer, 0, 0, buffer_offset, 4096), std::invalid_argument);
}

TEST_P(RocFileFallbackValidation, fallback_io_throws_if_op_could_overrun_buffer)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;
    size_t           size          = 10;
    hoff_t           buffer_offset = static_cast<hoff_t>(buffer->getLength()) - 9;
    ASSERT_THROW(Fallback().io(io_type, file, buffer, size, 0, buffer_offset, 4096), std::invalid_argument);
}

TEST_P(RocFileFallbackValidation, fallback_io_throws_on_negative_file_offset)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;
    ASSERT_THROW(Fallback().io(io_type, file, buffer, 0, -1, 0, 4096), std::invalid_argument);
}

TEST_P(RocFileFallbackValidation, fallback_io_truncates_size_to_MAX_RW_COUNT)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    expect_buffer_registration(mhip, hipMemoryTypeDevice);
    auto buf = reinterpret_cast<void *>(0xABABABAB);
    Context<DriverState>::get()->registerBuffer(buf, MAX_RW_COUNT + 1, 0);
    auto big_buffer = Context<DriverState>::get()->getBuffer(buf);

    EXPECT_CALL(msys, mmap).WillOnce(testing::Return(reinterpret_cast<void *>(0xFEFEFEFE)));
    switch (io_type) {
        case IoType::Read:
            EXPECT_CALL(msys, pread)
                .WillRepeatedly(testing::Invoke([](int, void *, size_t count, hoff_t) -> ssize_t {
                    return static_cast<ssize_t>(count);
                }));
            EXPECT_CALL(mhip, hipMemcpy).WillRepeatedly(testing::Return());
            break;
        case IoType::Write:
            EXPECT_CALL(mhip, hipMemcpy).WillRepeatedly(testing::Return());
            EXPECT_CALL(mhip, hipStreamSynchronize).WillRepeatedly(testing::Return());
            EXPECT_CALL(msys, pwrite)
                .WillRepeatedly(testing::Invoke([](int, void *, size_t count, hoff_t) -> ssize_t {
                    return static_cast<ssize_t>(count);
                }));
            break;
        default:
            FAIL();
    }

    EXPECT_CALL(msys, munmap);
    ASSERT_EQ(MAX_RW_COUNT, Fallback().io(io_type, file, big_buffer, SIZE_MAX, 0, 0, 16 * 1024 * 1024));
}

TEST_P(RocFileFallbackValidation, fallback_io_throws_on_bounce_buffer_allocation_failure)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;
    EXPECT_CALL(msys, mmap).WillOnce(testing::Throw(Sys::RuntimeError()));
    ASSERT_THROW(Fallback().io(io_type, file, buffer, 4096, 0, 0, 4096), Sys::RuntimeError);
}

TEST_P(RocFileFallbackValidation, fallback_io_allocates_chunk_sized_host_bounce_buffer)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;
    size_t           chunk_size = 1024 * 1024;
    auto             ptr        = reinterpret_cast<void *>(0xFEFEFEFE);
    EXPECT_CALL(msys, mmap(testing::_, chunk_size, testing::_, testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(ptr));
    switch (io_type) {
        case IoType::Read:
            EXPECT_CALL(msys, pread).WillOnce(testing::Return(0));
            break;
        case IoType::Write:
            EXPECT_CALL(mhip, hipMemcpy);
            EXPECT_CALL(mhip, hipStreamSynchronize);
            EXPECT_CALL(msys, pwrite).WillOnce(testing::Return(0));
            break;
        default:
            FAIL();
    }
    EXPECT_CALL(msys, munmap(ptr, chunk_size));
    ASSERT_EQ(0, Fallback().io(io_type, file, buffer, 4096, 0, 0, chunk_size));
}

INSTANTIATE_TEST_SUITE_P(FallbackValidationTests, RocFileFallbackValidation,
                         ::testing::Values(IoType::Read, IoType::Write));

struct RocFileWrite : public RocFileIO {

    ssize_t fake_pwrite(int fd, void *buf, size_t count, hoff_t offset)
    {
        (void)fd;

        if (offset < 0) {
            return -1;
        }

        if (count >= static_cast<size_t>(SSIZE_MAX) + 1) {
            return -1;
        }

        auto uoffset = static_cast<size_t>(offset);
        if (file_data.size() < uoffset + count) {
            file_data.resize(uoffset + count);
        }

        memcpy(file_data.data() + uoffset, buf, count);

        return static_cast<ssize_t>(count);
    }

    void expect_fallback_write(MHip &mhip, MSys &msys)
    {
        EXPECT_CALL(msys, mmap).WillOnce(testing::Invoke(::mmap));
        EXPECT_CALL(mhip, hipMemcpy).WillRepeatedly(testing::Invoke(this, &RocFileWrite::fake_hipMemcpy));
        EXPECT_CALL(mhip, hipStreamSynchronize).WillRepeatedly(testing::Return());
        EXPECT_CALL(msys, pwrite).WillRepeatedly(testing::Invoke(this, &RocFileWrite::fake_pwrite));
        EXPECT_CALL(msys, munmap).WillOnce(testing::Invoke(::munmap));
    }

    // Test if the file contains the correct data
    bool file_contains_expected_data(hoff_t file_offset, hoff_t buffer_offset, size_t count)
    {
        return contains_expected_data(file_data, file_offset, buffer_data, buffer_offset, count);
    }

    void randomize_device_buffer()
    {
        rand_fill(buffer_data);
    }

    void *nonnull_ptr = reinterpret_cast<void *>(0x1);
};

TEST_F(RocFileWrite, write_handles_pointer_get_attributes_error)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;
    EXPECT_CALL(msys, fstat);
    EXPECT_CALL(msys, fcntl);
    auto fh{Context<DriverState>::get()->registerFile(0xBADCAFE)};
    EXPECT_CALL(mhip, hipPointerGetAttributes).WillOnce(testing::Throw(Hip::RuntimeError(hipErrorUnknown)));
    ASSERT_EQ(rocFileWrite(fh, nonnull_ptr, 0, 0, 0), -static_cast<ssize_t>(hipErrorUnknown));
}

TEST_F(RocFileWrite, write_handles_unsupported_hip_memory_type)
{
    StrictMock<MSys> msys;
    EXPECT_CALL(msys, fstat);
    EXPECT_CALL(msys, fcntl);
    auto fh = Context<DriverState>::get()->registerFile(0xBADCAFE);
    for (const auto memoryType : UnsupportedHipMemoryTypes) {
        StrictMock<MHip>      mhip;
        hipPointerAttribute_t attrs;
        attrs.type = memoryType;
        EXPECT_CALL(mhip, hipPointerGetAttributes).WillOnce(testing::Return(attrs));
        ASSERT_EQ(rocFileWrite(fh, nonnull_ptr, 0, 0, 0), -static_cast<ssize_t>(rocFileHipMemoryTypeInvalid));
    }
}

TEST_F(RocFileWrite, write_handles_invalid_length_of_registered_buffer)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;
    size_t           buffer_length = 0;
    expect_buffer_registration(mhip, hipMemoryTypeDevice);
    Context<DriverState>::get()->registerBuffer(nonnull_ptr, buffer_length, 0);
    EXPECT_CALL(msys, fstat);
    EXPECT_CALL(msys, fcntl);
    auto fh{Context<DriverState>::get()->registerFile(0xBADCAFE)};
    ASSERT_EQ(rocFileWrite(fh, nonnull_ptr, buffer_length + 1, 0, 0),
              -static_cast<ssize_t>(rocFileInvalidValue));
}

TEST_F(RocFileWrite, write_rejects_invalid_file_handle)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;
    auto             fh = reinterpret_cast<rocFileHandle_t>(0xdeadbeef);
    ASSERT_EQ(rocFileWrite(fh, nonnull_ptr, 0, 0, 0), -rocFileHandleNotRegistered);
}

TEST_F(RocFileWrite, write_handles_mmap_error)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    EXPECT_CALL(msys, mmap).WillOnce(testing::Throw(Sys::RuntimeError()));
    ASSERT_EQ(rocFileWrite(file->getHandle(), buffer->getBuffer(), 4096, 0, 0), -1);
}

TEST_F(RocFileWrite, write_handles_hipMemcpy_error)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    EXPECT_CALL(msys, mmap).WillOnce(testing::Invoke(::mmap));
    EXPECT_CALL(mhip, hipMemcpy).WillOnce(testing::Throw(Hip::RuntimeError(hipErrorUnknown)));
    EXPECT_CALL(msys, munmap).WillOnce(testing::Invoke(::munmap));
    ASSERT_EQ(rocFileWrite(file->getHandle(), buffer->getBuffer(), buffer->getLength(), 0, 0),
              -hipErrorUnknown);
}

TEST_F(RocFileWrite, write_handles_hipstreamsynchronize_error)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    EXPECT_CALL(msys, mmap).WillOnce(testing::Invoke(::mmap));
    EXPECT_CALL(mhip, hipMemcpy);
    EXPECT_CALL(mhip, hipStreamSynchronize).WillOnce(testing::Throw(Hip::RuntimeError(hipErrorUnknown)));
    EXPECT_CALL(msys, munmap).WillOnce(testing::Invoke(::munmap));
    ASSERT_EQ(rocFileWrite(file->getHandle(), buffer->getBuffer(), 4096, 0, 0), -hipErrorUnknown);
}

TEST_F(RocFileWrite, write_handles_pwrite_error)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    const int pwrite_error = EBADF;

    EXPECT_CALL(msys, mmap).WillOnce(testing::Invoke(::mmap));
    EXPECT_CALL(mhip, hipMemcpy).WillOnce(testing::Invoke(this, &RocFileWrite::fake_hipMemcpy));
    EXPECT_CALL(mhip, hipStreamSynchronize);
    EXPECT_CALL(msys, pwrite).WillOnce(testing::Throw(Sys::RuntimeError(pwrite_error)));
    EXPECT_CALL(msys, munmap).WillOnce(testing::Invoke(::munmap));
    ASSERT_EQ(rocFileWrite(file->getHandle(), buffer->getBuffer(), 4096, 0, 0), -1);
    ASSERT_EQ(errno, pwrite_error);
}

TEST_F(RocFileWrite, write_with_fallback_backend)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    randomize_device_buffer();
    size_t size = buffer->getLength();

    expect_fallback_write(mhip, msys);
    ASSERT_EQ(rocFileWrite(file->getHandle(), buffer->getBuffer(), buffer->getLength(), 0, 0),
              buffer->getLength());
    ASSERT_TRUE(file_contains_expected_data(0, 0, size));
}

TEST_F(RocFileWrite, fallback_write_handles_zero_sized_write)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    ASSERT_EQ(0, Fallback().io(IoType::Write, file, buffer, 0, 0, 0));
}

TEST_F(RocFileWrite, fallback_write_throws_on_pwrite_exception)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    EXPECT_CALL(msys, mmap).WillOnce(testing::Invoke(::mmap));
    EXPECT_CALL(mhip, hipMemcpy);
    EXPECT_CALL(mhip, hipStreamSynchronize);
    EXPECT_CALL(msys, pwrite).WillOnce(testing::Throw(Sys::RuntimeError()));
    EXPECT_CALL(msys, munmap).WillOnce(testing::Invoke(::munmap));

    ASSERT_THROW(Fallback().io(IoType::Write, file, buffer, buffer->getLength(), 0, 0), Sys::RuntimeError);
}

TEST_F(RocFileWrite, fallback_write_throws_on_hipmemcpy_failure)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    EXPECT_CALL(msys, mmap).WillOnce(testing::Invoke(::mmap));
    EXPECT_CALL(mhip, hipMemcpy).WillOnce(testing::Throw(Hip::RuntimeError(hipErrorUnknown)));
    EXPECT_CALL(msys, munmap).WillOnce(testing::Invoke(::munmap));

    ASSERT_THROW(Fallback().io(IoType::Write, file, buffer, buffer->getLength(), 0, 0), Hip::RuntimeError);
}

TEST_F(RocFileWrite, fallback_write_throws_on_hipstreamsynchronize_error)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    EXPECT_CALL(msys, mmap).WillOnce(testing::Invoke(::mmap));
    EXPECT_CALL(mhip, hipMemcpy);
    EXPECT_CALL(mhip, hipStreamSynchronize).WillOnce(testing::Throw(Hip::RuntimeError(hipErrorUnknown)));
    EXPECT_CALL(msys, munmap).WillOnce(testing::Invoke(::munmap));

    ASSERT_THROW(Fallback().io(IoType::Write, file, buffer, buffer->getLength(), 0, 0), Hip::RuntimeError);
}

TEST_F(RocFileWrite, fallback_write_to_empty_file)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t size       = 64 * 1024;
    size_t chunk_size = 4096;

    randomize_device_buffer();
    expect_fallback_write(mhip, msys);

    ASSERT_EQ(size, Fallback().io(IoType::Write, file, buffer, size, 0, 0, chunk_size));
    ASSERT_TRUE(file_contains_expected_data(0, 0, size));
}

TEST_F(RocFileWrite, fallback_write_to_empty_file_at_file_offset)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t size        = 64 * 1024;
    size_t chunk_size  = 4096;
    hoff_t file_offset = 1024;

    randomize_device_buffer();
    expect_fallback_write(mhip, msys);

    ASSERT_EQ(size, Fallback().io(IoType::Write, file, buffer, size, file_offset, 0, chunk_size));
    ASSERT_TRUE(file_contains_expected_data(file_offset, 0, size));
}

TEST_F(RocFileWrite, fallback_write_to_empty_file_at_buffer_offset)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t size          = 64 * 1024;
    size_t chunk_size    = 4096;
    hoff_t buffer_offset = 1024;

    randomize_device_buffer();
    expect_fallback_write(mhip, msys);

    ASSERT_EQ(size, Fallback().io(IoType::Write, file, buffer, size, 0, buffer_offset, chunk_size));
    ASSERT_TRUE(file_contains_expected_data(0, buffer_offset, size));
}

TEST_F(RocFileWrite, fallback_write_to_empty_file_at_buffer_offset_file_offset)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t size          = 64 * 1024;
    size_t chunk_size    = 4096;
    hoff_t buffer_offset = 1024;
    hoff_t file_offset   = 512;

    randomize_device_buffer();
    expect_fallback_write(mhip, msys);

    ASSERT_EQ(size, Fallback().io(IoType::Write, file, buffer, size, file_offset, buffer_offset, chunk_size));
    ASSERT_TRUE(file_contains_expected_data(file_offset, buffer_offset, size));
}

TEST_F(RocFileWrite, fallback_write_overwite_entire_file)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    file_data.resize(buffer->getLength());
    randomize_device_buffer();
    expect_fallback_write(mhip, msys);

    ASSERT_EQ(buffer->getLength(), Fallback().io(IoType::Write, file, buffer, buffer->getLength(), 0, 0));
    ASSERT_TRUE(file_contains_expected_data(0, 0, file_data.size()));
}

TEST_F(RocFileWrite, fallback_write_to_file_subregion)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t file_length = buffer->getLength() * 2;
    hoff_t file_offset = buffer->getLength() / 2;
    file_data.resize(file_length);
    randomize_device_buffer();
    expect_fallback_write(mhip, msys);

    ASSERT_EQ(buffer->getLength(),
              Fallback().io(IoType::Write, file, buffer, buffer->getLength(), file_offset, 0));
    ASSERT_TRUE(file_contains_expected_data(file_offset, 0, buffer->getLength()));
}

TEST_F(RocFileWrite, fallback_write_append_non_empty_small_file)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    file_data.resize(64);
    size_t size        = 64 * 1024;
    hoff_t file_offset = 64;

    randomize_device_buffer();
    expect_fallback_write(mhip, msys);

    ASSERT_EQ(size, Fallback().io(IoType::Write, file, buffer, size, file_offset, 0));
    ASSERT_TRUE(file_contains_expected_data(file_offset, 0, size));
}

struct RocFileRead : public RocFileIO {

    void init_file(size_t length)
    {
        auto file_data_size_before = file_data.size();
        file_data.resize(length);

        if (file_data.size() <= file_data_size_before) {
            return;
        }

        rand_fill(file_data);
    }

    ssize_t fake_pread(int fd, void *buf, size_t count, hoff_t offset)
    {
        (void)fd;

        if (offset < 0) {
            return -1;
        }

        auto uoffset = static_cast<size_t>(offset);

        if (count >= static_cast<size_t>(SSIZE_MAX) + 1) {
            return -1;
        }

        if (file_data.size() < uoffset) {
            return 0;
        }

        if (file_data.size() < uoffset + count) {
            count = file_data.size() - uoffset;
        }

        memcpy(buf, file_data.data() + uoffset, count);

        return static_cast<ssize_t>(count);
    }

    bool device_buffer_contains_expected_data(hoff_t file_offset, hoff_t buffer_offset, size_t count)
    {
        return contains_expected_data(buffer_data, buffer_offset, file_data, file_offset, count);
    }

    void expect_fallback_read(MHip &mhip, MSys &msys)
    {
        EXPECT_CALL(msys, mmap).WillOnce(testing::Invoke(::mmap));
        EXPECT_CALL(msys, pread).WillRepeatedly(testing::Invoke(this, &RocFileRead::fake_pread));
        EXPECT_CALL(mhip, hipMemcpy).WillRepeatedly(testing::Invoke(this, &RocFileRead::fake_hipMemcpy));
        EXPECT_CALL(msys, munmap).WillOnce(testing::Invoke(::munmap));
    }

    void *nonnull_ptr = reinterpret_cast<void *>(0x1);
};

TEST_F(RocFileRead, read_handles_pointer_get_attributes_error)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;
    EXPECT_CALL(msys, fstat);
    EXPECT_CALL(msys, fcntl);
    auto fh{Context<DriverState>::get()->registerFile(0xBADCAFE)};
    EXPECT_CALL(mhip, hipPointerGetAttributes).WillOnce(testing::Throw(Hip::RuntimeError(hipErrorUnknown)));
    ASSERT_EQ(rocFileRead(fh, nonnull_ptr, 0, 0, 0), -static_cast<ssize_t>(hipErrorUnknown));
}

TEST_F(RocFileRead, read_handles_unsupported_hip_memory_type)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;
    EXPECT_CALL(msys, fstat);
    EXPECT_CALL(msys, fcntl);
    auto fh{Context<DriverState>::get()->registerFile(0xBADCAFE)};
    for (const auto memoryType : UnsupportedHipMemoryTypes) {
        hipPointerAttribute_t attrs;
        attrs.type = memoryType;
        EXPECT_CALL(mhip, hipPointerGetAttributes).WillOnce(testing::Return(attrs));
        ASSERT_EQ(rocFileRead(fh, nonnull_ptr, 0, 0, 0), -static_cast<ssize_t>(rocFileHipMemoryTypeInvalid));
    }
}

TEST_F(RocFileRead, read_handles_invalid_length_of_registered_buffer)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;
    EXPECT_CALL(msys, fstat);
    EXPECT_CALL(msys, fcntl);
    auto   fh{Context<DriverState>::get()->registerFile(0xBADCAFE)};
    size_t buffer_length{0};
    expect_buffer_registration(mhip, hipMemoryTypeDevice);
    Context<DriverState>::get()->registerBuffer(nonnull_ptr, buffer_length, 0);
    ASSERT_EQ(rocFileRead(fh, nonnull_ptr, buffer_length + 1, 0, 0),
              -static_cast<ssize_t>(rocFileInvalidValue));
}

TEST_F(RocFileRead, read_rejects_invalid_file_handle)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;
    auto             fh = reinterpret_cast<rocFileHandle_t>(0xdeadbeef);
    ASSERT_EQ(rocFileRead(fh, nullptr, 0, 0, 0), -rocFileHandleNotRegistered);
}

TEST_F(RocFileRead, read_handles_invalid_argument)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    // Negative file offset
    ASSERT_EQ(rocFileRead(file->getHandle(), buffer->getBuffer(), 4096, -1, 0), -rocFileInvalidValue);
    // Negative buffer offset
    ASSERT_EQ(rocFileRead(file->getHandle(), buffer->getBuffer(), 4096, 0, -1), -rocFileInvalidValue);
    // Buffer overrun
    ASSERT_EQ(rocFileRead(file->getHandle(), buffer->getBuffer(), buffer->getLength(), 0, 1),
              -rocFileInvalidValue);
}

TEST_F(RocFileRead, fallback_read_handles_zero_sized_read)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;
    ASSERT_EQ(0, Fallback().io(IoType::Read, file, buffer, 0, 0, 0));
}

TEST_F(RocFileRead, read_handles_mmap_error)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    EXPECT_CALL(msys, mmap).WillOnce(testing::Throw(Sys::RuntimeError()));
    ASSERT_EQ(rocFileRead(file->getHandle(), buffer->getBuffer(), 4096, 0, 0), -1);
}

TEST_F(RocFileRead, read_handles_pread_error)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    const int pread_error = EBADF;

    EXPECT_CALL(msys, mmap).WillOnce(testing::Invoke(::mmap));
    EXPECT_CALL(msys, pread).WillOnce(testing::Throw(Sys::RuntimeError(pread_error)));
    EXPECT_CALL(msys, munmap).WillOnce(testing::Invoke(::munmap));
    ASSERT_EQ(rocFileRead(file->getHandle(), buffer->getBuffer(), 4096, 0, 0), -1);
    ASSERT_EQ(errno, pread_error);
}

TEST_F(RocFileRead, read_handles_hipMemcpy_error)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t file_length = buffer->getLength();
    init_file(file_length);

    EXPECT_CALL(msys, mmap).WillOnce(testing::Invoke(::mmap));
    EXPECT_CALL(msys, pread).WillRepeatedly(testing::Invoke(this, &RocFileRead::fake_pread));
    EXPECT_CALL(mhip, hipMemcpy).WillOnce(testing::Throw(Hip::RuntimeError(hipErrorUnknown)));
    EXPECT_CALL(msys, munmap).WillOnce(testing::Invoke(::munmap));
    ASSERT_EQ(rocFileRead(file->getHandle(), buffer->getBuffer(), buffer->getLength(), 0, 0),
              -hipErrorUnknown);
}

TEST_F(RocFileRead, read_with_fallback_backend)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t file_length = buffer->getLength() * 3;
    init_file(file_length);

    size_t size          = buffer->getLength() / 2;
    hoff_t buffer_offset = buffer->getLength() / 4;
    hoff_t file_offset   = static_cast<hoff_t>(buffer->getLength());

    expect_fallback_read(mhip, msys);
    ASSERT_EQ(rocFileRead(file->getHandle(), buffer->getBuffer(), size, file_offset, buffer_offset), size);
    ASSERT_TRUE(device_buffer_contains_expected_data(file_offset, buffer_offset, size));
}

TEST_F(RocFileRead, fallback_read_throws_on_pread_exception)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;
    EXPECT_CALL(msys, mmap).WillOnce(testing::Invoke(::mmap));
    EXPECT_CALL(msys, pread).WillOnce(testing::Throw(Sys::RuntimeError()));
    EXPECT_CALL(msys, munmap).WillOnce(testing::Invoke(::munmap));
    ASSERT_THROW(Fallback().io(IoType::Read, file, buffer, 4096, 0, 0), Sys::RuntimeError);
}

TEST_F(RocFileRead, fallback_read_throws_on_hipmemcpy_failure)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t file_length = buffer->getLength();
    init_file(file_length);

    EXPECT_CALL(msys, mmap).WillOnce(testing::Invoke(::mmap));
    EXPECT_CALL(msys, pread).WillRepeatedly(testing::Invoke(this, &RocFileRead::fake_pread));
    EXPECT_CALL(mhip, hipMemcpy).WillOnce(testing::Throw(Hip::RuntimeError(hipErrorUnknown)));
    EXPECT_CALL(msys, munmap).WillOnce(testing::Invoke(::munmap));
    ASSERT_THROW(Fallback().io(IoType::Read, file, buffer, file_length, 0, 0), Hip::RuntimeError);
}

TEST_F(RocFileRead, fallback_read_handles_empty_file)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    const size_t file_length = 0;
    init_file(file_length);

    EXPECT_CALL(msys, mmap).WillOnce(testing::Invoke(::mmap));
    EXPECT_CALL(msys, pread).WillRepeatedly(testing::Invoke(this, &RocFileRead::fake_pread));
    EXPECT_CALL(msys, munmap).WillOnce(testing::Invoke(::munmap));
    ASSERT_EQ(file_length, Fallback().io(IoType::Read, file, buffer, buffer->getLength(), 0, 0));
    ASSERT_TRUE(device_buffer_contains_expected_data(0, 0, file_length));
}

TEST_F(RocFileRead, fallback_read_handles_short_preads)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t file_length = buffer->getLength();
    init_file(file_length);

    EXPECT_CALL(msys, mmap).WillOnce(testing::Invoke(::mmap));
    EXPECT_CALL(msys, pread)
        .WillOnce(testing::Invoke([this](int fd, void *buf, size_t count, hoff_t offset) -> ssize_t {
            return this->fake_pread(fd, buf, count / 2, offset);
        }))
        .WillRepeatedly(testing::Invoke([this](int fd, void *buf, size_t count, hoff_t offset) -> ssize_t {
            return this->fake_pread(fd, buf, count, offset);
        }));
    EXPECT_CALL(mhip, hipMemcpy).WillRepeatedly(testing::Invoke(this, &RocFileRead::fake_hipMemcpy));
    EXPECT_CALL(msys, munmap).WillOnce(testing::Invoke(::munmap));
    ASSERT_EQ(file_length, Fallback().io(IoType::Read, file, buffer, buffer->getLength(), 0, 0));
    ASSERT_TRUE(device_buffer_contains_expected_data(0, 0, file_length));
}

TEST_F(RocFileRead, fallback_read_handles_interrupted_pread)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t file_length = buffer->getLength();
    init_file(file_length);

    EXPECT_CALL(msys, mmap).WillOnce(testing::Invoke(::mmap));
    EXPECT_CALL(msys, pread)
        .WillOnce(testing::Throw(Sys::RuntimeError(EINTR)))
        .WillRepeatedly(testing::Invoke([this](int fd, void *buf, size_t count, hoff_t offset) -> ssize_t {
            return this->fake_pread(fd, buf, count, offset);
        }));
    EXPECT_CALL(mhip, hipMemcpy).WillRepeatedly(testing::Invoke(this, &RocFileRead::fake_hipMemcpy));
    EXPECT_CALL(msys, munmap).WillOnce(testing::Invoke(::munmap));
    ASSERT_EQ(file_length, Fallback().io(IoType::Read, file, buffer, buffer->getLength(), 0, 0));
    ASSERT_TRUE(device_buffer_contains_expected_data(0, 0, file_length));
}

TEST_F(RocFileRead, fallback_read_handles_file_smaller_than_buffer)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t file_length = buffer->getLength() / 2;
    init_file(file_length);

    expect_fallback_read(mhip, msys);
    ASSERT_EQ(file_length, Fallback().io(IoType::Read, file, buffer, buffer->getLength(), 0, 0));
    ASSERT_TRUE(device_buffer_contains_expected_data(0, 0, file_length));
}

TEST_F(RocFileRead, fallback_read_handles_file_same_size_as_buffer)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t file_length = buffer->getLength();
    init_file(file_length);

    expect_fallback_read(mhip, msys);
    ASSERT_EQ(file_length, Fallback().io(IoType::Read, file, buffer, buffer->getLength(), 0, 0));
    ASSERT_TRUE(device_buffer_contains_expected_data(0, 0, file_length));
}

TEST_F(RocFileRead, fallback_read_handles_file_larger_than_buffer)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t file_length = buffer->getLength() * 2;
    init_file(file_length);

    expect_fallback_read(mhip, msys);
    ASSERT_EQ(buffer->getLength(), Fallback().io(IoType::Read, file, buffer, buffer->getLength(), 0, 0));
    ASSERT_TRUE(device_buffer_contains_expected_data(0, 0, buffer->getLength()));
}

TEST_F(RocFileRead, fallback_read_handles_file_with_size_multiple_of_chunk_size)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t chunk_size  = 4096;
    size_t file_length = chunk_size;
    init_file(file_length);

    expect_fallback_read(mhip, msys);
    ASSERT_EQ(file_length, Fallback().io(IoType::Read, file, buffer, file_length, 0, 0, chunk_size));
    ASSERT_TRUE(device_buffer_contains_expected_data(0, 0, file_length));
}

TEST_F(RocFileRead, fallback_read_handles_files_with_size_not_multiple_of_chunk_size)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t chunk_size  = 4096;
    size_t file_length = chunk_size + 1;
    init_file(file_length);

    expect_fallback_read(mhip, msys);
    ASSERT_EQ(file_length, Fallback().io(IoType::Read, file, buffer, file_length, 0, 0, chunk_size));
    ASSERT_TRUE(device_buffer_contains_expected_data(0, 0, file_length));
}

TEST_F(RocFileRead, fallback_read_with_non_zero_file_offset)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t file_length = buffer->getLength() * 3;
    init_file(file_length);
    hoff_t file_offset = static_cast<hoff_t>(buffer->getLength());

    expect_fallback_read(mhip, msys);
    ASSERT_EQ(buffer->getLength(),
              Fallback().io(IoType::Read, file, buffer, buffer->getLength(), file_offset, 0));
    ASSERT_TRUE(device_buffer_contains_expected_data(file_offset, 0, buffer->getLength()));
}

TEST_F(RocFileRead, fallback_read_to_eof_with_non_zero_file_offset)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t file_length = buffer->getLength() * 2;
    init_file(file_length);
    hoff_t file_offset = static_cast<hoff_t>(buffer->getLength());

    expect_fallback_read(mhip, msys);
    ASSERT_EQ(buffer->getLength(),
              Fallback().io(IoType::Read, file, buffer, buffer->getLength(), file_offset, 0));
    ASSERT_TRUE(device_buffer_contains_expected_data(file_offset, 0, buffer->getLength()));
}

TEST_F(RocFileRead, fallback_read_past_eof_with_non_zero_file_offset)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t file_length = buffer->getLength() * 2;
    init_file(file_length);
    hoff_t file_offset = static_cast<hoff_t>(buffer->getLength()) + 1;

    expect_fallback_read(mhip, msys);
    ASSERT_EQ(buffer->getLength() - 1,
              Fallback().io(IoType::Read, file, buffer, buffer->getLength(), file_offset, 0));
    ASSERT_TRUE(device_buffer_contains_expected_data(file_offset, 0, buffer->getLength() - 1));
}

TEST_F(RocFileRead, fallback_read_can_read_single_byte_at_end_of_file)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t file_length = buffer->getLength();
    init_file(file_length);
    hoff_t file_offset = static_cast<hoff_t>(file_length) - 1;

    expect_fallback_read(mhip, msys);
    ASSERT_EQ(1, Fallback().io(IoType::Read, file, buffer, buffer->getLength(), file_offset, 0));
    ASSERT_TRUE(device_buffer_contains_expected_data(file_offset, 0, 1));
}

TEST_F(RocFileRead, fallback_read_emtpy_file_with_non_zero_file_offset)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t file_length = 0;
    init_file(file_length);
    hoff_t file_offset = static_cast<hoff_t>(buffer->getLength());

    expect_fallback_read(mhip, msys);
    ASSERT_EQ(0, Fallback().io(IoType::Read, file, buffer, buffer->getLength(), file_offset, 0));
    ASSERT_TRUE(device_buffer_contains_expected_data(0, 0, 0));
}

TEST_F(RocFileRead, fallback_read_with_non_zero_buffer_offset)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t file_length = buffer->getLength() * 2;
    init_file(file_length);
    hoff_t buffer_offset = static_cast<hoff_t>(1);

    expect_fallback_read(mhip, msys);
    ASSERT_EQ(buffer->getLength() - 1,
              Fallback().io(IoType::Read, file, buffer, buffer->getLength() - 1, 0, buffer_offset));
    ASSERT_TRUE(device_buffer_contains_expected_data(0, buffer_offset, buffer->getLength() - 1));
}

TEST_F(RocFileRead, fallback_read_can_read_into_last_byte_of_buffer)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t file_length = buffer->getLength();
    init_file(file_length);
    hoff_t buffer_offset = static_cast<hoff_t>(buffer->getLength() - 1);

    expect_fallback_read(mhip, msys);
    ASSERT_EQ(1, Fallback().io(IoType::Read, file, buffer, 1, 0, buffer_offset));
    ASSERT_TRUE(device_buffer_contains_expected_data(0, buffer_offset, 1));
}

TEST_F(RocFileRead, fallback_read_with_non_zero_buffer_offset_and_file_offset)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    size_t file_length = buffer->getLength();
    init_file(file_length);
    hoff_t file_offset   = 74;
    hoff_t buffer_offset = 97;
    size_t read_size     = buffer->getLength() / 2;

    expect_fallback_read(mhip, msys);
    ASSERT_EQ(read_size, Fallback().io(IoType::Read, file, buffer, read_size, file_offset, buffer_offset));
    ASSERT_TRUE(device_buffer_contains_expected_data(file_offset, buffer_offset, read_size));
}

struct RocFileIoBackendSelectionParam : public ::testing::TestWithParam<IoType> {

    IoType                                io_type;
    rocFileHandle_t                       handle;
    void                                 *buffer;
    size_t                                io_size;
    hoff_t                                file_offset;
    hoff_t                                buffer_offset;
    int                                   flags;
    std::shared_ptr<StrictMock<MFile>>    mfile;
    std::shared_ptr<StrictMock<MBuffer>>  mbuffer;
    std::shared_ptr<StrictMock<MBackend>> mbe1;
    std::shared_ptr<StrictMock<MBackend>> mbe2;
    std::shared_ptr<StrictMock<MBackend>> mbe3;
    StrictMock<MDriverState>              mds;

    RocFileIoBackendSelectionParam()
        : io_type{GetParam()}, handle{reinterpret_cast<rocFileHandle_t>(0xBADF00D)},
          buffer{reinterpret_cast<void *>(0xDEADBEEF)}, io_size{1024}, file_offset{32}, buffer_offset{64},
          flags{0}, mfile{std::make_shared<StrictMock<MFile>>()},
          mbuffer{std::make_shared<StrictMock<MBuffer>>()}, mbe1{std::make_shared<StrictMock<MBackend>>()},
          mbe2{std::make_shared<StrictMock<MBackend>>()}, mbe3{std::make_shared<StrictMock<MBackend>>()}
    {
    }
};

TEST_P(RocFileIoBackendSelectionParam, RocFileIoThrowsIfThereAreNoBackends)
{
    auto backends{std::vector<std::shared_ptr<Backend>>()};

    EXPECT_CALL(mds, getFileAndBuffer(handle, buffer, io_size, flags))
        .WillOnce(Return(file_buffer_pair{mfile, mbuffer}));
    EXPECT_CALL(mds, getBackends).WillOnce(Return(backends));

    switch (io_type) {
        case IoType::Read:
            ASSERT_EQ(rocFileRead(handle, buffer, io_size, file_offset, buffer_offset),
                      -rocFileInternalError);
            break;
        case IoType::Write:
            ASSERT_EQ(rocFileWrite(handle, buffer, io_size, file_offset, buffer_offset),
                      -rocFileInternalError);
            break;
        default:
            FAIL() << "Unhandled IO Type";
    }
}

TEST_P(RocFileIoBackendSelectionParam, RocFileIoThrowsIfAllBackendsRejectTheIO)
{
    std::vector<std::shared_ptr<Backend>> backends{mbe1, mbe2, mbe3};

    EXPECT_CALL(mds, getFileAndBuffer(handle, buffer, io_size, flags))
        .WillOnce(Return(file_buffer_pair{mfile, mbuffer}));
    EXPECT_CALL(mds, getBackends).WillOnce(Return(backends));
    EXPECT_CALL(*mbe1, score(Eq(mfile), Eq(mbuffer), io_size, file_offset, buffer_offset))
        .WillOnce(Return(-1));
    EXPECT_CALL(*mbe2, score(Eq(mfile), Eq(mbuffer), io_size, file_offset, buffer_offset))
        .WillOnce(Return(-1));
    EXPECT_CALL(*mbe3, score(Eq(mfile), Eq(mbuffer), io_size, file_offset, buffer_offset))
        .WillOnce(Return(-1));

    switch (io_type) {
        case IoType::Read:
            ASSERT_EQ(rocFileRead(handle, buffer, io_size, file_offset, buffer_offset),
                      -rocFileInternalError);
            break;
        case IoType::Write:
            ASSERT_EQ(rocFileWrite(handle, buffer, io_size, file_offset, buffer_offset),
                      -rocFileInternalError);
            break;
        default:
            FAIL() << "Unhandled IO Type";
    }
}

TEST_P(RocFileIoBackendSelectionParam, RocFileIoIssuesIoToHighestScoringBackend)
{
    std::vector<std::shared_ptr<Backend>> backends{mbe1, mbe2, mbe3};

    EXPECT_CALL(mds, getFileAndBuffer(handle, buffer, io_size, flags))
        .WillOnce(Return(file_buffer_pair{mfile, mbuffer}));
    EXPECT_CALL(mds, getBackends).WillOnce(Return(backends));
    EXPECT_CALL(*mbe1, score(Eq(mfile), Eq(mbuffer), io_size, file_offset, buffer_offset))
        .WillOnce(Return(0));
    EXPECT_CALL(*mbe2, score(Eq(mfile), Eq(mbuffer), io_size, file_offset, buffer_offset))
        .WillOnce(Return(2));
    EXPECT_CALL(*mbe3, score(Eq(mfile), Eq(mbuffer), io_size, file_offset, buffer_offset))
        .WillOnce(Return(1));

    switch (io_type) {
        case IoType::Read:
            EXPECT_CALL(*mbe2,
                        io(Eq(IoType::Read), Eq(mfile), Eq(mbuffer), io_size, file_offset, buffer_offset))
                .WillOnce(Return(io_size));
            ASSERT_EQ(rocFileRead(handle, buffer, io_size, file_offset, buffer_offset), io_size);
            break;
        case IoType::Write:
            EXPECT_CALL(*mbe2,
                        io(Eq(IoType::Write), Eq(mfile), Eq(mbuffer), io_size, file_offset, buffer_offset))
                .WillOnce(Return(io_size));
            ASSERT_EQ(rocFileWrite(handle, buffer, io_size, file_offset, buffer_offset), io_size);
            break;
        default:
            FAIL() << "Unhandled IO Type";
    }
}

INSTANTIATE_TEST_SUITE_P(RocFileIoBackendSelection, RocFileIoBackendSelectionParam,
                         ::testing::Values(IoType::Read, IoType::Write));

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
