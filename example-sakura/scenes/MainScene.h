#pragma once

#include <memory>
#include <string>
#include <vector>
#include <umbrellas/include-glm.h>
#include <umbrellas/access-modifiers.hpp>

#include "entt/entt.hpp"
#include "BaseScene.h"

class BeTexture;
struct BeBRPPointLightEntry;
struct BeBRPSunLightEntry;
class BeBRPSubmissionBuffer;
class BeWindow;
class BeInput;
class BeCamera;
class BeRenderer;
struct BeModel;
struct BePointLight;
struct BeDirectionalLight;

struct TransformComponent {
    glm::vec3 Position = {0.f, 0.f, 0.f};
    glm::quat Rotation = glm::quat(glm::vec3(0, 0, 0));
    glm::vec3 Scale = {1.f, 1.f, 1.f};
};

struct RenderComponent {
    std::shared_ptr<BeModel> Model;
    bool CastShadows = true;
};

struct NameComponent {
    std::string Name;
};

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
    std::weak_ptr<BeTexture> ShadowMap;
};

struct PointLightComponent {
    float Radius;
    glm::vec3 Color;
    float Power;

    bool CastsShadows = false;
    uint32_t ShadowMapResolution = 1024;
    float ShadowNearPlane = 0.1f; // far plane is radius
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
    entt::registry _registry;
    std::shared_ptr<BeBRPSubmissionBuffer> _submissionBuffer;
    std::shared_ptr<BeRenderer> _renderer;
    std::shared_ptr<BeWindow> _window;
    std::shared_ptr<BeCamera> _camera;
    std::shared_ptr<BeInput> _input;
    
    std::shared_ptr<BeModel> _cube, _anvil;
    
    expose
    MainScene(
        const std::shared_ptr<BeRenderer>& renderer,
        const std::shared_ptr<BeWindow>& window,
        const std::shared_ptr<BeInput>& input
    );
    ~MainScene() override = default;

    auto Prepare() -> void override;
    auto OnLoad() -> void override;
    auto Tick(float deltaTime) -> void override;
    
    auto GetRegistry() -> entt::registry& { return _registry; }
    auto GetCamera() -> std::shared_ptr<BeCamera> { return _camera; }
};
