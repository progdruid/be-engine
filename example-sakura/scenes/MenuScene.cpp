#include "MenuScene.h"

#include <algorithm>
#include <iostream>
#include <umbrellas/include-glfw.h>
#include <scenes/BeSceneManager.h>

#include "BeInput.h"
#include "BeRenderer.h"
#include "BeWindow.h"
#include "standard-game/BeStandardGame.h"
#include "imgui/BeImGuiPass.h"
#include "imgui/imgui.h"

MenuScene::MenuScene(BeStandardGame* game) : BeStandardBaseScene(game) {}
MenuScene::~MenuScene() = default;

auto MenuScene::OnLoad() -> void {

    auto imguiPass = std::make_unique<BeImGuiPass>(_game->Window);
    imguiPass->SetUICallback([this](){RunUI();});
    imguiPass->Initialise(*_game->Renderer);
    _sequence.Passes.clear();
    _sequence.Passes.push_back(std::move(imguiPass));

    _bodyFont = ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/i-hate-comic-sans.regular.ttf", 16.0f);
    _titleFont = ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/somelist.ttf", 16.0f);
}

auto MenuScene::Render() -> void {
    _game->Renderer->SetSequence(&_sequence);
}

auto MenuScene::Tick(float deltaTime) -> void {
    if (_game->Input->GetKeyDown(GLFW_KEY_ESCAPE)) {
        _game->Window->RequestClose();
    }
}

auto MenuScene::RunUI() -> void {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

    ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

    ImGui::PushFont(_bodyFont);

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

    const char* scenes[] = { "sakura", "showcase", "rift", "old", "video" };
    const int sceneCount = static_cast<int>(std::size(scenes));

    const float windowWidth = ImGui::GetWindowWidth();
    const float windowHeight = ImGui::GetWindowHeight();

    const float buttonWidth = 200.0f;
    const float buttonHeight = 50.0f;
    const float groupTop = windowHeight * 0.5f;
    const float groupHeight = windowHeight * 0.4f;
    const float minSpacing = 0.0f;
    const float maxSpacing = 0.0f;

    float spacing = sceneCount > 1
        ? (groupHeight - sceneCount * buttonHeight) / (sceneCount - 1)
        : 0.0f;
    spacing = std::clamp(spacing, minSpacing, maxSpacing);

    const float blockHeight = sceneCount * buttonHeight + (sceneCount - 1) * spacing;
    const float startY = groupTop + (groupHeight - blockHeight) * 0.5f;

    for (int i = 0; i < sceneCount; ++i) {
        ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
        ImGui::SetCursorPosY(startY + i * (buttonHeight + spacing));
        if (ImGui::Button(scenes[i], ImVec2(buttonWidth, buttonHeight))) {
            _game->SceneManager->RequestSceneChange(scenes[i]);
        }
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);

    ImGui::SetWindowFontScale(1.0);
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.9f);

    auto creditsText = "by @progdruid";
    auto creditsWidth = ImGui::CalcTextSize(creditsText).x;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - creditsWidth) * 0.5f);
    ImGui::Text(creditsText);

    ImGui::PopFont();

    ImGui::End();

    ImGui::PopStyleColor(2);
}
