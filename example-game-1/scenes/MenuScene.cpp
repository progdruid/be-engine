#include "MenuScene.h"
#include <scenes/BeSceneManager.h>

MenuScene::MenuScene(BeSceneManager* sceneManager) : BaseScene(sceneManager) {}

auto MenuScene::OnLoad() -> void {
    _sceneManager->RequestSceneChange("main");
}
