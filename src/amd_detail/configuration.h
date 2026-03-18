/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <optional>

namespace hipFile {

class Configuration {

    std::optional<bool> m_fastpath_override;

public:
    virtual ~Configuration() = default;

    /// @brief Checks if the fastpath backend is enabled
    /// @return true if the fastpath backend is enabled, false otherwise
    virtual bool fastpath() const noexcept;

    /// @brief Override fastpath backend enablement.
    ///
    /// If hipAmdFileRead/hipAmdFileWrite are not available fastpath() will
    /// return false even if fastpath(true) is called.
    virtual void fastpath(bool enabled) noexcept;

    /// @brief Checks if the fallback backend is enabled
    /// @return true if the fallback backend is enabled, false otherwise
    virtual bool fallback() const noexcept;

    /// @brief Shows the level of detail for stats collection
    /// @return 0 if stats collection disabled, higher levels of detail as value increases
    virtual unsigned int statsLevel() const noexcept;
};

}
