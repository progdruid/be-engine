#pragma once

#include <string>
#include <expected>

#include "Workspace.h"

auto Check(const Workspace& ws) -> std::expected<void, std::string>;
