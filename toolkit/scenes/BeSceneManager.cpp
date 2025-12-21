#include "BeSceneManager.h"
#include "BeScene.h"
#include <cassert>

auto BeSceneManager::RegisterScene(const std::string& name, std::unique_ptr<BeScene> scene) -> void {
    _scenes[name] = std::move(scene);
}

auto BeSceneManager::RequestSceneChange(const std::string& name) -> void {
    assert(_scenes.contains(name));
    _pendingSceneName = name;
    _hasPendingSceneChange = true;
}

auto BeSceneManager::ApplyPendingSceneChange() -> void {
    if (!_hasPendingSceneChange) {
        return;
    }

    if (_activeScene) {
        _activeScene->OnUnload();
    }

    _activeScene = _scenes[_pendingSceneName].get();
    _activeScene->OnLoad();

    _hasPendingSceneChange = false;
}

auto BeSceneManager::GetScene(const std::string& name) -> BeScene* {
    assert(_scenes.contains(name));
    return _scenes[name].get();
}
