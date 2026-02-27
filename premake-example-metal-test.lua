-- premake-example-metal-test.lua

project "example-metal-test"
    kind "WindowedApp"
    language "C++"
    cppdialect "C++23"
    location "example-metal-test"

    targetdir ("%{prj.location}/bin/%{cfg.architecture}/%{cfg.buildcfg}")
    objdir    ("%{prj.location}/obj/%{cfg.architecture}/%{cfg.buildcfg}")

    files {
        "%{prj.location}/**.mm",
        "%{prj.location}/**.h",
    }

    includedirs {
        "core/src",
        "vendor",
        "/opt/homebrew/include",
        "/usr/local/include",
    }

    libdirs {
        "/opt/homebrew/lib",
        "/usr/local/lib",
    }

    links {
        "glfw3",
        "Metal.framework",
        "MetalKit.framework",
        "QuartzCore.framework",
        "Foundation.framework",
        "Cocoa.framework",
        "IOKit.framework",
        "CoreVideo.framework",
    }

    buildoptions {
        "-std=c++23",
        "-fobjc-arc",
    }

    filter "configurations:Debug"
        symbols "On"
        defines { "DEBUG" }

    filter "configurations:Release"
        optimize "Full"
        defines { "NDEBUG" }

    filter {}
