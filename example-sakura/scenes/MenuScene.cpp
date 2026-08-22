#include "MenuScene.h"

#include <iostream>
#include <umbrellas/include-glfw.h>
#include <scenes/BeSceneManager.h>

#include "BeInput.h"
#include "BeRenderer.h"
#include "BeWindow.h"
#include "Game.h"
#include "imgui/BeImGuiPass.h"
#include "imgui/imgui.h"

MenuScene::MenuScene(Game* game) : BaseScene(game) {}
MenuScene::~MenuScene() = default;

auto MenuScene::OnLoad() -> void {

    auto imguiPass = std::make_unique<BeImGuiPass>(_gameIns->Window);
    imguiPass->SetUICallback([this](){RunUI();});
    imguiPass->Initialise(*_gameIns->Renderer);
    _sequence.Passes.clear();
    _sequence.Passes.push_back(std::move(imguiPass));

    ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/i-hate-comic-sans.regular.ttf", 16.0f);
    _titleFont = ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/somelist.ttf", 16.0f);
}

auto MenuScene::Render() -> void {
    _gameIns->Renderer->SetSequence(&_sequence);
}

auto MenuScene::Tick(float deltaTime) -> void {
    if (_gameIns->Input->GetKeyDown(GLFW_KEY_ESCAPE)) {
        _gameIns->Window->RequestClose();
    }
}

auto MenuScene::RunUI() -> void {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

    ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

    ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.3f);

    ImGui::SetWindowFontScale(10.f);
    ImGui::PushFont(_titleFont);
    auto titleText = "project <sakura>";
    auto textWidth = ImGui::CalcTextSize(titleText).x;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - textWidth) * 0.5f);
    ImGui::Text(titleText);
    ImGui::PopFont();

    ImGui::SetWindowFontScale(2.0f);
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.5f);

    // Style buttons with white interior and black outline
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);

    auto buttonWidth = 200.0f;
    auto buttonHeight = 70.f;
    auto windowWidth = ImGui::GetWindowWidth();
    ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);

    if (ImGui::Button("Sakura", ImVec2(buttonWidth, buttonHeight))) {
        _gameIns->SceneManager->RequestSceneChange("sakura");
    }
    
    ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.6f);
    
    if (ImGui::Button("Showcase", ImVec2(buttonWidth, buttonHeight))) {
        _gameIns->SceneManager->RequestSceneChange("showcase");
    }

    ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.7f);

    if (ImGui::Button("Rift", ImVec2(buttonWidth, buttonHeight))) {
        _gameIns->SceneManager->RequestSceneChange("rift");
    }

    ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.8f);

    if (ImGui::Button("Old", ImVec2(buttonWidth, buttonHeight))) {
        _gameIns->SceneManager->RequestSceneChange("old");
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);

    ImGui::SetWindowFontScale(1.0);
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.9f);

    auto creditsText = "by @progdruid";
    auto creditsWidth = ImGui::CalcTextSize(creditsText).x;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - creditsWidth) * 0.5f);
    ImGui::Text(creditsText);

    ImGui::End();

    ImGui::PopStyleColor(2);
}
