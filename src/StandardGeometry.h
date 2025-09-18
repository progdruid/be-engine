#pragma once

#include "glm.hpp"

namespace StandardGeometry {

    struct Triangle {
        constexpr static glm::vec3 Vertices[3] = {
            {0.0f,  0.5f, 0.0f},  // Top
            {0.5f, -0.5f, 0.0f},  // Right
            {-0.5f, -0.5f, 0.0f}  // Left
        };
        constexpr static glm::vec3 Normals[3] = {
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f}
        };
        constexpr static glm::vec2 UVs[3] = {
            {0.5f, 1.0f},
            {1.0f, 0.0f},
            {0.0f, 0.0f}
        };
        constexpr static uint32_t Indices[3] = {0, 1, 2};
    };

    struct Square {
        constexpr static glm::vec3 Vertices[4] = {
            {-0.5f,  0.5f, 0.0f},  // Top-left
            { 0.5f,  0.5f, 0.0f},  // Top-right
            { 0.5f, -0.5f, 0.0f},  // Bottom-right
            {-0.5f, -0.5f, 0.0f}   // Bottom-left
        };
        constexpr static glm::vec3 Normals[4] = {
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f}
        };
        constexpr static glm::vec2 UVs[4] = {
            {0.0f, 1.0f},
            {1.0f, 1.0f},
            {1.0f, 0.0f},
            {0.0f, 0.0f}
        };
        constexpr static uint32_t Indices[6] = {0, 1, 2, 0, 2, 3};
    };

}
