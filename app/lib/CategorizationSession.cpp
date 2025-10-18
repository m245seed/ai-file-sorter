#include "CategorizationSession.hpp"
#include "CryptoManager.hpp"
#include "Logger.hpp"
#include <stdexcept>
#include <iostream>
#include <algorithm> // For std::fill
#include <cstdlib>   // For std::getenv


CategorizationSession::CategorizationSession()
{
    const char* env_pc = std::getenv("ENV_PC");
    const char* env_rr = std::getenv("ENV_RR");

    if (!env_pc || !env_rr) {
        if (auto logger = Logger::get_logger("core_logger")) {
            logger->error("Missing environment variables for key decryption. ENV_PC present: {}, ENV_RR present: {}",
                          env_pc ? "yes" : "no",
                          env_rr ? "yes" : "no");
        }
        throw std::runtime_error("Missing environment variables for key decryption");
    }

    if (auto logger = Logger::get_logger("core_logger")) {
        logger->debug("Reconstructing API key using embedded environment variables.");
    }

    CryptoManager crypto(env_pc, env_rr);
    key = crypto.reconstruct();

    if (auto logger = Logger::get_logger("core_logger")) {
        logger->info("API key reconstructed successfully from embedded environment variables.");
    }
}


CategorizationSession::~CategorizationSession()
{
    // Securely clear key memory
    std::fill(key.begin(), key.end(), '\0');
}


LLMClient CategorizationSession::create_llm_client() const
{
    return LLMClient(key);
}
