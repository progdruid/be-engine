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
class BeLightingPass;
class BeScene;
class BeTexture;
class BeMaterial;
class BeShader;
class BeRenderPass;
struct BeModel;
struct BeDirectionalLight;
struct BePointLight;

class Game {
    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide uint32_t Width;
    hide uint32_t Height;
    hide std::shared_ptr<BeWindow> Window;
    hide std::shared_ptr<BeRenderer> Renderer;
    hide std::shared_ptr<BeInput> Input;
    hide std::shared_ptr<BeCamera> _camera;
    
    hide std::unique_ptr<BeSceneManager> SceneManager; 
    
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
