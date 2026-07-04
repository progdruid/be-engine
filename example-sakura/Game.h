#pragma once

#include <memory>
#include <vector>
#include <umbrellas/common.hpp>

class BeStandardRenderMachine;
class BeSceneManager;
class BeInput;
class BeRenderer;
class BeWindow;

class Game {
    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose uint32_t Width;
    expose uint32_t Height;
    expose std::shared_ptr<BeWindow> Window;
    expose std::shared_ptr<BeRenderer> Renderer;
    expose std::shared_ptr<BeInput> Input;
    expose std::unique_ptr<BeSceneManager> SceneManager; 
    expose std::shared_ptr<BeStandardRenderMachine> SRM;
    
    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose Game();
    expose ~Game();

    // public interface ////////////////////////////////////////////////////////////////////////////////////////////////
    expose auto Run() -> int;

    // private logic ///////////////////////////////////////////////////////////////////////////////////////////////////
    hide auto SetupScenes() -> void;
    hide auto MainLoop() -> void;
};
