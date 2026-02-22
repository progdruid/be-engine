#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <concepts>
#include <typeinfo>
#include <umbrellas/include-libassert.h>

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
        be_assert(
            dynamic_cast<T*>(_activeScene) != nullptr,
            "active scene type mismatch: requested " + 
            std::string(libassert::pretty_type_name<T>()) + 
            ", got " + typeid(*_activeScene).name()
        );
        return static_cast<T*>(_activeScene);
    }

    
    auto GetScene(const std::string& name) -> BeScene*;

    template<std::derived_from<BeScene> T>
    auto GetScene(const std::string& name) -> T* {
        auto scene = GetScene(name);
        be_assert(
            dynamic_cast<T*>(scene) != nullptr,
            "scene type mismatch for '" + name + 
            "': requested " + std::string(libassert::pretty_type_name<T>()) + 
            ", got " + typeid(*scene).name()
        );
        return static_cast<T*>(scene);
    }
};
