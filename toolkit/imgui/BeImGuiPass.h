#pragma once

#include <functional>
#include <memory>

#include "BeRenderPass.h"

class BeWindow;

class BeImGuiPass final : public BeRenderPass {
    hide
    std::shared_ptr<BeWindow> _window = nullptr;
    std::function<void()> _uiCallback = nullptr;
    bool _holdsBackendRef = false;
    
    expose
    explicit BeImGuiPass(const std::shared_ptr<BeWindow>& window);
    ~BeImGuiPass() override;

    auto Initialise() -> void override;
    auto Render(SenCommandBuffer& cmd) -> void override;
    auto GetPassName() const -> const std::string override { return "ImGui Pass"; }

    auto SetUICallback(const std::function<void()>& callback) -> void;
};

