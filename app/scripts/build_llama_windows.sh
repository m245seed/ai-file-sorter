#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LLAMA_DIR="$SCRIPT_DIR/../include/external/llama.cpp"

if [ ! -d "$LLAMA_DIR" ]; then
    echo "Missing llama.cpp submodule. Please run:"
    echo "  git submodule update --init --recursive"
    exit 1
fi

PRECOMPILED_LIBS_DIR="$SCRIPT_DIR/../lib/precompiled"
HEADERS_DIR="$SCRIPT_DIR/../include/llama"

cd "$LLAMA_DIR"
rm -rf build
mkdir -p build
cmake -S . -B build \
  -DGGML_CUDA=OFF \
  -DBUILD_SHARED_LIBS=OFF \
  -DGGML_BLAS=ON -DGGML_BLAS_VENDOR=OpenBLAS \
  -DGGML_OPENCL=ON -DGGML_VULKAN=OFF \
  -DGGML_SYCL=OFF \
  -DGGML_HIP=OFF \
  -DGGML_KLEIDIAI=OFF

cmake --build build --config Release -- -j$(nproc 2>/dev/null || echo 4)

mkdir -p "$PRECOMPILED_LIBS_DIR"
cp -f build/src/libllama.a "$PRECOMPILED_LIBS_DIR" || true
cp -f build/common/libcommon.a "$PRECOMPILED_LIBS_DIR" || true
cp -f build/ggml/src/libggml*.a "$PRECOMPILED_LIBS_DIR" || true
# cp -f build/ggml/src/ggml-cuda/libggml*.a "$PRECOMPILED_LIBS_DIR" || true
cp -f build/ggml/src/ggml-blas/libggml*.a "$PRECOMPILED_LIBS_DIR" || true

mkdir -p "$HEADERS_DIR"
cp include/llama.h "$HEADERS_DIR"
cp ggml/src/*.h "$HEADERS_DIR"
cp ggml/include/*.h "$HEADERS_DIR"