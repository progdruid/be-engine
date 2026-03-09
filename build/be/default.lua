
function Be.Default()
    language "C++"
    cppdialect "C++23"
    location  ("%{wks.location}/%{prj.name}")
    targetdir ("%{prj.location}/bin/%{cfg.architecture}-%{cfg.buildcfg}")
    objdir    ("%{prj.location}/obj/%{cfg.architecture}-%{cfg.buildcfg}")

    files {
        "%{prj.location}/**.c",
        "%{prj.location}/**.cpp",
        "%{prj.location}/**.h",
        "%{prj.location}/**.hpp",
        "%{prj.location}/assets/**.hlsl",
        "%{prj.location}/assets/**.hlsli",
    }

    includedirs {
        "%{prj.location}",
        "%{prj.location}/**/shaders",
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
end