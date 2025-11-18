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
    pri std::unique_ptr<BeWindow> _window;
    pri std::unique_ptr<BeRenderer> _renderer;
    pri std::unique_ptr<BeInput> _input;
    pri std::unique_ptr<BeCamera> _camera;
    pri std::unique_ptr<BeAssetRegistry> _assetRegistry;
    
    pri std::shared_ptr<BeModel> _plane, _witchItems, _cube, _macintosh, _pagoda, _disks, _anvil;
    pri std::unique_ptr<BeDirectionalLight> _directionalLight;
    pri std::vector<BePointLight> _pointLights;

    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    pub Game();
    pub ~Game();

    // public interface ////////////////////////////////////////////////////////////////////////////////////////////////
    pub auto Run() -> int;

    // private logic ///////////////////////////////////////////////////////////////////////////////////////////////////
    pri auto LoadAssets() -> void;
    pri auto SetupScene() -> void;
    pri auto SetupRenderPasses() -> void;
    pri auto SetupCamera(int width, int height) -> void;
    pri auto MainLoop() -> void;

    pri auto CreatePlane(size_t verticesPerSide) -> std::shared_ptr<BeModel>;
};
