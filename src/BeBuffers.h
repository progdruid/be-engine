#pragma once
#include <glm.hpp>
#include "BeModel.h"

struct UniformData {
    glm::mat4 ProjectionView {1.0f};
    glm::vec2 NearFarPlane {0.1f, 100.0f};
    glm::vec3 CameraPosition {0.0f, 0.0f, 0.0f};
    
    glm::vec3 AmbientColor {0.0f, 0.0f, 0.0f};
    //glm::vec3 DirectionalLightColor {1.0f, 1.0f, 1.0f};
    //glm::vec3 DirectionalLightVector = glm::normalize(glm::vec3(-1.0f, -1.0f, 0.0f));
    //float DirectionalLightPower = 1.0f;
};

struct alignas(16) UniformBufferGPU {
    glm::mat4x4 ProjectionView;         // 0-3 regs
    glm::mat4x4 InverseProjectionView;  // 4-7 regs
    glm::vec4 NearFarPlane;             // 8  reg:  x = near, y = far, z = 1/near, w = 1/far
    glm::vec4 CameraPosition;           // 9  reg:  xyz = position, w unused
    glm::vec4 AmbientColor;             // 10 reg:  xyz = color, w unused
    //glm::vec4 DirectionalLightVector;   // 11 reg:  xyz = direction, w unused
    //glm::vec3 DirectionalLightColor;    // 12 reg:  xyz = color
    //float DirectionalLightPower;        //          w = power
    
    explicit UniformBufferGPU(const UniformData& data) {
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


struct alignas(16) MaterialBufferGPU {
    glm::mat4x4 Model;

    glm::vec4 DiffuseColor  {1, 1, 1, 1};
    glm::vec3 SpecularColor {1, 1, 1};
    float Shininess = 32.f / 2048.f; // Scale down to [0, 1] range
    glm::vec3 SuperSpecularColor {1, 1, 1};
    float SuperSpecularPower = -1.f;

    explicit MaterialBufferGPU(const glm::mat4x4& model, const BeMaterial& material) {
        Model = model;
        DiffuseColor = glm::vec4(material.DiffuseColor, 1.f);
        SpecularColor = material.SpecularColor;
        Shininess = material.Shininess / 2048.f;
        SuperSpecularColor = material.SuperSpecularColor;
        SuperSpecularPower = material.SuperShininess;
    }
};

struct DirectionalLightData {
    glm::vec3 Direction;
    glm::vec3 Color;
    float Power;
};

struct alignas(16) DirectionalLightBufferGPU {
    glm::vec4 Direction;
    glm::vec3 Color;
    float Power;
    explicit DirectionalLightBufferGPU(const DirectionalLightData& light) {
        Direction = glm::vec4(light.Direction, 0.0f);
        Color = light.Color;
        Power = light.Power;
    }
};

struct PointLightData {
    glm::vec3 Position;
    float Radius;
    glm::vec3 Color;
    float Power;
};

struct alignas(16) PointLightBufferGPU {
    glm::vec3 Position;
    float Radius;
    glm::vec3 Color;
    float Power;
    explicit PointLightBufferGPU(const PointLightData& light) {
        Position = light.Position;
        Radius = light.Radius;
        Color = light.Color;
        Power = light.Power;
    }
};