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
    configurations { "Debug", "Release" }            -- list syntax is { ... } with commas [5]
    platforms { "Win64", "Win32" }                   -- simple strings or lists are allowed [5]
    location "."                                     -- generate into current folder (optional) [5]

project "Engine"
    kind "ConsoleApp"                                -- function + simple string, no parentheses needed [5]
    language "C++"                                   -- if using variables, parentheses become mandatory [5]
    cppdialect "C++20"                               -- use C++17/20/etc., not raw /std switches [21]

    targetdir ("%{wks.location}/bin/%{cfg.platform}/%{cfg.buildcfg}")  -- parentheses ok [5]
    objdir    ("%{wks.location}/bin-int/%{cfg.platform}/%{cfg.buildcfg}")

    files { 
        "src/**.cpp", 
        "src/**.c", 
        "src/**.h", 
        "src/**.hpp" 
    }

    includedirs { "src" }                                              -- simple include path [5]

    filter "platforms:Win64"                                           -- activate a filter… [7]
        system "windows"
        architecture "x86_64"

    filter "platforms:Win32"
        system "windows"
        architecture "x86"

    filter "configurations:Debug"
        symbols "On"
        defines { "DEBUG" }
        optimize "Off"

    filter "configurations:Release"
        symbols "Off"
        defines { "NDEBUG" }
        optimize "Full"

    filter { "toolset:msc*", "language:C++" }                          -- multiple filter terms go in a list [7]
        buildoptions { "/Zc:__cplusplus" }                             -- optional for correct __cplusplus [22]

    filter {}                                                          -- clear filters to avoid leakage [7]

project "MiscConfiguration"
    kind "Utility"                                -- Utility project, no build output
    files {
        "premake5.lua",
        ".gitignore",
        "README.md"
    }