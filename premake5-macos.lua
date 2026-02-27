-- premake5-macos.lua
-- Run: premake5 xcode4 --file=premake5-macos.lua
-- Generates Xcode project for macOS Metal build

include "premake-helpers.lua"

newaction {
    trigger = "clean",
    description = "Clean all generated project files",
    onStart = function() cleanGenerated() end
}

workspace "be"
    configurations { "Debug", "Release" }
    system "macosx"
    architecture "ARM64"
    location "."

include "premake-vendors.lua"
include "premake-core.lua"
include "premake-core-macos.lua"
include "premake-toolkit.lua"
include "premake-misc-configuration.lua"
include "premake-example-metal-test.lua"
-- include "premake-example-game-1.lua"   -- DX11 only
-- include "premake-example-sakura.lua"   -- DX11 only
