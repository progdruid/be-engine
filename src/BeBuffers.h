#pragma once
#include <glm.hpp>

struct UniformBufferData {
    glm::mat4 ProjectionView {1.0f};
    glm::vec3 CameraPosition {0.0f, 0.0f, 0.0f};
    
    glm::vec3 AmbientColor {0.0f, 0.0f, 0.0f};
    float AmbientIntensity = 0.5f;
    glm::vec3 DirectionalLightColor {1.0f, 1.0f, 1.0f};
    glm::vec3 DirectionalLightVector = glm::normalize(glm::vec3(-1.0f, -1.0f, 0.0f));
};

struct alignas(16) UniformBufferDataGPU {
    glm::mat4x4 ProjectionView;         // 0-3 regs
    glm::vec4 CameraPosition;           // 4 reg:   xyz = position, w unused
    glm::vec3 AmbientColor;             // 5 reg:   xyz = color, 
    float AmbientIntensity;             //          w = intensity
    glm::vec4 DirectionalLightColor;    // 6 reg:   xyz = color, w unused
    glm::vec4 DirectionalLightVector;   // 7 reg:   xyz = direction, w unused

    explicit UniformBufferDataGPU(const UniformBufferData& data) {
        ProjectionView = data.ProjectionView;
        CameraPosition = glm::vec4(data.CameraPosition, 0.0f);
        //AmbientColor_Intensity = glm::vec4(data.AmbientColor, data.AmbientIntensity);
        AmbientColor = glm::vec4(data.AmbientColor, 1.f);
        AmbientIntensity = data.AmbientIntensity;
        DirectionalLightColor = glm::vec4(data.DirectionalLightColor, 0.0f);
        DirectionalLightVector = glm::vec4(data.DirectionalLightVector, 0.0f);
    }
};


struct alignas(16) ObjectBufferData {
    glm::mat4x4 Model;
};