

-- core ----------------------------------------------------------------------------------------------------------------
function Be.DeclareCore()
    project "core"
    kind "StaticLib"

    includedirs {
        "%{wks.location}/vendor",
        "%{wks.location}/vendor/Assimp/include",
        "%{wks.location}/vendor/libassert/include",
        "%{wks.location}/vendor/cpptrace/include",
    }

    libdirs {
        "%{wks.location}/vendor/glfw/lib-vc2022",
        "%{wks.location}/vendor/Assimp/lib/x64",
        "%{wks.location}/vendor/cpptrace/bin/%{cfg.architecture}-%{cfg.buildcfg}",
        "%{wks.location}/vendor/libassert/bin/%{cfg.architecture}-%{cfg.buildcfg}",
    }
    links {
        "glfw3",
        "assimp-vc143-mt",

        -- directx 11
        "d3d11",
        "dxgi",
        
        -- libassert + cpptrace
        "libassert",
        "cpptrace",
        "dbghelp"
    }
    
    -- slang
    includedirs { "%{wks.location}/vendor/slang/include" }
    libdirs { "%{wks.location}/vendor/slang/lib" }
    links { "slang-compiler" }
    
    -- vulkan
    includedirs { "$(VULKAN_SDK)/Include" }
    libdirs { "$(VULKAN_SDK)/Lib" }
    links { "vulkan-1" }

    
    
    defines {
        "LIBASSERT_STATIC_DEFINE",
        "CPPTRACE_STATIC_DEFINE",
    }

    Be.Default()
end

function Be.CoreUse()
    includedirs {
        "%{wks.location}/core",
        "%{wks.location}/core/shaders",
        "%{wks.location}/vendor",
        "%{wks.location}/vendor/libassert/include",
        "%{wks.location}/vendor/cpptrace/include",
    }
    defines {
        "LIBASSERT_STATIC_DEFINE",
        "CPPTRACE_STATIC_DEFINE",
    }

    -- vulkan
    includedirs { "$(VULKAN_SDK)/Include" }

    links { "core" }
end

function Be.CorePostBuild()
    postbuildcommands {
        "{COPY} %{wks.location}/core/shaders %{cfg.targetdir}/shaders",
        "{COPY} %{wks.location}/vendor/Assimp/bin/x64/assimp-vc143-mt.dll %{cfg.targetdir}",
        "{COPY} %{wks.location}/vendor/slang/bin/slang-compiler.dll %{cfg.targetdir}"
    }
end




-- toolkit -------------------------------------------------------------------------------------------------------------
function Be.DeclareToolkit()
    project "toolkit"
    kind "StaticLib"

    libdirs {
        "%{wks.location}/vendor/glfw/lib-vc2022",
    }
    links {
        "glfw3",
        "d3d11",
        "dxgi",
        "d3dcompiler",
    }

    -- vulkan (needed for imgui_impl_vulkan)
    includedirs { "$(VULKAN_SDK)/Include" }

    Be.CoreUse()
    Be.Default()
end

function Be.ToolkitUse()
    includedirs { "%{wks.location}/toolkit" }
    links { "toolkit" }
end



-- misc-configuration --------------------------------------------------------------------------------------------------
function Be.DeclareMiscConfiguration()
    project "misc-configuration"
    kind "Utility"
    files {
        "%{wks.location}/.gitignore",
        "%{wks.location}/README.md",
        "%{wks.location}/CLAUDE.md",
        "%{wks.location}/LICENSE",
        "%{wks.location}/*DotSettings*",
        "%{wks.location}/premake5.lua",
        "%{wks.location}/premake5*.lua",
        "%{wks.location}/build/**.lua",
    }
end



-- examples ------------------------------------------------------------------------------------------------------------
function Be.DeclareExampleSakura()
    project "example-sakura"
    kind "ConsoleApp"
    debugdir "%{prj.location}/bin/%{cfg.architecture}-%{cfg.buildcfg}"

    postbuildcommands {
        "{COPY} %{prj.location}/assets %{cfg.targetdir}/assets"
    }

    Be.CoreUse()
    Be.CorePostBuild()
    Be.ToolkitUse()
    Be.Default()
end


function Be.DeclareExampleGame1()
    project "example-game-1"
    kind "ConsoleApp"
    debugdir "%{prj.location}/bin/%{cfg.architecture}-%{cfg.buildcfg}"

    postbuildcommands {
        "{COPY} %{prj.location}/assets %{cfg.targetdir}/assets"
    }

    Be.CoreUse()
    Be.CorePostBuild()
    Be.ToolkitUse()
    Be.Default()
end


function Be.DeclareExampleVulkan()
    project "example-vulkan"
    kind "ConsoleApp"
    debugdir "%{prj.location}/bin/%{cfg.architecture}-%{cfg.buildcfg}"
    includedirs { "%{wks.location}/vendor/slang/include" }
    postbuildcommands {
        "{COPY} %{prj.location}/assets %{cfg.targetdir}/assets"
    }
    Be.CoreUse()
    Be.CorePostBuild()
    Be.Default()
end


-- devtools ------------------------------------------------------------------------------------------------------------
function Be.DeclareDevtools()
    group "devtools"

    project "shader-boilerplate-autogen"
    kind "ConsoleApp"

    filter "configurations:Release"
        postbuildcommands {
            "{COPY} %{cfg.targetdir}/%{prj.name}.exe %{wks.location}/devtools/"
        }
    filter {}
    
    Be.CoreUse()
    Be.Default()
    location "%{wks.location}/devtools/shader-boilerplate-autogen"

    group ""
end
