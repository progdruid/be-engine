#pragma once
#include <memory>
#include <vector>
#include <umbrellas/access-modifiers.hpp>
#include <umbrellas/include-glm.h>

class BeTexture;
struct BeModel;

struct BeBRPGeometryEntry {
    glm::mat4 ModelMatrix;
    std::shared_ptr<BeModel> Model;
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
    
    hide
    std::vector<BeBRPGeometryEntry> _geometryEntries;
    std::vector<BeBRPSunLightEntry> _sunLightEntries;
    std::vector<BeBRPPointLightEntry> _pointLightEntries;
    
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
};
