#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <concepts>
#include <cassert>

class BeScene;

class BeSceneManager {
private:
    std::unordered_map<std::string, std::unique_ptr<BeScene>> _scenes;
    BeScene* _activeScene = nullptr;
    std::string _pendingSceneName;
    bool _hasPendingSceneChange = false;

public:
    auto RegisterScene(const std::string& name, std::unique_ptr<BeScene> scene) -> void;
    auto RequestSceneChange(const std::string& name) -> void;
    auto ApplyPendingSceneChange() -> void;

    auto GetActiveScene() const -> BeScene* { return _activeScene; }

    template<std::derived_from<BeScene> T>
    auto GetActiveScene() -> T* {
        assert(dynamic_cast<T*>(_activeScene) != nullptr);
        return static_cast<T*>(_activeScene);
    }

    
    auto GetScene(const std::string& name) -> BeScene*;

    template<std::derived_from<BeScene> T>
    auto GetScene(const std::string& name) -> T* {
        auto scene = GetScene(name);
        assert(dynamic_cast<T*>(scene) != nullptr);
        return static_cast<T*>(scene);
    }
};
