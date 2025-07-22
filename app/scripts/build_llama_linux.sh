#!/bin/bash
set -e

# Resolve script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LLAMA_DIR="$SCRIPT_DIR/../include/external/llama.cpp"

if [ ! -d "$LLAMA_DIR" ]; then
    echo "Missing llama.cpp submodule. Please run:"
    echo "  git submodule update --init --recursive"
    exit 1
fi

PRECOMPILED_LIBS_DIR="$SCRIPT_DIR/../lib/precompiled"
HEADERS_DIR="$SCRIPT_DIR/../include/llama"

# Determine CUDA setting from first argument (default OFF)
CUDASWITCH="OFF"
if [[ "${1,,}" == "cuda=on" ]]; then
    CUDASWITCH="ON"
fi

# Enter llama.cpp directory and build
cd "$LLAMA_DIR"
rm -rf build
mkdir -p build

# Compile shared libs:
echo "Inside script: CC=$CC, CXX=$CXX"

cmake -S . -B build -DGGML_CUDA=$CUDASWITCH \
      -DGGML_OPENCL=ON \
      -DGGML_BLAS=ON \
      -DGGML_BLAS_VENDOR=OpenBLAS \
      -DBUILD_SHARED_LIBS=ON \
      -DCMAKE_CUDA_HOST_COMPILER=/usr/bin/g++-10

cmake --build build --config Release -- -j$(nproc)

if [[ -d "$PRECOMPILED_LIBS_DIR" ]]; then
    rm -rf "$PRECOMPILED_LIBS_DIR"
fi

mkdir -p "$PRECOMPILED_LIBS_DIR"

# Copy dynamic libs
cp build/bin/*.so "$PRECOMPILED_LIBS_DIR"

# Copy headers
rm -rf "$HEADERS_DIR" && mkdir -p "$HEADERS_DIR"
cp include/llama.h "$HEADERS_DIR"
cp ggml/src/*.h "$HEADERS_DIR"
cp ggml/include/*.h "$HEADERS_DIR"

# Clean up
rm -rf build