#include "MenuScene.h"

#include <iostream>
#include <scenes/BeSceneManager.h>

#include "BeRenderer.h"
#include "imgui/BeImGuiPass.h"
#include "imgui/imgui.h"

MenuScene::MenuScene(
    BeSceneManager* sceneManager,
    const std::shared_ptr<BeRenderer>& renderer,
    const std::shared_ptr<BeAssetRegistry>& assetRegistry,
    const std::shared_ptr<BeWindow>& window,
    const std::shared_ptr<BeInput>& input
)
    : BaseScene(sceneManager)
    , _renderer(renderer)
    , _assetRegistry(assetRegistry)
    , _window(window)
    , _input(input)
{}

auto MenuScene::OnLoad() -> void {

    _renderer->ClearPasses();

    auto imguiPass = new BeImGuiPass(_window);
    _renderer->AddRenderPass(imguiPass);
    imguiPass->SetUICallback([this](){RunUI();});

    _renderer->InitialisePasses();
}

auto MenuScene::RunUI() -> void {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

    ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

    ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.3f);

    ImGui::SetWindowFontScale(10.f);
    
    float textWidth = ImGui::CalcTextSize("be!").x;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - textWidth) * 0.5f);
    ImGui::Text("be!");

    ImGui::SetWindowFontScale(1.5);
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.5f);

    // Style buttons with white interior and black outline
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);

    float buttonWidth = 150.0f;
    float windowWidth = ImGui::GetWindowWidth();
    ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);

    if (ImGui::Button("Play", ImVec2(buttonWidth, 50))) {
        _sceneManager->RequestSceneChange("main");
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);

    ImGui::SetWindowFontScale(1.0);
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.85f);

    float creditsWidth = ImGui::CalcTextSize("by Zak @progdruid").x;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - creditsWidth) * 0.5f);
    ImGui::Text("by Zak @progdruid");

    ImGui::End();

    ImGui::PopStyleColor(2);
}
