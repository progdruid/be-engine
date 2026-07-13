#pragma once

#include <string>
#include <expected>

#include "Workspace.h"

auto VerifyProject(const Project& project) -> std::expected<void, std::string>;
auto VerifyApp(const std::string& name) -> std::expected<const Project*, std::string>;
auto VerifyWorkspace() -> std::expected<void, std::string>;
