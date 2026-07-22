#pragma once

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "lua.hpp"
#include "LuaBridge/LuaBridge.h"

#include <umbrellas/common.hpp>
#include <umbrellas/include-glm.h>

template <class T>
struct BeLuaConverter {
    static auto From(const luabridge::LuaRef& ref) -> std::optional<T> {
        if constexpr (std::is_same_v<T, bool>) {
            if (!ref.isBool()) {
                return std::nullopt;
            }
            return ref.unsafe_cast<bool>();
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            if (!ref.isString()) {
                return std::nullopt;
            }
            return ref.unsafe_cast<std::string>();
        }
        else if constexpr (std::is_arithmetic_v<T>) {
            if (!ref.isNumber()) {
                return std::nullopt;
            }
            return static_cast<T>(ref.unsafe_cast<double>());
        }
        else {
            static_assert(sizeof(T) == 0, "no BeLuaConverter specialisation for this type");
        }
    }
};

class BeLuaValue {

    hide
    std::optional<luabridge::LuaRef> _ref;
    std::string _path;

    expose
    BeLuaValue() = default;
    BeLuaValue(luabridge::LuaRef ref, std::string path);

    auto operator[](std::string_view key) const -> BeLuaValue;
    auto operator[](int index) const -> BeLuaValue;

    auto Exists() const -> bool;
    auto IsTable() const -> bool;
    auto Count() const -> int;
    auto Path() const -> const std::string&;

    auto Pairs() const -> std::vector<std::pair<std::string, BeLuaValue>>;
    auto Array() const -> std::vector<BeLuaValue>;

    template <class T>
    auto Get() const -> std::optional<T> {
        if (!_ref || _ref->isNil()) {
            return std::nullopt;
        }

        auto value = BeLuaConverter<T>::From(*_ref);
        if (!value) {
            std::fprintf(stderr, "[lua] '%s' has unexpected type '%s'\n",
                _path.c_str(), lua_typename(_ref->state(), _ref->type()));
        }
        return value;
    }

    template <class T>
    auto GetOr(T fallback) const -> T {
        return Get<T>().value_or(std::move(fallback));
    }
};

template <int TLength>
struct BeLuaConverter<glm::vec<TLength, float, glm::defaultp>> {
    static auto FromHex(std::string_view hex) -> std::optional<glm::vec<TLength, float, glm::defaultp>> {
        if constexpr (TLength < 3) {
            return std::nullopt;
        }
        else {
            if (!hex.starts_with('#')) {
                return std::nullopt;
            }
            hex.remove_prefix(1);

            if (hex.size() != 6 && hex.size() != 8) {
                return std::nullopt;
            }
            for (const char digit : hex) {
                if (!std::isxdigit(static_cast<unsigned char>(digit))) {
                    return std::nullopt;
                }
            }

            const std::string digits(hex);
            if constexpr (TLength == 4) {
                if (digits.size() == 8) {
                    return HexColorRGBA(digits.c_str());
                }
                return glm::vec4(HexColor(digits.c_str()), 1.f);
            }
            else {
                return HexColor(digits.c_str());
            }
        }
    }

    static auto From(const luabridge::LuaRef& ref) -> std::optional<glm::vec<TLength, float, glm::defaultp>> {
        if (ref.isString()) {
            return FromHex(ref.unsafe_cast<std::string>());
        }

        if (!ref.isTable() || ref.length() < TLength) {
            return std::nullopt;
        }

        glm::vec<TLength, float, glm::defaultp> result;
        for (int i = 0; i < TLength; i++) {
            const auto component = ref[i + 1];
            if (!component.isNumber()) {
                return std::nullopt;
            }
            result[i] = component.template unsafe_cast<float>();
        }
        return result;
    }
};

template <>
struct BeLuaConverter<glm::quat> {
    static auto From(const luabridge::LuaRef& ref) -> std::optional<glm::quat> {
        if (!ref.isTable()) {
            return std::nullopt;
        }

        const auto degrees = BeLuaConverter<glm::vec3>::From(ref);
        if (!degrees) {
            return std::nullopt;
        }
        return glm::quat(glm::radians(*degrees));
    }
};

class BeLuaState {

    hide
    lua_State* _L = nullptr;

    expose
    BeLuaState();
    ~BeLuaState();
    BeLuaState(const BeLuaState&) = delete;
    auto operator=(const BeLuaState&) -> BeLuaState& = delete;

    auto DoFile(const std::filesystem::path& path) -> bool;
    auto Global(std::string_view name) const -> BeLuaValue;

    template <class... TArgs>
    auto Call(std::string_view name, TArgs&&... args) const -> BeLuaValue {
        const auto function = luabridge::getGlobal(_L, std::string(name).c_str());
        if (!function.isFunction()) {
            std::fprintf(stderr, "[lua] global '%.*s' is not a function\n", int(name.size()), name.data());
            return {};
        }

        auto result = function(std::forward<TArgs>(args)...);
        if (!result) {
            std::fprintf(stderr, "[lua] calling '%.*s' failed: %s\n",
                int(name.size()), name.data(), result.errorMessage().c_str());
            return {};
        }
        if (result.size() == 0) {
            return {};
        }

        return BeLuaValue(result[0], std::string(name) + "()");
    }
};
