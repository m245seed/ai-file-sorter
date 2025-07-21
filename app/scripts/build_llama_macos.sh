#!/bin/bash
set -e

# Resolve script directory (cross-shell portable)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LLAMA_DIR="$SCRIPT_DIR/../include/external/llama.cpp"

if [ ! -d "$LLAMA_DIR" ]; then
    echo "Missing llama.cpp submodule. Please run:"
    echo "  git submodule update --init --recursive"
    exit 1
fi

PRECOMPILED_LIBS_DIR="$SCRIPT_DIR/../lib/precompiled"
HEADERS_DIR="$SCRIPT_DIR/../include/llama"

ARCH=$(uname -m)
echo "Building on architecture: $ARCH"

# Enter llama.cpp directory and build
cd "$LLAMA_DIR"
rm -rf build
mkdir -p build
cmake -S . -B build \
  -DBUILD_SHARED_LIBS=ON \
  -DGGML_METAL=ON \
  -DGGML_BLAS=ON -DGGML_BLAS_VENDOR=Accelerate \
  -DGGML_CUDA=OFF \
  -DGGML_OPENCL=OFF \
  -DGGML_VULKAN=OFF \
  -DGGML_SYCL=OFF \
  -DGGML_HIP=OFF \
  -DGGML_KLEIDIAI=OFF \
  -DBLAS_LIBRARIES="-framework Accelerate"

cmake --build build --config Release -- -j$(sysctl -n hw.logicalcpu)

# Copy the resulting dynamic (.dylib) libraries
rm -rf "$PRECOMPILED_LIBS_DIR"
mkdir -p "$PRECOMPILED_LIBS_DIR"
cp build/bin/libllama.dylib "$PRECOMPILED_LIBS_DIR"
cp build/bin/libggml*.dylib "$PRECOMPILED_LIBS_DIR"
cp build/bin/libmtmd.dylib "$PRECOMPILED_LIBS_DIR"

# Copy headers
rm -rf "$HEADERS_DIR"
mkdir -p "$HEADERS_DIR"
cp include/llama.h "$HEADERS_DIR"
cp ggml/src/*.h "$HEADERS_DIR"
cp ggml/include/*.h "$HEADERS_DIR"