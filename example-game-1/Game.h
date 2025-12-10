#pragma once

#include <memory>
#include <vector>

#include "umbrellas/access-modifiers.hpp"

// Forward declarations
class BeMaterial;
class BeShader;
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
    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide uint32_t _width;
    hide uint32_t _height;
    hide std::unique_ptr<BeWindow> _window;
    hide std::unique_ptr<BeRenderer> _renderer;
    hide std::unique_ptr<BeInput> _input;
    hide std::unique_ptr<BeCamera> _camera;
    hide std::shared_ptr<BeAssetRegistry> _assetRegistry;
    
    hide std::shared_ptr<BeModel> _plane, _witchItems, _livingCube, _macintosh, _pagoda, _disks, _anvil;
    hide std::unique_ptr<BeDirectionalLight> _directionalLight;
    hide std::vector<BePointLight> _pointLights;

    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose Game();
    expose ~Game();

    // public interface ////////////////////////////////////////////////////////////////////////////////////////////////
    expose auto Run() -> int;

    // private logic ///////////////////////////////////////////////////////////////////////////////////////////////////
    hide auto LoadAssets() -> void;
    hide auto SetupScene() -> void;
    hide auto SetupRenderPasses() -> void;
    hide auto SetupCamera(int width, int height) -> void;
    hide auto MainLoop() -> void;

    hide auto CreatePlane(size_t verticesPerSide) -> std::shared_ptr<BeModel>;
};
