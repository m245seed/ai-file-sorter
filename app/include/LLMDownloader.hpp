#pragma once
#include <atomic>
#include <curl/system.h>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>


class LLMDownloader
{
public:
    LLMDownloader();
    void start_download(std::function<void(double)> progress_cb, std::function<void()> on_complete_cb);
    void try_resume_download();
    bool is_download_resumable() const;

    void start(std::function<void(double)> on_progress,
               std::function<void(bool, const std::string&)> on_complete);

    void cancel();

    std::chrono::steady_clock::time_point last_progress_update;
    ~LLMDownloader();

private:
    std::string url;
    std::string destination_dir;
    std::string download_destination;

    std::thread download_thread;
    std::atomic<bool> stop_requested;
    std::map<std::string, std::string> curl_headers;
    mutable std::mutex mutex;

    std::function<void(double)> progress_callback;
    std::function<void()> on_download_complete;
    long long real_content_length{0};

    bool resumable = false;

    static size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream);
    static size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata);
    static int progress_func(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t);

    std::string get_default_llm_destination();
    void parse_headers();
};