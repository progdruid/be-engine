#include "FullScene.h"

#include <span>
#include <vector>

#include "BeCamera.h"
#include "BeMaterial.h"
#include "BeRenderer.h"
#include "Components.h"
#include "Game.h"
#include "lua/BeLua.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

FullScene::FullScene(Game* game)
    : BaseScene(game)
{}

FullScene::~FullScene() {
    FullScene::OnUnload();
}

auto FullScene::OnLoad() -> void {
    _sceneWatch = BeFileWatcher::Register(
        [this] { return std::vector{ _sceneWatchFilePath }; },
        [this](std::span<const std::filesystem::path>) {
            _sceneLua = std::make_unique<BeLuaState>();
            if (!_sceneWatchFilePath.empty()) {
                _sceneLua->DoFile(_sceneWatchFilePath);
            }

            _sceneWatchFunction();
        }
    );
}

auto FullScene::OnUnload() -> void {
    if (_sceneWatch != 0) {
        BeFileWatcher::Unregister(_sceneWatch);
        _sceneWatch = 0;
    }
}

auto FullScene::SetWatchFile(std::filesystem::path filePath, std::function<void()> onReload) -> void {
    _sceneWatchFilePath = std::move(filePath);
    _sceneWatchFunction = std::move(onReload);

    _sceneLua = std::make_unique<BeLuaState>();
    if (!_sceneWatchFilePath.empty()) {
        _sceneLua->DoFile(_sceneWatchFilePath);
    }
}

auto FullScene::Prepare() -> void {
    const uint32_t screenWidth  = _gameIns->Renderer->GetSwapchainPixelWidth();
    const uint32_t screenHeight = _gameIns->Renderer->GetSwapchainPixelHeight();

    _camera = std::make_shared<BeCamera>();
    _camera->Width = screenWidth;
    _camera->Height = screenHeight;

    _machine = std::make_unique<BeStandardRenderMachine>(_gameIns->Renderer, _assetRegistry, screenWidth, screenHeight);

    Reload(ReloadMask::All);
}

auto FullScene::Reload(ReloadMask mask) -> void {
    if (HasAny(mask, ReloadMask::Settings)) DefineSettings();
    if (HasAny(mask, ReloadMask::Assets)) DefineAssets();
    if (HasAny(mask, ReloadMask::Scene)) DefineScene();
    if (HasAny(mask, ReloadMask::Passes)) DefinePasses();
}

auto FullScene::Tick(float deltaTime) -> void {
    _time += deltaTime;

    for (const auto [_, transform, circling] : _registry.view<TransformComponent, CirclingComponent>().each()) {
        const auto reference = std::abs(circling.Axis.y) > 0.999f ? glm::vec3(0.f, 0.f, 1.f) : glm::vec3(0.f, 1.f, 0.f);
        const auto start = glm::normalize(glm::cross(circling.Axis, reference)) * circling.Radius;
        const auto angle = glm::radians(circling.Phase + circling.Speed * _time);
        const auto rotation = glm::angleAxis(angle, circling.Axis);

        transform.Position = circling.Origin + rotation * start;
        if (circling.Rotate) {
            transform.Rotation = rotation;
        }
    }

    _camera->Update();
    auto& uniformMat = *_machine->UniformMaterial;
    const auto projView = _camera->GetProjectionMatrix() * _camera->GetViewMatrix();
    uniformMat.SetMatrix("CameraProjectionView", projView);
    uniformMat.SetMatrix("CameraInverseProjectionView", glm::inverse(projView));
    uniformMat.SetFloat4("NearFarPlane", { _camera->NearPlane, _camera->FarPlane, 1.0f / _camera->NearPlane, 1.0f / _camera->FarPlane });
    uniformMat.SetFloat3("CameraPosition", _camera->Position);
}

auto FullScene::Render() -> void {
    _machine->Activate();
    _machine->ClearFrame();

    for (const auto [entity, name, transform, render] : _registry.view<NameComponent, TransformComponent, RenderComponent>().each()) {
        _machine->AddGeometry({
            .Name = name.Name,
            .ModelMatrix = BeSRMGeometryEntry::CalculateModelMatrix(transform.Position, transform.Rotation, transform.Scale),
            .Prop = render.Prop,
            .CastShadows = render.CastShadows,
        });
    }

    for (const auto [entity, sunLight] : _registry.view<SunLightComponent>().each()) {
        _machine->AddSunLight({
            .Direction = sunLight.Direction,
            .Color = sunLight.Color,
            .Power = sunLight.Power,
            .CastsShadows = sunLight.CastsShadows,
            .ShadowViewProjection = BeSRMSunLightEntry::CalculateViewProj(
                sunLight.Direction,
                sunLight.ShadowCameraDistance,
                sunLight.ShadowMapWorldSize,
                sunLight.ShadowNearPlane,
                sunLight.ShadowFarPlane
            ),
        });
    }

    for (const auto [entity, name, transform, pointLight] : _registry.view<NameComponent, TransformComponent, PointLightComponent>().each()) {
        _machine->AddPointLight({
            .Name = name.Name,
            .Position = transform.Position,
            .Radius = pointLight.Radius,
            .Color = pointLight.Color,
            .Power = pointLight.Power,
            .CastsShadows = pointLight.CastsShadows,
            .ShadowNearPlane = pointLight.ShadowNearPlane,
        });
    }
}

auto FullScene::ApplyBaseSettings(const BeLuaValue& settings) -> void {
    const auto srm = settings["srm"];
    _machine->Settings.Shadow.Bias = srm["shadow"]["bias"].GetOr(_machine->Settings.Shadow.Bias);
    _machine->Settings.IBL.MaxSampleRadiance = srm["ibl"]["maxSampleRadiance"].GetOr(_machine->Settings.IBL.MaxSampleRadiance);
    _machine->Settings.Skybox.ClampRadiance = srm["skybox"]["clampRadiance"].GetOr(_machine->Settings.Skybox.ClampRadiance);

    const auto srmBloom = srm["bloom"];
    _machine->Settings.Bloom.Threshold = srmBloom["threshold"].GetOr(_machine->Settings.Bloom.Threshold);
    _machine->Settings.Bloom.Knee = srmBloom["knee"].GetOr(_machine->Settings.Bloom.Knee);
    _machine->Settings.Bloom.Intensity = srmBloom["intensity"].GetOr(_machine->Settings.Bloom.Intensity);
    _machine->Settings.Bloom.Clamp = srmBloom["clamp"].GetOr(_machine->Settings.Bloom.Clamp);
    _machine->Settings.Bloom.UpsampleRadius = srmBloom["upsampleRadius"].GetOr(_machine->Settings.Bloom.UpsampleRadius);

    const auto tonemapper = srm["tonemapper"];
    _machine->Settings.Tonemapper.Exposure = tonemapper["exposure"].GetOr(_machine->Settings.Tonemapper.Exposure);
    _machine->Settings.Tonemapper.Contrast = tonemapper["contrast"].GetOr(_machine->Settings.Tonemapper.Contrast);

    const auto camera = settings["camera"];
    _camera->NearPlane = camera["nearPlane"].GetOr(_camera->NearPlane);
    _camera->FarPlane = camera["farPlane"].GetOr(_camera->FarPlane);

    if (const auto ambient = settings["ambient"]["color"]; ambient.Exists()) {
        _machine->UniformMaterial->SetFloat3("AmbientColor", ambient.GetOr(glm::vec3(0.f)));
    }
}

auto FullScene::ApplyBaseScene(const BeLuaValue& objects) -> void {
    _registry.clear();

    for (const auto& [name, entityTable] : objects.Pairs()) {
        const auto entity = _registry.create();
        _registry.emplace<NameComponent>(entity, NameComponent{ .Name = name });

        if (const auto table = entityTable["transform"]; table.Exists()) {
            TransformComponent comp;
            comp.Position = table["position"].GetOr(comp.Position);
            comp.Rotation = table["rotation"].GetOr(comp.Rotation);
            comp.Scale = table["scale"].GetOr(comp.Scale);
            _registry.emplace<TransformComponent>(entity, comp);
        }

        if (const auto table = entityTable["render"]; table.Exists()) {
            const auto propName = table["prop"].Get<std::string>();
            if (propName) {
                if (const auto prop = _assetRegistry.GetProp(*propName).lock()) {
                    _registry.emplace<RenderComponent>(entity, RenderComponent{
                        .Prop = prop,
                        .CastShadows = table["castShadows"].GetOr(true),
                    });
                }
            }
        }

        if (const auto table = entityTable["circling"]; table.Exists()) {
            CirclingComponent comp;
            comp.Origin = table["origin"].GetOr(comp.Origin);
            comp.Axis = glm::normalize(table["axis"].GetOr(comp.Axis));
            comp.Radius = table["radius"].GetOr(comp.Radius);
            comp.Speed = table["speed"].GetOr(comp.Speed);
            comp.Phase = table["phase"].GetOr(comp.Phase);
            comp.Rotate = table["rotate"].GetOr(comp.Rotate);
            _registry.emplace<CirclingComponent>(entity, comp);
        }

        if (entityTable["static"].Exists()) {
            _registry.emplace<StaticTag>(entity);
        }

        if (const auto table = entityTable["sunLight"]; table.Exists()) {
            SunLightComponent comp;
            comp.Direction = glm::normalize(table["direction"].GetOr(comp.Direction));
            comp.Color = table["color"].GetOr(comp.Color);
            comp.Power = table["power"].GetOr(comp.Power);
            comp.CastsShadows = table["castsShadows"].GetOr(comp.CastsShadows);
            comp.ShadowCameraDistance = table["shadowCameraDistance"].GetOr(comp.ShadowCameraDistance);
            comp.ShadowMapWorldSize = table["shadowMapWorldSize"].GetOr(comp.ShadowMapWorldSize);
            comp.ShadowNearPlane = table["shadowNearPlane"].GetOr(comp.ShadowNearPlane);
            comp.ShadowFarPlane = table["shadowFarPlane"].GetOr(comp.ShadowFarPlane);
            _registry.emplace<SunLightComponent>(entity, comp);
        }

        if (const auto table = entityTable["pointLight"]; table.Exists()) {
            PointLightComponent comp;
            comp.Radius = table["radius"].GetOr(comp.Radius);
            comp.Color = table["color"].GetOr(comp.Color);
            comp.Power = table["power"].GetOr(comp.Power);
            comp.CastsShadows = table["castsShadows"].GetOr(comp.CastsShadows);
            comp.ShadowNearPlane = table["shadowNearPlane"].GetOr(comp.ShadowNearPlane);
            _registry.emplace<PointLightComponent>(entity, comp);
        }
    }
}
