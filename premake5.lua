-- premake5.lua


-- cleaning previously generated files and folders
local function cleanGenerated()
    os.rmdir("bin")
    os.rmdir("obj")
    os.rmdir("example-game-*/bin")
    os.rmdir("example-game-*/obj")
    os.remove("**.sln")
    os.remove("**.vcxproj")
    os.remove("**.vcxproj.filters")
    os.remove("**.vcxproj.user")
    os.remove("**.vcxitems")
    os.remove("**.vcxitems.filters")
end

if _ACTION == "vs2022" or _ACTION == "vs2019" then
    cleanGenerated()
end


-- workspace and project definitions
workspace "Be"
    configurations { "Debug", "Release" }
    system "windows"
    architecture "x86_64"
    location "."
    startproject "example-game-0"

project "Engine"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    targetdir ("%{wks.location}/bin/%{cfg.architecture}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/obj/%{cfg.architecture}/%{cfg.buildcfg}")

    files {
        "src/**.cpp",
        "src/**.c",
        "src/**.h",
        "src/**.hpp",
        "src/**.hlsl",
        "src/**.hlsli",
    }

    excludes {
        "example-game*/**",
    }

    includedirs {
        "src",
        "src/shaders",
        "vendor/Assimp/include",
        "vendor"
    }
    libdirs { "vendor/glfw/lib-vc2022", "vendor/Assimp/lib/x64" }
    links { "glfw3", "d3d11", "dxgi", "d3dcompiler", "assimp-vc143-mt" }

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
        buildoptions { "/Zc:__cplusplus" }

    filter {}

project "example-game-0"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"

    location "example-game-0"

    targetdir ("%{prj.location}/bin/%{cfg.architecture}/%{cfg.buildcfg}")
    objdir    ("%{prj.location}/obj/%{cfg.architecture}/%{cfg.buildcfg}")
    debugdir  ("%{prj.location}/bin/%{cfg.architecture}/%{cfg.buildcfg}")

    files {
        "example-game-0/**.cpp",
        "example-game-0/**.h",
        "example-game-0/**.hpp",
        "example-game-0/assets/**.hlsl",
        "example-game-0/assets/**.hlsli",
    }

    includedirs {
        "src",
        "src/shaders",
        "example-game-0",
        "vendor/Assimp/include",
        "vendor"
    }

    links { "Engine" }

    postbuildcommands {
        "{COPY} %{wks.location}/src/shaders %{cfg.targetdir}/standardShaders",
        "{COPY} %{prj.location}/assets %{cfg.targetdir}/assets",
        "{COPY} %{wks.location}/vendor/Assimp/bin/x64/assimp-vc143-mt.dll %{cfg.targetdir}"
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
        buildoptions { "/Zc:__cplusplus" }

    filter {}

project "MiscConfiguration"
    kind "Utility"
    files {
        "premake5.lua",
        ".gitignore",
        ".guidelines",
        "README.md",
        ".claude/*"
    }