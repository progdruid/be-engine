#include "BeLua.h"

#include "LuaBridge/detail/Iterator.h"

BeLuaValue::BeLuaValue(luabridge::LuaRef ref, std::string path)
    : _ref(std::move(ref)), _path(std::move(path)) {}

auto BeLuaValue::operator[](std::string_view key) const -> BeLuaValue {
    if (!_ref || !_ref->isTable()) {
        return {};
    }

    auto path = _path.empty() ? std::string(key) : _path + "." + std::string(key);
    return BeLuaValue(luabridge::LuaRef((*_ref)[std::string(key).c_str()]), std::move(path));
}

auto BeLuaValue::operator[](int index) const -> BeLuaValue {
    if (!_ref || !_ref->isTable()) {
        return {};
    }

    auto path = _path + "[" + std::to_string(index) + "]";
    return BeLuaValue(luabridge::LuaRef((*_ref)[index]), std::move(path));
}

auto BeLuaValue::Exists() const -> bool {
    return _ref && !_ref->isNil();
}

auto BeLuaValue::IsTable() const -> bool {
    return _ref && _ref->isTable();
}

auto BeLuaValue::Count() const -> int {
    if (!_ref || !_ref->isTable()) {
        return 0;
    }
    return _ref->length();
}

auto BeLuaValue::Path() const -> const std::string& {
    return _path;
}

auto BeLuaValue::Pairs() const -> std::vector<std::pair<std::string, BeLuaValue>> {
    std::vector<std::pair<std::string, BeLuaValue>> entries;
    if (!_ref || !_ref->isTable()) {
        return entries;
    }

    for (auto&& [key, value] : luabridge::pairs(*_ref)) {
        if (!key.isString()) {
            continue;
        }

        auto name = key.unsafe_cast<std::string>();
        auto path = _path.empty() ? name : _path + "." + name;
        entries.emplace_back(std::move(name), BeLuaValue(luabridge::LuaRef(value), std::move(path)));
    }
    return entries;
}

auto BeLuaValue::Array() const -> std::vector<BeLuaValue> {
    std::vector<BeLuaValue> entries;
    const auto count = Count();
    entries.reserve(count);
    for (int i = 1; i <= count; i++) {
        entries.push_back((*this)[i]);
    }
    return entries;
}

BeLuaState::BeLuaState() {
    _L = luaL_newstate();
    luaL_openlibs(_L);
}

BeLuaState::~BeLuaState() {
    if (_L) {
        lua_close(_L);
    }
}

auto BeLuaState::DoFile(const std::filesystem::path& path) -> bool {
    if (luaL_dofile(_L, path.string().c_str()) != LUA_OK) {
        const char* message = lua_tostring(_L, -1);
        std::fprintf(stderr, "[lua] %s\n", message ? message : "unknown error");
        lua_pop(_L, 1);
        return false;
    }
    return true;
}

auto BeLuaState::Global(std::string_view name) const -> BeLuaValue {
    auto global = luabridge::getGlobal(_L, std::string(name).c_str());
    if (global.isNil()) {
        return {};
    }
    return BeLuaValue(std::move(global), std::string(name));
}
