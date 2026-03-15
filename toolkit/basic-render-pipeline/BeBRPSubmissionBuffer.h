#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <umbrellas/access-modifiers.hpp>
#include <umbrellas/include-glm.h>

#include "BeMaterial.h"
#include "BeMesh.h"
#include "BeProp.h"
#include <sen-rhi/SenTypes.h>

class BeTexture;

struct BeBRPGeometryEntry {
    std::string Name;
    glm::mat4 ModelMatrix;
    std::shared_ptr<BeProp> Prop;
    bool CastShadows;

    static auto CalculateModelMatrix(
        glm::vec3 pos,
        glm::quat rot,
        glm::vec3 scale
    ) -> glm::mat4;
};

struct BeBRPSunLightEntry {
    // Light properties (for lighting pass)
    glm::vec3 Direction;
    glm::vec3 Color;
    float Power;

    // Shadow map properties (for shadow pass)
    bool CastsShadows;
    glm::mat4 ShadowViewProjection;
    uint32_t ShadowMapResolution;
    std::weak_ptr<BeTexture> ShadowMap;

    static auto CalculateViewProj(
        glm::vec3 direction,
        float shadowCameraDistance,
        float shadowMapWorldSize,
        float shadowNearPlane,
        float shadowFarPlane
    ) -> glm::mat4;
};

struct BeBRPPointLightEntry {
    std::string Name;
    glm::vec3 Position;
    float Radius;
    glm::vec3 Color;
    float Power;

    bool CastsShadows;
    uint32_t ShadowMapResolution;
    float ShadowNearPlane;
    std::weak_ptr<BeTexture> ShadowMap;
};

class BeBRPSubmissionBuffer {

    expose
    std::weak_ptr<BeMaterial> UniformMaterial;

    hide
    std::vector<BeBRPGeometryEntry> _geometryEntries;
    std::vector<BeBRPSunLightEntry> _sunLightEntries;
    std::vector<BeBRPPointLightEntry> _pointLightEntries;

    SenBuffer _sharedVertexBuffer;
    SenBuffer _sharedIndexBuffer;
    std::unordered_map<BeMesh*, std::vector<BeMeshSlice>> _meshSlices;
    std::vector<std::shared_ptr<BeMesh>> _registeredMeshes;

    expose
    explicit BeBRPSubmissionBuffer() = default;
    ~BeBRPSubmissionBuffer() = default;

    expose
    auto ClearEntries () -> void;
    auto SubmitGeometry (const BeBRPGeometryEntry& entry) -> void;
    auto SubmitSunLight(const BeBRPSunLightEntry& entry) -> void;
    auto SubmitPointLight(const BeBRPPointLightEntry& entry) -> void;

    expose
    auto GetGeometryEntries () const -> const std::vector<BeBRPGeometryEntry>&;
    auto GetSunLightEntries () const -> const std::vector<BeBRPSunLightEntry>&;
    auto GetPointLightEntries () const -> const std::vector<BeBRPPointLightEntry>&;

    expose
    auto RegisterMesh (const std::shared_ptr<BeMesh>& mesh) -> void;
    auto BakeMeshes () -> void;
    auto GetMeshSlices(BeMesh* mesh) const -> const std::vector<BeMeshSlice>& { return _meshSlices.at(mesh); }

    auto GetSharedVertexBuffer() const -> SenBuffer { return _sharedVertexBuffer; }
    auto GetSharedIndexBuffer() const -> SenBuffer { return _sharedIndexBuffer; }
};
