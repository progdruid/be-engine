#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <umbrellas/common.hpp>

#include "BeWindow.h"
#include "sen-rhi/SenTypes.h"

class BeRenderer;
class BeInput;
class BeSceneManager;

struct BeStandardGameConfig {
    std::string Title = "be";
    BeWindowMode WindowMode = BeWindowMode::Windowed;
    uint32_t Width = 1280;
    uint32_t Height = 720;
    SenPresentMode PresentMode = SenPresentMode::VSync;
};

class BeStandardGame {
    expose uint32_t Width;
    expose uint32_t Height;
    expose std::shared_ptr<BeWindow> Window;
    expose std::shared_ptr<BeRenderer> Renderer;
    expose std::shared_ptr<BeInput> Input;
    expose std::unique_ptr<BeSceneManager> SceneManager;

    expose explicit BeStandardGame(const BeStandardGameConfig& config);
    expose ~BeStandardGame();

    expose auto Run() -> int;

    hide auto MainLoop() -> void;
};
