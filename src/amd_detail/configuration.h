/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

namespace hipFile {

class IConfiguration {
public:
    virtual ~IConfiguration()
    {
    }

    /// @brief Checks if the fastpath backend is enabled
    /// @return true if the fastpath backend is enabled, false otherwise
    virtual bool fastpath() const noexcept = 0;

    /// @brief Checks if the fallback backend is enabled
    /// @return true if the fallback backend is enabled, false otherwise
    virtual bool fallback() const noexcept = 0;

    /// @brief Shows the level of detail for stats collection
    /// @return 0 if stats collection disabled, higher levels of detail as value increases
    virtual unsigned int statsLevel() const noexcept = 0;
};

class Configuration : public IConfiguration {
    bool         m_fastpath;
    bool         m_fallback;
    unsigned int m_statsLevel;

public:
    Configuration();

    bool         fastpath() const noexcept override;
    bool         fallback() const noexcept override;
    unsigned int statsLevel() const noexcept override;
};

}
