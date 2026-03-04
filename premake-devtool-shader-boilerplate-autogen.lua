-- premake-devtool-shader-boilerplate-autogen.lua

group "devtools"

project "shader-boilerplate-autogen"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++23"
    location "devtools/shader-boilerplate-autogen"

    targetdir ("%{prj.location}/bin/%{cfg.architecture}/%{cfg.buildcfg}")
    objdir    ("%{prj.location}/obj/%{cfg.architecture}/%{cfg.buildcfg}")

    files {
        "%{prj.location}/**.cpp",
        "%{prj.location}/**.h",
    }

    includedirs {
        "core/src",
        "vendor",
        "vendor/libassert/include",
        "vendor/cpptrace/include",
        "vendor/Assimp/include",
    }

    links { "core" }

    defines {
        "LIBASSERT_STATIC_DEFINE",
        "CPPTRACE_STATIC_DEFINE",
    }

    filter "configurations:Debug"
        symbols "On"
        defines { "DEBUG" }
        optimize "Off"

    filter "configurations:Release"
        symbols "Off"
        defines { "NDEBUG" }
        optimize "Full"
        postbuildcommands {
            "{COPY} %{cfg.targetdir}/%{prj.name}.exe %{wks.location}/devtools/"
        }

    filter { "toolset:msc*", "language:C++" }
        buildoptions { "/Zc:__cplusplus /Zc:preprocessor" }

    filter {}

group ""
