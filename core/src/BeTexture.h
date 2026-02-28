#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <umbrellas/include-glm.h>
#include <umbrellas/access-modifiers.hpp>

#include "BeTypes.h"

class BeRenderer;
struct BeTextureImpl;

class BeTexture {

    expose struct BeTextureDescriptor {
        std::string Name;
        bool IsCubemap = false;
        BeTextureFormat Format = BeTextureFormat::R8G8B8A8_UNorm;
        BeBindFlags BindFlags = BeBindFlags::ShaderResource;
        uint32_t Mips = 1;
        uint32_t Width = 1;
        uint32_t Height = 1;
        uint8_t* Data = nullptr;
    };

    expose class Builder {

        hide BeTextureDescriptor _descriptor;
        hide bool _addToRegistry = false;

        hide explicit Builder(std::string name);
        expose ~Builder();
        expose Builder(const Builder&) = delete;
        expose auto operator=(const Builder&) -> Builder& = delete;
        expose Builder(Builder&&) = default;
        expose auto operator=(Builder&&) -> Builder& = default;

        expose auto SetBindFlags(BeBindFlags bindFlags) -> Builder&&;
        expose auto SetFormat(BeTextureFormat format) -> Builder&&;
        expose auto SetMips(uint32_t mips) -> Builder&&;
        expose auto SetSize(uint32_t w, uint32_t h) -> Builder&&;
        expose auto SetCubemap(bool cubemap) -> Builder&&;

        expose auto FillWithColor(const glm::vec4& color) -> Builder&&;
        expose auto FillFromMemory(const uint8_t* src) -> Builder&&;
        expose auto LoadFromFile(const std::filesystem::path& file) -> Builder&&;

        hide static auto FlipVertically(uint32_t w, uint32_t h, uint8_t* data) -> void;

        expose auto AddToRegistry() -> Builder&&;

        expose auto Build(BeRenderer& renderer) -> std::shared_ptr<BeTexture>;
        expose auto BuildNoReturn(BeRenderer& renderer) -> void;

        friend class BeTexture;
    };

    expose static auto Create(std::string name) -> Builder { return Builder(std::move(name)); }

    expose std::string Name;
    expose uint32_t UniqueID;
    expose uint32_t Width;
    expose uint32_t Height;
    expose bool IsCubemap;
    expose uint32_t Mips;
    expose BeBindFlags BindFlags;
    expose BeTextureFormat Format;

    hide std::vector<BeViewport> _mipViewports;
    hide std::unique_ptr<BeTextureImpl> _impl;

    hide explicit BeTexture(const BeTextureDescriptor& descriptor);
    expose ~BeTexture();

    expose auto GetMipViewport(uint32_t mip) const -> const BeViewport&;

    expose auto GetPlatformImpl() const -> BeTextureImpl* { return _impl.get(); }

    hide auto CreatePlatformResources(BeRenderer& renderer, const uint8_t* initialData = nullptr) -> void;
    hide auto CreateMipViewports() -> void;

    friend class std::shared_ptr<BeTexture>;
};
