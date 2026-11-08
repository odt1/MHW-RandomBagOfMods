#pragma once

#include <filesystem>

namespace stick_deadzone::game::deadzone {

// Hooks the pad singleton's per-frame update; returns false when nothing was installed.
bool Install(const std::filesystem::path& config_path);

}  // namespace stick_deadzone::game::deadzone
