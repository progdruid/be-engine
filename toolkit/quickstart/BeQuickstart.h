#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <umbrellas/common.hpp>
#include <umbrellas/include-glm.h>

#include "BeCamera.h"
#include "BeInput.h"
#include "BeProp.h"
#include "BeWindow.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

class BeAssetRegistry;
class BeFreeCameraController;
class BeMesh;
class BeRenderer;
class BeShader;
class BeTexture;

struct BeQuickstartSunLight {
    glm::vec3 Direction { -0.5f, -1.0f, 0.3f };
    glm::vec3 Color { 1.0f };
    float Power = 3.0f;
    bool CastsShadows = true;
    uint32_t ShadowMapResolution = 2048;
    float ShadowCameraDistance = 60.0f;
    float ShadowMapWorldSize = 40.0f;
    float ShadowNearPlane = 0.1f;
    float ShadowFarPlane = 200.0f;
};

struct BeQuickstartPointLight {
    std::string Name;
    glm::vec3 Position { 0.0f };
    float Radius = 10.0f;
    glm::vec3 Color { 1.0f };
    float Power = 5.0f;
    bool CastsShadows = false;
    uint32_t ShadowMapResolution = 512;
    float ShadowNearPlane = 0.1f;
};

class BeQuickstart {
    
    expose
    static auto StandardShader() -> raw_ptr<BeShader>;
    static auto PhongShader() -> raw_ptr<BeShader>;
    
    
    expose
    std::string Title = "be quickstart";
    uint32_t Width = 1280;
    uint32_t Height = 720;
    BeWindowMode WindowMode = BeWindowMode::Windowed;
    glm::vec3 ClearColor { 0.02f, 0.02f, 0.03f };
    glm::vec3 AmbientColor { 0.03f };
    std::filesystem::path SkyHdrPath;
    uint32_t SkyCubemapSize = 512;
    bool DrawSkybox = true;
    bool UseFreeCamera = true;
    bool EnableBloom = true;

    std::function<void()> OnStart;
    std::function<void(float)> OnTick;
    std::function<void(BeStandardRenderMachine&)> ConfigurePipeline;

    
    expose
    std::shared_ptr<BeWindow> Window;
    std::shared_ptr<BeRenderer> Renderer;
    std::shared_ptr<BeInput> Input;
    std::shared_ptr<BeCamera> Camera;
    std::shared_ptr<BeStandardRenderMachine> SRM;

    float Time = 0.0f;

    
    hide
    std::unique_ptr<BeAssetRegistry> _assetRegistry;
    std::unique_ptr<BeFreeCameraController> _cameraController;
    std::vector<std::shared_ptr<BeTexture>> _sunShadowMaps;
    std::unordered_map<std::string, std::shared_ptr<BeTexture>> _pointShadowMaps;
    size_t _sunLightCursor = 0;
    bool _meshesBaked = false;
    
    
    expose
    BeQuickstart();
    ~BeQuickstart();

    BeQuickstart(const BeQuickstart&) = delete;
    auto operator=(const BeQuickstart&) -> BeQuickstart& = delete;

    auto Run() -> int;

    
    expose
    auto CreateProp(std::shared_ptr<BeMesh> mesh, raw_ptr<BeShader> shader = nullptr) -> std::shared_ptr<BeProp>;
    auto LoadProp(const std::filesystem::path& modelPath, raw_ptr<BeShader> shader = nullptr) -> std::shared_ptr<BeProp>;
    
    auto RenderProp(
        const std::string& name,
        const std::shared_ptr<BeProp>& prop,
        const glm::mat4& modelMatrix,
        bool castShadows = true
    ) -> void;

    auto RenderProp(
        const std::string& name,
        const std::shared_ptr<BeProp>& prop,
        glm::vec3 position,
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec3 scale = glm::vec3(1.0f),
        bool castShadows = true
    ) -> void;

    auto RenderSunLight(const BeQuickstartSunLight& light) -> void;
    auto RenderPointLight(const BeQuickstartPointLight& light) -> void;
};
