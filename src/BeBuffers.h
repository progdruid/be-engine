#pragma once
#include "BeModel.h"
#include "umbrellas/include-glm.h"

struct BeUniformData {
    glm::mat4 ProjectionView {1.0f};
    glm::vec2 NearFarPlane {0.1f, 100.0f};
    glm::vec3 CameraPosition {0.0f, 0.0f, 0.0f};
    
    glm::vec3 AmbientColor {0.0f, 0.0f, 0.0f};

    float Time;
};

struct alignas(16) BeUniformBufferGPU {
    glm::mat4x4 ProjectionView;         // 0-3 regs
    glm::mat4x4 InverseProjectionView;  // 4-7 regs
    glm::vec4 NearFarPlane;             // 8  reg:  x = near, y = far, z = 1/near, w = 1/far
    glm::vec4 CameraPosition;           // 9  reg:  xyz = position, w unused
    glm::vec4 AmbientColor;             // 10 reg:  xyz = color, w unused
    float Time;                         // 11 reg:  time since start of frame
    
    explicit BeUniformBufferGPU(const BeUniformData& data) {
        ProjectionView = data.ProjectionView;
        InverseProjectionView = glm::inverse(data.ProjectionView);
        NearFarPlane = glm::vec4(data.NearFarPlane, 1.0f / data.NearFarPlane.x, 1.0f / data.NearFarPlane.y);
        CameraPosition = glm::vec4(data.CameraPosition, 0.0f);
        AmbientColor = glm::vec4(data.AmbientColor, 1.f);
        Time = data.Time;
    }
};


struct alignas(16) BeObjectBufferGPU {
    glm::mat4x4 Model;
    glm::mat4x4 ViewProjection;
    glm::vec3 ViewerPosition;
    
    explicit BeObjectBufferGPU(const glm::mat4x4& model, const glm::mat4x4& viewProjection, const glm::vec3& viewerPosition) {
        Model = model;
        ViewProjection = viewProjection;
        ViewerPosition = viewerPosition;
    }
};

struct BeDirectionalLight {
    // Light properties (for lighting pass)
    glm::vec3 Direction;
    glm::vec3 Color;
    float Power;

    // Shadow map properties (for shadow pass)
    bool CastsShadows = true;
    uint32_t ShadowMapResolution;
    float ShadowCameraDistance;
    float ShadowMapWorldSize;
    float ShadowNearPlane;
    float ShadowFarPlane;
    std::shared_ptr<BeTexture> ShadowMap;

    glm::mat4 ViewProjection;

    inline auto CalculateMatrix() -> void {
        const float halfSize = ShadowMapWorldSize * 0.5f;
        const glm::mat4 lightOrtho = glm::orthoLH_ZO(-halfSize, halfSize, -halfSize, halfSize, ShadowNearPlane, ShadowFarPlane);
        const glm::vec3 lightPos = -Direction * ShadowCameraDistance;
        const glm::mat4 lightView = glm::lookAtLH(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ViewProjection = lightOrtho * lightView;
    }
};

struct alignas(16) BeDirectionalLightLightingBufferGPU {
    glm::vec4 Direction;
    glm::vec3 Color;
    float Power;
    glm::mat4x4 ViewProjection;
    float TexelSize;
    explicit BeDirectionalLightLightingBufferGPU(const BeDirectionalLight& light) {
        Direction = glm::vec4(light.Direction, 0.0f);
        Color = light.Color;
        Power = light.Power;
        ViewProjection = light.ViewProjection;
        TexelSize = 1.0f / light.ShadowMapResolution;
    }
};

struct BePointLight {
    expose
    glm::vec3 Position;
    float Radius;
    glm::vec3 Color;
    float Power;

    bool CastsShadows = false;
    uint32_t ShadowMapResolution = 1024;
    float ShadowNearPlane = 0.1f; // far plane is radius
    std::shared_ptr<BeTexture> ShadowMap;

    mutable bool Dirty = true;
};