#pragma once

#include <filesystem>

namespace auto_auto_sort::game::autosort {

// Hooks sMhMain's per-frame update and the GUI open/close; returns false when nothing was installed.
bool Install(const std::filesystem::path& config_path);

}  // namespace auto_auto_sort::game::autosort
