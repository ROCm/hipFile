/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "file-descriptor.h"
#include "msys.h"

#include <gtest/gtest.h>

using namespace hipFile;
using namespace testing;

HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

struct HipFileFileDescriptor : public Test {
    StrictMock<MSys> msys;
};

TEST_F(HipFileFileDescriptor, FileDescriptorClosesFileOnDestruction)
{
    FileDescriptor fd{42};
    EXPECT_CALL(msys, close(42));
}

TEST_F(HipFileFileDescriptor, FileDescriptorDoesNotCloseFileIfFileDescriptorIsNegativeOne)
{
    FileDescriptor fd{-1};
}

TEST_F(HipFileFileDescriptor, FileDescriptorHandlesMoveCopyConstructor)
{
    FileDescriptor fd_a{42};
    FileDescriptor fd_b{std::move(fd_a)};
    ASSERT_EQ(fd_a.get(), -1);
    EXPECT_CALL(msys, close(42));
}

TEST_F(HipFileFileDescriptor, EmptyFileDescriptorHandlesMoveAssignment)
{
    FileDescriptor fd_a{42};
    FileDescriptor fd_b{-1};
    fd_b = std::move(fd_a);
    ASSERT_EQ(fd_a.get(), -1);
    EXPECT_CALL(msys, close(42));
}

TEST_F(HipFileFileDescriptor, FileDescriptorHandlesMoveAssignment)
{
    FileDescriptor fd_a{42};
    FileDescriptor fd_b{43};
    EXPECT_CALL(msys, close(43));
    fd_b = std::move(fd_a);
    ASSERT_EQ(fd_a.get(), -1);
    EXPECT_CALL(msys, close(42));
}

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
