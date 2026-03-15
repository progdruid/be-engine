#pragma once
#include <memory>
#include <umbrellas/include-glm.h>
#include <umbrellas/access-modifiers.hpp>

class BeTexture;


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