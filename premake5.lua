-- premake5.lua


-- cleaning previously generated files and folders
local function cleanGenerated()
    os.rmdir("bin")
    os.rmdir("bin-int")
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
workspace "DXSandbox"
    configurations { "Debug", "Release" }
    system "windows"
    architecture "x86_64"  
    location "."                         

project "Engine"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"

    targetdir ("%{wks.location}/bin/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/bin-int/%{cfg.platform}/%{cfg.buildcfg}")

    files { 
        "src/**.cpp", 
        "src/**.c", 
        "src/**.h", 
        "src/**.hpp" 
    }

    includedirs { "src", "vendor/glfw/include" }
    libdirs { "vendor/glfw/lib-vc2022" }
    links { "glfw3", "d3d11", "dxgi", "d3dcompiler" }

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