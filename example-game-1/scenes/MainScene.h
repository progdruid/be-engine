#pragma once

#include <memory>
#include <string>
#include <vector>
#include <umbrellas/include-glm.h>
#include <umbrellas/common.hpp>

#include "entt/entt.hpp"
#include "BaseScene.h"
#include "BeAssetRegistry.h"

class BeCamera;
struct BeProp;
struct BeMesh;
class BeMaterial;
class BeTexture;
class BeStandardRenderMachine;

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

struct SunLightComponent {
    glm::vec3 Direction;
    glm::vec3 Color;
    float Power;

    bool CastsShadows = true;
    uint32_t ShadowMapResolution;
    float ShadowCameraDistance;
    float ShadowMapWorldSize;
    float ShadowNearPlane;
    float ShadowFarPlane;
    std::weak_ptr<BeTexture> ShadowMap;
};

struct PointLightComponent {
    float Radius;
    glm::vec3 Color;
    float Power;

    bool CastsShadows = false;
    uint32_t ShadowMapResolution = 1024;
    float ShadowNearPlane = 0.1f;
    std::weak_ptr<BeTexture> ShadowMap;
};

template<typename... Components>
auto CreateEntity(entt::registry& registry, Components&&... components) -> entt::entity {
    auto entity = registry.create();
    (registry.emplace<std::decay_t<Components>>(entity, std::forward<Components>(components)), ...);
    return entity;
}

class MainScene : public BaseScene {
    hide
    BeAssetRegistry _assetRegistry;
    entt::registry _registry;
    std::shared_ptr<BeCamera> _camera;
    float _elapsedTime = 0.0f;

    std::shared_ptr<BeProp> _plane, _witchItems, _cube, _macintosh, _pagoda, _disks, _anvil;
    std::shared_ptr<BeMaterial> _uniformMaterial;
    std::unique_ptr<BeStandardRenderMachine> _machine;

    expose
    explicit MainScene(Game* game);
    ~MainScene() override;

    auto Prepare() -> void override;
    auto LoadPasses() -> void;
    auto OnLoad() -> void override;
    auto Tick(float deltaTime) -> void override;
};
