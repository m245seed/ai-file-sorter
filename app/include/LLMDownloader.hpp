#pragma once
#include "Settings.hpp"
#include <atomic>
#include <curl/system.h>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <curl/curl.h>


class LLMDownloader
{
public:
    LLMDownloader();
    void init_if_needed();
    bool is_inited();
    void start_download(std::function<void(double)> progress_cb,
                        std::function<void()> on_complete_cb,
                        std::function<void(const std::string &)> on_status_text);
    void try_resume_download();
    bool is_download_resumable() const;
    bool is_download_complete() const;
    
    void start(std::function<void(double)> on_progress,
               std::function<void(bool, const std::string&)> on_complete);

    std::chrono::steady_clock::time_point last_progress_update;
    std::string url;
    std::string download_destination;
    long long real_content_length{0};
    
    ~LLMDownloader();

    enum class DownloadStatus {
        NotStarted,
        InProgress,
        Complete
    };
    
    DownloadStatus get_download_status() const;
    void cancel_download();

private:
    bool initialized{false};
    std::string destination_dir;

    std::thread download_thread;
    std::map<std::string, std::string> curl_headers;
    mutable std::mutex mutex;

    std::function<void(double)> progress_callback;
    std::function<void()> on_download_complete;
    std::function<void(const std::string&)> on_status_text;

    bool resumable = false;

    static size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream);
    static size_t discard_callback(char *ptr, size_t size, size_t nmemb, void *userdata);
    static size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata);
    static int progress_func(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t);

    std::string get_default_llm_destination();
    void parse_headers();
    void perform_download();
    void mark_download_resumable();
    void notify_download_complete();
    void setup_common_curl_options(CURL *curl);
    void setup_header_curl_options(CURL *curl);
    void setup_download_curl_options(CURL *curl, FILE *fp, long resume_offset);
    long determine_resume_offset() const;
    FILE *open_output_file(long resume_offset) const;

    std::atomic<bool> cancel_requested{false};
    long resume_offset = 0;
};