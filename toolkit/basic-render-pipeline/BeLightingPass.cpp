#include "BeLightingPass.h"

#include <scope_guard/scope_guard.hpp>
#include <umbrellas/include-glm.h>

#include "BeAssetRegistry.h"
#include "BeBRPSubmissionBuffer.h"
#include "BeMaterial.h"
#include "BePipeline.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"

BeLightingPass::BeLightingPass() = default;
BeLightingPass::~BeLightingPass() = default;

void BeLightingPass::Initialise() {
    _directionalLightShader = BeAssetRegistry::GetShader("directional-light").lock();
    const auto& directionalScheme = BeAssetRegistry::GetMaterialScheme("directional-light-material");
    _directionalLightMaterial = BeMaterial::Create("DirectionalLightMaterial", directionalScheme, true, *_renderer);
    _directionalLightMaterial->SetTexture("Depth", InputDepthTexture.lock());
    _directionalLightMaterial->SetTexture("Diffuse", InputTexture0.lock());
    _directionalLightMaterial->SetTexture("WorldNormal", InputTexture1.lock());
    _directionalLightMaterial->SetTexture("Specular_Shininess", InputTexture2.lock());

    _pointLightShader = BeAssetRegistry::GetShader("point-light").lock();
    const auto& pointScheme = BeAssetRegistry::GetMaterialScheme("point-light-material");
    _pointLightMaterial = BeMaterial::Create("PointLightMaterial", pointScheme, true, *_renderer);
    _pointLightMaterial->SetTexture("Depth", InputDepthTexture.lock());
    _pointLightMaterial->SetTexture("Diffuse", InputTexture0.lock());
    _pointLightMaterial->SetTexture("WorldNormal", InputTexture1.lock());
    _pointLightMaterial->SetTexture("Specular_Shininess", InputTexture2.lock());

    _emissiveAddShader = BeAssetRegistry::GetShader("emissive-add").lock();
    const auto& emissiveScheme = BeAssetRegistry::GetMaterialScheme("emissive-add-material");
    _emissiveMaterial = BeMaterial::Create("EmissiveMaterial", emissiveScheme, false, *_renderer);
    _emissiveMaterial->SetTexture("InputEmissive", InputTexture3.lock());
}

auto BeLightingPass::Render() -> void {
    const auto& submissionBuffer = *SubmissionBuffer.lock();
    const auto& pipeline = _renderer->GetPipeline();

    pipeline->BindTargets({ OutputTexture }, nullptr, true);
    pipeline->SetAdditiveBlending();
    SCOPE_EXIT {
        pipeline->ClearTargets();
        pipeline->ClearBlendState();
    };

    pipeline->BindShader(_directionalLightShader, BeShaderType::Vertex | BeShaderType::Pixel);

    const auto& sunLight = submissionBuffer.GetSunLightEntries()[0];
    _directionalLightMaterial->SetFloat("HasShadowMap", sunLight.CastsShadows ? 1.0f : 0.0f);
    _directionalLightMaterial->SetFloat3("Direction", sunLight.Direction);
    _directionalLightMaterial->SetFloat3("Color", sunLight.Color);
    _directionalLightMaterial->SetFloat("Power", sunLight.Power);
    _directionalLightMaterial->SetMatrix("ProjectionView", sunLight.ShadowViewProjection);
    _directionalLightMaterial->SetFloat("TexelSize", 1.0f / sunLight.ShadowMapResolution);
    _directionalLightMaterial->SetTexture("ShadowMap", sunLight.ShadowMap.lock());
    pipeline->BindMaterialAutomatic(_directionalLightMaterial);

    pipeline->Draw(4, 0);
    _directionalLightMaterial->SetTexture("ShadowMap", nullptr);
    pipeline->Clear();

    pipeline->BindShader(_pointLightShader, BeShaderType::Vertex | BeShaderType::Pixel);
    for (const auto& pointLight : submissionBuffer.GetPointLightEntries()) {
        _pointLightMaterial->SetFloat3("Position", pointLight.Position);
        _pointLightMaterial->SetFloat("Radius", pointLight.Radius);
        _pointLightMaterial->SetFloat3("Color", pointLight.Color);
        _pointLightMaterial->SetFloat("Power", pointLight.Power);
        _pointLightMaterial->SetFloat("HasShadowMap", pointLight.CastsShadows ? 1.0f : 0.0f);
        _pointLightMaterial->SetFloat("ShadowMapResolution", pointLight.ShadowMapResolution);
        _pointLightMaterial->SetFloat("ShadowNearPlane", pointLight.ShadowNearPlane);
        _pointLightMaterial->SetTexture("PointLightShadowMap", pointLight.ShadowMap.lock());
        pipeline->BindMaterialAutomatic(_pointLightMaterial);

        pipeline->Draw(4, 0);
    }

    _pointLightMaterial->SetTexture("PointLightShadowMap", nullptr);
    pipeline->Clear();

    pipeline->BindShader(_emissiveAddShader, BeShaderType::Vertex | BeShaderType::Pixel);
    pipeline->BindMaterialAutomatic(_emissiveMaterial);
    pipeline->Draw(4, 0);
    pipeline->Clear();
}
