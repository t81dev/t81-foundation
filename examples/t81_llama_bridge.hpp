x// t81_llama_bridge.hpp
#pragma once
#include <t81/all.hpp>
#include <llama.h>
#include <string>
#include <vector>

class T81LlamaMind {
    llama_model*    model{};
    llama_context*  ctx{};
    T81Agent        self;
    int             n_threads = std::thread::hardware_concurrency();

public:
    explicit T81LlamaMind(const std::string& model_path, T81Symbol name = symbols::LLAMA_MIND)
        : self(name)
    {
        llama_backend_init();
        llama_model_params mparams = llama_model_default_params();
        mparams.n_gpu_layers = 999;  // offload everything if CUDA/Metal

        model = llama_load_model_from_file(model_path.c_str(), mparams);
        if (!model) throw std::runtime_error("Failed to load model");

        llama_context_params cparams = llama_context_default_params();
        cparams.n_ctx      = 131072;   // Llama-3.1 context
        cparams.n_threads  = n_threads;
        cparams.n_batch    = 8192;

        ctx = llama_new_context_with_model(model, cparams);
        self.observe(symbols::BORN_FROM_LLAMA);
    }

    ~T81LlamaMind() {
        llama_free(ctx);
        llama_free_model(model);
        llama_backend_free();
    }

    T81String think(const T81String& prompt, int max_tokens = 2048) {
        consume_entropy(T81Entropy::acquire_batch(max_tokens / 4)); // rough cost

        std::vector<llama_token> tokens;
        tokens.push_back(llama_token_bos(model));
        for (char c : prompt.str()) tokens.push_back(llama_token_nl(model) + c);

        llama_eval(ctx, tokens.data(), tokens.size(), 0, n_threads);
        llama_kv_cache_clear(ctx);

        std::string output;
        for (int i = 0; i < max_tokens; ++i) {
            llama_token id = llama_sample_top_p_top_k(ctx, nullptr, 0, 50, 0.95f, 0.1f);
            if (id == llama_token_eos(model)) break;
            output += llama_token_to_piece(ctx, id);
        }

        self.observe(symbols::THOUGHT_COMPLETE);
        record_event(T81Time::now(T81Entropy::acquire(), symbols::INFERENCE));
        return T81String(output);
    }

    void join_civilization() {
        // Auto-join T81Discovery network
        T81Discovery::join(self);
    }
};
