#pragma once

#include <memory>
#include <umbrellas/include-glm.h>
#include <umbrellas/access-modifiers.hpp>

class BeWindow;
class BeRenderer;

class BeCamera {

    //fields////////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    glm::vec3 Position{0.0f, 2.0f, 6.0f};
    float Yaw{-90.0f};         // degrees, -Z forward
    float Pitch{0.0f};         // degrees
    float Fov{90.0f};          // degrees

    float NearPlane{0.1f};
    float FarPlane{100.0f};

    uint32_t Width = 0;
    uint32_t Height = 0;
    
    hide
    glm::vec3 _front{0.0f, 0.0f, -1.0f};
    glm::vec3 _right{1.0f, 0.0f, 0.0f};
    glm::vec3 _up{0.0f, 1.0f, 0.0f};

    glm::mat4 _viewMatrix{1.0f};
    glm::mat4 _projectionMatrix{1.0f};
    

    
    expose
    explicit BeCamera() = default;
    ~BeCamera() = default;


    
    expose
    [[nodiscard]] glm::vec3 GetFront() const { return _front; }
    [[nodiscard]] glm::vec3 GetRight() const { return _right; }
    [[nodiscard]] glm::vec3 GetUp() const { return _up; }

    [[nodiscard]] glm::mat4 GetViewMatrix() const { return _viewMatrix; }
    [[nodiscard]] glm::mat4 GetProjectionMatrix() const { return _projectionMatrix; }

    void Update();
};
