#include "BeProp.h"

#include "BeShader.h"
#include "BeMaterial.h"

auto BeProp::FromMesh(std::shared_ptr<BeMesh> mesh, std::weak_ptr<BeShader> shader, const std::string& schemeLink) -> std::shared_ptr<BeProp> {
    auto prop = std::make_shared<BeProp>();
    prop->Mesh = std::move(mesh);
    prop->Shader = shader.lock();

    const auto& scheme = prop->Shader->GetMaterialScheme(schemeLink);
    for (size_t i = 0; i < prop->Mesh->Slices.size(); ++i) {
        auto material = BeMaterial::Create(scheme, true);
        prop->Materials.push_back(material);
        prop->Slices.push_back({
            .Material = material,
            .TwoSided = false,
        });
    }

    return prop;
}
