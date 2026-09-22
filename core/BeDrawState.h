#pragma once
#include <cstdint>
#include <common.hpp>

#include "BeBackend.h"
#include "SenTypes.h"

struct BeShader;

class BeDrawState {

    // types ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose class Builder {

        hide const BeShader* _shader;
        hide BeBackend::StaticKey _key;
        hide BeBackend::DynamicState _dynamic;

        hide explicit Builder (const BeShader& shader);

        expose auto SetTopology (SenTopology topology) -> Builder&&;
        expose auto SetCull (SenCull mode) -> Builder&&;
        expose auto SetFrontFace (SenFrontFace frontFace) -> Builder&&;
        expose auto SetFill (SenFill mode) -> Builder&&;
        expose auto SetDepthClip (bool enable) -> Builder&&;
        expose auto SetBlend (const SenBlendState& blend) -> Builder&&;
        expose auto SetDepth (const SenDepthState& depthStencil) -> Builder&&;
        expose auto SetDepthBias (float constant, float slopeScaled) -> Builder&&;

        expose auto Build() -> BeDrawState;

        friend class BeDrawState;
    };

    // static part /////////////////////////////////////////////////////////////////////////////////////////////////////
    expose static auto Create (const BeShader& shader) -> Builder { return Builder(shader); }

    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide const BeShader* _shader = nullptr;
    hide uint32_t _staticKeyId = UINT32_MAX;
    hide BeBackend::DynamicState _dynamic;

    // public interface ////////////////////////////////////////////////////////////////////////////////////////////////
    expose BeDrawState() = default;

    expose auto IsValid() const -> bool { return _shader != nullptr; }
    expose auto GetShader() const -> const BeShader& { return *_shader; }
    expose auto GetStaticKeyId() const -> uint32_t { return _staticKeyId; }
    expose auto GetDynamic() const -> const BeBackend::DynamicState& { return _dynamic; }
};
