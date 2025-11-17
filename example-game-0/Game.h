#pragma once

#include <memory>
#include <vector>

// Forward declarations
class BeWindow;
class BeRenderer;
class BeInput;
class BeCamera;
class BeAssetRegistry;
struct BeRenderPass;
struct BeModel;
struct BeDirectionalLight;
struct BePointLight;

class Game {
private:
    std::unique_ptr<BeWindow> _window;
    std::unique_ptr<BeRenderer> _renderer;
    std::unique_ptr<BeInput> _input;
    std::unique_ptr<BeCamera> _camera;
    std::unique_ptr<BeAssetRegistry> _assetRegistry;

    std::shared_ptr<BeModel> _witchItems, _cube, _macintosh, _pagoda, _disks, _anvil;
    std::unique_ptr<BeDirectionalLight> _directionalLight;
    std::vector<BePointLight> _pointLights;

public:
    Game();
    ~Game();

public:
    auto Run() -> int;

private:
    auto LoadAssets() -> void;
    auto SetupScene() -> void;
    auto SetupRenderPasses() -> void;
    auto SetupCamera(int width, int height) -> void;
    auto MainLoop() -> void;
};
