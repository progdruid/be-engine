#include "BeProp.h"

#include "BeShader.h"
#include "BeShaderLibrary.h"
#include "BeMaterial.h"

auto BeProp::FromMesh(std::shared_ptr<BeMesh> mesh, raw_ptr<BeShader> shader, const std::string& schemeLink) -> std::shared_ptr<BeProp> {
    auto prop = std::make_shared<BeProp>();
    prop->Mesh = std::move(mesh);
    prop->State = BeDrawState::Create(*shader).Build();

    const auto& scheme = BeShaderLibrary::GetShaderScheme(*shader, schemeLink);
    for (size_t i = 0; i < prop->Mesh->Slices.size(); ++i) {
        auto material = BeMaterial::Create(scheme);
        prop->Materials.push_back(material);
        prop->Slices.push_back({
            .Material = material,
            .TwoSided = false,
        });
    }

    return prop;
}
