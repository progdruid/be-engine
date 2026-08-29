#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string>

#include <umbrellas/common.hpp>

#include "BaseScene.h"
#include "BeAssetRegistry.h"
#include "BeFileWatcher.h"
#include "entt/entt.hpp"

class BeCamera;
class BeStandardRenderMachine;
class BeLuaState;
class BeLuaValue;
class BeMaterial;
class BeTexture;

enum class ReloadMask : uint8_t {
    None = 0,
    Settings = 1 << 0,
    Assets = 1 << 1,
    Scene = 1 << 2,
    Passes = 1 << 3,
    All = Settings | Assets | Scene | Passes,
};
ENABLE_BITMASK(ReloadMask);

class FullScene : public BaseScene {

    protect
    BeAssetRegistry _assetRegistry;
    entt::registry _registry;
    std::shared_ptr<BeCamera> _camera;
    std::unique_ptr<BeStandardRenderMachine> _machine;
    float _time = 0.0f;
    std::unique_ptr<BeLuaState> _sceneLua;

    hide
    std::filesystem::path _sceneWatchFilePath;
    std::function<void()> _sceneWatchFunction;
    BeFileWatcher::WatchId _sceneWatch = 0;

    expose
    explicit FullScene(Game* game);
    ~FullScene() override;

    auto OnLoad() -> void override;
    auto OnUnload() -> void override;
    auto Prepare() -> void override;
    auto Tick(float deltaTime) -> void override;
    auto Render() -> void override;

    protect
    virtual auto Reload(ReloadMask mask = ReloadMask::Settings | ReloadMask::Scene | ReloadMask::Passes) -> void;

    virtual auto DefineSettings() -> void {}
    virtual auto DefineAssets() -> void {}
    virtual auto DefineScene() -> void {}
    virtual auto DefinePasses() -> void {}

    auto ApplyLuaSettings(const BeLuaValue& settings) -> void;
    auto ApplyLuaAssets(const BeLuaValue& assets) -> void;
    auto ApplyLuaScene(const BeLuaValue& objects) -> void;
    auto SetWatchFile(std::filesystem::path filePath, std::function<void()> onReload) -> void;

    hide
    auto ResolveTexture(const BeLuaValue& value) -> std::shared_ptr<BeTexture>;
    auto ApplyMaterialSet(BeMaterial& material, const std::string& key, const BeLuaValue& value) -> void;
};
