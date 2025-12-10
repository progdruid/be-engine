
# This is *be* engine.

### Making
To make solution run `./premake5 vs2022` (yes, premake5.exe is included)

### Structure
* `Engine` - static lib to link against
* `MiscConfiguration` - *no-build* project with miscellaneous files, like this one.
* `example-game-1` - executable project, example game, links against `Engine`

To modify the project structure please modify `premake5.lua`

### Enjoy!