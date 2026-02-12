-- premake-toolkit.lua

project "toolkit"
    kind "StaticLib"
    language "C++"
    cppdialect "C++23"
    location "toolkit"

    targetdir ("%{prj.location}/bin/%{cfg.architecture}/%{cfg.buildcfg}")
    objdir    ("%{prj.location}/obj/%{cfg.architecture}/%{cfg.buildcfg}")

    files {
        "%{prj.location}/**.cpp",
        "%{prj.location}/**.c",
        "%{prj.location}/**.h",
        "%{prj.location}/**.hpp",
    }

    includedirs {
        "%{prj.location}",
        "core/src",
        "vendor",
        "vendor/libassert/include",
        "vendor/cpptrace/include",
        "vendor/Assimp/include",
    }
    libdirs { "vendor/glfw/lib-vc2022", "vendor/Assimp/lib/x64" }
    links {
        "core",
        "glfw3",
        "d3d11",
        "dxgi",
        "d3dcompiler",
        "assimp-vc143-mt"
    }

    filter "configurations:Debug"
        symbols "On"
        defines { "DEBUG" }
        optimize "Off"

    filter "configurations:Release"
        symbols "Off"
        defines { "NDEBUG" }
        optimize "Full"

    filter { "toolset:msc*", "language:C++" }
        buildoptions { "/Zc:__cplusplus /Zc:preprocessor" }

    filter {}