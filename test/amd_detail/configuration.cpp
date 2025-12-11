/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "configuration.h"
#include "environment.h"
#include "hipfile-warnings.h"
#include "mhip.h"
#include "msys.h"

#include <gtest/gtest.h>

using namespace hipFile;
using namespace testing;
using namespace std;

HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

struct ConfigurationExpectation;

struct ConfigurationExpectationBuilder {
    StrictMock<MSys>           &m_msys;
    StrictMock<MHip>           &m_mhip;
    std::optional<const char *> m_env_force_compat_mode;
    std::optional<const char *> m_env_allow_compat_mode;
    void                       *m_hip_amd_file_read{reinterpret_cast<void *>(0xDEADBEEF)};
    void                       *m_hip_amd_file_write{reinterpret_cast<void *>(0x0BADF00D)};

    ConfigurationExpectationBuilder(StrictMock<MSys> &msys, StrictMock<MHip> &mhip)
        : m_msys(msys), m_mhip(mhip)
    {
    }

    ConfigurationExpectationBuilder &env_force_compat_mode(const char *value)
    {
        m_env_force_compat_mode = value;
        return *this;
    }

    ConfigurationExpectationBuilder &env_allow_compat_mode(const char *value)
    {
        m_env_allow_compat_mode = value;
        return *this;
    }

    ConfigurationExpectationBuilder &hip_amd_file_read(void *value)
    {
        m_hip_amd_file_read = value;
        return *this;
    }

    ConfigurationExpectationBuilder &hip_amd_file_write(void *value)
    {
        m_hip_amd_file_write = value;
        return *this;
    }

    ConfigurationExpectation build();
};

struct ConfigurationExpectation {
    ConfigurationExpectation(const ConfigurationExpectationBuilder &builder)
    {
        if (builder.m_env_force_compat_mode) {
            EXPECT_CALL(builder.m_msys, getenv(StrEq(hipFile::Environment::FORCE_COMPAT_MODE)))
                .WillOnce(Return(const_cast<char *>(builder.m_env_force_compat_mode.value())));
        }
        else {
            EXPECT_CALL(builder.m_msys, getenv(StrEq(hipFile::Environment::FORCE_COMPAT_MODE)));
        }

        if (builder.m_env_allow_compat_mode) {
            EXPECT_CALL(builder.m_msys, getenv(StrEq(hipFile::Environment::ALLOW_COMPAT_MODE)))
                .WillOnce(Return(const_cast<char *>(builder.m_env_allow_compat_mode.value())));
        }
        else {
            EXPECT_CALL(builder.m_msys, getenv(StrEq(hipFile::Environment::ALLOW_COMPAT_MODE)));
        }

        EXPECT_CALL(builder.m_mhip, hipRuntimeGetVersion).Times(2);
        EXPECT_CALL(builder.m_mhip, hipGetProcAddress(StrEq("hipAmdFileRead"), _, _, _))
            .WillOnce(Return(builder.m_hip_amd_file_read));
        EXPECT_CALL(builder.m_mhip, hipGetProcAddress(StrEq("hipAmdFileWrite"), _, _, _))
            .WillOnce(Return(builder.m_hip_amd_file_write));
    }
};

ConfigurationExpectation
ConfigurationExpectationBuilder::build()
{
    return ConfigurationExpectation(*this);
}

struct HipFileConfiguration : public Test {
    StrictMock<MSys> msys;
    StrictMock<MHip> mhip;
};

TEST_F(HipFileConfiguration, FastpathEnabledIfForceCompatModeEnvironmentVariableIsNotSetOrInvalid)
{
    ConfigurationExpectationBuilder{msys, mhip}.build();
    ASSERT_TRUE(Configuration().fastpath());
}

TEST_F(HipFileConfiguration, FastpathEnabledIfForceCompatModeEnvironmentVariableIsFalse)
{
    ConfigurationExpectationBuilder{msys, mhip}.env_force_compat_mode("false").build();
    ASSERT_TRUE(Configuration().fastpath());
}

TEST_F(HipFileConfiguration, FastpathDisabledIfForceCompatModeEnvironmentVariableIsTrue)
{
    ConfigurationExpectationBuilder{msys, mhip}.env_force_compat_mode("true").build();
    ASSERT_FALSE(Configuration().fastpath());
}

TEST_F(HipFileConfiguration, FastpathDisabledIfHipAmdFileReadIsNotFound)
{
    ConfigurationExpectationBuilder{msys, mhip}.hip_amd_file_read(nullptr).build();
    ASSERT_FALSE(Configuration().fastpath());
}

TEST_F(HipFileConfiguration, FastpathDisabledIfHipAmdFileWriteIsNotFound)
{
    ConfigurationExpectationBuilder{msys, mhip}.hip_amd_file_write(nullptr).build();
    ASSERT_FALSE(Configuration().fastpath());
}

TEST_F(HipFileConfiguration, FallbackEnabledIfAllowCompatModeEnvironmentVariableIsNotSetOrInvalid)
{
    ConfigurationExpectationBuilder{msys, mhip}.build();
    ASSERT_TRUE(Configuration().fallback());
}

TEST_F(HipFileConfiguration, FallbackEnabledIfAllowCompatModeEnvironmentVariableIsTrue)
{
    ConfigurationExpectationBuilder{msys, mhip}.env_allow_compat_mode("true").build();
    ASSERT_TRUE(Configuration().fallback());
}

TEST_F(HipFileConfiguration, FallbackDisabledIfAllowCompatModeEnvironmentVariableIsFalse)
{
    ConfigurationExpectationBuilder{msys, mhip}.env_allow_compat_mode("false").build();
    ASSERT_FALSE(Configuration().fallback());
}

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
