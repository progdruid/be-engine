#pragma once
#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include "BeModel.h"

#include "BeRenderPass.h"

class BeGeometryPass final : public BeRenderPass {
public:
    struct ObjectEntry {
        std::string Name;
        glm::vec3 Position = {0.f, 0.f, 0.f};
        glm::quat Rotation = glm::quat(glm::vec3(0, 0, 0));
        glm::vec3 Scale = {1.f, 1.f, 1.f};
        BeModel* Model;
        std::vector<BeModel::BeDrawSlice> DrawSlices;
        BeShader* Shader;
    };

public:
    std::string OutputTexture0Name;
    std::string OutputTexture1Name;
    std::string OutputTexture2Name;
    std::string OutputDepthTextureName;
    
private:
    ComPtr<ID3D11Buffer> _materialBuffer;
    ComPtr<ID3D11Buffer> _sharedVertexBuffer;
    ComPtr<ID3D11Buffer> _sharedIndexBuffer;
    ComPtr<ID3D11DepthStencilState> _depthStencilState;
    
    std::vector<ObjectEntry> _objects;
    
    BeTexture _whiteFallbackTexture {glm::vec4(1.0f)};
    
public:
    explicit BeGeometryPass();
    ~BeGeometryPass() override;
    
    auto Initialise() -> void override;
    auto Render() -> void override;

    auto SetObjects (const std::vector<ObjectEntry>& objects) -> void;
};
