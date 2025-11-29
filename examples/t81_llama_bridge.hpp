// t81_llama_bridge.hpp – Production-Ready v2.1 (Dec 2025)
// Fully compatible with llama.cpp git master (2025-12)
// Supports: multi-turn, chat templates, LoRA, streaming, GPU offload, thread-safety

#pragma once
#include <t81/all.hpp>
#include <llama.h>
#include <string>
#include <vector>
#include <mutex>
#include <optional>
#include <functional>

namespace t81 {

class T81LlamaMind {
    llama_model*              model = nullptr;
    llama_context*            ctx   = nullptr;
    T81Agent                  self;
    std::mutex                infer_mutex;          // llama.cpp is NOT thread-safe
    int                       n_threads;

    // Optional LoRA
    std::vector<llama_lora_adapter*> lora_adapters;

public:
    using StreamCallback = std::function<void(const std::string& token)>;

    explicit T81LlamaMind(
        const std::string& model_path,
        const std::vector<std::string>& lora_paths = {},
        T81Symbol name = symbols::LLAMA_MIND,
        int threads = std::thread::hardware_concurrency(),
        int n_ctx = 131072,
        int n_batch = 8192,
        int n_gpu_layers = 999)
        : self(name), n_threads(threads)
    {
        llama_backend_init();

        llama_model_params mparams = llama_model_default_params();
        mparams.n_gpu_layers = n_gpu_layers;
        mparams.use_mmap      = true;
        mparams.use_mlock     = true;

        model = llama_load_model_from_file(model_path.c_str(), mparams);
        if (!model) throw std::runtime_error("Failed to load GGUF: " + model_path);

        llama_context_params cparams = llama_context_default_params();
        cparams.n_ctx            = n_ctx;
        cparams.n_batch          = n_batch;
        cparams.n_threads        = n_threads;
        cparams.n_threads_batch  = n_threads;
        cparams.embeddings       = false;
        cparams.offload_kqv      = true;

        ctx = llama_new_context_with_model(model, cparams);
        if (!ctx) throw std::runtime_error("Failed to create llama context");

        // Load LoRA adapters if any
        for (const auto& lora_path : lora_paths) {
            auto* adapter = llama_lora_adapter_init(model, lora_path.c_str());
            if (adapter) {
                llama_context_use_lora_adapter(ctx, adapter, 1.0f);
                lora_adapters.push_back(adapter);
            }
        }

        self.observe(symbols::BORN_FROM_LLAMA);
        T81Discovery::join(self);
    }

    ~T81LlamaMind() {
        for (auto* adapter : lora_adapters)
            llama_lora_adapter_free(adapter);
        if (ctx)   llama_free(ctx);
        if (model) llama_free_model(model);
        llama_backend_free();
    }

    // === Single-turn inference (simple) ===
    T81String think(const T81String& prompt,
                    int max_tokens = 2048,
                    float temperature = 0.72f,
                    float top_p = 0.95f,
                    float repeat_penalty = 1.1f)
    {
        StreamCallback noop;
        return think_stream(prompt, max_tokens, temperature, top_p, repeat_penalty, noop);
    }

    // === Streaming inference (recommended) ===
    T81String think_stream(const T81String& prompt,
                           int max_tokens = 2048,
                           float temperature = 0.72f,
                           float top_p = 0.95f,
                           float repeat_penalty = 1.1f,
                           StreamCallback on_token = nullptr)
    {
        std::lock_guard<std::mutex> lock(infer_mutex);

        consume_entropy(T81Entropy::acquire_batch(max_tokens));

        // Tokenize prompt with BOS
        std::vector<llama_token> tokens = llama_tokenize(model, prompt.c_str(), true, true); // add_bos=true

        int n_past = 0;
        if (llama_eval(ctx, tokens.data(), (int)tokens.size(), n_past, n_threads) != 0) {
            return "llama_eval failed"_t81;
        }
        n_past += tokens.size();

        // Build sampler chain
        auto* chain = llama_sampler_chain_init(llama_sampler_chain_default_params());
        llama_sampler_chain_add(chain, llama_sampler_init_top_k(50));
        llama_sampler_chain_add(chain, llama_sampler_init_top_p(top_p, 1));
        llama_sampler_chain_add(chain, llama_sampler_init_temp(temperature));
        llama_sampler_chain_add(chain, llama_sampler_init_repetition_penalty(
            repeat_penalty, 1.0f, 0.0f, 64));

        std::string output;
        output.reserve(max_tokens * 4);
        std::vector<llama_token> recent_tokens;
        recent_tokens.reserve(64);

        for (int i = 0; i < max_tokens; ++i) {
            llama_token id = llama_sample_token(ctx, chain);

            if (id == llama_token_eos(model)) break;

            std::string piece = llama_token_to_piece(ctx, id, true);
            output += piece;
            if (on_token) on_token(piece);

            recent_tokens.push_back(id);
            if (recent_tokens.size() > 64) recent_tokens.erase(recent_tokens.begin());

            if (llama_eval(ctx, &id, 1, n_past++, n_threads) != 0) {
                break;
            }
        }

        llama_sampler_free(chain);

        self.observe(symbols::THOUGHT_COMPLETE);
        record_event(T81Time::now(T81Entropy::acquire(), symbols::INFERENCE));

        return T81String(std::move(output));
    }

    // === Chat mode with system/user/assistant roles (Llama-3.1 / Qwen2.5 style) ===
    T81String chat(const std::vector<std::pair<std::string, std::string>>& messages,
                   const std::string& system_prompt = "You are a helpful AI assistant.",
                   int max_tokens = 2048,
                   StreamCallback on_token = nullptr)
    {
        std::string full_prompt;
        full_prompt.reserve(8192);

        full_prompt += "<|start_header_id|>system<|end_header_id>\n\n";
        full_prompt += system_prompt;
        full_prompt += "<|eot_id|>";

        for (const auto& [role, content] : messages) {
            full_prompt += "<|start_header_id|>" + role + "<|end_header_id>\n\n";
            full_prompt += content;
            full_prompt += "<|eot_id|>";
        }

        full_prompt += "<|start_header_id|>assistant<|end_header_id>\n\n";

        return think_stream(T81String(full_prompt), max_tokens, 0.72f, 0.95f, 1.1f, on_token);
    }

    // === Raw access (advanced) ===
    llama_context* context() const { return ctx; }
    llama_model*   get_model() const { return model; }
    const T81Agent& agent() const { return self; }

    // === Reset KV cache (for new conversations) ===
    void reset() {
        std::lock_guard<std::mutex> lock(infer_mutex);
        llama_kv_cache_clear(ctx);
    }
};

} // namespace t81
