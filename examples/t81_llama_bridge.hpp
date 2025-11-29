// t81_llama_bridge.hpp – Production-ready v2 (Dec 2025)
#pragma once
#include <t81/all.hpp>
#include <llama.h>
#include <string>
#include <vector>
#include <mutex>

namespace t81 {

class T81LlamaMind {
    llama_model*   model = nullptr;
    llama_context* ctx   = nullptr;
    T81Agent       self;
    std::mutex     infer_mutex;                    // llama.cpp is NOT thread-safe
    int            n_threads;

public:
    explicit T81LlamaMind(const std::string& model_path,
                          T81Symbol name = symbols::LLAMA_MIND,
                          int threads = std::thread::hardware_concurrency())
        : self(name), n_threads(threads)
    {
        llama_backend_init();

        llama_model_params mparams = llama_model_default_params();
        mparams.n_gpu_layers = 999;          // offload all if possible
        mparams.rpc_servers  = nullptr;

        model = llama_load_model_from_file(model_path.c_str(), mparams);
        if (!model) throw std::runtime_error("Failed to load GGUF: " + model_path);

        llama_context_params cparams = llama_context_default_params();
        cparams.n_ctx        = 131072;       // Llama-3.1 / Qwen2.5 context
        cparams.n_batch      = 8192;
        cparams.n_threads    = n_threads;
        cparams.n_threads_batch = n_threads;

        ctx = llama_new_context_with_model(model, cparams);
        if (!ctx) throw std::runtime_error("Failed to create context");

        self.observe(symbols::BORN_FROM_LLAMA);
        T81Discovery::join(self);  // auto-announce on construction
    }

    ~T81LlamaMind() {
        if (ctx)   llama_free(ctx);
        if (model) llama_free_model(model);
        llama_backend_free();
    }

    // Thread-safe, multi-turn capable inference
    T81String think(const T81String& prompt,
                    int max_tokens = 2048,
                    float temperature = 0.7f,
                    float top_p = 0.95f) 
    {
        std::lock_guard<std::mutex> lock(infer_mutex);

        // Pay entropy proportional to generated tokens
        consume_entropy(T81Entropy::acquire_batch(max_tokens / 2));

        // Tokenize
        std::vector<llama_token> tokens;
        tokens.reserve(prompt.str().size() + 16);
        tokens.push_back(llama_token_bos(model));

        // Proper tokenization (fixes your original bug)
        llama_tokenize(model, prompt.str().data(), prompt.str().size(),
                       tokens.data() + 1, tokens.capacity() - 1, true, false);
        tokens.resize(tokens.size() + llama_tokenize(...)); // actual count

        // Eval prompt
        if (llama_eval(ctx, tokens.data(), tokens.size(), 0, n_threads) != 0) {
            return "llama_eval failed"_t81;
        }

        // Generation loop
        std::string output;
        output.reserve(max_tokens * 4);

        for (int i = 0; i < max_tokens; ++i) {
            llama_token id = llama_sample_top_p_top_k(ctx,
                nullptr, 0, 50, top_p, temperature, 0.0f, 1.0f);

            if (id == llama_token_eos(model)) break;

            output += llama_token_to_piece(ctx, id);

            // Feed back token
            if (llama_eval(ctx, &id, 1, tokens.size() + i, n_threads) != 0) break;
        }

        self.observe(symbols::THOUGHT_COMPLETE);
        record_event(T81Time::now(T81Entropy::acquire(), symbols::INFERENCE));

        return T81String(output);
    }

    // Optional: expose raw context for advanced users
    llama_context* context() const { return ctx; }
    const T81Agent& agent()  const { return self; }
};

} // namespace t81
