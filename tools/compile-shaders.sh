#!/bin/bash
#
# Shader compilation using Slang (shader-slang.org)
# Compiles .hlsl files to platform-specific formats
#
# Usage: compile-shaders.sh <shader-dir> <output-dir> <target>
#   target: dxbc | metal
#
# Examples:
#   ./compile-shaders.sh example-sakura/assets/shaders build/shaders dxbc
#   ./compile-shaders.sh example-sakura/assets/shaders build/shaders metal

set -euo pipefail

SHADER_DIR="${1:?Usage: compile-shaders.sh <shader-dir> <output-dir> <target>}"
OUTPUT_DIR="${2:?Usage: compile-shaders.sh <shader-dir> <output-dir> <target>}"
TARGET="${3:?Usage: compile-shaders.sh <shader-dir> <output-dir> <target>}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOCAL_SLANGC="$SCRIPT_DIR/.bin/slang/bin/slangc"
SLANGC="${SLANGC:-slangc}"

if [[ "$SLANGC" == "slangc" ]] && [[ -x "$LOCAL_SLANGC" ]]; then
    SLANGC="$LOCAL_SLANGC"
fi

if ! command -v "$SLANGC" &>/dev/null; then
    echo "Error: slangc not found. Install from https://shader-slang.org or set SLANGC env var."
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

compile_shader() {
    local src="$1"
    local entry="$2"
    local profile="$3"
    local stage="$4"
    local stem
    stem=$(basename "$src" .hlsl)

    local ext
    case "$TARGET" in
        dxbc)   ext="dxbc" ;;
        metal)  ext="metal" ;;
        spirv)  ext="spv" ;;
        *)      echo "Unknown target: $TARGET"; exit 1 ;;
    esac

    local out="$OUTPUT_DIR/${stem}.${stage}.${ext}"

    if [[ "$TARGET" == "metal" ]] && [[ "$stage" == "hs" || "$stage" == "ds" ]]; then
        echo "  skipping $stem ($stage) for metal target"
        return 0
    fi

    local slang_args=(
        "$src"
        -target "$TARGET"
        -entry "$entry"
        -profile "$profile"
        -o "$out"
        -I "$(dirname "$src")"
        -I "core/src/shaders"
    )

    if [[ "$TARGET" == "spirv" ]]; then
        slang_args+=(
            -fvk-b-shift 0 all
            -fvk-t-shift 1024 all
            -fvk-s-shift 2048 all
            -fvk-u-shift 3072 all
        )
    elif [[ "$TARGET" == "metal" ]]; then
        slang_args+=(
            -fvk-b-shift 0 all
            -fvk-t-shift 0 all
            -fvk-s-shift 8 all
            -fvk-u-shift 0 all
        )
    fi

    echo "  $stem ($stage) -> $out"
    "$SLANGC" "${slang_args[@]}" 2>&1 || {
        echo "  FAILED: $src ($stage)"
        return 1
    }
}

parse_and_compile() {
    local src="$1"
    local header
    header=$(grep -A 100 '@be-shader:' "$src" | head -100)
    local file_failed=0

    local vertex_fn
    vertex_fn=$(echo "$header" | sed -n 's/.*"vertex"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -1)
    if [[ -n "$vertex_fn" ]]; then
        compile_shader "$src" "$vertex_fn" "vs_5_0" "vs" || file_failed=1
    fi

    local pixel_fn
    pixel_fn=$(echo "$header" | sed -n 's/.*"pixel"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -1)
    if [[ -n "$pixel_fn" ]]; then
        compile_shader "$src" "$pixel_fn" "ps_5_0" "ps" || file_failed=1
    fi

    local hull_fn
    hull_fn=$(echo "$header" | sed -n 's/.*"hull"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -1)
    if [[ -n "$hull_fn" ]]; then
        compile_shader "$src" "$hull_fn" "hs_5_0" "hs" || file_failed=1
    fi

    local domain_fn
    domain_fn=$(echo "$header" | sed -n 's/.*"domain"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -1)
    if [[ -n "$domain_fn" ]]; then
        compile_shader "$src" "$domain_fn" "ds_5_0" "ds" || file_failed=1
    fi

    return $file_failed
}

echo "Compiling shaders: $SHADER_DIR -> $OUTPUT_DIR (target: $TARGET)"
echo ""

FAIL_COUNT=0
for hlsl in "$SHADER_DIR"/*.hlsl; do
    [[ -f "$hlsl" ]] || continue

    if grep -q '@be-shader:' "$hlsl"; then
        echo "Processing: $(basename "$hlsl")"
        parse_and_compile "$hlsl" || ((FAIL_COUNT++))
    fi
done

echo ""
if [[ $FAIL_COUNT -gt 0 ]]; then
    echo "Done with $FAIL_COUNT failures."
    exit 1
else
    echo "Done. All shaders compiled successfully."
fi
