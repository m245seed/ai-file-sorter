#pragma once
#include <string>
#include <functional>


class LLMDownloader
{
public:
    LLMDownloader();
    void start_download(std::function<void(double)> progress_callback, std::function<void()> on_complete);
    void try_resume_download();
    bool is_download_resumable();

private:
    std::string url;
    std::string destination;
};