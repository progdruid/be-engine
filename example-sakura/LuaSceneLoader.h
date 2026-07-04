#pragma once

#include <string>
#include <string_view>
#include <functional>
#include <unordered_map>
#include <filesystem>

#include "lua.hpp"
#include "LuaBridge/LuaBridge.h"

#include "entt/entt.hpp"
#include <umbrellas/common.hpp>

using ComponentParserFunc = std::function<
    void(
        entt::registry&, 
        entt::entity, 
        std::string_view entityName, 
        luabridge::LuaRef
    )
>;

class LuaSceneLoader {
    
    hide
    lua_State* _L = nullptr;
    std::unordered_map<std::string, ComponentParserFunc> _parsers;
    
    expose
    LuaSceneLoader();
    ~LuaSceneLoader();

    expose
    void AddComponentParser(std::string componentKey, ComponentParserFunc parser);
    void Load(const std::filesystem::path& path, entt::registry& registry);
};
