-- build/be.lua
Be = Be or {}

require("build/be/projects")
require("build/be/default")

function Be.CleanProjectFiles()
    local projectDirs = {
        "core",
        "toolkit",
        "example-game-1",
        "example-sakura",
        "example-vulkan",
        "devtools",
    }

    os.remove("be.sln")
    os.remove("misc-configuration.vcxproj")
    os.remove("misc-configuration.vcxproj.filters")
    os.remove("misc-configuration.vcxproj.user")

    for _, dir in ipairs(projectDirs) do
        os.remove(dir .. "/**.vcxproj")
        os.remove(dir .. "/**.vcxproj.filters")
        os.remove(dir .. "/**.vcxproj.user")
    end
end

function Be.CleanBinaries()
    local projectDirs = {
        "core",
        "toolkit",
        "example-game-1",
        "example-sakura",
        "example-vulkan",
        "devtools/shader-boilerplate-autogen",
    }

    for _, dir in ipairs(projectDirs) do
        os.rmdir(dir .. "/bin")
        os.rmdir(dir .. "/obj")
    end
end

function Be.Declare()
    workspace "be"
    configurations { "Debug", "Release" }
    system "windows"
    architecture "x64"
    location(".")

    Be.DeclareCore()
    Be.DeclareToolkit()
    Be.DeclareMiscConfiguration()
    Be.DeclareExampleGame1()
    Be.DeclareExampleSakura()
    Be.DeclareExampleVulkan()
    Be.DeclareDevtools()
end

return Be
