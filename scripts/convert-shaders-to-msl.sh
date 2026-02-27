#!/bin/bash
# Batch convert HLSL shaders to MSL via SPIRV-Cross
# Usage: ./scripts/convert-shaders-to-msl.sh <shader-dir> <output-dir>
#
# Prerequisites:
#   brew install spirv-cross directxshadercompiler
#
# This reads be-engine's @be-shader metadata to find entry points,
# then runs: HLSL → DXC → SPIR-V → SPIRV-Cross → MSL

SHADER_DIR="${1:-.}"
OUTPUT_DIR="${2:-./metal-shaders}"

mkdir -p "$OUTPUT_DIR"

for hlsl_file in "$SHADER_DIR"/*.hlsl; do
    [ -f "$hlsl_file" ] || continue
    
    base=$(basename "$hlsl_file" .hlsl)
    echo "Converting: $base"
    
    # Extract entry points from @be-shader metadata
    vertex_entry=$(grep -A 20 '@be-shader:' "$hlsl_file" | grep '"vertex"' | sed 's/.*: *"\([^"]*\)".*/\1/')
    pixel_entry=$(grep -A 20 '@be-shader:' "$hlsl_file" | grep '"pixel"' | sed 's/.*: *"\([^"]*\)".*/\1/')
    
    if [ -z "$vertex_entry" ]; then
        echo "  No vertex entry found, skipping"
        continue
    fi
    
    # Vertex shader: HLSL → SPIR-V
    echo "  Compiling vertex: $vertex_entry"
    dxc -spirv -T vs_6_0 -E "$vertex_entry" "$hlsl_file" -Fo "$OUTPUT_DIR/${base}.vert.spv" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "  FAILED: vertex SPIR-V compilation"
        continue
    fi
    
    # Vertex SPIR-V → MSL
    spirv-cross --msl "$OUTPUT_DIR/${base}.vert.spv" --output "$OUTPUT_DIR/${base}.vert.metal" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "  FAILED: vertex MSL conversion"
        continue
    fi
    
    if [ -n "$pixel_entry" ]; then
        # Pixel shader: HLSL → SPIR-V
        echo "  Compiling pixel: $pixel_entry"
        dxc -spirv -T ps_6_0 -E "$pixel_entry" "$hlsl_file" -Fo "$OUTPUT_DIR/${base}.frag.spv" 2>/dev/null
        if [ $? -ne 0 ]; then
            echo "  FAILED: pixel SPIR-V compilation"
            continue
        fi
        
        # Pixel SPIR-V → MSL
        spirv-cross --msl "$OUTPUT_DIR/${base}.frag.spv" --output "$OUTPUT_DIR/${base}.frag.metal" 2>/dev/null
        if [ $? -ne 0 ]; then
            echo "  FAILED: pixel MSL conversion"
            continue
        fi
    fi
    
    # Clean up SPIR-V intermediates
    rm -f "$OUTPUT_DIR/${base}.vert.spv" "$OUTPUT_DIR/${base}.frag.spv"
    
    echo "  Done: ${base}.vert.metal + ${base}.frag.metal"
done

echo ""
echo "Conversion complete. Output in: $OUTPUT_DIR"
