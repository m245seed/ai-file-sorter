#pragma once
#include "Types.hpp"
#include <string>

class ILLMClient {
public:
    virtual ~ILLMClient() = default;
    virtual std::string categorize_file(const std::string& file_name, FileType file_type) = 0;
};