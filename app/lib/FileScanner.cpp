#include "FileScanner.hpp"
#include <iostream>
#include <filesystem>
#include <unordered_set>
#include <gtk/gtk.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;


std::vector<FileEntry>
FileScanner::get_directory_entries(const std::string &directory_path,
                                   FileScanOptions options)
{
    std::vector<FileEntry> file_paths_and_names;

    for (const auto &entry : fs::directory_iterator(directory_path)) {
        std::string full_path = entry.path().string();
        std::string file_name = entry.path().filename().string();
        bool is_hidden = is_file_hidden(full_path);
        bool should_add = false;
        FileType file_type;

        if (is_junk_file(file_name)) continue;

        if (has_flag(options, FileScanOptions::Files) &&
            fs::is_regular_file(entry)) {
            if (has_flag(options, FileScanOptions::HiddenFiles) || !is_hidden) {
                file_type = FileType::File;
                should_add = true;
            }
        }
        else if (has_flag(options, FileScanOptions::Directories) &&
                 fs::is_directory(entry)) {
            if (has_flag(options, FileScanOptions::HiddenFiles) || !is_hidden) {
                file_type = FileType::Directory;
                should_add = true;
            }
        }

        if (should_add) {
            file_paths_and_names.push_back({full_path, file_name, file_type});
        }
    }

    return file_paths_and_names;
}


bool FileScanner::is_file_hidden(const fs::path &path) {
#ifdef _WIN32
    DWORD attrs = GetFileAttributesW(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES) &&
           (attrs & FILE_ATTRIBUTE_HIDDEN);
#else
    return path.filename().string().starts_with(".");
#endif
}


bool FileScanner::is_junk_file(const std::string& name) {
    static const std::unordered_set<std::string> junk = {
        ".DS_Store", "Thumbs.db", "desktop.ini"
    };
    return junk.contains(name);
}