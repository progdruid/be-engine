#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <umbrellas/access-modifiers.hpp>
#include <umbrellas/include-glm.h>

#include "BeModel.h"

class BeTexture;
class BeRenderer;
struct BeBRPSubmissionBufferImpl;

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
    glm::vec3 Direction;
    glm::vec3 Color;
    float Power;

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
    std::unique_ptr<BeBRPSubmissionBufferImpl> _impl;

    hide
    std::vector<BeBRPGeometryEntry> _geometryEntries;
    std::vector<BeBRPSunLightEntry> _sunLightEntries;
    std::vector<BeBRPPointLightEntry> _pointLightEntries;

    std::unordered_map<BeModel*, std::vector<BeDrawSlice>> _modelDrawSlices;
    std::vector<std::shared_ptr<BeModel>> _registeredModels;

    expose
    explicit BeBRPSubmissionBuffer();
    ~BeBRPSubmissionBuffer();

    auto Init(BeRenderer& renderer) -> void;

    expose
    auto ClearEntries() -> void;
    auto SubmitGeometry(const BeBRPGeometryEntry& entry) -> void;
    auto SubmitSunLight(const BeBRPSunLightEntry& entry) -> void;
    auto SubmitPointLight(const BeBRPPointLightEntry& entry) -> void;

    expose
    auto GetGeometryEntries() const -> const std::vector<BeBRPGeometryEntry>&;
    auto GetSunLightEntries() const -> const std::vector<BeBRPSunLightEntry>&;
    auto GetPointLightEntries() const -> const std::vector<BeBRPPointLightEntry>&;

    expose
    auto RegisterModel(const std::shared_ptr<BeModel>& model) -> void;
    auto BakeModels() -> void;
    auto GetDrawSlicesForModel(const std::shared_ptr<BeModel>& model) const -> const std::vector<BeDrawSlice>& { return _modelDrawSlices.at(model.get()); }

    expose auto GetPlatformImpl() const -> BeBRPSubmissionBufferImpl* { return _impl.get(); }
};
