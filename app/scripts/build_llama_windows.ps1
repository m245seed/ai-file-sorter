$ErrorActionPreference = "Stop"

# Parse optional argument (cuda=on or cuda=off)
$useCuda = "OFF"
foreach ($arg in $args) {
    if ($arg -match "^cuda=(on|off)$") {
        $useCuda = ($Matches[1].ToUpper())
    }
}

Write-Host "`nCUDA Support: $useCuda`n"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$llamaDir = Join-Path $scriptDir "..\include\external\llama.cpp"

if (-not (Test-Path $llamaDir)) {
    Write-Host "Missing llama.cpp submodule. Please run:"
    Write-Host "  git submodule update --init --recursive"
    exit 1
}

$precompiledLibsDir = Join-Path $scriptDir "..\lib\precompiled"
$headersDir = Join-Path $scriptDir "..\include\llama"

Push-Location $llamaDir

if (Test-Path "build") {
    Remove-Item -Recurse -Force "build"
}
New-Item -ItemType Directory -Path "build" | Out-Null

# Base CMake options
$cmakeArgs = @(
    "-S", ".", "-B", "build",
    "-DCMAKE_C_COMPILER=`"C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe`"",
    "-DCMAKE_CXX_COMPILER=`"C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe`"",
    "-DBUILD_SHARED_LIBS=ON",
    "-DGGML_CUDA=$useCuda",
    "-DGGML_BLAS=OFF",
    "-DGGML_BLAS_VENDOR=OpenBLAS",
    "-DGGML_OPENCL=OFF",
    "-DGGML_VULKAN=OFF",
    "-DGGML_SYCL=OFF",
    "-DGGML_HIP=OFF",
    "-DGGML_KLEIDIAI=OFF"
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$curlInclude = Join-Path $scriptDir "vcpkg_installed\x64-windows\include"
$curlLib = Join-Path $scriptDir "vcpkg_installed\x64-windows\lib\libcurl.lib"

$openBlasInclude = Join-Path $scriptDir "vcpkg_installed\x64-windows\include\openblas"
$openBlasLib = Join-Path $scriptDir "vcpkg_installed\x64-windows\lib\openblas.lib"

$cmakeArgs += "-DBLAS_INCLUDE_DIRS=`"$openBlasInclude`""
$cmakeArgs += "-DBLAS_LIBRARIES=`"$openBlasLib`""

$cmakeArgs += "-DCURL_INCLUDE_DIR=`"$curlInclude`""
$cmakeArgs += "-DCURL_LIBRARY=`"$curlLib`""

$pkgconfExe = Join-Path $scriptDir "vcpkg_installed\x64-windows\tools\pkgconf\pkgconf.exe"
$cmakeArgs += "-DPKG_CONFIG_EXECUTABLE=`"$pkgconfExe`""

# Only add CUDA paths if enabled
if ($useCuda -eq "ON") {
    $cudaRoot = "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.9"
    $includeDir = "$cudaRoot/include"
    $libDir = "$cudaRoot/lib/x64/cudart.lib"
    $cmakeArgs += "-DCUDA_TOOLKIT_ROOT_DIR=`"$cudaRoot`""
    $cmakeArgs += "-DCUDA_INCLUDE_DIRS=`"$includeDir`""
    $cmakeArgs += "-DCUDA_CUDART=`"$libDir`""
}

& cmake @cmakeArgs

cmake --build build --config Release -- /m

Pop-Location

# Clean and repopulate precompiled DLLs
if (Test-Path $precompiledLibsDir) {
    Remove-Item -Recurse -Force $precompiledLibsDir
}
New-Item -ItemType Directory -Force -Path $precompiledLibsDir | Out-Null

$releaseBin = Join-Path $llamaDir "build\bin\Release"
Get-ChildItem "$releaseBin\ggml*.dll" -ErrorAction SilentlyContinue | Copy-Item -Destination $precompiledLibsDir
Copy-Item "$releaseBin\llama.dll" -Destination $precompiledLibsDir -ErrorAction SilentlyContinue

# Copy headers
New-Item -ItemType Directory -Force -Path $headersDir | Out-Null
Copy-Item "$llamaDir\include\llama.h" -Destination $headersDir
Copy-Item "$llamaDir\ggml\src\*.h" -Destination $headersDir -ErrorAction SilentlyContinue
Copy-Item "$llamaDir\ggml\include\*.h" -Destination $headersDir -ErrorAction SilentlyContinue
