#include "LocalLLMClient.hpp"
#include "Utils.hpp"
#include "llama.h"
#include <string>
#include <vector>
#include <cctype>
#include <cstdio>
#include <stdexcept>
#include <regex>


void silent_logger(enum ggml_log_level, const char *, void *) {}


LocalLLMClient::LocalLLMClient(const std::string& model_path)
    : model_path(model_path)
{
    llama_log_set(silent_logger, nullptr);
    ggml_backend_load_all();

    int ngl = Utils::determine_ngl_cuda();

    llama_model_params model_params = llama_model_default_params();
    printf("ngl set to %d\n", ngl);
    model_params.n_gpu_layers = ngl;

    model = llama_model_load_from_file(model_path.c_str(), model_params);
    if (!model) {
        throw std::runtime_error("Failed to load model");
    }

    vocab = llama_model_get_vocab(model);

    ctx_params = llama_context_default_params();
    int n_ctx = 1024;
    ctx_params.n_ctx = n_ctx;
    ctx_params.n_batch = n_ctx;
    ctx = llama_init_from_model(model, ctx_params);
    if (!ctx) {
        llama_model_free(model);
        throw std::runtime_error("Failed to create context");
    }

    smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(smpl, llama_sampler_init_min_p(0.05f, 1));
    llama_sampler_chain_add(smpl, llama_sampler_init_temp(0.8f));
    llama_sampler_chain_add(smpl, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
}


std::string LocalLLMClient::make_prompt(const std::string& file_name, FileType file_type)
{
    std::string prompt;
    if (file_type == FileType::File) {
        prompt = "\nCategorize this file:\n" + file_name + "\n";
    } else {
        prompt = "\nCategorize the directory:\n" + file_name + "\n";
    }

    std::string instruction = R"(<|begin_of_text|><|start_header_id|>system<|end_header_id|>
    You are a file categorization assistant. You must always follow the exact format. If the file is an installer, determine the type of software it installs. Base your answer strictly on the filename, extension, and description. The output must be:
    <Main category> : <Subcategory>
    Main category must be broad (one or two words, plural). Subcategory must be specific, relevant, and never just repeat the main category. Output exactly one line. Do not explain, add line breaks, or use words like 'Subcategory'. If uncertain, always make your best guess based on the name only. Do not apologize or state uncertainty. Never say you lack information.
    Examples:
    Texts : Documents
    Productivity : File managers
    Tables : Financial logs
    Utilities : Task managers
    <|eot_id|><|start_header_id|>user<|end_header_id|>
    )" + prompt + R"(<|eot_id|><|start_header_id|>assistant<|end_header_id|>)";

    return instruction;
}


std::string LocalLLMClient::generate_response(const std::string &prompt, int n_predict)
{
    llama_free(ctx);
    ctx = llama_init_from_model(model, ctx_params);

    std::vector<llama_chat_message> messages;
    messages.push_back({"user", prompt.c_str()});
    const char * tmpl = llama_model_chat_template(model, nullptr);
    std::vector<char> formatted_prompt(8192);

    int actual_len = llama_chat_apply_template(tmpl, messages.data(), messages.size(), true,
                                            formatted_prompt.data(), formatted_prompt.size());
    if (actual_len < 0) {
        fprintf(stderr, "Failed to apply chat template\n");
        return "";
    }
    std::string final_prompt(formatted_prompt.data(), actual_len);

    const int n_prompt = -llama_tokenize(vocab, final_prompt.c_str(), final_prompt.size(), NULL, 0, true, true);
    std::vector<llama_token> prompt_tokens(n_prompt);

    if (llama_tokenize(vocab, final_prompt.c_str(), final_prompt.size(), prompt_tokens.data(), prompt_tokens.size(), true, true) < 0) {
        fprintf(stderr, "%s: error: failed to tokenize the prompt\n", __func__);
        llama_model_free(model);
        return "";
    }

    llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());
    llama_token new_token_id;
    std::string output;

    const int max_tokens = n_predict;
    int generated_tokens = 0;
    for (int n_pos = 0; generated_tokens < max_tokens; ) {
        if (llama_decode(ctx, batch)) {
            break;
        }

        n_pos += batch.n_tokens;
        new_token_id = llama_sampler_sample(smpl, ctx, -1);

        if (llama_vocab_is_eog(vocab, new_token_id)) {
            break;
        }

        if (n_pos >= n_prompt) {
            char buf[128];
            int n = llama_token_to_piece(vocab, new_token_id, buf, sizeof(buf), 0, true);
            if (n < 0) break;
            output.append(buf, n);
            generated_tokens++;
        }

        batch = llama_batch_get_one(&new_token_id, 1);
    }

    while (!output.empty() && std::isspace(output.front())) {
        output.erase(output.begin());
    }

    return sanitize_output(output);
}


std::string LocalLLMClient::categorize_file(const std::string& file_name, FileType file_type)
{
    std::string prompt = make_prompt(file_name, file_type);
    return generate_response(prompt, 64);
}


std::string LocalLLMClient::sanitize_output(std::string& output)
{
    output.erase(0, output.find_first_not_of(" \t\n\r\f\v"));
    output.erase(output.find_last_not_of(" \t\n\r\f\v") + 1);

    std::regex pattern(R"(([^:\s][^\n:]*?\s*:\s*[^\n]+))");
    std::smatch match;
    if (std::regex_search(output, match, pattern)) {
    std::string result = match[1];

    result.erase(0, result.find_first_not_of(" \t\n\r\f\v"));
    result.erase(result.find_last_not_of(" \t\n\r\f\v") + 1);

    size_t paren_pos = result.find(" (");
    if (paren_pos != std::string::npos) {
        result.erase(paren_pos);
        result.erase(result.find_last_not_of(" \t\n\r\f\v") + 1);
    }

    return result;
}

    return output;
}


LocalLLMClient::~LocalLLMClient()
{
    if (smpl) llama_sampler_free(smpl);
    if (ctx) llama_free(ctx);
    if (model) llama_model_free(model);
}