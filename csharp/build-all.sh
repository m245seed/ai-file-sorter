#!/bin/bash

# Build script for AI File Sorter C# Version
# Creates self-contained executables for different platforms

echo "Building AI File Sorter for all platforms..."
echo ""

# Clean previous builds
echo "Cleaning previous builds..."
dotnet clean -c Release
rm -rf publish/

# Build for Windows x64
echo ""
echo "Building for Windows (x64)..."
dotnet publish -c Release -r win-x64 --self-contained -o publish/win-x64 \
    /p:PublishSingleFile=true /p:IncludeNativeLibrariesForSelfExtract=true

# Build for Linux x64
echo ""
echo "Building for Linux (x64)..."
dotnet publish -c Release -r linux-x64 --self-contained -o publish/linux-x64 \
    /p:PublishSingleFile=true /p:IncludeNativeLibrariesForSelfExtract=true

# Build for macOS x64 (Intel)
echo ""
echo "Building for macOS (x64 - Intel)..."
dotnet publish -c Release -r osx-x64 --self-contained -o publish/osx-x64 \
    /p:PublishSingleFile=true /p:IncludeNativeLibrariesForSelfExtract=true

# Build for macOS arm64 (Apple Silicon)
echo ""
echo "Building for macOS (arm64 - Apple Silicon)..."
dotnet publish -c Release -r osx-arm64 --self-contained -o publish/osx-arm64 \
    /p:PublishSingleFile=true /p:IncludeNativeLibrariesForSelfExtract=true

echo ""
echo "Build complete!"
echo ""
echo "Published executables are in the publish/ directory:"
echo "  - Windows (x64):            publish/win-x64/AiFileSorter.exe"
echo "  - Linux (x64):              publish/linux-x64/AiFileSorter"
echo "  - macOS (Intel):            publish/osx-x64/AiFileSorter"
echo "  - macOS (Apple Silicon):    publish/osx-arm64/AiFileSorter"
