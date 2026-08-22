#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <umbrellas/common.hpp>
#include <umbrellas/include-glm.h>

#include "BeMaterial.h"
#include "BeMesh.h"
#include "BePassSequence.h"
#include "BeProp.h"
#include <sen-rhi/SenTypes.h>

class BeTexture;
class BeRenderer;
class BeShader;
class BeRenderPass;
class BeAssetRegistry;

// =====================================================================================================================

enum class BeSRMLightingModel { PBR, Phong };

// =====================================================================================================================
// Centralized tunables
// =====================================================================================================================

struct BeSRMSettings {
    struct {
        float Bias = 0.001f;
        uint32_t DirectionalResolution = 2048;
        uint32_t PointResolution = 1024;
        uint32_t MaxDirectional = 2;
        uint32_t MaxPoint = 20;
    } Shadow;

    struct {
        float MaxSampleRadiance = 100.0f;
    } IBL;

    struct {
        float ClampRadiance = 0.0f;
    } Skybox;

    struct {
        float Threshold = 1.f;
        float Knee = 0.7f;
        float Intensity = 1.f;
        float Clamp = 16.0f;
        float UpsampleRadius = 1.0f;
    } Bloom;

    struct {
        float Exposure = 1.f;
        float Contrast = 1.f;
    } Tonemapper;
};

// =====================================================================================================================
// Frame submission entries
// =====================================================================================================================

struct BeSRMGeometryEntry {
    std::string Name;
    glm::mat4 ModelMatrix;
    std::shared_ptr<BeProp> Prop;
    bool CastShadows;

    static auto CalculateModelMatrix(glm::vec3 pos, glm::quat rot, glm::vec3 scale) -> glm::mat4;
};

struct BeSRMSunLightEntry {
    glm::vec3 Direction;
    glm::vec3 Color;
    float Power;

    bool CastsShadows;
    glm::mat4 ShadowViewProjection;

    static auto CalculateViewProj(
        glm::vec3 direction,
        float shadowCameraDistance,
        float shadowMapWorldSize,
        float shadowNearPlane,
        float shadowFarPlane
    ) -> glm::mat4;
};

struct BeSRMPointLightEntry {
    std::string Name;
    glm::vec3 Position;
    float Radius;
    glm::vec3 Color;
    float Power;

    bool CastsShadows;
    float ShadowNearPlane;
};

// =====================================================================================================================

class BeStandardRenderMachine {

    hide
    std::weak_ptr<BeRenderer> _renderer;
    BeAssetRegistry& _assetRegistry;
    uint32_t _width;
    uint32_t _height;

    int _debugChannel = -1;

    std::unordered_map<std::string, std::shared_ptr<BeTexture>> _textureRegistry;
    std::vector<std::shared_ptr<BeTexture>> _gbufferTargets;
    std::shared_ptr<BeTexture> _depthTarget;

    BePassSequence _sequence;

    std::shared_ptr<BeTexture> _envCubemap;
    std::shared_ptr<BeTexture> _irradianceCubemap;
    std::shared_ptr<BeTexture> _prefilteredCubemap;
    std::shared_ptr<BeTexture> _brdfLutTexture;
    std::unique_ptr<BeRenderPass> _environmentBakePass;

    std::vector<BeSRMGeometryEntry> _geometryEntries;
    std::vector<BeSRMSunLightEntry> _sunLightEntries;
    std::vector<BeSRMPointLightEntry> _pointLightEntries;

    std::shared_ptr<BeTexture> _directionalShadowArray;
    std::shared_ptr<BeTexture> _pointShadowArray;

    SenBuffer _sharedVertexBuffer;
    SenBuffer _sharedIndexBuffer;
    std::unordered_map<BeMesh*, std::vector<BeMeshSlice>> _meshSlices;
    std::vector<std::shared_ptr<BeMesh>> _registeredMeshes;

    std::shared_ptr<BeMaterial> _tonemapperMaterial;

    expose
    std::shared_ptr<BeMaterial> UniformMaterial;
    BeSRMSettings Settings;

    // lifetime --------------------------------------------------------------------------------------------------------
    expose
    explicit BeStandardRenderMachine(std::weak_ptr<BeRenderer> renderer, BeAssetRegistry& assetRegistry, uint32_t width, uint32_t height);
    ~BeStandardRenderMachine();

    // texture registry ------------------------------------------------------------------------------------------------
    expose
    auto ClearTargets() -> void;
    auto DeclareGBufferTarget(const std::string& name, SenFormat format) -> std::shared_ptr<BeTexture>;
    auto DeclareDepthTarget(const std::string& name, SenFormat format) -> std::shared_ptr<BeTexture>;
    auto DeclareTextureTarget(const std::string& name, SenFormat format, bool storage = false, uint32_t mips = 1) -> std::shared_ptr<BeTexture>;
    auto GetRenderTexture(const std::string& name) const -> std::shared_ptr<BeTexture>;

    // pass builders ---------------------------------------------------------------------------------------------------
    expose
    auto AddShadowPass() -> void;
    auto AddGeometryPass() -> void;
    auto AddLightingPass(const std::string& outputName) -> void;
    auto AddBloomPass(uint32_t mipCount, const std::string& inputName, const std::string& outputName, std::shared_ptr<BeTexture> dirtTexture = nullptr) -> void;
    auto AddFullscreenPass(raw_ptr<BeShader> shader, std::shared_ptr<BeMaterial> material, const std::vector<std::string>& outputNames) -> void;
    auto AddTonemapperPass(const std::string& inputName, const std::string& outputName) -> void;
    auto AddBackbufferPass(const std::string& inputName, glm::vec3 clearColor = {}) -> void;
    auto AddPass(std::unique_ptr<BeRenderPass> pass) -> void;

    auto AddEnvironmentBakePass(std::shared_ptr<BeTexture> equirect, uint32_t cubemapSize = 512) -> void;
    auto BakeEnvironment() -> void;
    auto GetEnvironmentCubemap() const -> std::shared_ptr<BeTexture> { return _envCubemap; }
    auto GetIrradianceCubemap() const -> std::shared_ptr<BeTexture> { return _irradianceCubemap; }
    auto GetPrefilteredCubemap() const -> std::shared_ptr<BeTexture> { return _prefilteredCubemap; }
    auto GetBrdfLutTexture() const -> std::shared_ptr<BeTexture> { return _brdfLutTexture; }

    auto AddSkyboxPass(const std::string& outputName) -> void;

    expose
    auto ClearPasses() -> void;
    auto InitialisePasses() -> void;
    auto Activate() -> void;

    // debug channel (−1 = normal, 0..N = G-buffer targets in declaration order) ---------------------------------------
    expose
    auto SetDebugChannel(int channel) -> void;
    auto GetDebugChannelTexture() const -> std::shared_ptr<BeTexture>;
    auto GetGBufferTargetCount() const -> size_t { return _gbufferTargets.size(); }

    // frame submission ------------------------------------------------------------------------------------------------
    expose
    auto ClearFrame() -> void;
    auto AddGeometry(const BeSRMGeometryEntry& entry) -> void;
    auto AddSunLight(const BeSRMSunLightEntry& entry) -> void;
    auto AddPointLight(const BeSRMPointLightEntry& entry) -> void;

    expose
    auto GetGeometryEntries() const -> const std::vector<BeSRMGeometryEntry>&;
    auto GetSunLightEntries() const -> const std::vector<BeSRMSunLightEntry>&;
    auto GetPointLightEntries() const -> const std::vector<BeSRMPointLightEntry>&;

    // shadow arrays (lazily (re)allocated from Settings.Shadow) -------------------------------------------------------
    expose
    auto EnsureShadowArrays() -> void;
    auto GetDirectionalShadowArray() const -> std::shared_ptr<BeTexture> { return _directionalShadowArray; }
    auto GetPointShadowArray() const -> std::shared_ptr<BeTexture> { return _pointShadowArray; }
    
    // asset loading ---------------------------------------------------------------------------------------------------
    expose auto LoadProp(
        const std::filesystem::path& modelPath, 
        raw_ptr<BeShader> shader,
        BeSRMLightingModel model = BeSRMLightingModel::PBR
    ) -> std::shared_ptr<BeProp>;
    
    // mesh baking -----------------------------------------------------------------------------------------------------
    expose
    auto ClearMeshes() -> void;
    auto RegisterMesh(const std::shared_ptr<BeMesh>& mesh) -> void;
    auto BakeMeshes() -> void;
    auto GetMeshSlices(BeMesh* mesh) const -> const std::vector<BeMeshSlice>& { return _meshSlices.at(mesh); }
    auto GetSharedVertexBuffer() const -> SenBuffer { return _sharedVertexBuffer; }
    auto GetSharedIndexBuffer() const -> SenBuffer { return _sharedIndexBuffer; }

    // internal use by passes ------------------------------------------------------------------------------------------
    expose
    auto GetAssetRegistry() const -> BeAssetRegistry& { return _assetRegistry; }
};
