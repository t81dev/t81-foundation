// t81_llama_bridge.hpp – Production-Ready v2.3 (December 2025)
// Now with true multi-turn chat sessions (stateful, fast, correct)
#pragma once

#include <t81/all.hpp>
#include <llama.h>
#include <string>
#include <vector>
#include <mutex>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

namespace t81 {

class T81LlamaMind;

class T81ChatSession {
    friend class T81LlamaMind;

    T81LlamaMind*                mind;
    std::vector<llama_chat_message> history;
    struct OwnedMessage {
        std::string role;
        std::string content;
        OwnedMessage(std::string r, std::string c)
            : role(std::move(r)), content(std::move(c)) {}
    };
    std::vector<std::unique_ptr<OwnedMessage>> owned_history;
    std::string                  system_prompt;
    uint64_t                     session_id;
    T81Symbol                    tag;

    T81ChatSession(T81LlamaMind* m, std::string sys, uint64_t id)
        : mind(m), system_prompt(std::move(sys)), session_id(id), tag(symbols::CHAT_SESSION) {}
    
    void push_history(std::string role, std::string content) {
        auto owned = std::make_unique<OwnedMessage>(std::move(role), std::move(content));
        history.push_back({owned->role.c_str(), owned->content.c_str()});
        owned_history.push_back(std::move(owned));
    }

public:
    // Add a user message and get assistant reply (streaming)
    T81String say(const std::string& user_message,
                  int max_tokens = 2048,
                  float temperature = 0.72f,
                  const std::vector<std::string>& extra_stop = {},
                  T81LlamaMind::StreamCallback on_token = nullptr);

    // Continue assistant response (e.g. after tool call)
    T81String continue_response(int max_tokens = 1024,
                                T81LlamaMind::StreamCallback on_token = nullptr);

    // Reset this conversation (start fresh)
    void clear() {
        history.clear();
        owned_history.clear();
        mind->reset();  // clears KV cache
    }

    // Read-only access
    const std::vector<llama_chat_message>& messages() const { return history; }
    uint64_t id() const { return session_id; }
};

class T81LlamaMind {
    // === unchanged core ===
    llama_model*                     model = nullptr;
    llama_context*                   ctx   = nullptr;
    llama_lora_adapter_container*    lora_container = nullptr;
    T81Agent                         self;
    std::mutex                       infer_mutex;
    int                              n_threads;

    // Session tracking (optional, but recommended in production)
    uint64_t                         next_session_id = 1;
    std::unordered_map<uint64_t, std::weak_ptr<T81ChatSession>> sessions;

public:
    using StreamCallback = std::function<void(const std::string& token)>;

    // === Constructor unchanged (v2.2 version) ===
    explicit T81LlamaMind(
        const std::string& model_path,
        const std::vector<std::string>& lora_paths = {},
        const std::vector<float>& lora_scales = {},
        T81Symbol name = symbols::LLAMA_MIND,
        int threads = std::thread::hardware_concurrency(),
        int n_ctx = 131072,
        int n_batch = 8192,
        int n_gpu_layers = 999,
        bool use_mlock = true);

    ~T81LlamaMind();

    // === Create a new persistent chat session ===
    std::shared_ptr<T81ChatSession> new_chat(const std::string& system_prompt = "You are a helpful, concise AI assistant.") {
        auto session = std::shared_ptr<T81ChatSession>(
            new T81ChatSession(this, system_prompt, next_session_id++),
            [this](T81ChatSession* s) {
                std::lock_guard<std::mutex> lock(infer_mutex);
                sessions.erase(s->session_id);
                delete s;
            });
        sessions[session->session_id] = session;
        return session;
    }

    // === Low-level raw inference (same as v2.2) ===
    T81String think_stream(
        const std::string& prompt,
        int max_tokens = 2048,
        float temperature = 0.72f,
        float top_p = 0.95f,
        float repeat_penalty = 1.1f,
        const std::vector<std::string>& stop_strings = {},
        StreamCallback on_token = nullptr);

    T81String think(const std::string& prompt, int max_tokens = 2048) {
        return think_stream(prompt, max_tokens);
    }

    // === Structured output ===
    T81String think_grammar(const std::string& prompt,
                            const std::string& grammar,
                            int max_tokens = 2048,
                            StreamCallback on_token = nullptr);

    // === Internal: called by T81ChatSession ===
    T81String generate_from_history(
        const std::vector<llama_chat_message>& full_history,
        int max_tokens,
        float temperature,
        const std::vector<std::string>& stop_strings,
        StreamCallback on_token);

    void reset() {
        std::lock_guard<std::mutex> lock(infer_mutex);
        llama_kv_cache_clear(ctx);
    }

    // Raw access
    llama_context* context() const { return ctx; }
    llama_model*   get_model() const { return model; }
    const T81Agent& agent() const { return self; }
};

// ===================================================================
// T81ChatSession implementation (inline for header-only)
// ===================================================================

inline T81String T81ChatSession::say(
    const std::string& user_message,
    int max_tokens,
    float temperature,
    const std::vector<std::string>& extra_stop,
    T81LlamaMind::StreamCallback on_token)
{
    // Append user message
    push_history("user", user_message);

    // Build full message list including system prompt
    std::vector<llama_chat_message> full_msgs;
    if (!system_prompt.empty()) {
        full_msgs.push_back({"system", system_prompt.c_str()});
    }
    for (const auto& msg : history) {
        full_msgs.push_back(msg);
    }

    // Default stop sequences for modern models
    std::vector<std::string> stops = {
        "<|eot_id|>", "<|im_end|>", "<|end_of_text|>", "<|assistant|>"
    };
    stops.insert(stops.end(), extra_stop.begin(), extra_stop.end());

    // Generate assistant response
    T81String response = mind->generate_from_history(
        full_msgs, max_tokens, temperature, stops, on_token);

    // Append assistant reply to history
    push_history("assistant", std::string(response));

    return response;
}

inline T81String T81ChatSession::continue_response(
    int max_tokens,
    T81LlamaMind::StreamCallback on_token)
{
    if (history.empty() || std::string(history.back().role) != "assistant") {
        return "Nothing to continue"_t81;
    }

    std::vector<llama_chat_message> full_msgs;
    if (!system_prompt.empty()) {
        full_msgs.push_back({"system", system_prompt.c_str()});
    }
    full_msgs.insert(full_msgs.end(), history.begin(), history.end() - 1); // exclude partial assistant
    full_msgs.push_back({"assistant", history.back().content}); // partial

    std::vector<std::string> stops = {"<|eot_id|>", "<|im_end|>", "<|end_of_text|>"};

    T81String more = mind->generate_from_history(
        full_msgs, max_tokens, 0.72f, stops, on_token);

    // Append to last assistant message
    auto& latest = *owned_history.back();
    latest.content += std::string(more);
    history.back().content = latest.content.c_str();

    return more;
}

// ===================================================================
// Core generation used by sessions
// ===================================================================

inline T81String T81LlamaMind::generate_from_history(
    const std::vector<llama_chat_message>& full_history,
    int max_tokens,
    float temperature,
    const std::vector<std::string>& stop_strings,
    StreamCallback on_token)
{
    std::lock_guard<std::mutex> lock(infer_mutex);
    consume_entropy(T81Entropy::acquire_batch(max_tokens));

    char buffer[128*1024];
    int len = llama_chat_apply_template(
        model, nullptr, full_history.data(), (int)full_history.size(),
        true, buffer, sizeof(buffer));

    if (len <= 0 || len >= (int)sizeof(buffer)) {
        return "Chat template failed"_t81;
    }

    return think_stream(std::string(buffer, len),
                       max_tokens, temperature, 0.95f, 1.1f,
                       stop_strings, on_token);
}

} // namespace t81
