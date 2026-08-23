#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <umbrellas/include-glm.h>
#include <umbrellas/common.hpp>
#include <sen-rhi/SenTypes.h>

class BeAssetRegistry;

class BeTexture {

    // types ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose struct BeTextureDescriptor {
        std::string Name;
        bool IsCubemap = false;
        uint32_t ArrayLength = 1;
        SenFormat Format = SenFormat::RGBA8_Unorm;
        SenTextureUsage Usage = SenTextureUsage::ShaderResource;
        uint32_t Mips = 1;
        uint32_t Width = 1;
        uint32_t Height = 1;
        uint8_t* Data = nullptr;
        bool GenerateMips = false;
    };

    expose class Builder {

        hide BeTextureDescriptor _descriptor;
        hide BeAssetRegistry* _registry = nullptr;
        hide bool _autoMips      = false;

        hide explicit Builder (std::string name);
        expose ~Builder ();
        expose Builder (const Builder&) = delete;
        expose auto operator=(const Builder&) -> Builder& = delete;
        expose Builder (Builder&&) = default;
        expose auto operator=(Builder&&) -> Builder& = default;

        expose auto SetUsage(SenTextureUsage usage) -> Builder&&;
        expose auto SetFormat(SenFormat format) -> Builder&&;
        expose auto SetMips(uint32_t mips) -> Builder&&;
        expose auto SetMipsAuto() -> Builder&&;
        expose auto GenerateMips() -> Builder&&;
        expose auto SetSize(uint32_t w, uint32_t h) -> Builder&& ;
        expose auto SetCubemap(bool cubemap) -> Builder&& ;
        expose auto SetArrayLength(uint32_t length) -> Builder&& ;

        expose auto FillWithColor (const glm::vec4& color) -> Builder&&;
        expose auto FillFromMemory (const uint8_t* src) -> Builder&&;
        expose auto LoadFromFile (const std::filesystem::path& file) -> Builder&&;
        expose auto LoadFromFileHdr (const std::filesystem::path& file) -> Builder&&;

        hide static auto FlipVertically (uint32_t w, uint32_t h, uint8_t* data) -> void;

        expose auto AddToRegistry (BeAssetRegistry& registry) -> Builder&&;

        expose auto Build() -> std::shared_ptr<BeTexture>;
        expose auto BuildNoReturn() -> void;

        friend class BeTexture;
    }; 

    // static part /////////////////////////////////////////////////////////////////////////////////////////////////////
    expose static auto Create (std::string name) -> Builder { return Builder (std::move(name)); }
    
    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose std::string Name;
    expose SenTexture Handle;
    expose uint32_t Width;
    expose uint32_t Height;
    expose bool IsCubemap;
    expose uint32_t ArrayLength;
    expose uint32_t Mips;
    expose SenTextureUsage Usage;
    expose SenFormat Format;
    expose uint64_t Generation = 0;

    hide std::vector<SenViewport> _mipViewports;

    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide explicit BeTexture(const BeTextureDescriptor& descriptor);
    expose ~BeTexture();

    // public interface ////////////////////////////////////////////////////////////////////////////////////////////////
    expose auto GetMipViewport (const uint32_t mip) const -> const SenViewport&;
    expose auto GetViewport () const -> const SenViewport& { return GetMipViewport(0); }
    expose auto Resize (uint32_t width, uint32_t height) -> void;

    // private logic ///////////////////////////////////////////////////////////////////////////////////////////////////
    hide auto CreateMipViewports() -> void;
    
    // befriending shared_ptr for constructor/destructor access because ours are private
    friend class std::shared_ptr<BeTexture>;
};

