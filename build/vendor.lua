-- build/vendor.lua
Vendor = Vendor or {}

require("build/vendor/cpptrace")
require("build/vendor/libassert")

Vendor.DIRS = {
    "vendor/cpptrace",
    "vendor/libassert",
}

function Vendor.CleanProjectFiles()
    os.remove("vendor.sln")
    for _, dir in ipairs(Vendor.DIRS) do
        os.remove(dir .. "/**.vcxproj")
        os.remove(dir .. "/**.vcxproj.filters")
        os.remove(dir .. "/**.vcxproj.user")
    end
end

function Vendor.CleanBinaries()
    for _, dir in ipairs(Vendor.DIRS) do
        os.rmdir(dir .. "/bin")
        os.rmdir(dir .. "/obj")
    end
end

function Vendor.GenerateVersionHeaders()
    -- cpptrace v1.0.4
    local cpptraceDir = _MAIN_SCRIPT_DIR .. "/vendor/cpptrace/include/cpptrace"
    os.mkdir(cpptraceDir)
    local f = io.open(cpptraceDir .. "/version.hpp", "w")
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
    local libassertDir = _MAIN_SCRIPT_DIR .. "/vendor/libassert/include/libassert"
    os.mkdir(libassertDir)
    f = io.open(libassertDir .. "/version.hpp", "w")
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

function Vendor.Declare()
    Vendor.GenerateVersionHeaders()

    workspace "vendor"
    configurations { "Debug", "Release" }
    system "windows"
    architecture "x64"
    location "."

    group "vendor"
    Vendor.DeclareCpptrace()
    Vendor.DeclareLibassert()
    group ""
end

return Vendor
