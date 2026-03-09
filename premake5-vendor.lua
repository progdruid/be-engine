local Vendor = require("build/vendor")

if _ACTION == "vs2022" or _ACTION == "vs2019" then
    Vendor.CleanProjectFiles()
    Vendor.Declare()
end
