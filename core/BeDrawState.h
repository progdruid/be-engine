#pragma once
#include <cstdint>
#include <umbrellas/common.hpp>

#include "BeBackend.h"
#include "sen-rhi/SenTypes.h"

struct BeShader;

class BeDrawState {

    // types ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose class Builder {

        hide const BeShader* _shader;
        hide BeBackend::StaticKey _key;

        hide explicit Builder (const BeShader& shader);

        expose auto SetTopology (SenTopology topology) -> Builder&&;
        expose auto SetCull (SenCullMode mode) -> Builder&&;
        expose auto SetFill (SenFillMode mode) -> Builder&&;
        expose auto SetRasterizer (const SenRasterizerState& rasterizer) -> Builder&&;
        expose auto SetBlend (const SenBlendState& blend) -> Builder&&;
        expose auto SetDepthStencil (const SenDepthStencilState& depthStencil) -> Builder&&;

        expose auto Build() -> BeDrawState;

        friend class BeDrawState;
    };

    // static part /////////////////////////////////////////////////////////////////////////////////////////////////////
    expose static auto Create (const BeShader& shader) -> Builder { return Builder(shader); }

    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide const BeShader* _shader = nullptr;
    hide uint32_t _staticKeyId = UINT32_MAX;

    // public interface ////////////////////////////////////////////////////////////////////////////////////////////////
    expose BeDrawState() = default;

    expose auto IsValid() const -> bool { return _shader != nullptr; }
    expose auto GetShader() const -> const BeShader& { return *_shader; }
    expose auto GetStaticKeyId() const -> uint32_t { return _staticKeyId; }
};
