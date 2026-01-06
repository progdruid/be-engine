#pragma once

#include <libassert/assert.hpp>
#include <string>

#define BE_FORMAT_FIRST(first, ...) \
    "\n\nInfo:\n" + std::string(first) + "\n" __VA_OPT__(, __VA_ARGS__)

#define be_assert(condition, ...) \
    DEBUG_ASSERT(condition __VA_OPT__(, BE_FORMAT_FIRST(__VA_ARGS__)))
    
