-- premake5.lua


-- cleaning previously generated files and folders
local function cleanGenerated()
    os.rmdir("core/bin")
    os.rmdir("core/obj")
    os.rmdir("toolkit/bin")
    os.rmdir("toolkit/obj")
    os.rmdir("example-game-1/bin")
    os.rmdir("example-game-1/obj")
    os.rmdir("example-quick-start/bin")
    os.rmdir("example-quick-start/obj")
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

newaction {
    trigger = "clean",
    description = "Clean all generated project files",
    onStart = function()
        cleanGenerated()
    end
}



-- workspace and project definitions
workspace "be"
    configurations { "Debug", "Release" }
    system "windows"
    architecture "x86_64"
    location "."
    startproject "example-quick-start"


-- core
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
        "%{prj.location}/src/**.beshade",
        "%{prj.location}/src/**.hlsl",
        "%{prj.location}/src/**.hlsli",
    }

    includedirs {
        "%{prj.location}/src",
        "%{prj.location}/src/shaders",
        "vendor/libassert/%{cfg.buildcfg}/include",
        "vendor/Assimp/include",
        "vendor"
    }
    libdirs {
        "vendor/glfw/lib-vc2022",
        "vendor/libassert/%{cfg.buildcfg}/lib",
        "vendor/Assimp/lib/x64"
    }
    links {
        "glfw3",
        "assimp-vc143-mt",
        
        -- directx 11
        "d3d11",
        "dxgi",
        "d3dcompiler",
        
        -- libassert
        "assert",
        "cpptrace",
        "dbghelp"
    }

    defines { "LIBASSERT_STATIC_DEFINE" }

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


-- toolkit
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
        "vendor/libassert/%{cfg.buildcfg}/include",
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

    -- filter { "files:**.hlsl" }
    --     buildaction "None"

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


-- misc project
project "misc-configuration"
    kind "Utility"
    files {
        "premake5.lua",
        ".gitignore",
        "README.md",
        "CLAUDE.md",
        "*DotSettings*"
    }


-- example project
project "example-game-1"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++23"

    location "example-game-1"

    targetdir ("%{prj.location}/bin/%{cfg.architecture}/%{cfg.buildcfg}")
    objdir    ("%{prj.location}/obj/%{cfg.architecture}/%{cfg.buildcfg}")
    debugdir  ("%{prj.location}/bin/%{cfg.architecture}/%{cfg.buildcfg}")

    files {
        "%{prj.location}/**.cpp",
        "%{prj.location}/**.h",
        "%{prj.location}/**.hpp",
        "%{prj.location}/assets/**.beshade",
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
        "vendor/libassert/%{cfg.buildcfg}/include",
    }

    links { "core", "toolkit" }

    postbuildcommands {
        "{COPY} %{wks.location}/core/src/shaders %{cfg.targetdir}/standardShaders",
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
        buildoptions { "/Zc:__cplusplus /Zc:preprocessor" }

    filter {}
    
    
--project "example-quick-start"
--    kind "ConsoleApp"
--    language "C++"
--    cppdialect "C++23"
--
--    location "example-quick-start"
--
--    targetdir ("%{prj.location}/bin/%{cfg.architecture}/%{cfg.buildcfg}")
--    objdir    ("%{prj.location}/obj/%{cfg.architecture}/%{cfg.buildcfg}")
--    debugdir  ("%{prj.location}/bin/%{cfg.architecture}/%{cfg.buildcfg}")
--
--    files {
--        "%{prj.location}/**.cpp",
--        "%{prj.location}/**.h",
--        "%{prj.location}/**.hpp",
--        "%{prj.location}/assets/**.beshade",
--        "%{prj.location}/assets/**.hlsl",
--        "%{prj.location}/assets/**.hlsli",
--    }
--
--    includedirs {
--        "core/src",
--        "core/src/shaders",
--        "toolkit",
--        "%{prj.location}",
--        "vendor",
--        "vendor/Assimp/include",
--    }
--
--    links { "core", "toolkit" }
--
--    postbuildcommands {
--        "{COPY} %{wks.location}/core/src/shaders %{cfg.targetdir}/standardShaders",
--        "{COPY} %{prj.location}/assets %{cfg.targetdir}/assets",
--        "{COPY} %{wks.location}/vendor/Assimp/bin/x64/assimp-vc143-mt.dll %{cfg.targetdir}"
--    }
--
--    filter { "files:**.hlsl" }
--        buildaction "None"
--
--    filter "configurations:Debug"
--        symbols "On"
--        defines { "DEBUG" }
--        optimize "Off"
--
--    filter "configurations:Release"
--        symbols "Off"
--        defines { "NDEBUG" }
--        optimize "Full"
--
--    filter { "toolset:msc*", "language:C++" }
--        buildoptions { "/Zc:__cplusplus /Zc:preprocessor" }
--
--    filter {}