#include "Utils.hpp"
#include <cuda_runtime.h>
#include <filesystem>
#include <stdlib.h>
#include <string>
#include <glibmm/fileutils.h>
#ifdef _WIN32
    #include <windows.h>
#elif __linux__
    #include <unistd.h>
    #include <limits.h>
#elif __APPLE__
    #include <mach-o/dyld.h>
    #include <limits.h>
#endif
#include <iostream>


bool Utils::is_network_available()
{
#ifdef _WIN32
    int result = system("ping -n 1 google.com > NUL 2>&1");
#else
    int result = system("ping -c 1 google.com > /dev/null 2>&1");
#endif
    return result == 0;
}


std::string Utils::get_executable_path()
{
#ifdef _WIN32
    char result[MAX_PATH];
    GetModuleFileName(NULL, result, MAX_PATH);
    return std::string(result);
#elif __linux__
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    return (count != -1) ? std::string(result, count) : "";
#elif __APPLE__
    char result[PATH_MAX];
    uint32_t size = sizeof(result);
    if (_NSGetExecutablePath(result, &size) == 0) {
        return std::string(result);
    } else {
        throw std::runtime_error("Path buffer too small");
    }
#else
    throw std::runtime_error("Unsupported platform");
#endif
}


bool Utils::is_valid_directory(const char *path)
{
    return g_file_test(path, G_FILE_TEST_IS_DIR);
}


std::vector<unsigned char> Utils::hex_to_vector(const std::string& hex) {
    std::vector<unsigned char> data;
    for (size_t i = 0; i < hex.length(); i += 2) {
        unsigned char byte = static_cast<unsigned char>(std::stoi(hex.substr(i, 2), nullptr, 16));
        data.push_back(byte);
    }
    return data;
}


const char* Utils::to_cstr(const std::u8string& u8str) {
    return reinterpret_cast<const char*>(u8str.c_str());
}


void Utils::ensure_directory_exists(const std::string &dir)
{
    try {
        if (!std::filesystem::exists(dir)) {
            std::filesystem::create_directories(dir);
        }
    } catch (const std::exception &e) {
        std::cerr << "Error creating log directory: " << e.what() << std::endl;
        throw;
    }
}


bool Utils::is_os_windows() {
#if defined(_WIN32)
    return true;
#else
    return false;
#endif
}

    
bool Utils::is_os_macos() {
#if defined(__APPLE__)
    return true;
#else
    return false;
#endif
}

    
bool Utils::is_os_linux() {
#if defined(__linux__)
    return true;
#else
    return false;
#endif
}


std::string Utils::format_size(curl_off_t bytes)
{
    char buffer[64];
    if (bytes >= (1LL << 30))
        snprintf(buffer, sizeof(buffer), "%.2f GB", bytes / (double)(1LL << 30));
    else if (bytes >= (1LL << 20))
        snprintf(buffer, sizeof(buffer), "%.2f MB", bytes / (double)(1LL << 20));
    else if (bytes >= (1LL << 10))
        snprintf(buffer, sizeof(buffer), "%.2f KB", bytes / (double)(1LL << 10));
    else
        snprintf(buffer, sizeof(buffer), "%.2f B", bytes / (double)(1LL << 10));
    return buffer;
}


int Utils::get_ngl(int vram_mb) {
    if (vram_mb < 2048) return 0;

    int step = (vram_mb - 2048) / 512;
    return std::min(12 + step * 2, 32);
}


int Utils::determine_ngl_cuda()
{
    int device = 0;
    cudaDeviceProp prop;

    if (cudaGetDeviceProperties(&prop, device) != cudaSuccess) {
        std::cerr << "Warning: Unable to get CUDA device properties. Using safe fallback value.\n";
        return 0;
    }

    size_t total_mem_bytes = 0;
    if (cudaMemGetInfo(nullptr, &total_mem_bytes) != cudaSuccess) {
        total_mem_bytes = prop.totalGlobalMem;
    }

    int vram_mb = static_cast<int>(total_mem_bytes / (1024 * 1024));

    return get_ngl(vram_mb);
}


template <typename Func>
void Utils::run_on_main_thread(Func&& func)
{
    auto* task = new std::function<void()>(std::forward<Func>(func));
    g_idle_add([](gpointer data) -> gboolean {
        auto* f = static_cast<std::function<void()>*>(data);
        (*f)();
        delete f;
        return G_SOURCE_REMOVE;
    }, task);
}


std::string Utils::get_default_llm_destination()
{
    const char* home = std::getenv("HOME");

    if (Utils::is_os_windows()) {
        const char* appdata = std::getenv("APPDATA");
        if (!appdata) throw std::runtime_error("APPDATA not set");
        return std::filesystem::path(appdata) / "aifilesorter" / "llms";
    }

    if (!home) throw std::runtime_error("HOME not set");

    if (Utils::is_os_macos()) {
        return std::filesystem::path(home) / "Library" / "Application Support" / "aifilesorter" / "llms";
    }

    return std::filesystem::path(home) / ".local" / "share" / "aifilesorter" / "llms";
}


std::string Utils::get_file_name_from_url(std::string url)
{
    auto last_slash = url.find_last_of('/');
    if (last_slash == std::string::npos || last_slash == url.length() - 1) {
        throw std::runtime_error("Invalid download URL: can't extract filename");
    }
    std::string filename = url.substr(last_slash + 1);

    return filename;
}


std::string Utils::make_default_path_to_file_from_download_url(std::string url)
{
    std::string filename = get_file_name_from_url(url);
    std::string path_to_file = (std::filesystem::path(get_default_llm_destination()) / filename).string();
    return path_to_file;
}