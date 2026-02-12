-- premake-misc-configuration.lua

-- misc project
project "misc-configuration"
    kind "Utility"
    files {
        ".gitignore",
        
        "README.md",
        "CLAUDE.md",
        "LICENSE",
        
        "*DotSettings*",
        
        "premake5.lua",
        "premake-core.lua",
        "premake-toolkit.lua",
        "premake-vendors.lua",
        "premake-misc-configuration.lua",
        "premake-example-game-1.lua",
        "premake-example-sakura.lua",
    }