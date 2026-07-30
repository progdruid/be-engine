#include "BeQuickstart.h"

#include <umbrellas/include-glfw.h>
#include <umbrellas/include-libassert.h>

#include "BeAssetRegistry.h"
#include "BeCamera.h"
#include "BeInput.h"
#include "BeMaterial.h"
#include "BeRenderer.h"
#include "BeShaderLibrary.h"
#include "BeTexture.h"
#include "BeWindow.h"
#include "free-camera/BeFreeCameraController.h"

BeQuickstart::BeQuickstart() = default;
BeQuickstart::~BeQuickstart() = default;

auto BeQuickstart::StandardShader() -> raw_ptr<BeShader> {
    return BeShaderLibrary::GetShader("standard-pbr");
}

auto BeQuickstart::PhongShader() -> raw_ptr<BeShader> {
    return BeShaderLibrary::GetShader("standard-phong");
}

auto BeQuickstart::CreateProp(std::shared_ptr<BeMesh> mesh, raw_ptr<BeShader> shader) -> std::shared_ptr<BeProp> {
    be_assert(!_meshesBaked, "props must be created from OnStart, before the first frame");

    auto prop = BeProp::FromMesh(std::move(mesh), shader ? shader : StandardShader(), "geometry-main");
    SRM->RegisterMesh(prop->Mesh);
    return prop;
}

auto BeQuickstart::LoadProp(const std::filesystem::path& modelPath, raw_ptr<BeShader> shader) -> std::shared_ptr<BeProp> {
    be_assert(!_meshesBaked, "props must be created from OnStart, before the first frame");

    return SRM->LoadProp(modelPath, shader ? shader : StandardShader());
}

auto BeQuickstart::RenderProp(
    const std::string& name,
    const std::shared_ptr<BeProp>& prop,
    const glm::mat4& modelMatrix,
    bool castShadows
) -> void {
    SRM->AddGeometry({
        .Name = name,
        .ModelMatrix = modelMatrix,
        .Prop = prop,
        .CastShadows = castShadows,
    });
}

auto BeQuickstart::RenderProp(
    const std::string& name,
    const std::shared_ptr<BeProp>& prop,
    glm::vec3 position,
    glm::quat rotation,
    glm::vec3 scale,
    bool castShadows
) -> void {
    RenderProp(name, prop, BeSRMGeometryEntry::CalculateModelMatrix(position, rotation, scale), castShadows);
}

auto BeQuickstart::RenderSunLight(const BeQuickstartSunLight& light) -> void {
    std::shared_ptr<BeTexture> shadowMap;

    if (light.CastsShadows) {
        if (_sunLightCursor >= _sunShadowMaps.size()) {
            _sunShadowMaps.push_back(
                BeTexture::Create("Quickstart_SunShadowMap" + std::to_string(_sunLightCursor))
                .SetUsage(SenTextureUsage::DepthStencil | SenTextureUsage::ShaderResource)
                .SetFormat(SenFormat::Depth32)
                .SetSize(light.ShadowMapResolution, light.ShadowMapResolution)
                .Build()
            );
        }
        shadowMap = _sunShadowMaps[_sunLightCursor];
        _sunLightCursor++;
    }

    SRM->AddSunLight({
        .Direction = light.Direction,
        .Color = light.Color,
        .Power = light.Power,
        .CastsShadows = light.CastsShadows,
        .ShadowViewProjection = BeSRMSunLightEntry::CalculateViewProj(
            light.Direction,
            light.ShadowCameraDistance,
            light.ShadowMapWorldSize,
            light.ShadowNearPlane,
            light.ShadowFarPlane
        ),
        .ShadowMapResolution = light.ShadowMapResolution,
        .ShadowMap = shadowMap,
    });
}

auto BeQuickstart::RenderPointLight(const BeQuickstartPointLight& light) -> void {
    std::shared_ptr<BeTexture> shadowMap;

    if (light.CastsShadows) {
        auto found = _pointShadowMaps.find(light.Name);
        if (found == _pointShadowMaps.end()) {
            found = _pointShadowMaps.emplace(
                light.Name,
                BeTexture::Create("Quickstart_PointShadowMap_" + light.Name)
                .SetUsage(SenTextureUsage::DepthStencil | SenTextureUsage::ShaderResource)
                .SetFormat(SenFormat::Depth32)
                .SetCubemap(true)
                .SetSize(light.ShadowMapResolution, light.ShadowMapResolution)
                .Build()
            ).first;
        }
        shadowMap = found->second;
    }

    SRM->AddPointLight({
        .Name = light.Name,
        .Position = light.Position,
        .Radius = light.Radius,
        .Color = light.Color,
        .Power = light.Power,
        .CastsShadows = light.CastsShadows,
        .ShadowMapResolution = light.ShadowMapResolution,
        .ShadowNearPlane = light.ShadowNearPlane,
        .ShadowMap = shadowMap,
    });
}

auto BeQuickstart::Run() -> int {
    Window = std::make_shared<BeWindow>(static_cast<int>(Width), static_cast<int>(Height), Title, WindowMode);
    Renderer = std::make_shared<BeRenderer>(
        Window->GetReportedPixelWidth(),
        Window->GetReportedPixelHeight(),
        static_cast<void*>(Window->GetGlfwWindow())
    );
    Renderer->LaunchDevice();

    const uint32_t pixelWidth = Renderer->GetSwapchainPixelWidth();
    const uint32_t pixelHeight = Renderer->GetSwapchainPixelHeight();

    Input = std::make_shared<BeInput>(Window->GetGlfwWindow());

    Camera = std::make_shared<BeCamera>();
    Camera->Width = pixelWidth;
    Camera->Height = pixelHeight;
    Camera->NearPlane = 0.1f;
    Camera->FarPlane = 500.0f;
    Camera->Position = { 0.0f, 2.0f, -6.0f };
    Camera->Update();
    _cameraController = std::make_unique<BeFreeCameraController>(Camera.get());

    _assetRegistry = std::make_unique<BeAssetRegistry>();
    SRM = std::make_shared<BeStandardRenderMachine>(Renderer, *_assetRegistry, pixelWidth, pixelHeight);

    if (OnStart)
        OnStart();

    SRM->BakeMeshes();
    _meshesBaked = true;

    SRM->DeclareGBufferTarget("Quickstart_Albedo", SenFormat::R11G11B10_Float);
    SRM->DeclareGBufferTarget("Quickstart_WorldNormal", SenFormat::RGBA16_Float);
    SRM->DeclareGBufferTarget("Quickstart_ORM", SenFormat::RGBA8_Unorm);
    SRM->DeclareGBufferTarget("Quickstart_Emissive", SenFormat::R11G11B10_Float);
    SRM->DeclareDepth("Quickstart_Depth", SenFormat::Depth32);
    SRM->DeclareTexture("Quickstart_HDR", SenFormat::R11G11B10_Float);
    SRM->DeclareTexture("Quickstart_Bloom", SenFormat::R11G11B10_Float);
    SRM->DeclareTexture("Quickstart_Tonemapped", SenFormat::R11G11B10_Float);

    SRM->UniformMaterial->SetFloat3("AmbientColor", AmbientColor);

    if (ConfigurePipeline) {
        ConfigurePipeline(*SRM);
    } else {
        const bool hasSky = !SkyHdrPath.empty();
        if (hasSky) {
            auto skyTexture = BeTexture::Create("Quickstart_Sky")
                .LoadFromFileHdr(SkyHdrPath)
                .Build();
            SRM->AddEnvironmentBakePass(std::move(skyTexture), SkyCubemapSize);
            SRM->BakeEnvironment();
        }

        SRM->AddShadowPass();
        SRM->AddGeometryPass();
        SRM->AddLightingPass("Quickstart_HDR");

        if (hasSky && DrawSkybox)
            SRM->AddSkyboxPass("Quickstart_HDR");

        const char* tonemapperInput = "Quickstart_HDR";
        if (EnableBloom) {
            SRM->AddBloomPass(5, "Quickstart_HDR", "Quickstart_Bloom");
            tonemapperInput = "Quickstart_Bloom";
        }

        SRM->AddTonemapperPass(tonemapperInput, "Quickstart_Tonemapped");
        SRM->AddBackbufferPass("Quickstart_Tonemapped", ClearColor);
    }

    SRM->BuildPasses();
    SRM->Activate();

    double lastTime = glfwGetTime();

    while (!Window->ShouldClose()) {
        Window->PollEvents();
        Input->Update();

        const double now = glfwGetTime();
        const auto deltaTime = static_cast<float>(now - lastTime);
        lastTime = now;
        Time += deltaTime;

        if (Input->GetKeyDown(GLFW_KEY_ESCAPE))
            break;

        if (UseFreeCamera)
            _cameraController->Update(deltaTime, Input.get());
        else
            Camera->Update();

        const auto projView = Camera->GetProjectionMatrix() * Camera->GetViewMatrix();
        auto& uniformMaterial = *SRM->UniformMaterial;
        uniformMaterial.SetMatrix("CameraProjectionView", projView);
        uniformMaterial.SetMatrix("CameraInverseProjectionView", glm::inverse(projView));
        uniformMaterial.SetFloat4("NearFarPlane", {
            Camera->NearPlane,
            Camera->FarPlane,
            1.0f / Camera->NearPlane,
            1.0f / Camera->FarPlane
        });
        uniformMaterial.SetFloat3("CameraPosition", Camera->Position);

        SRM->ClearFrame();
        _sunLightCursor = 0;

        if (OnTick)
            OnTick(deltaTime);

        Renderer->Render();
    }

    BeShaderLibrary::Shutdown();
    return 0;
}
