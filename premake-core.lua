-- premake-core.lua

local function cleanGenerated_core ()
    os.rmdir ("core/bin")
    os.rmdir ("core/obj")
    os.remove("core/**.vcxproj")
    os.remove("core/**.vcxproj.filters")
    os.remove("core/**.vcxproj.user")
    os.remove("core/**.vcxitems")
    os.remove("core/**.vcxitems.filters")
end


project "core"
    kind "StaticLib"
    language "C++"
    cppdialect "C++23"
    location "core"

    targetdir ("%{prj.location}/bin/%{cfg.architecture}/%{cfg.buildcfg}")
    objdir    ("%{prj.location}/obj/%{cfg.architecture}/%{cfg.buildcfg}")

    files {
        "%{prj.location}/src/**.h",
        "%{prj.location}/src/**.hpp",
        "%{prj.location}/src/**.hlsl",
        "%{prj.location}/src/**.hlsli",
        -- Shared .cpp files (not in platform/)
        "%{prj.location}/src/*.cpp",
    }

    -- Platform-specific source files
    filter "system:windows"
        files {
            "%{prj.location}/src/platform/dx11/**.cpp",
            "%{prj.location}/src/platform/dx11/**.h",
        }
        removefiles {
            "%{prj.location}/src/platform/metal/**",
            "%{prj.location}/src/rhi/**",
        }
        defines { "BE_DX11" }

    filter "system:macosx"
        files {
            "%{prj.location}/src/platform/metal/**.mm",
            "%{prj.location}/src/platform/metal/**.h",
        }
        removefiles {
            "%{prj.location}/src/platform/dx11/**",
            "%{prj.location}/src/BeWindow.cpp",
            "%{prj.location}/src/rhi/**",
        }
        defines { "BE_METAL" }

    filter {}

    includedirs {
        "%{prj.location}/src",
        "%{prj.location}/src/shaders",
        "vendor/libassert/include",
        "vendor/cpptrace/include",
        "vendor/Assimp/include",
        "vendor"
    }

    filter "system:windows"
        libdirs {
            "vendor/glfw/lib-vc2022",
            "vendor/Assimp/lib/x64"
        }
        links {
            "glfw3",
            "assimp-vc143-mt",
            "d3d11",
            "dxgi",
            "d3dcompiler",
            "libassert",
            "cpptrace",
            "dbghelp"
        }

    filter "system:macosx"
        buildoptions {
            "-fobjc-arc",
            "-isystem ../vendor",
            "-isystem ../vendor/libassert/include",
            "-isystem ../vendor/cpptrace/include",
            "-isystem /opt/homebrew/include",
            "-isystem /usr/local/include",
        }
        libdirs {
            "/opt/homebrew/lib",
            "/usr/local/lib",
        }
        links {
            "glfw3",
            "assimp",
            "Metal.framework",
            "QuartzCore.framework",
            "Cocoa.framework",
            "IOKit.framework",
            "CoreVideo.framework",
        }

    filter {}

    defines {
        "LIBASSERT_STATIC_DEFINE",
        "CPPTRACE_STATIC_DEFINE",
    }

    filter { "files:**.hlsl" }
        buildaction "None"

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
