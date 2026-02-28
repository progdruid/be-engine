#pragma once
#include <cstdint>
#include <memory>
#include <type_traits>

template<typename E>
struct EnableBitmaskOperators : std::false_type {};

#define ENABLE_BITMASK(E) \
template<> struct EnableBitmaskOperators<E> : std::true_type {}

template<class T>
constexpr auto operator|(T a, T b) -> T requires EnableBitmaskOperators<T>::value {
    using U = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<U>(a) | static_cast<U>(b));
}

template<class T>
constexpr auto operator&(T a, T b) -> T requires EnableBitmaskOperators<T>::value {
    using U = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<U>(a) & static_cast<U>(b));
}

template<class T>
constexpr auto operator^(T a, T b) -> T requires EnableBitmaskOperators<T>::value {
    using U = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<U>(a) ^ static_cast<U>(b));
}

template<class T>
constexpr auto operator~(T a) -> T requires EnableBitmaskOperators<T>::value {
    using U = std::underlying_type_t<T>;
    return static_cast<T>(~static_cast<U>(a));
}

template<class T>
constexpr auto operator|=(T& a, T b) -> T& requires EnableBitmaskOperators<T>::value {
    return a = (a | b);
}

template<class T>
constexpr auto operator&=(T& a, T b) -> T& requires EnableBitmaskOperators<T>::value {
    return a = (a & b);
}

template<class T>
constexpr auto operator^=(T& a, T b) -> T& requires EnableBitmaskOperators<T>::value {
    return a = (a ^ b);
}

template<class T>
constexpr auto HasAny(T value, T mask) -> bool requires EnableBitmaskOperators<T>::value {
    using U = std::underlying_type_t<T>;
    return (static_cast<U>(value) & static_cast<U>(mask)) != 0;
}

template<class T>
constexpr auto HasAll(T value, T mask) -> bool requires EnableBitmaskOperators<T>::value {
    using U = std::underlying_type_t<T>;
    return (static_cast<U>(value) & static_cast<U>(mask)) == static_cast<U>(mask);
}


enum class BeTextureFormat : uint16_t {
    Unknown = 0,
    R8G8B8A8_UNorm,
    R11G11B10_Float,
    R16G16B16A16_Float,
    R32_Typeless,
    R24G8_Typeless,
    R16_Typeless,
    R32_Float,
    R16_UNorm,
    R8_UNorm,
};

enum class BeBindFlags : uint32_t {
    None            = 0,
    ShaderResource  = 1 << 0,
    RenderTarget    = 1 << 1,
    DepthStencil    = 1 << 2,
    VertexBuffer    = 1 << 3,
    IndexBuffer     = 1 << 4,
    ConstantBuffer  = 1 << 5,
};
ENABLE_BITMASK(BeBindFlags);

enum class BeTopology : uint8_t {
    Undefined = 0,
    TriangleList,
    TriangleStrip,
    PatchList3,
};

enum class BeCullMode : uint8_t {
    Back = 0,
    Front,
    None,
};

struct BeViewport {
    float TopLeftX = 0.f;
    float TopLeftY = 0.f;
    float Width = 0.f;
    float Height = 0.f;
    float MinDepth = 0.f;
    float MaxDepth = 1.f;
};

enum class BeShaderType : uint8_t {
    None = 0,
    Vertex = 1 << 0,
    Pixel = 1 << 1,
    Tesselation = 1 << 2,
    All = Vertex | Pixel | Tesselation
};
ENABLE_BITMASK(BeShaderType);

struct BeSamplerImpl;
using BeSampler = std::shared_ptr<BeSamplerImpl>;

struct BeBlendStateImpl;
using BeBlendState = std::shared_ptr<BeBlendStateImpl>;
