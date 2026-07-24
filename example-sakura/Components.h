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

struct CirclingComponent {
    glm::vec3 Origin = {0.f, 0.f, 0.f};
    glm::vec3 Axis = {0.f, 1.f, 0.f};
    float Radius = 1.f;
    float Speed = 15.f;
    float Phase = 0.f;
    bool Rotate = false;
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
    glm::vec3 Direction = {0.f, -1.f, 0.f};
    glm::vec3 Color = {1.f, 1.f, 1.f};
    float Power = 1.f;

    // shadow
    bool CastsShadows = true;
    uint32_t ShadowMapResolution = 2048;
    float ShadowCameraDistance = 100.f;
    float ShadowMapWorldSize = 100.f;
    float ShadowNearPlane = 0.1f;
    float ShadowFarPlane = 400.f;
    std::shared_ptr<BeTexture> ShadowMap;
};

struct PointLightComponent {
    float Radius = 10.f;
    glm::vec3 Color = {1.f, 1.f, 1.f};
    float Power = 1.f;

    bool CastsShadows = false;
    uint32_t ShadowMapResolution = 1024;
    float ShadowNearPlane = 0.1f; // far plane is radius
    std::shared_ptr<BeTexture> ShadowMap;
};

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