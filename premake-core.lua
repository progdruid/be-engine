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
        "%{prj.location}/src/**.cpp",
        "%{prj.location}/src/**.c",
        "%{prj.location}/src/**.h",
        "%{prj.location}/src/**.hpp",
        "%{prj.location}/src/**.hlsl",
        "%{prj.location}/src/**.hlsli",
    }

    includedirs {
        "%{prj.location}/src",
        "%{prj.location}/src/shaders",
        "vendor/libassert/include",
        "vendor/cpptrace/include",
        "vendor/Assimp/include",
        "vendor/slang/include",
        "vendor"
    }
    libdirs {
        "vendor/glfw/lib-vc2022",
        "vendor/Assimp/lib/x64",
        "vendor/slang/lib"
    }
    links {
        "glfw3",
        "assimp-vc143-mt",

        -- directx 11
        "d3d11",
        "dxgi",

        -- slang
        "slang-compiler",

        -- libassert + cpptrace
        "libassert",
        "cpptrace",
        "dbghelp"
    }

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