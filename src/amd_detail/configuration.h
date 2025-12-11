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
};

class Configuration : public IConfiguration {
    bool m_fastpath;
    bool m_fallback;

public:
    Configuration();

    bool fastpath() const noexcept override;
    bool fallback() const noexcept override;
};

}
