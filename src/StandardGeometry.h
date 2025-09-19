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

    struct Cube {
        // 6 faces * 4 vertices per face
        constexpr static glm::vec3 Vertices[24] = {
            // Front face
            {-0.5f, -0.5f,  0.5f},
            { 0.5f, -0.5f,  0.5f},
            { 0.5f,  0.5f,  0.5f},
            {-0.5f,  0.5f,  0.5f},
            // Back face
            { 0.5f, -0.5f, -0.5f},
            {-0.5f, -0.5f, -0.5f},
            {-0.5f,  0.5f, -0.5f},
            { 0.5f,  0.5f, -0.5f},
            // Left face
            {-0.5f, -0.5f, -0.5f},
            {-0.5f, -0.5f,  0.5f},
            {-0.5f,  0.5f,  0.5f},
            {-0.5f,  0.5f, -0.5f},
            // Right face
            { 0.5f, -0.5f,  0.5f},
            { 0.5f, -0.5f, -0.5f},
            { 0.5f,  0.5f, -0.5f},
            { 0.5f,  0.5f,  0.5f},
            // Top face
            {-0.5f,  0.5f,  0.5f},
            { 0.5f,  0.5f,  0.5f},
            { 0.5f,  0.5f, -0.5f},
            {-0.5f,  0.5f, -0.5f},
            // Bottom face
            {-0.5f, -0.5f, -0.5f},
            { 0.5f, -0.5f, -0.5f},
            { 0.5f, -0.5f,  0.5f},
            {-0.5f, -0.5f,  0.5f}
        };

        constexpr static glm::vec3 Normals[24] = {
            // Front
            {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
            // Back
            {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f},
            // Left
            {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
            // Right
            {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
            // Top
            {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
            // Bottom
            {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}
        };

        constexpr static glm::vec2 UVs[24] = {
            // Each face: (0,0), (1,0), (1,1), (0,1)
            {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},
            {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},
            {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},
            {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},
            {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},
            {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
        };

        constexpr static uint32_t Indices[36] = {
            // 2 triangles per face
            0, 1, 2, 0, 2, 3,         // Front
            4, 5, 6, 4, 6, 7,         // Back
            8, 9,10, 8,10,11,         // Left
           12,13,14,12,14,15,         // Right
           16,17,18,16,18,19,         // Top
           20,21,22,20,22,23          // Bottom
        };
    };

    struct SimpleCube {
        constexpr static glm::vec3 Vertices[8] = {
            {-0.5f, -0.5f, -0.5f}, // 0: left-bottom-back
            { 0.5f, -0.5f, -0.5f}, // 1: right-bottom-back
            { 0.5f,  0.5f, -0.5f}, // 2: right-top-back
            {-0.5f,  0.5f, -0.5f}, // 3: left-top-back
            {-0.5f, -0.5f,  0.5f}, // 4: left-bottom-front
            { 0.5f, -0.5f,  0.5f}, // 5: right-bottom-front
            { 0.5f,  0.5f,  0.5f}, // 6: right-top-front
            {-0.5f,  0.5f,  0.5f}  // 7: left-top-front
        };

        constexpr static glm::vec3 Normals[8] = {
            // These are just placeholder normals; for smooth shading, you may want to average face normals
            {-1.0f, -1.0f, -1.0f},
            { 1.0f, -1.0f, -1.0f},
            { 1.0f,  1.0f, -1.0f},
            {-1.0f,  1.0f, -1.0f},
            {-1.0f, -1.0f,  1.0f},
            { 1.0f, -1.0f,  1.0f},
            { 1.0f,  1.0f,  1.0f},
            {-1.0f,  1.0f,  1.0f}
        };

        constexpr static glm::vec2 UVs[8] = {
            {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},
            {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
        };

        constexpr static uint32_t Indices[36] = {
            // Front face
            4, 5, 6, 4, 6, 7,
            // Back face
            1, 0, 3, 1, 3, 2,
            // Left face
            0, 4, 7, 0, 7, 3,
            // Right face
            5, 1, 2, 5, 2, 6,
            // Top face
            3, 7, 6, 3, 6, 2,
            // Bottom face
            0, 1, 5, 0, 5, 4
        };
    };

    
}
