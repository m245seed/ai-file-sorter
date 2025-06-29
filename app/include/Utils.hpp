#ifndef UTILS_HPP
#define UTILS_HPP

#include <functional>
#include <string>
#include <vector>
#include <curl/system.h>


class Utils {
public:
    Utils();
    ~Utils();
    static bool is_network_available();
    static std::string get_executable_path();
    static bool is_valid_directory(const char *path);
    static std::vector<unsigned char> hex_to_vector(const std::string &hex);
    static const char* to_cstr(const std::u8string& u8str);
    static void ensure_directory_exists(const std::string &dir);
    static bool is_os_windows();
    static bool is_os_macos();
    static bool is_os_linux();
    static std::string format_size(curl_off_t bytes);
    static int determine_ngl_cuda();
    template <typename Func> void run_on_main_thread(Func &&func);
    static std::string get_default_llm_destination();
    static std::string get_file_name_from_url(std::string url);
    static std::string make_default_path_to_file_from_download_url(std::string url);
    static bool is_cuda_available();
    static bool is_opencl_available(std::vector<std::string>* device_names = nullptr);
    static int get_installed_cuda_runtime_version();
    static std::string get_cudart_dll_name();

private:
    static int get_ngl(int vram_mb);
};

#endif