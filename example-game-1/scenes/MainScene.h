#pragma once

#include <memory>
#include <string>
#include <vector>
#include <umbrellas/include-glm.h>
#include <umbrellas/access-modifiers.hpp>

#include "entt/entt.hpp"
#include "BaseScene.h"

class BeCamera;
struct BeProp;
struct BeMesh;
struct BePointLight;
struct BeDirectionalLight;

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

class MainScene : public BaseScene {
    hide
    entt::registry _registry;
    std::shared_ptr<BeCamera> _camera;
    std::shared_ptr<BeDirectionalLight> _directionalLight;
    std::vector<BePointLight> _pointLights;

    std::shared_ptr<BeProp> _plane, _witchItems, _cube, _macintosh, _pagoda, _disks, _anvil;

    expose
    explicit MainScene(Game* game);
    ~MainScene() override = default;

    auto Prepare() -> void override;
    auto OnLoad() -> void override;
    auto Tick(float deltaTime) -> void override;
    
};
