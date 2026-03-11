#include "BeGeometryPass.h"

#include <cassert>
#include <scope_guard/scope_guard.hpp>
#include <umbrellas/include-glm.h>
#include <sen-rhi/SenBackend.h>

#include "BeAssetRegistry.h"
#include "BeBRPSubmissionBuffer.h"
#include "BeMaterial.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"

BeGeometryPass::BeGeometryPass() = default;
BeGeometryPass::~BeGeometryPass() = default;

auto BeGeometryPass::Initialise() -> void {
    auto objectScheme = BeAssetRegistry::GetMaterialScheme("object-material-for-geometry-pass");
    _objectMaterial = BeMaterial::Create("object", objectScheme, true, *_renderer);
}

auto BeGeometryPass::Render() -> void
{
    auto& cmd = _renderer->GetCommandBuffer();
    const auto submissionBuffer = SubmissionBuffer.lock();

    cmd.BeginPass({
        .ColorAttachments = {
            { OutputTexture0.lock()->Handle, 0, -1, SenLoadOp::Clear, {0, 0, 0, 0} },
            { OutputTexture1.lock()->Handle, 0, -1, SenLoadOp::Clear, {0, 0, 0, 0} },
            { OutputTexture2.lock()->Handle, 0, -1, SenLoadOp::Clear, {0, 0, 0, 0} },
            { OutputTexture3.lock()->Handle, 0, -1, SenLoadOp::Clear, {0, 0, 0, 0} },
        },
        .DepthAttachment = SenDepthAttachment{ OutputDepthTexture.lock()->Handle },
        .Viewport = { 0, 0, float(_renderer->GetWidth()), float(_renderer->GetHeight()), 0, 1 },
    });
    SCOPE_EXIT { cmd.EndPass(); };

    cmd.SetVertexBuffer(submissionBuffer->GetSharedVertexBuffer(), sizeof(BeFullVertex));
    cmd.SetIndexBuffer(submissionBuffer->GetSharedIndexBuffer());
    SCOPE_EXIT {
        cmd.ClearVertexBuffer();
        cmd.ClearIndexBuffer();
    };

    const auto& entries = SubmissionBuffer.lock()->GetGeometryEntries();
    for (const auto& entry : entries) {
        const auto shader = entry.Prop->Shader;
        be_assert(shader);

        if (!_objectBindings.contains(shader.get())) {
            _objectBindings[shader.get()].Make(_objectMaterial, shader);
        }

        _objectMaterial->SetMatrix("Model", entry.ModelMatrix);
        _objectMaterial->SetMatrix("ProjectionView", _renderer->UniformData.ProjectionView);
        _objectMaterial->SetFloat3("ViewerPosition", _renderer->UniformData.CameraPosition);
        cmd.SetBindGroup(_objectBindings[shader.get()].Resolve(), 1);

        const auto& meshSlices = submissionBuffer->GetMeshSlices(entry.Prop->Mesh.get());
        for (size_t i = 0; i < meshSlices.size(); ++i) {
            const auto& meshSlice = meshSlices[i];
            auto& propSlice = entry.Prop->Slices[i];

            PipelineKey key{ shader.get(), propSlice.TwoSided };
            if (!_shaderPipelines.contains(key)) {
                auto pipelineDesc = shader->GetPipelineDesc();
                pipelineDesc.RasterizerState.CullMode = propSlice.TwoSided ? SenCullMode::None : SenCullMode::Back;
                pipelineDesc.BindGroupLayouts = {
                    _renderer->GetUniformBindGroupLayout(),          // set 0: renderer uniforms
                    _objectBindings[shader.get()].GetLayout(),       // set 1: object material
                    propSlice.Binding.GetLayout(),                   // set 2: surface material
                };
                pipelineDesc.RenderTargetFormats = {
                    OutputTexture0.lock()->Format,
                    OutputTexture1.lock()->Format,
                    OutputTexture2.lock()->Format,
                    OutputTexture3.lock()->Format,
                };
                pipelineDesc.DepthStencilFormat = OutputDepthTexture.lock()->Format;
                _shaderPipelines[key] = SenBackend::CreatePipeline(pipelineDesc);
            }
            cmd.SetPipeline(_shaderPipelines[key]);

            cmd.SetBindGroup(propSlice.Binding.Resolve(), 2);
            cmd.DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);
        }
    }
}
