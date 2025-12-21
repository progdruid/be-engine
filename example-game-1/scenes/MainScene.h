#pragma once

#include <memory>
#include <string>
#include <vector>
#include <umbrellas/include-glm.h>
#include <umbrellas/access-modifiers.hpp>

#include "entt/entt.hpp"
#include "BaseScene.h"

class BeWindow;
class BeInput;
class BeCamera;
class BeAssetRegistry;
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

class MainScene : public BaseScene {
    hide
    entt::registry _registry;
    std::shared_ptr<BeRenderer> _renderer;
    std::shared_ptr<BeAssetRegistry> _assetRegistry;
    std::shared_ptr<BeWindow> _window;
    std::shared_ptr<BeCamera> _camera;
    std::shared_ptr<BeInput> _input;
    std::shared_ptr<BeDirectionalLight> _directionalLight;
    std::vector<BePointLight> _pointLights;

    std::shared_ptr<BeModel> _plane, _witchItems, _livingCube, _macintosh, _pagoda, _disks, _anvil;

    expose
    MainScene(
        const std::shared_ptr<BeRenderer>& renderer,
        const std::shared_ptr<BeAssetRegistry>& assetRegistry,
        const std::shared_ptr<BeWindow>& window,
        const std::shared_ptr<BeInput>& input
    );
    ~MainScene() override = default;

    auto Prepare() -> void override;
    auto OnLoad() -> void override;
    auto Tick(float deltaTime) -> void override;
    auto DrawUI() -> void;
    
    auto GetRegistry() -> entt::registry& { return _registry; }
    auto GetCamera() -> std::shared_ptr<BeCamera> { return _camera; }
    
private:
    auto CreatePlane(size_t verticesPerSide) -> std::shared_ptr<BeModel>;
};
