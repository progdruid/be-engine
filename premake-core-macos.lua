-- premake-core-macos.lua
-- macOS/Metal-specific configuration for core library
-- Include this after premake-core.lua when building for macOS

project "core"

    filter "system:macosx"

        files {
            "%{prj.location}/src/rhi/metal/**.mm",
            "%{prj.location}/src/rhi/metal/**.h",
        }

        removefiles {
            "%{prj.location}/src/rhi/dx11/**",
        }

        links {
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

        defines {
            "BE_METAL",
        }

        -- Remove DX11-specific links
        removelinks {
            "d3d11",
            "dxgi",
            "d3dcompiler",
            "dbghelp",
        }

        -- Remove Windows-specific libs
        removelinks {
            "glfw3",
        }

        -- macOS GLFW is typically installed via homebrew
        libdirs {
            "/opt/homebrew/lib",
            "/usr/local/lib",
        }
        includedirs {
            "/opt/homebrew/include",
            "/usr/local/include",
        }
        links {
            "glfw3",
        }

    filter "system:windows"

        defines {
            "BE_DX11",
        }

        removefiles {
            "%{prj.location}/src/rhi/metal/**",
        }

    filter {}
