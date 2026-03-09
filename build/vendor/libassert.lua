-- build/vendor/libassert.lua
function Vendor.DeclareLibassert()
    project "libassert"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    location("%{wks.location}/vendor/libassert")

    targetdir ("%{prj.location}/bin/%{cfg.architecture}-%{cfg.buildcfg}")
    objdir    ("%{prj.location}/obj/%{cfg.architecture}-%{cfg.buildcfg}")

    files {
        "%{prj.location}/src/assert.cpp",
        "%{prj.location}/src/analysis.cpp",
        "%{prj.location}/src/utils.cpp",
        "%{prj.location}/src/stringification.cpp",
        "%{prj.location}/src/platform.cpp",
        "%{prj.location}/src/printing.cpp",
        "%{prj.location}/src/paths.cpp",
        "%{prj.location}/src/tokenizer.cpp",
        "%{prj.location}/include/**.hpp",
    }

    includedirs {
        "%{prj.location}/include",
        "%{wks.location}/vendor/cpptrace/include",
    }

    defines {
        "LIBASSERT_STATIC_DEFINE",
        "CPPTRACE_STATIC_DEFINE",
    }

    links { "cpptrace" }

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
