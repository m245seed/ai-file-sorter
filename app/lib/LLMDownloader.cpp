#include "LLMDownloader.hpp"
#include <cstdlib>
#include <curl/curl.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <glibmm/main.h>
#include <Utils.hpp>


LLMDownloader::LLMDownloader()
    : url([] {
        const char* env_url = std::getenv("LOCAL_LLM_DOWNLOAD_URL");
        if (!env_url) {
            throw std::runtime_error("Environment variable LOCAL_LLM_DOWNLOAD_URL is not set");
        }
        return std::string(env_url);
    }()),
      destination_dir(get_default_llm_destination()),
      stop_requested(false)
{
    auto last_slash = url.find_last_of('/');
    if (last_slash == std::string::npos || last_slash == url.length() - 1) {
        throw std::runtime_error("Invalid download URL: can't extract filename");
    }

    std::string filename = url.substr(last_slash + 1);

    std::filesystem::create_directories(destination_dir);

    download_destination = (std::filesystem::path(destination_dir) / filename).string();
    last_progress_update = std::chrono::steady_clock::now();
    parse_headers();
}

size_t LLMDownloader::write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    // size_t bytes = size * nmemb;
    // std::cout << "[write_data] Writing " << bytes << " bytes to file" << std::endl;
    return fwrite(ptr, size, nmemb, stream);
}


void LLMDownloader::parse_headers()
{
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    std::lock_guard<std::mutex> lock(mutex);
    curl_headers.clear();
    real_content_length = 0;
    resumable = false;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);  // HEAD request
    curl_easy_setopt(curl, CURLOPT_HEADER, 1L);  // Get headers only
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &LLMDownloader::header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error("CURL HEAD request failed: " + std::string(curl_easy_strerror(res)));
    }
}


int LLMDownloader::progress_func(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
    curl_off_t, curl_off_t)
{
    auto* self = static_cast<LLMDownloader*>(clientp);

    if (self->stop_requested) {
        return 1; // non-zero return code will make curl abort
    }

    curl_off_t effective_total = dltotal;
    if (effective_total == 0 && self->real_content_length > 0) {
        effective_total = self->real_content_length;
    }

    if (effective_total == 0) {
        return 0;
    }

    double progress = static_cast<double>(dlnow) / static_cast<double>(effective_total);

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - self->last_progress_update);

    if (elapsed.count() > 100) {
        self->last_progress_update = now;

        Glib::signal_idle().connect_once([self, progress]() {
            if (self->progress_callback) {
                self->progress_callback(progress);
            }
        });
    }

    return 0;
}


size_t LLMDownloader::header_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
    size_t total_size = size * nitems;
    auto* self = static_cast<LLMDownloader*>(userdata);
    std::string header(buffer, total_size);

    // Normalize and parse header
    auto colon_pos = header.find(':');
    if (colon_pos != std::string::npos) {
        std::string key = header.substr(0, colon_pos);
        std::string value = header.substr(colon_pos + 1);

        // Trim spaces
        key.erase(std::remove_if(key.begin(), key.end(), ::isspace), key.end());
        value.erase(0, value.find_first_not_of(" \t\r\n"));
        value.erase(value.find_last_not_of(" \t\r\n") + 1);

        // Lowercase key
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        // Store all headers
        self->curl_headers[key] = value;

        if (key == "content-length") {
            try {
                self->real_content_length = std::stoll(value);
                // std::cout << "[header_callback] Got Content-Length: " << self->real_content_length << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[header_callback] Failed to parse Content-Length: " << e.what() << std::endl;
            }            
        }
    }

    return total_size;
}


void LLMDownloader::start_download(std::function<void(double)> progress_cb,
                                   std::function<void()> on_complete_cb)
{
    progress_callback = std::move(progress_cb);
    on_download_complete = std::move(on_complete_cb);

    download_thread = std::thread([this]() {
        CURL* curl = curl_easy_init();
        if (!curl) {
            throw std::runtime_error("Failed to initialize curl");
        }

        FILE* fp = nullptr;
        long resume_from = 0;

        // === Handle resumable state ===
        if (is_download_resumable()) {
            FILE* temp_fp = fopen(download_destination.c_str(), "rb");
            if (temp_fp) {
                fseek(temp_fp, 0, SEEK_END);
                resume_from = ftell(temp_fp);
                fclose(temp_fp);
                std::cout << "[start_download] Resuming from byte: " << resume_from << std::endl;

                if (resume_from >= real_content_length) {
                    // std::cout << "File already fully downloaded. Skipping download.\n";
                    curl_easy_cleanup(curl);  // cleanup before early return
                    Glib::signal_idle().connect_once([this]() {
                        if (on_download_complete) {
                            on_download_complete();
                        }
                    });
                    return;
                }
            }
        }

        // === Open file for writing/resuming ===
        fp = fopen(download_destination.c_str(), resume_from > 0 ? "ab" : "wb");
        if (!fp) {
            curl_easy_cleanup(curl);
            throw std::runtime_error("Failed to open file for writing: " + download_destination);
        }

        // === CURL options ===
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &LLMDownloader::header_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &LLMDownloader::write_data);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, &LLMDownloader::progress_func);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
        curl_easy_setopt(curl, CURLOPT_RESUME_FROM, resume_from);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        CURLcode res = curl_easy_perform(curl);

        // === Cleanup (always do these if they were set) ===
        if (fp) fclose(fp);
        if (curl) curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            std::cerr << "[start_download] Download failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            std::lock_guard<std::mutex> lock(mutex);
            resumable = true;
            Glib::signal_idle().connect_once([this]() {
                if (on_download_complete) {
                    on_download_complete();
                }
            });
        }
    });

    // download_thread.detach();
}


// void LLMDownloader::try_resume_download()
// {
//     start_download(progress_callback, on_download_complete);
// }


bool LLMDownloader::is_download_resumable() const
{
    std::lock_guard<std::mutex> lock(mutex);

    auto it = curl_headers.find("accept-ranges");
    auto len_it = curl_headers.find("content-length");

    bool ranges = (it != curl_headers.end() && it->second == "bytes");
    bool has_length = (len_it != curl_headers.end() && std::stoll(len_it->second) > 0);

    return ranges && has_length;
}


std::string LLMDownloader::get_default_llm_destination() {
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


LLMDownloader::~LLMDownloader() {
    if (download_thread.joinable()) {
        download_thread.join(); // Attends la fin du thread avant de détruire
    }
}