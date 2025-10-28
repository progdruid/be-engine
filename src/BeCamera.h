#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

class BeCamera {
    //fields///////////////////////////////////////////////////////////////////////////////////////
private:
    glm::vec3 _front{0.0f, 0.0f, -1.0f};
    glm::vec3 _right{1.0f, 0.0f, 0.0f};
    glm::vec3 _up{0.0f, 1.0f, 0.0f};

    glm::mat4 _viewMatrix{1.0f};
    glm::mat4 _projectionMatrix{1.0f};

public:
    glm::vec3 Position{0.0f, 2.0f, 6.0f};
    float Yaw{-90.0f};         // degrees, -Z forward
    float Pitch{0.0f};         // degrees
    float Fov{90.0f};          // degrees

    float Width{1920.0f};
    float Height{1080.0f};
    float NearPlane{0.1f};
    float FarPlane{100.0f};

    //initialisation///////////////////////////////////////////////////////////////////////////////////////
    BeCamera() = default;
    ~BeCamera() = default;

    //public interface///////////////////////////////////////////////////////////////////////////////////////
    [[nodiscard]] glm::vec3 getFront() const { return _front; }
    [[nodiscard]] glm::vec3 getRight() const { return _right; }
    [[nodiscard]] glm::vec3 getUp() const { return _up; }

    [[nodiscard]] glm::mat4 getViewMatrix() const { return _viewMatrix; }
    [[nodiscard]] glm::mat4 getProjectionMatrix() const { return _projectionMatrix; }

    void updateMatrices();
};