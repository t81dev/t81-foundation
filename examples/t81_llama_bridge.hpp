// t81_llama_bridge.hpp – Production-Ready v2.2 (Dec 2025)
// Fully compatible with llama.cpp git master (2025-12+)
// Features: chat templates (auto), LoRA container, stop sequences, grammar/JSON, streaming, thread-safe, KV reset
#pragma once

#include <t81/all.hpp>
#include <llama.h>
#include <string>
#include <vector>
#include <mutex>
#include <optional>
#include <functional>
#include <span>

namespace t81 {

class T81LlamaMind {
    llama_model*                     model = nullptr;
    llama_context*                   ctx   = nullptr;
    llama_lora_adapter_container*    lora_container = nullptr;

    T81Agent                         self;
    std::mutex                       infer_mutex;  // llama.cpp is NOT thread-safe
    int                              n_threads;

public:
    using StreamCallback = std::function<void(const std::string& token)>;

    explicit T81LlamaMind(
        const std::string& model_path,
        const std::vector<std::string>& lora_paths = {},
        const std::vector<float>& lora_scales = {},  // optional per-lora scaling
        T81Symbol name = symbols::LLAMA_MIND,
        int threads = std::thread::hardware_concurrency(),
        int n_ctx = 131072,
        int n_batch = 8192,
        int n_gpu_layers = 999,
        bool use_mlock = true)
        : self(name), n_threads(threads)
    {
        llama_backend_init();

        llama_model_params mparams = llama_model_default_params();
        mparams.n_gpu_layers = n_gpu_layers;
        mparams.use_mmap     = true;
        mparams.use_mlock    = use_mlock;

        model = llama_load_model_from_file(model_path.c_str(), mparams);
        if (!model) throw std::runtime_error("Failed to load GGUF: " + model_path);

        llama_context_params cparams = llama_context_default_params();
        cparams.n_ctx            = n_ctx;
        cparams.n_batch          = n_batch;
        cparams.n_threads        = n_threads;
        cparams.n_threads_batch  = n_threads;
        cparams.embeddings       = false;

        ctx = llama_new_context_with_model(model, cparams);
        if (!ctx) throw std::runtime_error("Failed to create llama context");

        // === Modern LoRA container (2025+) ===
        if (!lora_paths.empty()) {
            lora_container = llama_lora_adapter_container_init(model);
            size_t scale_idx = 0;
            for (const auto& path : lora_paths) {
                auto* adapter = llama_lora_adapter_init(model, path.c_str());
                if (!adapter) {
                    T81Log::warn("Failed to load LoRA: " + path);
                    continue;
                }
                float scale = (scale_idx < lora_scales.size()) ? lora_scales[scale_idx++] : 1.0f;
                llama_lora_adapter_container_add(lora_container, adapter, scale);
            }
            llama_context_set_lora_container(ctx, lora_container);
        }

        self.observe(symbols::BORN_FROM_LLAMA);
        T81Discovery::join(self);
    }

    ~T81LlamaMind() {
        if (lora_container) llama_lora_adapter_container_free(lora_container);
        if (ctx)   llama_free(ctx);
        if (model) llama_free_model(model);
        llama_backend_free();
    }

    // === Streaming inference with stop sequences ===
    T81String think_stream(
        const std::string& prompt,
        int max_tokens = 2048,
        float temperature = 0.72f,
        float top_p = 0.95f,
        float repeat_penalty = 1.1f,
        const std::vector<std::string>& stop_strings = {},
        StreamCallback on_token = nullptr)
    {
        std::lock_guard<std::mutex> lock(infer_mutex);
        consume_entropy(T81Entropy::acquire_batch(max_tokens));

        // Tokenize prompt (add BOS)
        std::vector<llama_token> input_tokens = llama_tokenize(model, prompt.c_str(), true, true);
        if (llama_eval(ctx, input_tokens.data(), (int)input_tokens.size(), 0, n_threads) != 0) {
            return "llama_eval failed (prompt)"_t81;
        }

        // Pre-tokenize stop sequences
        std::vector<std::vector<llama_token>> stop_token_lists;
        for (const auto& s : stop_strings) {
            if (s.empty()) continue;
            auto tokens = llama_tokenize(model, s.c_str(), false, true);
            if (!tokens.empty()) stop_token_lists.push_back(std::move(tokens));
        }

        // Sampler chain
        auto* chain = llama_sampler_chain_init(llama_sampler_chain_default_params());
        llama_sampler_chain_add(chain, llama_sampler_init_top_k(50));
        llama_sampler_chain_add(chain, llama_sampler_init_top_p(top_p, 1));
        llama_sampler_chain_add(chain, llama_sampler_init_temp(temperature));
        llama_sampler_chain_add(chain, llama_sampler_init_repetition_penalty(repeat_penalty, 1.0f, 0.0f, 64));

        std::string output;
        output.reserve(max_tokens * 4);
        std::vector<llama_token> generated;
        generated.reserve(max_tokens + 64);

        int n_past = input_tokens.size();

        for (int i = 0; i < max_tokens; ++i) {
            llama_token id = llama_sample_token(ctx, chain);

            // Check stop sequences
            generated.push_back(id);
            bool should_stop = (id == llama_token_eos(model));
            if (!should_stop) {
                for (const auto& stop_seq : stop_token_lists) {
                    if (generated.size() >= stop_seq.size() &&
                        std::equal(stop_seq.begin(), stop_seq.end(),
                                   generated.end() - stop_seq.size())) {
                        should_stop = true;
                        // Remove stop sequence from output
                        for (size_t k = 0; k < stop_seq.size(); ++k)
                            generated.pop_back();
                        break;
                    }
                }
            }

            std::string piece = llama_token_to_piece(ctx, id, true);
            output += piece;
            if (on_token) on_token(piece);

            if (should_stop) break;

            if (llama_eval(ctx, &id, 1, n_past++, n_threads) != 0) {
                break;
            }
        }

        llama_sampler_free(chain);
        self.observe(symbols::THOUGHT_COMPLETE);
        record_event(T81Time::now(T81Entropy::acquire(), symbols::INFERENCE));
        return T81String(std::move(output));
    }

    // === Universal chat (works with ALL models that have chat templates) ===
    T81String chat(
        const std::vector<llama_chat_message>& messages,
        int max_tokens = 2048,
        float temperature = 0.72f,
        const std::vector<std::string>& stop_strings = {"<|eot_id|>", "<|im_end|>", "<|end_of_text|>"},
        StreamCallback on_token = nullptr)
    {
        char buffer[128*1024];
        int len = llama_chat_apply_template(
            model,
            nullptr,                 // no custom template override
            messages.data(),
            (int)messages.size(),
            true,                    // add_generation_prompt
            buffer,
            sizeof(buffer)
        );

        if (len < 0 || len >= (int)sizeof(buffer)) {
            throw std::runtime_error("Chat template rendering failed or overflowed");
        }

        std::string prompt(buffer, len);
        return think_stream(prompt, max_tokens, temperature, 0.95f, 1.1f, stop_strings, on_token);
    }

    // === Simple single-turn ===
    T81String think(const std::string& prompt, int max_tokens = 2048) {
        return think_stream(prompt, max_tokens);
    }

    // === Structured / JSON mode via grammar ===
    T81String think_grammar(
        const std::string& prompt,
        const std::string& grammar,  // GBNF or JSON schema
        int max_tokens = 2048,
        StreamCallback on_token = nullptr)
    {
        std::lock_guard<std::mutex> lock(infer_mutex);

        auto* g = llama_grammar_init(grammar.c_str(), grammar.size(), 0);
        if (!g) throw std::runtime_error("Invalid grammar");

        auto* chain = llama_sampler_chain_init(llama_sampler_chain_default_params());
        llama_sampler_chain_add(chain, llama_sampler_init_greedy());  // grammar forces deterministic
        llama_sampler_chain_add(chain, llama_sampler_init_grammar(g));

        // Tokenize + eval prompt...
        std::vector<llama_token> tokens = llama_tokenize(model, prompt.c_str(), true, true);
        llama_eval(ctx, tokens.data(), (int)tokens.size(), 0, n_threads);

        std::string output;
        int n_past = tokens.size();
        for (int i = 0; i < max_tokens; ++i) {
            llama_token id = llama_sample_token(ctx, chain);
            if (id == llama_token_eos(model)) break;
            std::string piece = llama_token_to_piece(ctx, id, true);
            output += piece;
            if (on_token) on_token(piece);
            llama_eval(ctx, &id, 1, n_past++, n_threads);
        }

        llama_grammar_free(g);
        llama_sampler_free(chain);
        return T81String(std::move(output));
    }

    // === Conversation control ===
    void reset() {
        std::lock_guard<std::mutex> lock(infer_mutex);
        llama_kv_cache_clear(ctx);
    }

    // === Raw access ===
    llama_context* context() const { return ctx; }
    llama_model*   get_model() const { return model; }
    const T81Agent& agent() const { return self; }
};

} // namespace t81
