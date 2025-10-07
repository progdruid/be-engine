-- premake5.lua


-- cleaning previously generated files and folders
local function cleanGenerated()
    os.rmdir("bin")
    os.rmdir("obj")
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
    startproject "Engine"

project "Engine"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"

    targetdir ("%{wks.location}/bin/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/obj/%{cfg.platform}/%{cfg.buildcfg}")

    files { 
        "src/**.cpp", 
        "src/**.c", 
        "src/**.h", 
        "src/**.hpp",
        "src/**.hlsl",
        "src/**.hlsli",
        "assets/**.hlsl", 
        "assets/**.hlsli", 
    }
    

    includedirs { "src", "src/shaders", "vendor/glfw/include", "vendor/glm", "vendor/Assimp/include", "vendor/stb_image" }
    libdirs { "vendor/glfw/lib-vc2022", "vendor/Assimp/lib/x64" }
    links { "glfw3", "d3d11", "dxgi", "d3dcompiler", "assimp-vc143-mt" }

    postbuildcommands {
        "{COPY} %{wks.location}/vendor/Assimp/bin/x64/assimp-vc143-mt.dll %{cfg.targetdir}"
    }

    filter { "files:**.hlsl" }
        flags { "ExcludeFromBuild" }

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
        "README.md"
    }