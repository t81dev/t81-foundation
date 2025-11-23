#pragma once

#include <vector>
#include <string>
#include <optional>
#include <cstdint>
#include <iostream>

namespace axion {

/// Simple ternary unit (placeholder for your real T81/T729 unit type).
struct T81Unit {
    int value = 0; // -1, 0, +1 or a packed ternary value
};

/// AxionKernel: user-space C++ analogue of the old axion-ai.cweb kernel module.
/// - Manages a ternary stack
/// - Supports snapshot / rollback
/// - Exposes a simple NLP-ish command parser
class AxionKernel {
public:
    AxionKernel() = default;

    // --- Stack operations ----------------------------------------------------

    void push(const T81Unit& unit) {
        stack_.push_back(unit);
    }

    std::optional<T81Unit> pop() {
        if (stack_.empty()) {
            return std::nullopt;
        }
        T81Unit top = stack_.back();
        stack_.pop_back();
        return top;
    }

    std::size_t depth() const noexcept {
        return stack_.size();
    }

    // --- Execution / optimization -------------------------------------------

    // Placeholder for your TBIN execution engine (was axion_tbin_execute()).
    void execute_tbin() {
        if (stack_.size() < 2) {
            std::cout << "[Axion] TBIN execute: insufficient stack depth (" 
                      << stack_.size() << ")\n";
            return;
        }

        // Minimal demo: fold the stack into a single value via ternary sum.
        int acc = 0;
        for (const auto& u : stack_) {
            acc += u.value; // you’ll likely replace this with ternary-op logic
        }

        std::cout << "[Axion] TBIN execute: folded stack → " << acc << "\n";
        stack_.clear();
        stack_.push_back(T81Unit{acc});
    }

    // --- Snapshot / rollback -------------------------------------------------

    void take_snapshot() {
        snapshot_ = stack_;
        snapshot_valid_ = true;
        std::cout << "[Axion] snapshot taken at depth " << stack_.size() << "\n";
    }

    bool rollback() {
        if (!snapshot_valid_) {
            std::cout << "[Axion] rollback requested but no snapshot available\n";
            return false;
        }
        stack_ = snapshot_;
        std::cout << "[Axion] rollback complete; depth = " << stack_.size()
                  << "\n";
        return true;
    }

    // --- NLP-style command parser -------------------------------------------

    /// Rough C++ analogue of axion_parse_command(const char* cmd) from C code.
    /// Recognizes:
    ///   - "optimize" → run TBIN execution
    ///   - "rollback" → rollback to snapshot
    ///   - "snapshot" → take snapshot
    void parse_command(const std::string& cmd) {
        if (contains(cmd, "optimize")) {
            std::cout << "[Axion] NLP: optimizing stack\n";
            execute_tbin();
        } else if (contains(cmd, "rollback")) {
            std::cout << "[Axion] NLP: rollback requested\n";
            rollback();
        } else if (contains(cmd, "snapshot")) {
            std::cout << "[Axion] NLP: taking snapshot\n";
            take_snapshot();
        } else {
            std::cout << "[Axion] NLP: unrecognized command: \"" << cmd << "\"\n";
        }
    }

private:
    static bool contains(const std::string& haystack,
                         const std::string& needle) {
        return haystack.find(needle) != std::string::npos;
    }

    std::vector<T81Unit> stack_;
    std::vector<T81Unit> snapshot_;
    bool snapshot_valid_ = false;
};

} // namespace axion

