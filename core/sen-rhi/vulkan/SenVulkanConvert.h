#pragma once
#include <vulkan/vulkan_core.h>
#include <sen-rhi/SenTypes.h>
#include <umbrellas/bitmask.hpp>
#include <umbrellas/include-libassert.h>

namespace Sen::Vulkan {

    inline auto BytesPerPixel(SenFormat format) -> uint32_t {
        switch (format) {
            case SenFormat::RGBA8_Unorm:     return 4;
            case SenFormat::BGRA8_Unorm:     return 4;
            case SenFormat::RGBA16_Float:    return 8;
            case SenFormat::R11G11B10_Float: return 4;
            case SenFormat::Depth32:         return 4;
            case SenFormat::RGB32_Float:     return 12;
            case SenFormat::RGBA32_Float:    return 16;
            case SenFormat::RG32_Float:      return 8;
            default:                         return 4;
        }
    }

    inline auto ToImageUsageFlags(SenTextureUsage usage, bool hasInitialData) -> VkImageUsageFlags {
        VkImageUsageFlags flags = 0;
        if (HasAny(usage, SenTextureUsage::ShaderResource)) flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        if (HasAny(usage, SenTextureUsage::RenderTarget))   flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (HasAny(usage, SenTextureUsage::DepthStencil))   flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        if (HasAny(usage, SenTextureUsage::Storage))        flags |= VK_IMAGE_USAGE_STORAGE_BIT;
        if (hasInitialData)                                  flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        return flags;
    }

    inline auto ToFilter(SenFilter filter) -> VkFilter {
        switch (filter) {
            case SenFilter::Point:       return VK_FILTER_NEAREST;
            case SenFilter::Linear:      return VK_FILTER_LINEAR;
            case SenFilter::Anisotropic: return VK_FILTER_LINEAR;
        }
        be_assert(false, "Unknown SenFilter");
        return VK_FILTER_LINEAR;
    }

    inline auto ToMipmapMode(SenFilter filter) -> VkSamplerMipmapMode {
        switch (filter) {
            case SenFilter::Point:       return VK_SAMPLER_MIPMAP_MODE_NEAREST;
            case SenFilter::Linear:      return VK_SAMPLER_MIPMAP_MODE_LINEAR;
            case SenFilter::Anisotropic: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        }
        be_assert(false, "Unknown SenFilter");
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }

    inline auto ToAddressMode(SenAddressMode address) -> VkSamplerAddressMode {
        switch (address) {
            case SenAddressMode::Wrap:   return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case SenAddressMode::Clamp:  return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case SenAddressMode::Mirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        }
        be_assert(false, "Unknown SenAddressMode");
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    }

    inline auto ToCompareOp(SenComparisonFunc func) -> VkCompareOp {
        switch (func) {
            case SenComparisonFunc::Never:        return VK_COMPARE_OP_NEVER;
            case SenComparisonFunc::Less:         return VK_COMPARE_OP_LESS;
            case SenComparisonFunc::Equal:        return VK_COMPARE_OP_EQUAL;
            case SenComparisonFunc::LessEqual:    return VK_COMPARE_OP_LESS_OR_EQUAL;
            case SenComparisonFunc::Greater:      return VK_COMPARE_OP_GREATER;
            case SenComparisonFunc::NotEqual:     return VK_COMPARE_OP_NOT_EQUAL;
            case SenComparisonFunc::GreaterEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
            case SenComparisonFunc::Always:       return VK_COMPARE_OP_ALWAYS;
        }
        be_assert(false, "Unknown SenComparisonFunc");
        return VK_COMPARE_OP_LESS;
    }

    inline auto ToFormat(SenFormat format) -> VkFormat {
        switch (format) {
            case SenFormat::RGBA8_Unorm:     return VK_FORMAT_R8G8B8A8_UNORM;
            case SenFormat::BGRA8_Unorm:     return VK_FORMAT_B8G8R8A8_UNORM;
            case SenFormat::RGBA16_Float:    return VK_FORMAT_R16G16B16A16_SFLOAT;
            case SenFormat::R11G11B10_Float: return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
            case SenFormat::Depth32:         return VK_FORMAT_D32_SFLOAT;
            case SenFormat::RGB32_Float:     return VK_FORMAT_R32G32B32_SFLOAT;
            case SenFormat::RGBA32_Float:    return VK_FORMAT_R32G32B32A32_SFLOAT;
            case SenFormat::RG32_Float:      return VK_FORMAT_R32G32_SFLOAT;
            default:                         return VK_FORMAT_UNDEFINED;
        }
    }

    inline auto FromVkFormat(VkFormat format) -> SenFormat {
        switch (format) {
            case VK_FORMAT_R8G8B8A8_UNORM:          return SenFormat::RGBA8_Unorm;
            case VK_FORMAT_B8G8R8A8_UNORM:          return SenFormat::BGRA8_Unorm;
            case VK_FORMAT_R16G16B16A16_SFLOAT:     return SenFormat::RGBA16_Float;
            case VK_FORMAT_B10G11R11_UFLOAT_PACK32: return SenFormat::R11G11B10_Float;
            case VK_FORMAT_D32_SFLOAT:              return SenFormat::Depth32;
            case VK_FORMAT_R32G32B32_SFLOAT:        return SenFormat::RGB32_Float;
            case VK_FORMAT_R32G32B32A32_SFLOAT:     return SenFormat::RGBA32_Float;
            case VK_FORMAT_R32G32_SFLOAT:           return SenFormat::RG32_Float;
            default:                                return SenFormat::Unknown;
        }
    }

    inline auto ToTopology(SenTopology topology) -> VkPrimitiveTopology {
        switch (topology) {
            case SenTopology::TriangleList:  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            case SenTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            case SenTopology::LineList:      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case SenTopology::PointList:     return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            case SenTopology::PatchList3:    return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
            default:                         return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }
    }

    inline auto ToBlendFactor(SenBlendFactor factor) -> VkBlendFactor {
        switch (factor) {
            case SenBlendFactor::Zero:        return VK_BLEND_FACTOR_ZERO;
            case SenBlendFactor::One:         return VK_BLEND_FACTOR_ONE;
            case SenBlendFactor::SrcColor:    return VK_BLEND_FACTOR_SRC_COLOR;
            case SenBlendFactor::InvSrcColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
            case SenBlendFactor::SrcAlpha:    return VK_BLEND_FACTOR_SRC_ALPHA;
            case SenBlendFactor::InvSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            case SenBlendFactor::DstColor:    return VK_BLEND_FACTOR_DST_COLOR;
            case SenBlendFactor::InvDstColor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
            case SenBlendFactor::DstAlpha:    return VK_BLEND_FACTOR_DST_ALPHA;
            case SenBlendFactor::InvDstAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        }
        be_assert(false, "Unknown SenBlendFactor");
        return VK_BLEND_FACTOR_ONE;
    }

    inline auto ToBlendOp(SenBlendOp op) -> VkBlendOp {
        switch (op) {
            case SenBlendOp::Add:             return VK_BLEND_OP_ADD;
            case SenBlendOp::Subtract:        return VK_BLEND_OP_SUBTRACT;
            case SenBlendOp::ReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
            case SenBlendOp::Min:             return VK_BLEND_OP_MIN;
            case SenBlendOp::Max:             return VK_BLEND_OP_MAX;
        }
        be_assert(false, "Unknown SenBlendOp");
        return VK_BLEND_OP_ADD;
    }

    inline auto ToCullMode(SenCullMode mode) -> VkCullModeFlags {
        switch (mode) {
            case SenCullMode::None:  return VK_CULL_MODE_NONE;
            case SenCullMode::Front: return VK_CULL_MODE_FRONT_BIT;
            case SenCullMode::Back:  return VK_CULL_MODE_BACK_BIT;
        }
        be_assert(false, "Unknown SenCullMode");
        return VK_CULL_MODE_BACK_BIT;
    }

    inline auto ToFillMode(SenFillMode mode) -> VkPolygonMode {
        switch (mode) {
            case SenFillMode::Solid:      return VK_POLYGON_MODE_FILL;
            case SenFillMode::Wireframe:  return VK_POLYGON_MODE_LINE;
        }
        be_assert(false, "Unknown SenFillMode");
        return VK_POLYGON_MODE_FILL;
    }

    inline auto ToShaderStageFlags(SenShaderStageFlags stages) -> VkShaderStageFlags {
        VkShaderStageFlags flags = 0;
        if (HasAny(stages, SenShaderStageFlags::Vertex))  flags |= VK_SHADER_STAGE_VERTEX_BIT;
        if (HasAny(stages, SenShaderStageFlags::Pixel))   flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
        if (HasAny(stages, SenShaderStageFlags::Hull))    flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        if (HasAny(stages, SenShaderStageFlags::Domain))  flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        if (HasAny(stages, SenShaderStageFlags::Compute)) flags |= VK_SHADER_STAGE_COMPUTE_BIT;
        return flags;
    }

    inline auto ToImageLayout(SenResourceState state) -> VkImageLayout {
        switch (state) {
            case SenResourceState::Undefined:        return VK_IMAGE_LAYOUT_UNDEFINED;
            case SenResourceState::ShaderRead:       return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            case SenResourceState::ColorAttachment:  return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            case SenResourceState::DepthAttachment:  return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            case SenResourceState::TransferDst:      return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            case SenResourceState::Present:          return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            case SenResourceState::UnorderedAccess:  return VK_IMAGE_LAYOUT_GENERAL;
        }
        be_assert(false, "Unknown SenResourceState");
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }

    // Synchronisation scope (stage + access) for a layout, used to fill sync2 image barriers.
    inline auto ScopeForLayout(VkImageLayout layout, VkPipelineStageFlags2& stage, VkAccessFlags2& access) -> void {
        switch (layout) {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                stage  = VK_PIPELINE_STAGE_2_NONE;
                access = VK_ACCESS_2_NONE;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
                access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                stage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                access = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                stage  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
                access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                stage  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                access = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
                break;
            case VK_IMAGE_LAYOUT_GENERAL:
                stage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
                access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                // Present is synchronised by the swapchain semaphore, not by this barrier.
                stage  = VK_PIPELINE_STAGE_2_NONE;
                access = VK_ACCESS_2_NONE;
                break;
            default:
                be_assert(false, "ScopeForLayout: unsupported layout");
                stage  = VK_PIPELINE_STAGE_2_NONE;
                access = VK_ACCESS_2_NONE;
        }
    }

} // namespace Sen::Vulkan
