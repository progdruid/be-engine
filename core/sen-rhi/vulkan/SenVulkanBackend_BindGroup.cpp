#include "SenVulkanBackend.h"

#include <format>
#include <sen-rhi/vulkan/SenVulkanConvert.h>
#include <umbrellas/include-libassert.h>

auto SenVulkanBackend::CreateBindGroup(const SenBindGroupDesc& desc) -> SenBindGroup {
    VkDescriptorSetLayout layout = CreateDescriptorSetLayoutFromDesc(desc);

    VkDescriptorSetAllocateInfo allocInfo {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = _descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &layout,
    };

    SenVulkanBindGroupEntry entry {};
    entry.BindGroupDesc = desc;
    VkResult result = vkAllocateDescriptorSets(_device, &allocInfo, &entry.Set);
    be_assert(result == VK_SUCCESS, "Failed to allocate descriptor set!");

    size_t imageCount = desc.Textures.size() + desc.Samplers.size() + desc.StorageTextures.size();
    std::vector<VkDescriptorImageInfo> imageInfos(imageCount);
    std::vector<VkDescriptorBufferInfo> bufferInfos(desc.Buffers.size() + desc.StorageBuffers.size());
    std::vector<VkWriteDescriptorSet> writes;
    writes.reserve(imageCount + desc.Buffers.size() + desc.StorageBuffers.size());

    for (size_t i = 0; i < desc.Textures.size(); ++i) {
        const auto& texture = desc.Textures[i];
        const auto& textureEntry = LookupTexture(texture);
        const uint8_t binding = desc.TextureSlots[i];

        const uint32_t mip = (i < desc.TextureMips.size()) ? desc.TextureMips[i] : SEN_FULL_MIPS;
        const VkImageView view = (mip != SEN_FULL_MIPS && mip < textureEntry.MipSRVs.size())
            ? textureEntry.MipSRVs[mip]
            : textureEntry.SRV;

        imageInfos[i] = VkDescriptorImageInfo {
            .imageView   = view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        writes.push_back(VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = entry.Set,
            .dstBinding      = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo      = &imageInfos[i],
        });
    }

    size_t samplerImageIdx = desc.Textures.size();
    for (size_t i = 0; i < desc.Samplers.size(); ++i) {
        const auto& sampler = desc.Samplers[i];
        const auto& samplerEntry = LookupSampler(sampler);
        const uint8_t binding = desc.SamplerSlots[i];

        imageInfos[samplerImageIdx] = VkDescriptorImageInfo {
            .sampler = samplerEntry.Sampler,
        };
        writes.push_back(VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = entry.Set,
            .dstBinding      = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER,
            .pImageInfo      = &imageInfos[samplerImageIdx],
        });
        samplerImageIdx++;
    }

    for (size_t i = 0; i < desc.Buffers.size(); ++i) {
        const auto& buffer = desc.Buffers[i];
        const auto& bufferEntry = LookupBuffer(buffer);
        const uint8_t binding = desc.BufferSlots[i];

        bufferInfos[i] = VkDescriptorBufferInfo {
            .buffer = bufferEntry.Buffer,
            .offset = 0,
            .range  = bufferEntry.Size,
        };
        writes.push_back(VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = entry.Set,
            .dstBinding      = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo     = &bufferInfos[i],
        });
    }

    size_t storageImageIdx = desc.Textures.size() + desc.Samplers.size();
    for (size_t i = 0; i < desc.StorageTextures.size(); ++i) {
        const auto& texture = desc.StorageTextures[i];
        const auto& textureEntry = LookupTexture(texture);
        const uint8_t binding = desc.StorageTextureSlots[i];

        imageInfos[storageImageIdx] = VkDescriptorImageInfo {
            .imageView   = textureEntry.SRV,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };
        writes.push_back(VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = entry.Set,
            .dstBinding      = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo      = &imageInfos[storageImageIdx],
        });
        storageImageIdx++;
    }

    size_t storageBufferIdx = desc.Buffers.size();
    for (size_t i = 0; i < desc.StorageBuffers.size(); ++i) {
        const auto& buffer = desc.StorageBuffers[i];
        const auto& bufferEntry = LookupBuffer(buffer);
        const uint8_t binding = desc.StorageBufferSlots[i];

        bufferInfos[storageBufferIdx] = VkDescriptorBufferInfo {
            .buffer = bufferEntry.Buffer,
            .offset = 0,
            .range  = bufferEntry.Size,
        };
        writes.push_back(VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = entry.Set,
            .dstBinding      = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = &bufferInfos[storageBufferIdx],
        });
        storageBufferIdx++;
    }

    if (!writes.empty()) {
        vkUpdateDescriptorSets(_device, uint32_t(writes.size()), writes.data(), 0, nullptr);
    }

    vkDestroyDescriptorSetLayout(_device, layout, nullptr);

    const SenBindGroup handle { _nextBindGroupId++ };
    _bindGroups[handle.ID] = entry;
    return handle;
}

auto SenVulkanBackend::DestroyBindGroup(SenBindGroup handle) -> void {
    auto it = _bindGroups.find(handle.ID);
    if (it != _bindGroups.end()) {
        vkFreeDescriptorSets(_device, _descriptorPool, 1, &it->second.Set);
        _bindGroups.erase(it);
    }
}

auto SenVulkanBackend::LookupBindGroup(SenBindGroup handle) -> SenVulkanBindGroupEntry& {
    return _bindGroups.at(handle.ID);
}

auto SenVulkanBackend::CreateDescriptorSetLayoutFromDesc(const SenBindGroupDesc& desc) -> VkDescriptorSetLayout {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    for (const auto& slot : desc.TextureSlots) {
        bindings.push_back(VkDescriptorSetLayoutBinding {
            .binding         = slot,
            .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = 1,
            .stageFlags      = Sen::Vulkan::ToShaderStageFlags(desc.Stages),
        });
    }

    for (const auto& slot : desc.SamplerSlots) {
        bindings.push_back(VkDescriptorSetLayoutBinding {
            .binding         = slot,
            .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags      = Sen::Vulkan::ToShaderStageFlags(desc.Stages),
        });
    }

    for (const auto& slot : desc.BufferSlots) {
        bindings.push_back(VkDescriptorSetLayoutBinding {
            .binding         = slot,
            .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags      = Sen::Vulkan::ToShaderStageFlags(desc.Stages),
        });
    }

    for (const auto& slot : desc.StorageTextureSlots) {
        bindings.push_back(VkDescriptorSetLayoutBinding {
            .binding         = slot,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags      = Sen::Vulkan::ToShaderStageFlags(desc.Stages),
        });
    }

    for (const auto& slot : desc.StorageBufferSlots) {
        bindings.push_back(VkDescriptorSetLayoutBinding {
            .binding         = slot,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags      = Sen::Vulkan::ToShaderStageFlags(desc.Stages),
        });
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = uint32_t(bindings.size()),
        .pBindings    = bindings.data(),
    };

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VkResult result = vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &layout);
    be_assert(result == VK_SUCCESS, "Failed to create descriptor set layout!");
    return layout;
}

auto SenVulkanBackend::PrintBindGroup(SenBindGroup handle) -> std::string {
    std::string s;
    s += std::format("[BindGroup] handle.ID={} valid={}\n", handle.ID, handle.IsValid());
    if (!handle.IsValid()) { return s; }

    auto& entry = _bindGroups.at(handle.ID);
    s += std::format("\tVkDescriptorSet = {}\n", (void*)entry.Set);

    const auto& desc = entry.BindGroupDesc;

    s += std::format("\tLayout structure:\n");
    s += std::format("\t\tTextureSlots ({}):\n", desc.TextureSlots.size());
    for (const auto& slot : desc.TextureSlots) {
        s += std::format("\t\t\tslot={}\n", slot);
    }
    s += std::format("\t\tSamplerSlots ({}):\n", desc.SamplerSlots.size());
    for (const auto& slot : desc.SamplerSlots) {
        s += std::format("\t\t\tslot={}\n", slot);
    }
    s += std::format("\t\tBufferSlots ({}):\n", desc.BufferSlots.size());
    for (const auto& slot : desc.BufferSlots) {
        s += std::format("\t\t\tslot={}\n", slot);
    }

    s += std::format("\tTextures ({}):\n", desc.Textures.size());
    for (size_t i = 0; i < desc.Textures.size(); ++i) {
        const auto& tex = desc.Textures[i];
        s += std::format("\t\t[{}] SenTexture.ID={} valid={}\n", i, tex.ID, tex.IsValid());
        if (tex.IsValid()) {
            const auto& t = _textures.at(tex.ID);
            s += std::format("\t\t\tVkImage       = {}\n", (void*)t.Image);
            s += std::format("\t\t\tSRV           = {}\n", (void*)t.SRV);
            s += std::format("\t\t\tVkFormat      = {}\n", (uint32_t)t.Format);
            s += std::format("\t\t\tMipLayouts[0] = {}\n", (uint32_t)(t.MipLayouts.empty() ? 0u : t.MipLayouts[0]));
        }
    }

    s += std::format("\tSamplers ({}):\n", desc.Samplers.size());
    for (size_t i = 0; i < desc.Samplers.size(); ++i) {
        const auto& smp = desc.Samplers[i];
        s += std::format("\t\t[{}] SenSampler.ID={} valid={}\n", i, smp.ID, smp.IsValid());
        if (smp.IsValid()) {
            const auto& s2 = _samplers.at(smp.ID);
            s += std::format("\t\t\tVkSampler = {}\n", (void*)s2.Sampler);
        }
    }

    s += std::format("\tBuffers ({}):\n", desc.Buffers.size());
    for (size_t i = 0; i < desc.Buffers.size(); ++i) {
        const auto& buf = desc.Buffers[i];
        s += std::format("\t\t[{}] SenBuffer.ID={} valid={}\n", i, buf.ID, buf.IsValid());
        if (buf.IsValid()) {
            const auto& b = _buffers.at(buf.ID);
            s += std::format("\t\t\tVkBuffer = {}\n", (void*)b.Buffer);
            s += std::format("\t\t\tSize     = {}\n", b.Size);
            s += std::format("\t\t\tAccess   = {}\n", (uint32_t)b.Access);
        }
    }

    s += std::format("\tImageTextures for auto-barrier ({}):\n", desc.Textures.size());
    for (size_t i = 0; i < desc.Textures.size(); ++i) {
        const auto& tex = desc.Textures[i];
        s += std::format("\t\t[{}] SenTexture.ID={} valid={}\n", i, tex.ID, tex.IsValid());
        if (tex.IsValid()) {
            const auto& t = _textures.at(tex.ID);
            s += std::format("\t\t\tMipLayouts[0] = {}\n", (uint32_t)(t.MipLayouts.empty() ? 0u : t.MipLayouts[0]));
        }
    }

    return s;
}
