#ifndef LLMCLIENT_HPP
#define LLMCLIENT_HPP

#include "ILLMClient.hpp"
#include <Types.hpp>
#include <string>

class LLMClient : public ILLMClient {
public:
    LLMClient(const std::string &api_key);
    ~LLMClient() override;
    std::string categorize_file(const std::string& file_name,
                                const std::string& file_path,
                                FileType file_type) override;

private:
    std::string api_key;
    std::string send_api_request(std::string json_payload);
    std::string make_payload(const std::string &file_name,
                             const std::string &file_path,
                             const FileType file_type);
};

#endif
