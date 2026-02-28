#include "BeBackbufferPass.h"

#include "BeAssetRegistry.h"
#include "BeMaterial.h"
#include "BePipeline.h"
#include "BeRenderer.h"
#include "BeTexture.h"
#include "BeShader.h"

BeBackbufferPass::BeBackbufferPass() = default;
BeBackbufferPass::~BeBackbufferPass() = default;

auto BeBackbufferPass::Initialise() -> void {
    _backbufferShader = BeAssetRegistry::GetShader("backbuffer").lock();
    auto scheme = BeAssetRegistry::GetMaterialScheme("backbuffer-material");
    _backbufferMaterial = BeMaterial::Create("Backbuffer Material", scheme, false, *_renderer);
    _backbufferMaterial->SetTexture("InputTexture", InputTexture.lock());
}

auto BeBackbufferPass::Render() -> void {
    const auto& pipeline = _renderer->GetPipeline();

    pipeline->BindBackbuffer(glm::vec4(ClearColor, 1.0f));

    pipeline->BindShader(_backbufferShader, BeShaderType::Vertex | BeShaderType::Pixel);
    pipeline->BindMaterialAutomatic(_backbufferMaterial);

    pipeline->Draw(4, 0);

    pipeline->Clear();
    pipeline->UnbindBackbuffer();
}
