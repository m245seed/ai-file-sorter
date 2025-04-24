#include "LLMDownloader.hpp"
#include <cstdlib>
#include <stdexcept>

LLMDownloader::LLMDownloader()
{
    const char* env_url = std::getenv("LOCAL_LLM_DOWNLOAD_URL");
    if (!env_url) {
        throw std::runtime_error("Environment variable LOCAL_LLM_DOWNLOAD_URL is not set");
    }
    url = std::string(env_url);

    const char* env_dest = std::getenv("LOCAL_LLM_DOWNLOAD_DEST");
    if (!env_dest) {
        throw std::runtime_error("Environment variable LOCAL_LLM_DOWNLOAD_DEST is not set");
    }
    destination = std::string(env_dest);
}


void LLMDownloader::start_download(std::function<void(double)> progress_callback,
                                   std::function<void()> on_complete)
{
    // Start async download here and call progress_callback(progress)
    // and on_complete() when done
}

void LLMDownloader::try_resume_download() {
    // Dummy
}


bool LLMDownloader::is_download_resumable() {
    // Dummy
    return false;
}
