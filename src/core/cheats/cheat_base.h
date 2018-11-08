// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

namespace Core {
class System;
}

namespace Cheats {
class CheatBase {
public:
    virtual ~CheatBase() = default;
    virtual void Execute(Core::System& system) = 0;

    virtual bool GetEnabled() const = 0;
    virtual void SetEnabled(bool enabled) = 0;

    virtual std::string GetComments() const = 0;
    virtual std::string GetName() const = 0;
    virtual std::string GetType() const = 0;

    virtual std::string ToString() const = 0;
};
} // namespace Cheats
