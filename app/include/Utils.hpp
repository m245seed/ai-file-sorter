#ifndef UTILS_HPP
#define UTILS_HPP

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
};

#endif