#pragma once

#include <memory>
#include <entt/entt.hpp>
#include <umbrellas/access-modifiers.hpp>
#include <umbrellas/include-glm.h>

class BeAssetRegistry;
class BeRenderer;
class BeWindow;
class BeCamera;
class BeInput;
struct BeModel;

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

class Game {
private:
    uint32_t _width;
    uint32_t _height;
    std::shared_ptr<BeWindow> _window;
    std::shared_ptr<BeRenderer> _renderer;
    std::unique_ptr<BeInput> _input;
    std::unique_ptr<BeCamera> _camera;
    std::shared_ptr<BeAssetRegistry> _assetRegistry;

    entt::registry _registry;
    
public:
    Game();
    ~Game();

    auto Run() -> int;

private:
    auto SetupRenderPasses() -> void;
    auto MainLoop() -> void;
};
