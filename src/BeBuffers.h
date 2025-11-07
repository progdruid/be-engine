#pragma once
#include "BeModel.h"
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

struct BeUniformData {
    glm::mat4 ProjectionView {1.0f};
    glm::vec2 NearFarPlane {0.1f, 100.0f};
    glm::vec3 CameraPosition {0.0f, 0.0f, 0.0f};
    
    glm::vec3 AmbientColor {0.0f, 0.0f, 0.0f};
    //glm::vec3 DirectionalLightColor {1.0f, 1.0f, 1.0f};
    //glm::vec3 DirectionalLightVector = glm::normalize(glm::vec3(-1.0f, -1.0f, 0.0f));
    //float DirectionalLightPower = 1.0f;
};

struct alignas(16) BeUniformBufferGPU {
    glm::mat4x4 ProjectionView;         // 0-3 regs
    glm::mat4x4 InverseProjectionView;  // 4-7 regs
    glm::vec4 NearFarPlane;             // 8  reg:  x = near, y = far, z = 1/near, w = 1/far
    glm::vec4 CameraPosition;           // 9  reg:  xyz = position, w unused
    glm::vec4 AmbientColor;             // 10 reg:  xyz = color, w unused
    //glm::vec4 DirectionalLightVector;   // 11 reg:  xyz = direction, w unused
    //glm::vec3 DirectionalLightColor;    // 12 reg:  xyz = color
    //float DirectionalLightPower;        //          w = power
    
    explicit BeUniformBufferGPU(const BeUniformData& data) {
        ProjectionView = data.ProjectionView;
        InverseProjectionView = glm::inverse(data.ProjectionView);
        NearFarPlane = glm::vec4(data.NearFarPlane, 1.0f / data.NearFarPlane.x, 1.0f / data.NearFarPlane.y);
        CameraPosition = glm::vec4(data.CameraPosition, 0.0f);
        AmbientColor = glm::vec4(data.AmbientColor, 1.f);
        //DirectionalLightVector = glm::vec4(data.DirectionalLightVector, 0.0f);
        //DirectionalLightColor = data.DirectionalLightColor;
        //DirectionalLightPower = data.DirectionalLightPower;
    }
};


struct alignas(16) BeMaterialBufferGPU {
    glm::mat4x4 Model;

    glm::vec4 DiffuseColor  {1, 1, 1, 1};
    glm::vec3 SpecularColor {1, 1, 1};
    float Shininess = 32.f / 2048.f; // Scale down to [0, 1] range
    glm::vec3 SuperSpecularColor {1, 1, 1};
    float SuperSpecularPower = -1.f;

    explicit BeMaterialBufferGPU(const glm::mat4x4& model, const BeMaterial& material) {
        Model = model;
        DiffuseColor = glm::vec4(material.DiffuseColor, 1.f);
        SpecularColor = material.SpecularColor;
        Shininess = material.Shininess / 2048.f;
        SuperSpecularColor = material.SuperSpecularColor;
        SuperSpecularPower = material.SuperShininess;
    }
};

struct BeDirectionalLight {
    // Light properties (for lighting pass)
    glm::vec3 Direction;
    glm::vec3 Color;
    float Power;

    // Shadow map properties (for shadow pass)
    float ShadowMapResolution;
    float ShadowCameraDistance;
    float ShadowMapWorldSize;
    float ShadowNearPlane;
    float ShadowFarPlane;
    std::string ShadowMapTextureName;

    // Calculated once per frame (both passes use this)
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
    glm::vec3 Position;
    float Radius;
    glm::vec3 Color;
    float Power;

    bool CastsShadows = false;
    float ShadowMapResolution = 1024.0f;
    float ShadowNearPlane = 0.1f; // far plane is radius
    std::string ShadowMapTextureName;
};

struct alignas(16) BePointLightLightingBufferGPU {
    glm::vec3 Position;
    float Radius;
    glm::vec3 Color;
    float Power;

    float HasShadowMap;
    float ShadowMapResolution;
    float ShadowNearPlane;
    float Padding;
    
    explicit BePointLightLightingBufferGPU(const BePointLight& light) {
        Position = light.Position;
        Radius = light.Radius;
        Color = light.Color;
        Power = light.Power;
        HasShadowMap = light.CastsShadows ? 1.0f : 0.0f;
        ShadowMapResolution = light.ShadowMapResolution;
        ShadowNearPlane = light.ShadowNearPlane;
        Padding = 0.0f;
    }
};

struct alignas(16) BeShadowpassBufferGPU {
    glm::mat4x4 LightProjectionView;
    glm::mat4x4 ObjectModel;
};
