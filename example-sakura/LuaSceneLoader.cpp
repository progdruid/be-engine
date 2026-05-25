#include "LuaSceneLoader.h"

#include <filesystem>

#include "LuaBridge/detail/Iterator.h"

#include "Components.h"

LuaSceneLoader::LuaSceneLoader() {
    _L = luaL_newstate();
    luaL_openlibs(_L);
}

LuaSceneLoader::~LuaSceneLoader() {
    if (_L) {
        lua_close(_L);
    }
}

void LuaSceneLoader::AddComponentParser(std::string componentKey, ComponentParserFunc parser) {
    _parsers[std::move(componentKey)] = std::move(parser);
}

void LuaSceneLoader::Load(const std::filesystem::path& path, entt::registry& registry) {
    if (luaL_dofile(_L, path.string().c_str()) != LUA_OK) {
        lua_pop(_L, 1);
        return;
    }

    auto makeSceneFn = luabridge::getGlobal(_L, "makeScene");
    if (!makeSceneFn.isFunction()) {
        return;
    }

    auto result = makeSceneFn();
    if (!result) {
        return;
    }

    auto sceneTable = result[0];
    if (!sceneTable.isTable()) {
        return;
    }

    for (auto&& [key, entityTable] : luabridge::pairs(sceneTable)) {
        if (!key.isString() || !entityTable.isTable()) {
            continue;
        }

        std::string entityName = key.unsafe_cast<std::string>();
        auto entity = registry.create();
        registry.emplace<NameComponent>(entity, NameComponent{ .Name = entityName });

        for (auto&& [compKey, compData] : luabridge::pairs(entityTable)) {
            if (!compKey.isString()) {
                continue;
            }

            std::string componentKey = compKey.unsafe_cast<std::string>();
            auto it = _parsers.find(componentKey);
            if (it != _parsers.end()) {
                it->second(registry, entity, entityName, luabridge::LuaRef(compData));
            }
        }
    }

}
