#pragma once
#include <array>
#include <cstddef>
#include <string>
#include <vector>
#include <umbrellas/common.hpp>

#include "BeShaderTools.h"
#include "sen-rhi/SenCommandBuffer.h"
#include "sen-rhi/SenTypes.h"

class BeShader;
class BeMaterial;

class BeRoot {
    hide
    const BeShaderTools::RootLayout* _layout;
    std::array<std::byte, SenMaxRootConstantSize> _data {};
    std::vector<bool> _written;

    expose
    explicit BeRoot(const BeShader& shader);

    expose
    auto Use(const std::string& link, BeMaterial& material) -> BeRoot&;
    auto Push(SenCommandBuffer& cmd) -> void;
};
