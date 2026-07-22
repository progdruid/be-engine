# bechef

Content pipeline CLI. Cooks assets and shaders, validates the workspace, generates shader boilerplate.

```bash
bechef cook      --app <name> --out <dir> [--mode copy|symlink] [--root <dir>]
bechef check     [--root <dir>]
bechef shadergen [--project <name>] [--file <path>] [--check] [--watch] [--root <dir>]
```

## The Wall

`Load.cpp` reads the workspace once, into `Workspace`. Everything the tool knows is there. Verbs
read it; they never re-derive it and never touch the disk to answer a question.

| On the wall | Type | Meaning |
|-------------|------|---------|
| Config | `WorkspaceConfig` | `workspace.bechef` at the root: `modules`, `apps`, `ignore` |
| Project | `Project` | `<name>/project.bechef`: `depends`, `shaders`, `assets` |
| LocalShaders | `vector<ShaderFile>` | every file in the project's own shader dirs, parsed |
| LocalAssetFiles | `vector<SourceFile>` | every file in the project's own asset dirs, ignore-filtered |
| Scope | `vector<const Project*>` | transitive `depends`, topologically ordered, includes the project itself |
| Schemes | `VisibleSchemes` | material schemes visible to a project, i.e. those declared anywhere in its scope |
| Binds | `vector<ResolvedBind>` | each shader `bind` paired with the scheme it names |
| FlatShaders | `vector<const ShaderFile*>` | the scope's shaders flattened into one namespace |

A shader's `bind` names a scheme. The scheme must be declared inside the project's own scope,
which is what makes cross-module binds work and what makes an out-of-scope bind an error.

`LocalShaders` holds *every* file in the shader dirs, not just `.hlsl`. An `.hlsli` include simply
parses to a shader that declares nothing: no `@be-shader`, no schemes, no boilerplate region, so
generation skips it. It still has to be on the wall because cook flattens it into the runtime
`shaders/` dir alongside the shaders that include it, and because two includes with the same
filename in one scope collide exactly like two shaders would.

Something earns a place on the wall when it is derived purely from workspace contents *and* has a
failure mode. The cook plan does not qualify: it is output layout parameterized by the
invocation (`--out`, `--mode`), so it stays in `Cook.cpp`. The flat shader
namespace does qualify, and its failure mode is the name collision.

## The invariant

After `LoadWorkspace`, the wall is complete. Every error the tool can report is already sitting on
it as an `unexpected`. No verb discovers a new one.

That is why nothing needs to run before anything else. `Verify` walks the wall and collects
errors. `GenerateShaderSource` reads a `ShaderFile` and cannot fail. `Cook` propagates
`FlatShaders`. There is no phase order to remember and no "must run first" assert to violate.

## Files

| File | Role |
|------|------|
| `main.cpp` | verb dispatch |
| `CommandGrammar` | line grammar: `verb --flag value` / `verb positional`. Parses both the CLI and the `.bechef` files |
| `Workspace` | the data. Wall types and lookups |
| `Load` | the only reader of disk inputs. Builds the wall |
| `Verify` | walks the wall, collects errors |
| `ShaderBoilerplate` | generates the `@be-auto-boilerplate` region text |
| `ShaderGen` | writes regions, `--check` fails on stale, `--watch` regenerates on save |
| `Cook` | flattens shaders, copies assets, prunes stale outputs |
| `Try.h` | `bechef_try` |

## Error handling

`std::expected<T, std::string>` throughout. Invalid data is held as data: a broken project or
shader keeps its identity and its error instead of aborting the run, so one bad workspace reports
all of its problems at once, each exactly once. Errors are user-facing config and shader mistakes.
Use `be_assert` for internal invariants.

`bechef_try(declaration, expression)` propagates. A third argument adds context, with the error
appended last:

```cpp
bechef_try(const auto* project, VerifyApp(app));
bechef_try(const auto parsed, grammar.Parse(command), "{}/project.bechef: {}", name);
```

## Cook layout

Shader dirs of the app and its whole closure flatten into one runtime `shaders/`, so shader
filenames must be unique across that closure. Asset dirs copy to `assets/` preserving their tree.
`--mode symlink` makes edits live.

`be_cook_app` runs `shadergen --check` before cooking, so a stale region fails the build.
