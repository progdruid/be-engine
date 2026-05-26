#include "Components.h"
#include "LuaSceneLoader.h"

#include "BeAssetRegistry.h"
#include "BeTexture.h"
#include "sen-rhi/SenTypes.h"

static glm::vec3 ReadVec3(luabridge::LuaRef t) {
    return glm::vec3(t[1].unsafe_cast<float>(), t[2].unsafe_cast<float>(), t[3].unsafe_cast<float>());
}

void RegisterComponentParsers(LuaSceneLoader& loader) {
    loader.AddComponentParser("transform", [](entt::registry& reg, entt::entity e, std::string_view entityName, luabridge::LuaRef tbl) {
        TransformComponent comp;
        if (tbl["position"].isTable()) {
            comp.Position = ReadVec3(tbl["position"]);
        }
        if (tbl["scale"].isTable()) {
            comp.Scale = ReadVec3(tbl["scale"]);
        }
        if (tbl["rotation"].isTable()) {
            auto deg = ReadVec3(tbl["rotation"]);
            comp.Rotation = glm::quat(deg * (glm::pi<float>() / 180.0f));
        }
        reg.emplace<TransformComponent>(e, comp);
    });

    loader.AddComponentParser("render", [](entt::registry& reg, entt::entity e, std::string_view entityName, luabridge::LuaRef tbl) {
        if (!tbl["prop"].isString()) {
            return;
        }
        auto propName = tbl["prop"].unsafe_cast<std::string>();
        if (auto prop = BeAssetRegistry::GetProp(propName).lock()) {
            bool castShadows = tbl["castShadows"].isBool() ? tbl["castShadows"].unsafe_cast<bool>() : true;
            reg.emplace<RenderComponent>(e, RenderComponent{ .Prop = prop, .CastShadows = castShadows });
        }
    });

    loader.AddComponentParser("static", [](entt::registry& reg, entt::entity e, std::string_view entityName, luabridge::LuaRef tbl) {
        reg.emplace<StaticTag>(e);
    });

    loader.AddComponentParser("pointLight", [](entt::registry& reg, entt::entity e, std::string_view entityName, luabridge::LuaRef tbl) {
        PointLightComponent comp;
        if (tbl["radius"].isNumber()) { comp.Radius = tbl["radius"].unsafe_cast<float>(); }
        if (tbl["color"].isTable())   { comp.Color = ReadVec3(tbl["color"]); }
        if (tbl["power"].isNumber())  { comp.Power = tbl["power"].unsafe_cast<float>(); }
        if (tbl["castsShadows"].isBool())        { comp.CastsShadows = tbl["castsShadows"].unsafe_cast<bool>(); }
        if (tbl["shadowMapResolution"].isNumber()) { comp.ShadowMapResolution = tbl["shadowMapResolution"].unsafe_cast<uint32_t>(); }
        if (tbl["shadowNearPlane"].isNumber())    { comp.ShadowNearPlane = tbl["shadowNearPlane"].unsafe_cast<float>(); }

        if (comp.CastsShadows) {
            comp.ShadowMap = BeTexture::Create(std::string(entityName) + "_ShadowMap")
                .SetUsage(SenTextureUsage::DepthStencil | SenTextureUsage::ShaderResource)
                .SetFormat(SenFormat::Depth32)
                .SetCubemap(true)
                .SetSize(comp.ShadowMapResolution, comp.ShadowMapResolution)
                .Build();
        }
        reg.emplace<PointLightComponent>(e, comp);
    });

    loader.AddComponentParser("sunLight", [](entt::registry& reg, entt::entity e, std::string_view entityName, luabridge::LuaRef tbl) {
        SunLightComponent comp;
        if (tbl["direction"].isTable())          { comp.Direction = glm::normalize(ReadVec3(tbl["direction"])); }
        if (tbl["color"].isTable())              { comp.Color = ReadVec3(tbl["color"]); }
        if (tbl["power"].isNumber())             { comp.Power = tbl["power"].unsafe_cast<float>(); }
        if (tbl["castsShadows"].isBool())        { comp.CastsShadows = tbl["castsShadows"].unsafe_cast<bool>(); }
        if (tbl["shadowMapResolution"].isNumber()) { comp.ShadowMapResolution = tbl["shadowMapResolution"].unsafe_cast<uint32_t>(); }
        if (tbl["shadowCameraDistance"].isNumber()) { comp.ShadowCameraDistance = tbl["shadowCameraDistance"].unsafe_cast<float>(); }
        if (tbl["shadowMapWorldSize"].isNumber()) { comp.ShadowMapWorldSize = tbl["shadowMapWorldSize"].unsafe_cast<float>(); }
        if (tbl["shadowNearPlane"].isNumber())   { comp.ShadowNearPlane = tbl["shadowNearPlane"].unsafe_cast<float>(); }
        if (tbl["shadowFarPlane"].isNumber())    { comp.ShadowFarPlane = tbl["shadowFarPlane"].unsafe_cast<float>(); }

        if (comp.CastsShadows) {
            comp.ShadowMap = BeTexture::Create(std::string(entityName) + "_ShadowMap")
                .SetUsage(SenTextureUsage::DepthStencil | SenTextureUsage::ShaderResource)
                .SetFormat(SenFormat::Depth32)
                .SetSize(comp.ShadowMapResolution, comp.ShadowMapResolution)
                .Build();
        }
        reg.emplace<SunLightComponent>(e, comp);
    });
}
