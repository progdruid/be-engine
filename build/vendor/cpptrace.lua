-- build/vendor/cpptrace.lua
function Vendor.DeclareCpptrace()
    project "cpptrace"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    location("%{wks.location}/vendor/cpptrace")

    targetdir ("%{prj.location}/bin/%{cfg.architecture}-%{cfg.buildcfg}")
    objdir    ("%{prj.location}/obj/%{cfg.architecture}-%{cfg.buildcfg}")

    files {
        "%{prj.location}/src/**.cpp",
        "%{prj.location}/src/**.hpp",
        "%{prj.location}/src/**.h",
        "%{prj.location}/include/**.hpp",
        "%{prj.location}/include/**.h",
    }

    includedirs {
        "%{prj.location}/include",
        "%{prj.location}/src",
    }

    defines {
        "CPPTRACE_STATIC_DEFINE",
        "CPPTRACE_GET_SYMBOLS_WITH_DBGHELP",
        "CPPTRACE_UNWIND_WITH_DBGHELP",
        "CPPTRACE_DEMANGLE_WITH_WINAPI",
        "NOMINMAX",
    }

    links { "dbghelp" }

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
