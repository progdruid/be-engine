#pragma once

#include <format>
#include <expected>
#include <utility>

#define BECHEF_CAT_(a, b) a##b
#define BECHEF_CAT(a, b) BECHEF_CAT_(a, b)

#define BECHEF_TRY_PLAIN(holder, declaration, expression) \
    auto holder = (expression); \
    if (!holder) return std::unexpected(holder.error()); \
    declaration = std::move(*holder)

#define BECHEF_TRY_FORMAT(holder, declaration, expression, ...) \
    auto holder = (expression); \
    if (!holder) return std::unexpected(std::format(__VA_ARGS__, holder.error())); \
    declaration = std::move(*holder)

#define BECHEF_TRY_PICK(_1, _2, _3, _4, _5, _6, name, ...) name

#define bechef_try(...) \
    BECHEF_TRY_PICK(__VA_ARGS__, \
        BECHEF_TRY_FORMAT, BECHEF_TRY_FORMAT, BECHEF_TRY_FORMAT, BECHEF_TRY_FORMAT, \
        BECHEF_TRY_PLAIN, BECHEF_TRY_UNUSED \
    )(BECHEF_CAT(_try_, __LINE__), __VA_ARGS__)
