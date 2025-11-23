#pragma once

#include <cstdint>
#include <cstdio>

namespace axion {

class SignalBus {
public:
    static void signal(std::uint8_t code) {
        last_signal_ = code;
        if (verbosity_ > 1) {
            std::printf("[Axion] SIGNAL: code %u → τ[%d]\n",
                        static_cast<unsigned>(code),
                        AXION_REGISTER_INDEX);
        }
    }

    static int get_optimization() {
        if (verbosity_ > 1) {
            std::printf("[Axion] GET: last_signal = %d\n", last_signal_);
        }
        return last_signal_;
    }

    static void set_verbosity(int v) {
        verbosity_ = v;
    }

private:
    static inline int last_signal_ = 0;
    static inline int verbosity_ = 0;

    // Mirror the old AXION_REGISTER_INDEX constant.
    static constexpr int AXION_REGISTER_INDEX = 27; // τ27 in your comments
};

} // namespace axion

extern "C" {

// Legacy C-style interface if you want to keep existing callsites.
void axion_signal(std::uint8_t signal_code) {
    axion::SignalBus::signal(signal_code);
}

int axion_get_optimization(void) {
    return axion::SignalBus::get_optimization();
}

} // extern "C"

