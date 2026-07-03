#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
FRONTEND_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
COMPILER_DIR="$(cd "$FRONTEND_DIR/.." && pwd)"

OUT_DIR="$FRONTEND_DIR/public/wasm"
mkdir -p "$OUT_DIR"

SRC=(
  "$COMPILER_DIR/src/scanner.cpp"
  "$COMPILER_DIR/src/parser.cpp"
  "$COMPILER_DIR/src/token.cpp"
  "$COMPILER_DIR/src/ast.cpp"
  "$COMPILER_DIR/src/visitor.cpp"
  "$COMPILER_DIR/src/visitor_utils.cpp"
  "$COMPILER_DIR/src/codegen_visitor.cpp"
  "$COMPILER_DIR/src/asm.cpp"
  "$COMPILER_DIR/src/asm_peephole.cpp"
  "$COMPILER_DIR/src/type_checker_visitor.cpp"
  "$COMPILER_DIR/src/type_semantics.cpp"
  "$COMPILER_DIR/src/dead_code_eliminator.cpp"
  "$COMPILER_DIR/src/expression_optimizer.cpp"
  "$SCRIPT_DIR/ast_printer.cpp"
  "$SCRIPT_DIR/glue.cpp"
)

em++ "${SRC[@]}" \
  -std=c++23 \
  -I"$COMPILER_DIR/include" \
  -I"$SCRIPT_DIR" \
  -O2 \
  -fexceptions \
  -s WASM=1 \
  -s EXPORTED_FUNCTIONS=_compile_code,_free_allocated,___cxa_increment_exception_refcount,___cxa_decrement_exception_refcount,___cxa_can_catch,___cxa_get_exception_ptr \
  -s DISABLE_EXCEPTION_CATCHING=0 \
  -s EXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s MODULARIZE=1 \
  -s EXPORT_NAME=createCompiler \
  -s ENVIRONMENT=web,node \
  -s FILESYSTEM=0 \
  -s NO_EXIT_RUNTIME=1 \
  -o "$OUT_DIR/compiler.js"

# Patch UTF8ArrayToString to avoid TextDecoder crash on resizable ArrayBuffer
sed -i 's/UTF8Decoder.decode(heapOrArray.subarray(idx,endPtr)/UTF8Decoder.decode(heapOrArray.slice(idx,endPtr)/' "$OUT_DIR/compiler.js"

echo "WASM build complete: $OUT_DIR/compiler.{js,wasm}"
