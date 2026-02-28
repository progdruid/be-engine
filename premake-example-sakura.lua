-- premake-example-sakura.lua


project "example-sakura"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++23"

    location "example-sakura"

    targetdir ("%{prj.location}/bin/%{cfg.architecture}/%{cfg.buildcfg}")
    objdir    ("%{prj.location}/obj/%{cfg.architecture}/%{cfg.buildcfg}")
    debugdir  ("%{prj.location}/bin/%{cfg.architecture}/%{cfg.buildcfg}")

    files {
        "%{prj.location}/**.cpp",
        "%{prj.location}/**.h",
        "%{prj.location}/**.hpp",
        "%{prj.location}/assets/**.hlsl",
        "%{prj.location}/assets/**.hlsli",
    }

    includedirs {
        "core/src",
        "core/src/shaders",
        "toolkit",
        "%{prj.location}",
        "vendor",
        "vendor/Assimp/include",
        "vendor/libassert/include",
        "vendor/cpptrace/include",
    }

    links { "core", "toolkit" }

    filter "system:windows"
        postbuildcommands {
            "{COPY} %{wks.location}/core/src/shaders %{cfg.targetdir}/src/shaders",
            "{COPY} %{prj.location}/assets %{cfg.targetdir}/assets",
            "{COPY} %{wks.location}/vendor/Assimp/bin/x64/assimp-vc143-mt.dll %{cfg.targetdir}"
        }

    filter "system:macosx"
        postbuildcommands {
            "{COPY} %{wks.location}/core/src/shaders %{cfg.targetdir}/src/shaders",
            "{COPY} %{prj.location}/assets %{cfg.targetdir}/assets",
        }
        links {
            "Metal.framework",
            "QuartzCore.framework",
            "Cocoa.framework",
            "IOKit.framework",
            "CoreVideo.framework",
        }

    filter {}

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
