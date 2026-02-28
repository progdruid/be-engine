-- premake/vendor.lua

local function generateVersionHeaders()
    -- cpptrace v1.0.4
    local cpptrace_dir = "vendor/cpptrace/include/cpptrace"
    os.mkdir(cpptrace_dir)
    local f = io.open(cpptrace_dir .. "/version.hpp", "w")
    f:write('#ifndef CPPTRACE_VERSION_HPP\n')
    f:write('#define CPPTRACE_VERSION_HPP\n')
    f:write('\n')
    f:write('#define CPPTRACE_VERSION_MAJOR 1\n')
    f:write('#define CPPTRACE_VERSION_MINOR 0\n')
    f:write('#define CPPTRACE_VERSION_PATCH 4\n')
    f:write('#define CPPTRACE_VERSION_STRING "1.0.4"\n')
    f:write('\n')
    f:write('#endif\n')
    f:close()

    -- libassert v2.2.1
    local libassert_dir = "vendor/libassert/include/libassert"
    os.mkdir(libassert_dir)
    f = io.open(libassert_dir .. "/version.hpp", "w")
    f:write('#ifndef LIBASSERT_VERSION_HPP\n')
    f:write('#define LIBASSERT_VERSION_HPP\n')
    f:write('\n')
    f:write('#define LIBASSERT_VERSION_MAJOR 2\n')
    f:write('#define LIBASSERT_VERSION_MINOR 2\n')
    f:write('#define LIBASSERT_VERSION_PATCH 1\n')
    f:write('#define LIBASSERT_VERSION_STRING "2.2.1"\n')
    f:write('\n')
    f:write('#endif\n')
    f:close()
end

generateVersionHeaders()


group "vendor"

-- cpptrace (v1.0.4)
project "cpptrace"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    location "vendor/cpptrace"

    targetdir ("%{prj.location}/bin/%{cfg.architecture}/%{cfg.buildcfg}")
    objdir    ("%{prj.location}/obj/%{cfg.architecture}/%{cfg.buildcfg}")

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
        "NOMINMAX",
    }

    filter "system:windows"
        defines {
            "CPPTRACE_GET_SYMBOLS_WITH_DBGHELP",
            "CPPTRACE_UNWIND_WITH_DBGHELP",
            "CPPTRACE_DEMANGLE_WITH_WINAPI",
        }
        links { "dbghelp" }

    filter "system:macosx"
        defines {
            "CPPTRACE_GET_SYMBOLS_WITH_LIBDL",
            "CPPTRACE_UNWIND_WITH_EXECINFO",
            "CPPTRACE_DEMANGLE_WITH_CXXABI",
        }
        buildoptions {
            "-isystem include",
            "-isystem ../libassert/include",
        }

    filter {}

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


-- libassert (v2.2.1)
project "libassert"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    location "vendor/libassert"

    targetdir ("%{prj.location}/bin/%{cfg.architecture}/%{cfg.buildcfg}")
    objdir    ("%{prj.location}/obj/%{cfg.architecture}/%{cfg.buildcfg}")

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
        "vendor/cpptrace/include",
    }

    defines {
        "LIBASSERT_STATIC_DEFINE",
        "CPPTRACE_STATIC_DEFINE",
    }

    links { "cpptrace" }

    filter "system:macosx"
        buildoptions {
            "-isystem include",
            "-isystem ../cpptrace/include",
        }

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


group ""
