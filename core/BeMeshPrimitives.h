#pragma once
#include <cstdint>
#include <memory>
#include <numbers>

#include "BeMesh.h"

namespace BeMeshPrimitives {

    inline auto Plane(uint32_t subdivisions = 1) -> std::shared_ptr<BeMesh> {
        auto mesh = std::make_shared<BeMesh>();

        const uint32_t vertsPerSide = subdivisions + 1;
        const float cellSize = 1.0f / subdivisions;

        mesh->Vertices.reserve(vertsPerSide * vertsPerSide);
        for (uint32_t y = 0; y < vertsPerSide; ++y) {
            for (uint32_t x = 0; x < vertsPerSide; ++x) {
                BeFullVertex vertex{};

                float posX = x * cellSize - 0.5f;
                float posZ = y * cellSize - 0.5f;

                vertex.Position = {-posX, 0.0f, posZ};
                vertex.Normal = {0.0f, 1.0f, 0.0f};
                vertex.UV0 = {x * cellSize, y * cellSize};

                mesh->Vertices.push_back(vertex);
            }
        }

        mesh->Indices.reserve(subdivisions * subdivisions * 6);
        for (uint32_t y = 0; y < subdivisions; ++y) {
            for (uint32_t x = 0; x < subdivisions; ++x) {
                uint32_t topLeft = y * vertsPerSide + x;
                uint32_t topRight = y * vertsPerSide + (x + 1);
                uint32_t bottomLeft = (y + 1) * vertsPerSide + x;
                uint32_t bottomRight = (y + 1) * vertsPerSide + (x + 1);

                mesh->Indices.push_back(topLeft);
                mesh->Indices.push_back(topRight);
                mesh->Indices.push_back(bottomLeft);

                mesh->Indices.push_back(topRight);
                mesh->Indices.push_back(bottomRight);
                mesh->Indices.push_back(bottomLeft);
            }
        }

        mesh->Slices.push_back({
            .IndexCount = static_cast<uint32_t>(mesh->Indices.size()),
            .StartIndexLocation = 0,
            .BaseVertexLocation = 0,
        });

        return mesh;
    }

    inline auto Cube() -> std::shared_ptr<BeMesh> {
        auto mesh = std::make_shared<BeMesh>();

        // 6 faces, 4 vertices each = 24 vertices
        // Positions negated on X to match codebase convention
        struct FaceData {
            glm::vec3 normal;
            glm::vec3 positions[4];
            glm::vec2 uvs[4];
        };

        // Each face defined with CW winding when viewed from outside
        const FaceData faces[] = {
            // +Y (top)
            { {0, 1, 0}, { {-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f} },
              { {0,0}, {1,0}, {1,1}, {0,1} } },
            // -Y (bottom)
            { {0, -1, 0}, { {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f} },
              { {0,0}, {1,0}, {1,1}, {0,1} } },
            // +Z (front)
            { {0, 0, 1}, { {0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, 0.5f} },
              { {0,0}, {1,0}, {1,1}, {0,1} } },
            // -Z (back)
            { {0, 0, -1}, { {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f} },
              { {0,0}, {1,0}, {1,1}, {0,1} } },
            // +X (right)
            { {1, 0, 0}, { {0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, -0.5f} },
              { {0,0}, {1,0}, {1,1}, {0,1} } },
            // -X (left)
            { {-1, 0, 0}, { {-0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, 0.5f} },
              { {0,0}, {1,0}, {1,1}, {0,1} } },
        };

        mesh->Vertices.reserve(24);
        mesh->Indices.reserve(36);

        for (const auto& face : faces) {
            uint32_t base = static_cast<uint32_t>(mesh->Vertices.size());

            for (int i = 0; i < 4; ++i) {
                BeFullVertex v{};
                v.Position = {-face.positions[i].x, face.positions[i].y, face.positions[i].z};
                v.Normal = {-face.normal.x, face.normal.y, face.normal.z};
                v.UV0 = face.uvs[i];
                mesh->Vertices.push_back(v);
            }

            // CW winding: 0-1-2, 0-2-3
            mesh->Indices.push_back(base + 0);
            mesh->Indices.push_back(base + 1);
            mesh->Indices.push_back(base + 2);

            mesh->Indices.push_back(base + 0);
            mesh->Indices.push_back(base + 2);
            mesh->Indices.push_back(base + 3);
        }

        mesh->Slices.push_back({
            .IndexCount = static_cast<uint32_t>(mesh->Indices.size()),
            .StartIndexLocation = 0,
            .BaseVertexLocation = 0,
        });

        return mesh;
    }

    inline auto Sphere(uint32_t rings = 16, uint32_t segments = 32) -> std::shared_ptr<BeMesh> {
        auto mesh = std::make_shared<BeMesh>();

        constexpr float pi = std::numbers::pi_v<float>;

        mesh->Vertices.reserve((rings + 1) * (segments + 1));

        for (uint32_t ring = 0; ring <= rings; ++ring) {
            float phi = pi * static_cast<float>(ring) / static_cast<float>(rings);
            float y = cos(phi) * 0.5f;
            float radiusAtRing = sin(phi) * 0.5f;

            for (uint32_t seg = 0; seg <= segments; ++seg) {
                float theta = 2.0f * pi * static_cast<float>(seg) / static_cast<float>(segments);

                float x = radiusAtRing * cos(theta);
                float z = radiusAtRing * sin(theta);

                glm::vec3 normal = glm::normalize(glm::vec3(x, y, z));

                BeFullVertex v{};
                v.Position = {-x, y, z};
                v.Normal = {-normal.x, normal.y, normal.z};
                v.UV0 = {
                    static_cast<float>(seg) / static_cast<float>(segments),
                    static_cast<float>(ring) / static_cast<float>(rings)
                };
                mesh->Vertices.push_back(v);
            }
        }

        mesh->Indices.reserve(rings * segments * 6);

        for (uint32_t ring = 0; ring < rings; ++ring) {
            for (uint32_t seg = 0; seg < segments; ++seg) {
                uint32_t current = ring * (segments + 1) + seg;
                uint32_t next = current + (segments + 1);

                // CW winding
                mesh->Indices.push_back(current);
                mesh->Indices.push_back(current + 1);
                mesh->Indices.push_back(next);

                mesh->Indices.push_back(current + 1);
                mesh->Indices.push_back(next + 1);
                mesh->Indices.push_back(next);
            }
        }

        mesh->Slices.push_back({
            .IndexCount = static_cast<uint32_t>(mesh->Indices.size()),
            .StartIndexLocation = 0,
            .BaseVertexLocation = 0,
        });

        return mesh;
    }

}
