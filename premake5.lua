-- premake5.lua
local Be     = require("build/be")
local Vendor = require("build/vendor")


-- options ----------------------------------------------------------------------

newoption {
    trigger = "clean",
    description = "Also clean bin/ and obj/ build outputs"
}


-- actions ----------------------------------------------------------------------

newaction {
    trigger = "clean",
    description = "Clean all generated project files and build outputs",
    onStart = function()
        Be.CleanProjectFiles()
        Be.CleanBinaries()
        Vendor.CleanProjectFiles()
        Vendor.CleanBinaries()
    end
}

newaction {
    trigger = "cook-vendors",
    description = "Clean and rebuild all vendor libraries for all configurations",
    onStart = function()
        print("--- Cleaning leftovers ---")
        Vendor.CleanProjectFiles()
        Vendor.CleanBinaries()

        print("--- Ordering vendor solution ---")
        local premake = 'premake5.exe --file=premake5-vendor.lua vs2022'
        assert(os.execute(premake), "Failed to generate vendor solution")

        local configurations = { "Debug", "Release" }
        for _, cfg in ipairs(configurations) do
            print("--- Cooking vendor " .. cfg .. " ---")
            local cmd = 'msbuild vendor.sln /p:Configuration=' .. cfg .. ' /p:Platform=x64 /m /v:minimal'
            assert(os.execute(cmd), "Failed to build vendor " .. cfg)
        end

        print("--- Vendor served. Doneness: Medium-rare ---")
    end
}



-- generation -------------------------------------------------------------------

if _ACTION == "vs2022" or _ACTION == "vs2019" then
    Be.CleanProjectFiles()
    print("Cleaned past project files")
    if _OPTIONS["clean"] then
        Be.CleanBinaries()
        print("Cleaned binaries")
    end
    Be.Declare()
end
