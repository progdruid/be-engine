#include "BeSceneManager.h"
#include "BeScene.h"
#include <cassert>

auto BeSceneManager::RegisterScene(const std::string& name, std::unique_ptr<BeScene> scene) -> void {
    _scenes[name] = std::move(scene);
}

auto BeSceneManager::RequestSceneChange(const std::string& name) -> void {
    be_assert(
        _scenes.contains(name),
        "scene manager doesn't contain the scene: " + name
    );
    _pendingSceneName = name;
    _hasPendingSceneChange = true;
}

auto BeSceneManager::ApplyPendingSceneChange() -> void {
    if (!_hasPendingSceneChange) {
        return;
    }
    _hasPendingSceneChange = false;

    if (_activeScene) {
        _activeScene->OnUnload();
    }

    _activeScene = _scenes[_pendingSceneName].get();
    _activeScene->OnLoad();
}

auto BeSceneManager::GetScene(const std::string& name) -> BeScene* {
    be_assert(
        _scenes.contains(name),
        "scene manager doesn't contain the scene: " + name
    );
    return _scenes[name].get();
}
