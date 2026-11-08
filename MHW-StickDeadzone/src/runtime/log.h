#pragma once

#include <filesystem>
#include <format>
#include <string_view>
#include <utility>

namespace stick_deadzone::log {

bool Initialize(const std::filesystem::path& path);
void Write(std::string_view text);

template <typename... Args>
void Log(std::format_string<Args...> format, Args&&... args) {
    Write(std::format(format, std::forward<Args>(args)...));
}

}  // namespace stick_deadzone::log
