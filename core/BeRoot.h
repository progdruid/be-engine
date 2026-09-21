#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <umbrellas/common.hpp>

#include "BeShaderTools.h"
#include "sen-rhi/SenCmd.h"
#include "sen-rhi/SenTypes.h"

struct BeShader;
class BeMaterial;

class BeRoot {
    hide
    const BeShaderTools::RootLayout* _layout;
    std::array<std::byte, SenMaxRootConstantSize> _data {};
    uint32_t _written = 0;

    expose
    explicit BeRoot(const BeShader& shader);

    expose
    auto Use(const std::string& link, BeMaterial& material) -> BeRoot&;

    hide
    auto Push(SenCommandList cmd) const -> void;

    friend class BePass;
};
