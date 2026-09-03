#include "BeStandardFullScene.h"

#include <cstdio>
#include <span>
#include <vector>

#include "BeCamera.h"
#include "BeMaterial.h"
#include "BeMeshPrimitives.h"
#include "BeProp.h"
#include "BeRenderer.h"
#include "BeShaderLibrary.h"
#include "BeTexture.h"
#include "Components.h"
#include "BeStandardGame.h"
#include "lua/BeLua.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

BeStandardFullScene::BeStandardFullScene(BeStandardGame* game)
    : BeStandardBaseScene(game)
{}

BeStandardFullScene::~BeStandardFullScene() {
    BeStandardFullScene::OnUnload();
}

auto BeStandardFullScene::OnLoad() -> void {
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

auto BeStandardFullScene::OnUnload() -> void {
    _coroutineScheduler.Clear();

    if (_sceneWatch != 0) {
        BeFileWatcher::Unregister(_sceneWatch);
        _sceneWatch = 0;
    }
}

auto BeStandardFullScene::SetWatchFile(std::filesystem::path filePath, std::function<void()> onReload) -> void {
    _sceneWatchFilePath = std::move(filePath);
    _sceneWatchFunction = std::move(onReload);

    _sceneLua = std::make_unique<BeLuaState>();
    if (!_sceneWatchFilePath.empty()) {
        _sceneLua->DoFile(_sceneWatchFilePath);
    }
}

auto BeStandardFullScene::Prepare() -> void {
    const uint32_t screenWidth  = _game->Renderer->GetSwapchainPixelWidth();
    const uint32_t screenHeight = _game->Renderer->GetSwapchainPixelHeight();

    _camera = std::make_shared<BeCamera>();
    _camera->Width = screenWidth;
    _camera->Height = screenHeight;

    _machine = std::make_unique<BeStandardRenderMachine>(_game->Renderer, screenWidth, screenHeight);

    Reload(ReloadMask::All);
}

auto BeStandardFullScene::Reload(ReloadMask mask) -> void {
    if (HasAny(mask, ReloadMask::Settings)) DefineSettings();
    if (HasAny(mask, ReloadMask::Assets)) DefineAssets();
    if (HasAny(mask, ReloadMask::Scene)) DefineScene();
    if (HasAny(mask, ReloadMask::Passes)) DefinePasses();
}

auto BeStandardFullScene::Tick(float deltaTime) -> void {
    _time += deltaTime;

    _coroutineScheduler.Update(deltaTime);

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

    _camera->Width = _game->Renderer->GetSwapchainPixelWidth();
    _camera->Height = _game->Renderer->GetSwapchainPixelHeight();
    _camera->Update();
    auto& uniformMat = *_machine->UniformMaterial;
    const auto projView = _camera->GetProjectionMatrix() * _camera->GetViewMatrix();
    uniformMat.SetMatrix("CameraProjectionView", projView);
    uniformMat.SetMatrix("CameraInverseProjectionView", glm::inverse(projView));
    uniformMat.SetFloat4("NearFarPlane", { _camera->NearPlane, _camera->FarPlane, 1.0f / _camera->NearPlane, 1.0f / _camera->FarPlane });
    uniformMat.SetFloat3("CameraPosition", _camera->Position);
    uniformMat.SetFloat1("Time", _time);
}

auto BeStandardFullScene::Render() -> void {
    _machine->PollRenderer();
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

auto BeStandardFullScene::ApplyLuaSettings(const BeLuaValue& settings) -> void {
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

    const auto backbuffer = settings["backbuffer"];
    _machine->Settings.Backbuffer.BackgroundColor = backbuffer["backgroundColor"].GetOr(_machine->Settings.Backbuffer.BackgroundColor);
    _machine->Settings.Backbuffer.DiscardFar = backbuffer["discardFar"].GetOr(_machine->Settings.Backbuffer.DiscardFar);

    const auto camera = settings["camera"];
    _camera->NearPlane = camera["nearPlane"].GetOr(_camera->NearPlane);
    _camera->FarPlane = camera["farPlane"].GetOr(_camera->FarPlane);

    if (const auto ambient = settings["ambient"]["color"]; ambient.Exists()) {
        _machine->UniformMaterial->SetFloat3("AmbientColor", ambient.GetOr(glm::vec3(0.f)));
    }
}

auto BeStandardFullScene::ApplyLuaScene(const BeLuaValue& objects) -> void {
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

auto BeStandardFullScene::ApplyLuaAssets(const BeLuaValue& assets) -> void {
    for (const auto& [name, texture] : assets["textures"].Pairs()) {
        const auto file = texture["file"].Get<std::string>();
        if (!file) {
            std::fprintf(stderr, "[lua] texture '%s' has no file, skipping\n", name.c_str());
            continue;
        }
        BeTexture::Create(name)
            .LoadFromFile(*file)
            .AddToRegistry(_assetRegistry)
            .BuildNoReturn();
    }

    for (const auto& [name, propDef] : assets["props"].Pairs()) {
        const auto shaderName = propDef["shader"].Get<std::string>();
        if (!shaderName) {
            std::fprintf(stderr, "[lua] prop '%s' has no shader, skipping\n", name.c_str());
            continue;
        }
        const auto shader = BeShaderLibrary::GetShader(*shaderName);

        const auto meshDef = propDef["mesh"];
        std::shared_ptr<BeProp> prop;

        if (const auto file = meshDef["file"].Get<std::string>()) {
            const auto model = meshDef["lighting"].GetOr(std::string("pbr")) == "phong"
                ? BeSRMLightingModel::Phong
                : BeSRMLightingModel::PBR;
            prop = _machine->LoadProp(*file, shader, model);
        }
        else {
            const auto primitive = meshDef["primitive"].GetOr(std::string("cube"));
            std::shared_ptr<BeMesh> mesh;
            if (primitive == "plane") {
                mesh = BeMeshPrimitives::Plane(meshDef["subdivisions"].GetOr(1u));
            } else if (primitive == "sphere") {
                mesh = BeMeshPrimitives::Sphere(meshDef["rings"].GetOr(16u), meshDef["segments"].GetOr(32u));
            } else {
                mesh = BeMeshPrimitives::Cube();
            }
            prop = BeProp::FromMesh(mesh, shader, propDef["material"].GetOr(std::string("geometry-main")));
            _machine->RegisterMesh(prop->Mesh);
        }

        for (const auto& [key, value] : propDef["set"].Pairs()) {
            for (const auto& material : prop->Materials) {
                ApplyMaterialSet(*material, key, value);
            }
        }

        _assetRegistry.AddProp(name, prop);
    }
}

auto BeStandardFullScene::ResolveTexture(const BeLuaValue& value) -> std::shared_ptr<BeTexture> {
    if (const auto name = value.Get<std::string>()) {
        if (!_assetRegistry.HasTexture(*name)) {
            std::fprintf(stderr, "[lua] texture '%s' is not registered\n", name->c_str());
            return nullptr;
        }
        return _assetRegistry.GetTexture(*name).lock();
    }

    const auto name = value["name"].Get<std::string>();
    const auto file = value["file"].Get<std::string>();
    if (!name || !file) {
        std::fprintf(stderr, "[lua] inline texture needs a name and a file\n");
        return nullptr;
    }
    return BeTexture::Create(*name)
        .LoadFromFile(*file)
        .AddToRegistry(_assetRegistry)
        .Build();
}

auto BeStandardFullScene::ApplyMaterialSet(BeMaterial& material, const std::string& key, const BeLuaValue& value) -> void {
    const auto& scheme = material.GetScheme();

    for (const auto& property : scheme.Properties) {
        if (property.Name != key) { continue; }

        switch (property.PropertyType) {
            using enum BeMaterialPropertyDescriptor::Type;
            case Float:  if (const auto v = value.Get<float>())     material.SetFloat1(key, *v); break;
            case Float2: if (const auto v = value.Get<glm::vec2>()) material.SetFloat2(key, *v); break;
            case Float3: if (const auto v = value.Get<glm::vec3>()) material.SetFloat3(key, *v); break;
            case Float4: if (const auto v = value.Get<glm::vec4>()) material.SetFloat4(key, *v); break;
            case Matrix: break;
        }
        return;
    }

    for (const auto& texture : scheme.Textures) {
        if (texture.Name != key) { continue; }

        if (const auto resolved = ResolveTexture(value)) {
            material.SetTexture(key, resolved);
        }
        return;
    }

    for (const auto& sampler : scheme.Samplers) {
        if (sampler.Name != key) { continue; }

        if (const auto name = value.Get<std::string>()) {
            material.SetSampler(key, BeShaderLibrary::GetSampler(*name));
        }
        return;
    }
}
