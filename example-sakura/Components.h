#pragma once

#include <memory>
#include <string>
#include <umbrellas/include-glm.h>

#include "entt/entt.hpp"

struct BeProp;
class BeTexture;

struct TransformComponent {
    glm::vec3 Position = {0.f, 0.f, 0.f};
    glm::quat Rotation = glm::quat(glm::vec3(0, 0, 0));
    glm::vec3 Scale = {1.f, 1.f, 1.f};
};

struct RenderComponent {
    std::shared_ptr<BeProp> Prop;
    bool CastShadows = true;
};

struct NameComponent {
    std::string Name;
};

struct StaticTag {};

struct SunLightComponent {
    // light
    glm::vec3 Direction;
    glm::vec3 Color;
    float Power;
    
    // shadow
    bool CastsShadows = true;
    uint32_t ShadowMapResolution;
    float ShadowCameraDistance;
    float ShadowMapWorldSize;
    float ShadowNearPlane;
    float ShadowFarPlane;
    std::shared_ptr<BeTexture> ShadowMap;
};

struct PointLightComponent {
    float Radius;
    glm::vec3 Color;
    float Power;

    bool CastsShadows = false;
    uint32_t ShadowMapResolution = 1024;
    float ShadowNearPlane = 0.1f; // far plane is radius
    std::shared_ptr<BeTexture> ShadowMap;
};

class LuaSceneLoader;
class BeAssetRegistry;
void RegisterComponentParsers(LuaSceneLoader& loader, BeAssetRegistry& registry);

template<typename... Components>
auto CreateEntity(entt::registry& registry, Components&&... components) -> entt::entity {
    auto entity = registry.create();
    ([&]<typename T>(T&& comp) {
        if constexpr (std::is_empty_v<std::decay_t<T>>)
            registry.emplace<std::decay_t<T>>(entity);
        else
            registry.emplace<std::decay_t<T>>(entity, std::forward<T>(comp));
    }(std::forward<Components>(components)), ...);
    return entity;
}