#pragma once

#include <memory>
#include <vector>

#include "BeRenderer.h"
#include "umbrellas/access-modifiers.hpp"

class BeSceneManager;
class BeWindow;
class BeRenderer;
class BeInput;
class BeCamera;
class BeAssetRegistry;
class BeLightingPass;
class BeScene;
class BeTexture;
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
    hide std::shared_ptr<BeRenderer> _renderer;
    hide std::shared_ptr<BeInput> _input;
    hide std::shared_ptr<BeCamera> _camera;
    hide std::shared_ptr<BeAssetRegistry> _assetRegistry;
    
    hide std::unique_ptr<BeSceneManager> _sceneManager; 
    
    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose Game();
    expose ~Game();

    // public interface ////////////////////////////////////////////////////////////////////////////////////////////////
    expose auto Run() -> int;

    // private logic ///////////////////////////////////////////////////////////////////////////////////////////////////
    hide auto SetupCamera(int width, int height) -> void;
    hide auto SetupScenes() -> void;
    hide auto MainLoop() -> void;
};
