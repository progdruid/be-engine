#include "MenuScene.h"

#include <iostream>
#include <scenes/BeSceneManager.h>

#include "BeRenderer.h"
#include "Game.h"
#include "imgui/BeImGuiPass.h"
#include "imgui/imgui.h"

MenuScene::MenuScene(Game* game) : BaseScene(game) {}

auto MenuScene::OnLoad() -> void {

    GameIns->Renderer->ClearPasses();

    auto imguiPass = new BeImGuiPass(GameIns->Window);
    GameIns->Renderer->AddRenderPass(imguiPass);
    imguiPass->SetUICallback([this](){RunUI();});

    GameIns->Renderer->InitialisePasses();
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
        GameIns->SceneManager->RequestSceneChange("main");
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
