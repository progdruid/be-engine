-- premake5.lua

include "premake-helpers.lua"

if _ACTION == "vs2022" or _ACTION == "vs2019" then
    cleanGenerated()
end

newaction {
    trigger = "clean",
    description = "Clean all generated project files",
    onStart = function() cleanGenerated() end
}


workspace "be"
    configurations { "Debug", "Release" }
    system "windows"
    architecture "x86_64"
    location "."

include "premake-vendors.lua"
include "premake-core.lua"
include "premake-toolkit.lua"
include "premake-misc-configuration.lua"
include "premake-example-game-1.lua"
include "premake-example-sakura.lua"
include "premake-devtool-shader-boilerplate-autogen.lua"