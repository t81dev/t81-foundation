#include <benchmark/benchmark.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <map>
#include <mutex>
#include <array>
#include <cstdio>
#include <limits>
#include <sstream>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <cstring>

namespace {

enum class BenchmarkProfile {
    Smoke,
    Full,
    Deep,
};

constexpr const char* kDefaultSmokeFilter =
    "BM_(ArithThroughput|NegationSpeed|RoundtripAccuracy|overflow|PackingDensity|"
    "MemoryBandwidth|Add_1024_bit|Add_2048_bit|T81LangCompile|LimbArithThroughput|"
    "LimbAdd_T81Native|LimbAdd_T81Limb|LimbAdd_Int128|vs_).*";
constexpr const char* kDefaultFullFilter =
    "BM_(ArithThroughput|NegationSpeed|RoundtripAccuracy|overflow|PackingDensity|"
    "MemoryBandwidth|Add_.*|T81LangCompile|LimbArithThroughput|LimbAdd_.*|vs_.*|"
    "VMSimulation_.*|NativeCall_.*|TensorPromotion.*|Lexer_.*|Base81_.*|Overhead_.*)";
constexpr const char* kDefaultSmokeMinTime = "0.02s";
constexpr const char* kDefaultFullMinTime = "0.01s";

bool HasBenchmarkArg(int argc, char** argv, std::string_view flag_prefix, std::string_view exact_flag) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if (arg == exact_flag || arg.rfind(flag_prefix, 0) == 0) {
            return true;
        }
    }
    return false;
}

BenchmarkProfile ResolveBenchmarkProfile() {
    const char* profile = std::getenv("T81_BENCHMARK_PROFILE");
    if (profile == nullptr) return BenchmarkProfile::Smoke;
    std::string lowered;
    for (const unsigned char c : std::string_view(profile)) {
        lowered.push_back(static_cast<char>(std::tolower(c)));
    }
    if (lowered == "deep" || lowered == "all" || lowered == "exhaustive") {
        return BenchmarkProfile::Deep;
    }
    if (lowered == "full") {
        return BenchmarkProfile::Full;
    }
    return BenchmarkProfile::Smoke;
}

std::vector<std::string> BuildEffectiveBenchmarkArgs(int argc, char** argv) {
    std::vector<std::string> effective_args;
    effective_args.reserve(static_cast<std::size_t>(argc) + 3U);
    for (int i = 0; i < argc; ++i) {
        effective_args.emplace_back(argv[i]);
    }

    const BenchmarkProfile profile = ResolveBenchmarkProfile();
    if (profile == BenchmarkProfile::Deep) {
        return effective_args;
    }

    const bool has_filter =
        HasBenchmarkArg(argc, argv, "--benchmark_filter=", "--benchmark_filter");
    const bool has_min_time =
        HasBenchmarkArg(argc, argv, "--benchmark_min_time=", "--benchmark_min_time");

    if (!has_filter) {
        const char* filter =
            profile == BenchmarkProfile::Full ? kDefaultFullFilter : kDefaultSmokeFilter;
        effective_args.emplace_back(std::string("--benchmark_filter=") + filter);
    }
    if (!has_min_time) {
        const char* min_time =
            profile == BenchmarkProfile::Full ? kDefaultFullMinTime : kDefaultSmokeMinTime;
        effective_args.emplace_back(std::string("--benchmark_min_time=") + min_time);
    }
    return effective_args;
}

}  // namespace

struct BenchmarkResult {
    std::string name;
    std::string t81_result_str;
    std::string t81_native_result_str;
    std::string binary_result_str;
    double t81_result_val = 0.0;
    double t81_native_result_val = 0.0;
    double binary_result_val = 0.0;
    double t81_latency_seconds = 0.0;
    double t81_native_latency_seconds = 0.0;
    double binary_latency_seconds = 0.0;
    std::string bandwidth_result_str;
    double t81_bandwidth_val = 0.0;
    double t81_native_bandwidth_val = 0.0;
    double binary_bandwidth_val = 0.0;
    std::string t81_classic_advantage;
    std::string t81_native_advantage;
    std::string t81_classic_note;
    std::string t81_native_note;
    std::string binary_note;
    std::string t81_latency_str;
    std::string t81_native_latency_str;
    std::string binary_latency_str;
    std::string analysis;
    bool t81_classic_skipped = false;
    bool t81_native_skipped = false;
    bool binary_skipped = false;
    std::string t81_classic_skip_reason;
    std::string t81_native_skip_reason;
    std::string binary_skip_reason;
    bool has_t81_flow = false;
    bool has_t81_native_flow = false;
    bool has_binary_flow = false;
    long long t81_arg_rank = std::numeric_limits<long long>::min();
    long long t81_native_arg_rank = std::numeric_limits<long long>::min();
    long long binary_arg_rank = std::numeric_limits<long long>::min();
    bool throughput_ratio_computed = false;
    std::string throughput_ratio_str;
    double throughput_ratio_val = 0.0;
    bool latency_speedup_computed = false;
    std::string latency_speedup_str;
    double latency_speedup_val = 0.0;
    bool native_throughput_ratio_computed = false;
    std::string native_throughput_ratio_str;
    double native_throughput_ratio_val = 0.0;
    bool native_latency_speedup_computed = false;
    std::string native_latency_speedup_str;
    double native_latency_speedup_val = 0.0;
    bool t81_classic_inconsistent = false;
    bool t81_native_inconsistent = false;
    bool binary_inconsistent = false;
    double t81_work_per_iter = 0.0;
    double t81_native_work_per_iter = 0.0;
    double binary_work_per_iter = 0.0;
    bool t81_work_defined = false;
    bool t81_native_work_defined = false;
    bool binary_work_defined = false;
};

std::map<std::string, BenchmarkResult> final_results;
std::mutex final_results_mutex;

const std::map<std::string, std::pair<std::string, std::string>> T81_ADVANTAGES = {
    {"BM_T81LangCompile", {"Deterministic frontend compile", {}}},
    {"BM_ArithThroughput", {"Exact rounding, no FP error", {}}},
    {"BM_NegationSpeed", {"Free negation (no borrow)", {}}},
    {"BM_RoundtripAccuracy", {"No sign-bit tax", {}}},
    {"BM_OverflowDetection", {"Deterministic, provable", {}}},
    {"BM_PackingDensity_Theoretical", {"Theoretical maximum", {}}},
    {"BM_PackingDensity_Achieved", {"Achieved bits/trit", {}}},
    {"BM_PackingDensity_Practical", {"Practical size ratio", {}}},
    {"BM_LimbArithThroughput", {"48-trit Kogge-Stone addition", {}}},
    {"BM_NativeWeightsLoad", {{}, "Governed ternary-native weights load path"}},
    {"BM_NativeWeightsPromote", {{}, "Governed ternary-native tensor materialization path"}},
    {"BM_NativeWeightsLoadAndExp", {{}, "Governed ternary-native exponentiation path"}},
    {"BM_NativeWeightsLoadAndTQUANT", {{}, "Governed ternary-native quantization path"}},
    {"BM_NativeWeightsLoadAndTACT", {{}, "Governed ternary-native activation path"}},
    {"BM_NativeWeightsLoadAndTERNACCUM", {{}, "Governed ternary-native dot-product path"}},
    {"BM_NativeWeightsLoadAndSiLU", {{}, "Governed ternary-native SiLU path"}},
    {"BM_NativeWeightsLoadAndSoftmax", {{}, "Governed ternary-native softmax path"}},
    {"BM_NativeWeightsLoadAndRMSNorm", {{}, "Governed ternary-native RMSNorm path"}},
    {"BM_NativeWeightsLoadAndRoPE", {{}, "Governed ternary-native RoPE path"}},
    {"BM_NativeWeightsLoadAndTWEMBED", {{}, "Governed ternary-native embedding gather path"}},
    {"BM_NativeWeightsLoadAndTWMATMUL", {{}, "Governed ternary-native matmul path"}},
    {"BM_NativeWeightsLoadAndTATTN", {{}, "Governed ternary-native attention path"}},
    {"BM_NegationSpeed_T81Native", {{}, "PSHUFB-powered native negation"}},
    {"BM_LimbAdd_T81Native", {{}, "Register-native prefix addition"}}
};

static FILE* OpenCommandPipe(const std::string& command) {
#ifdef _WIN32
    return _popen(command.c_str(), "r");
#else
    return popen(command.c_str(), "r");
#endif
}

static int CloseCommandPipe(FILE* pipe) {
#ifdef _WIN32
    return _pclose(pipe);
#else
    return pclose(pipe);
#endif
}

std::string RunCommand(const std::string& command) {
    std::array<char, 128> buffer;
    std::string output;
    FILE* pipe = OpenCommandPipe(command);
    if (!pipe) {
        return {};
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }
    CloseCommandPipe(pipe);
    while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) {
        output.pop_back();
    }
    return output;
}

double ExtractLatency(const ::benchmark::BenchmarkReporter::Run& run) {
    static constexpr std::array<const char*, 2> kLatencyKeys = {"cpu_time", "real_time"};
    for (const auto* key : kLatencyKeys) {
        auto it = run.counters.find(key);
        if (it != run.counters.end()) {
            return it->second;
        }
    }
    double latency_seconds = 0.0;
    const double real_time = run.GetAdjustedRealTime();
    const double real_multiplier = GetTimeUnitMultiplier(run.time_unit);
    if (real_multiplier > 0.0) {
        latency_seconds = real_time / real_multiplier;
    } else {
        latency_seconds = real_time;
    }
    if (latency_seconds > 0.0) {
        return latency_seconds;
    }
    const double cpu_time = run.GetAdjustedCPUTime();
    const double cpu_multiplier = GetTimeUnitMultiplier(run.time_unit);
    if (cpu_multiplier > 0.0) {
        latency_seconds = cpu_time / cpu_multiplier;
    } else {
        latency_seconds = cpu_time;
    }
    if (latency_seconds > 0.0) {
        return latency_seconds;
    }
    return 0.0;
}

std::string FormatLatency(double seconds) {
    if (!std::isfinite(seconds) || seconds <= 0.0) {
        return "below timer resolution";
    }
    if (seconds < 1e-9) {
        return "<1 ns (below timer resolution)";
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    if (seconds < 1e-6) {
        oss << (seconds * 1e9) << " ns";
    } else if (seconds < 1e-3) {
        oss << (seconds * 1e6) << " µs";
    } else if (seconds < 1.0) {
        oss << (seconds * 1e3) << " ms";
    } else {
        oss << seconds << " s";
    }
    return oss.str();
}

std::string FormatBandwidth(double bytes_per_second) {
    if (!std::isfinite(bytes_per_second) || bytes_per_second <= 0.0) {
        return {};
    }
    struct Scale {
        double threshold;
        const char* suffix;
    };
    constexpr Scale kScales[] = {
        {1e9, "GB/s"},
        {1e6, "MB/s"},
        {1e3, "KB/s"},
    };
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    for (const auto& scale : kScales) {
        if (bytes_per_second >= scale.threshold) {
            oss << (bytes_per_second / scale.threshold) << " " << scale.suffix;
            return oss.str();
        }
    }
    oss << bytes_per_second << " B/s";
    return oss.str();
}

std::string FormatThroughput(double items_per_second) {
    if (!std::isfinite(items_per_second) || items_per_second <= 0.0) {
        return "below timer resolution";
    }
    if (items_per_second > 1e15) {
        return "below timer resolution";
    }
    if (items_per_second <= 0.0) {
        return {};
    }
    struct Scale {
        double threshold;
        const char* suffix;
    };
    constexpr Scale kScales[] = {
        {1e9, "Gops/s"},
        {1e6, "Mops/s"},
        {1e3, "Kops/s"},
    };
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    for (const auto& scale : kScales) {
        if (items_per_second >= scale.threshold) {
            oss << (items_per_second / scale.threshold) << " " << scale.suffix;
            return oss.str();
        }
    }
    oss << items_per_second << " ops/s";
    return oss.str();
}

enum class FlowKind {
    kUnknown,
    kT81Classic,
    kT81Native,
    kBinary,
};

static std::string ToLower(std::string_view input) {
    std::string result;
    result.reserve(input.size());
    for (unsigned char c : input) {
        result += static_cast<char>(std::tolower(c));
    }
    return result;
}

static bool IsBelowResolutionMarker(const std::string& s) {
    return s.find("below timer resolution") != std::string::npos;
}

static std::string FormatMetric(double value, const char* unit) {
    if (!std::isfinite(value)) return "not-applicable (metric undefined)";
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4) << value << " " << unit;
    return oss.str();
}

static long long ParseArgRank(const std::string& suffix) {
    if (suffix.empty()) return 0;
    const auto pos = suffix.find_last_of('/');
    const std::string token = (pos == std::string::npos) ? suffix : suffix.substr(pos + 1);
    if (token.empty()) return 0;
    for (char c : token) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return 0;
    }
    try {
        return std::stoll(token);
    } catch (...) {
        return 0;
    }
}

static bool HasFlowSuffix(std::string_view suffix) {
    if (suffix.empty()) return false;
    const std::string lower = ToLower(suffix);
    static constexpr std::array<std::string_view, 8> kIndicators = {
        "binary", "int", "t81", "packed", "cell", "native", "float", "ternary"
    };
    for (const auto indicator : kIndicators) {
        if (lower.find(indicator) != std::string::npos) {
            return true;
        }
    }
    return false;
}

static FlowKind DetermineFlowKind(const std::string& base_name, const std::string& suffix) {
    const std::string lower_base = ToLower(base_name);
    const std::string lower_suffix = ToLower(suffix);
    if (!suffix.empty()) {
        if (lower_suffix.find("binary") != std::string::npos ||
            lower_suffix.find("int") != std::string::npos) {
            return FlowKind::kBinary;
        }
        if (lower_suffix.find("native") != std::string::npos) {
            return FlowKind::kT81Native;
        }
        if (lower_suffix.find("t81") != std::string::npos ||
            lower_suffix.find("ternary") != std::string::npos ||
            lower_suffix.find("packed") != std::string::npos ||
            lower_suffix.find("cell") != std::string::npos) {
            return FlowKind::kT81Classic;
        }
    }
    if (lower_base.find("native") != std::string::npos) {
        return FlowKind::kT81Native;
    }
    if (lower_base.find("t81") != std::string::npos ||
        lower_base.find("ternary") != std::string::npos ||
        lower_base.find("packed") != std::string::npos) {
        return FlowKind::kT81Classic;
    }
    if (lower_base.find("int64") != std::string::npos ||
        lower_base.find("int128") != std::string::npos ||
        lower_base.find("binary") != std::string::npos) {
        return FlowKind::kBinary;
    }
    return FlowKind::kUnknown;
}

std::string BuildT81AdvantageDisplay(const BenchmarkResult& r) {
    std::string display;
    if (!r.t81_classic_advantage.empty()) {
        display = "Classic: " + r.t81_classic_advantage;
    }
    if (!r.t81_native_advantage.empty()) {
        if (!display.empty()) {
            display += " | ";
        }
        display += "Native: " + r.t81_native_advantage;
    }
    return display;
}

std::string BuildNotesDisplay(const BenchmarkResult& r) {
    std::ostringstream oss;
    bool first = true;
    if (!r.t81_classic_note.empty()) {
        oss << "Classic: " << r.t81_classic_note;
        first = false;
    }
    if (!r.t81_native_note.empty()) {
        if (!first) oss << " | ";
        oss << "Native: " << r.t81_native_note;
        first = false;
    }
    if (!r.binary_note.empty()) {
        if (!first) oss << " | ";
        oss << "Binary: " << r.binary_note;
    }
    return oss.str();
}

static bool BinaryBaselineSemanticallyUnavoidable(const BenchmarkResult& r);
static bool T81ClassicSemanticallyUnavoidable(const BenchmarkResult& r);
static bool IsNativeOnlyFamily(const BenchmarkResult& r) {
    return (r.has_t81_native_flow || !r.t81_native_result_str.empty() || !r.t81_native_note.empty()) &&
           !(r.has_t81_flow || !r.t81_result_str.empty() || !r.t81_classic_note.empty());
}
static bool IsBinaryOnlyFamily(const BenchmarkResult& r) {
    return (r.has_binary_flow || !r.binary_result_str.empty() || !r.binary_note.empty()) &&
           !(r.has_t81_flow || !r.t81_result_str.empty() || !r.t81_classic_note.empty()) &&
           !(r.has_t81_native_flow || !r.t81_native_result_str.empty() || !r.t81_native_note.empty());
}
static std::string FormatRatio(double value) {
    std::ostringstream oss;
    if (!std::isfinite(value) || value <= 0.0) return "below timer resolution";
    if (value < 1.0) {
        oss << std::scientific << std::setprecision(2) << value << "x";
        return oss.str();
    }
    oss << std::fixed << std::setprecision(2) << value << "x";
    return oss.str();
}

std::string BuildAnalysis(const BenchmarkResult& r) {
    std::ostringstream oss;
    if ((r.t81_classic_inconsistent || r.binary_inconsistent || r.t81_native_inconsistent) &&
        !(IsNativeOnlyFamily(r) && !r.has_binary_flow) &&
        !(IsBinaryOnlyFamily(r) && !(r.has_t81_flow || r.has_t81_native_flow))) {
        oss << "DEFECT: inconsistent work definition";
        return oss.str();
    }
    if (!r.throughput_ratio_computed) {
        if (BinaryBaselineSemanticallyUnavoidable(r)) {
            oss << "Throughput ratio not computed (semantic: no binary baseline)";
        } else if (T81ClassicSemanticallyUnavoidable(r) &&
                   !(r.has_t81_flow || r.has_t81_native_flow)) {
            oss << "Throughput ratio not computed (semantic)";
        } else if (IsBelowResolutionMarker(r.t81_result_str) || IsBelowResolutionMarker(r.binary_result_str)) {
            oss << "Throughput ratio below timer resolution";
        } else if ((r.t81_classic_skipped || r.binary_skipped)) {
            oss << "Throughput ratio skipped (no counters emitted)";
        } else {
            oss << "Throughput ratio not-implemented (missing baseline)";
        }
    } else {
        oss << FormatRatio(r.throughput_ratio_val) << " throughput ratio";
        const double ratio = r.throughput_ratio_val;
        if (ratio > 1.05) {
            oss << " — T81 leads";
            const auto advantage = BuildT81AdvantageDisplay(r);
            if (!advantage.empty()) {
                oss << " (" << advantage << ")";
            }
        } else if (ratio < 0.95) {
            oss << " — binary wins";
        } else {
            oss << " — throughputs comparable";
        }
    }
    if (r.latency_speedup_computed) {
        oss << "; " << FormatRatio(r.latency_speedup_val)
            << " iteration time ratio (Binary/T81 classic)";
    } else if (BinaryBaselineSemanticallyUnavoidable(r)) {
        oss << "; iteration time ratio not computed (semantic: no binary baseline)";
    } else if (T81ClassicSemanticallyUnavoidable(r) &&
               !(r.has_t81_flow || r.has_t81_native_flow)) {
        oss << "; iteration time ratio not computed (semantic)";
    } else if (IsBelowResolutionMarker(r.t81_latency_str) || IsBelowResolutionMarker(r.binary_latency_str)) {
        oss << "; iteration time ratio below timer resolution";
    }
    if (!r.throughput_ratio_computed && !r.latency_speedup_computed) {
        return oss.str();
    }
    if (!r.t81_latency_str.empty() && !r.binary_latency_str.empty()) {
        oss << "; iteration times " << r.t81_latency_str << " vs " << r.binary_latency_str;
    }
    if (r.native_throughput_ratio_computed || r.native_latency_speedup_computed) {
        if (r.native_throughput_ratio_computed) {
            oss << "; native throughput ratio "
                << FormatRatio(r.native_throughput_ratio_val)
                << " (T81 native/Binary)";
        }
        if (r.native_latency_speedup_computed) {
            oss << "; native iteration time ratio "
                << FormatRatio(r.native_latency_speedup_val)
                << " (Binary/T81 native)";
        }
    }
    return oss.str();
}

static bool NameContains(std::string_view name, std::string_view token) {
    const std::string lower_name = ToLower(name);
    const std::string lower_token = ToLower(token);
    return lower_name.find(lower_token) != std::string::npos;
}

static bool BinaryBaselineSemanticallyUnavoidable(const BenchmarkResult& r) {
    // Packing-density metrics are ternary-encoding properties, not direct throughput parity tests.
    if (NameContains(r.name, "PackingDensity")) return true;
    return false;
}

static bool T81ClassicSemanticallyUnavoidable(const BenchmarkResult& r) {
    // Binary silent-overflow semantics have no ternary equivalent in this project.
    if (NameContains(r.name, "overflow_binary_silent")) return true;
    return false;
}

static bool HasInconsistentCounters(const ::benchmark::BenchmarkReporter::Run& run, double latency_seconds) {
    if (latency_seconds <= 0.0 || !std::isfinite(latency_seconds)) return false;
    const auto items_it = run.counters.find("items_per_second");
    const auto work_it = run.counters.find("work_per_iter");
    if (items_it == run.counters.end() || work_it == run.counters.end()) return false;
    // If the benchmark published an explicit work unit, trust that annotation and
    // avoid the fallback latency-vs-throughput consistency heuristic. The
    // heuristic is only useful when we have to infer work implicitly.
    if (work_it->second > 0.0 && std::isfinite(work_it->second)) return false;
    const double items_per_second = items_it->second;
    const double work_per_iter = work_it->second;
    if (items_per_second <= 0.0 || work_per_iter <= 0.0 ||
        !std::isfinite(items_per_second) || !std::isfinite(work_per_iter)) {
        return false;
    }
    const double implied = work_per_iter / latency_seconds;
    if (!std::isfinite(implied) || implied <= 0.0) return false;
    const double rel_err = std::abs(implied - items_per_second) / items_per_second;
    return rel_err > 0.25;
}

class CustomReporter : public ::benchmark::BenchmarkReporter {
public:
    CustomReporter() {}
    bool ReportContext(const Context&) override { return true; }

    void ReportRuns(const std::vector<Run>& reports) override {
        std::lock_guard<std::mutex> guard(final_results_mutex);
        for (const auto& run : reports) {
            if (run.run_type != benchmark::BenchmarkReporter::Run::RT_Iteration) {
                continue;
            }
            std::string run_name = run.benchmark_name();
            std::string family = run_name;
            std::string suffix;
            const auto slash_pos = run_name.find('/');
            if (slash_pos != std::string::npos) {
                family = run_name.substr(0, slash_pos);
                suffix = run_name.substr(slash_pos + 1);
            } else {
                family = run_name;
            }

            // Further strip flow suffixes from family (e.g., BM_Add_T81 -> BM_Add)
            const auto last_underscore = family.find_last_of('_');
            if (last_underscore != std::string::npos) {
                std::string candidate = family.substr(last_underscore + 1);
                if (HasFlowSuffix(candidate)) {
                    if (suffix.empty()) {
                        suffix = candidate;
                    } else {
                        suffix = candidate + "/" + suffix;
                    }
                    family = family.substr(0, last_underscore);
                }
            }
            if (family.empty()) {
                family = run_name;
            }

            const FlowKind flow_kind = DetermineFlowKind(family, suffix);
            const bool is_t81_classic = flow_kind == FlowKind::kT81Classic;
            const bool is_t81_native = flow_kind == FlowKind::kT81Native;
            const bool is_binary = flow_kind == FlowKind::kBinary;
            const long long arg_rank = ParseArgRank(suffix);

            if (final_results.find(family) == final_results.end()) {
                final_results[family].name = family;
                if (auto it = T81_ADVANTAGES.find(family); it != T81_ADVANTAGES.end()) {
                    final_results[family].t81_classic_advantage = it->second.first;
                    final_results[family].t81_native_advantage = it->second.second;
                }
            }
            if (is_t81_classic) {
                final_results[family].t81_classic_note = run.report_label;
            }
            if (is_t81_native) {
                final_results[family].t81_native_note = run.report_label;
            }
            if (is_binary) {
                final_results[family].binary_note = run.report_label;
            }

            const bool skipped = run.skipped != benchmark::internal::NotSkipped;
            const std::string skip_reason = run.skip_message.empty() ?
                "no counters emitted" : run.skip_message;
            if (skipped) {
                if (is_t81_classic) {
                    final_results[family].t81_classic_skipped = true;
                    final_results[family].t81_classic_skip_reason = skip_reason;
                } else if (is_t81_native) {
                    final_results[family].t81_native_skipped = true;
                    final_results[family].t81_native_skip_reason = skip_reason;
                } else if (is_binary) {
                    final_results[family].binary_skipped = true;
                    final_results[family].binary_skip_reason = skip_reason;
                } else {
                    final_results[family].t81_classic_skipped = true;
                    final_results[family].t81_classic_skip_reason = skip_reason;
                }
                continue;
            }

            std::string summary;
            double gops = 0.0;
            bool throughput_recorded = false;
            double items_per_second = 0.0;
            auto items_it = run.counters.find("items_per_second");
            auto bandwidth_it = run.counters.find("bytes_per_second");
            if (items_it != run.counters.end()) {
                items_per_second = items_it->second;
            }
            if (items_per_second > 0.0) {
                if (items_per_second > 0.0) {
                    gops = items_per_second / 1e9;
                    throughput_recorded = true;
                }
                summary = FormatThroughput(items_per_second);
                if (summary.empty()) {
                    summary = "0 ops/s";
                }
            } else {
                std::stringstream ss;
                bool first = true;
                for (auto const& [key, val] : run.counters) {
                    if (!first) ss << ", ";
                    ss << key << ": " << std::fixed << std::setprecision(2) << val;
                    first = false;
                }
                summary = ss.str();
            }
            bool bandwidth_recorded = false;
            double bandwidth = 0.0;
            if (bandwidth_it != run.counters.end()) {
                bandwidth = bandwidth_it->second;
                if (bandwidth > 0.0) {
                    bandwidth_recorded = true;
                    summary = FormatBandwidth(bandwidth);
                }
            }

            double latency = ExtractLatency(run);
            std::string latency_str = FormatLatency(latency);
            const bool inconsistent_counters = HasInconsistentCounters(run, latency);
            const auto work_it = run.counters.find("work_per_iter");
            const double work_counter_value =
                (work_it != run.counters.end()) ? static_cast<double>(work_it->second) : 0.0;
            bool has_work = work_it != run.counters.end() &&
                            work_counter_value > 0.0 &&
                            std::isfinite(work_counter_value);
            double work_per_iter = has_work ? work_counter_value : 0.0;
            bool inferred_work = false;
            if (!has_work && items_per_second > 0.0 && latency > 0.0 &&
                std::isfinite(items_per_second) && std::isfinite(latency)) {
                const double inferred = items_per_second * latency;
                if (std::isfinite(inferred) && inferred > 0.0) {
                    has_work = true;
                    work_per_iter = inferred;
                    inferred_work = true;
                }
            }
            if (!has_work && bandwidth > 0.0 && latency > 0.0 &&
                std::isfinite(bandwidth) && std::isfinite(latency)) {
                const double inferred = bandwidth * latency;
                if (std::isfinite(inferred) && inferred > 0.0) {
                    has_work = true;
                    work_per_iter = inferred;
                    inferred_work = true;
                }
            }
            std::string effective_label = run.report_label;
            if (effective_label.empty() && has_work) {
                std::ostringstream label;
                if (inferred_work) {
                    label << "work: inferred units/iter=" << std::fixed << std::setprecision(0) << work_per_iter;
                } else {
                    label << "work: units/iter=" << std::fixed << std::setprecision(0) << work_per_iter;
                }
                effective_label = label.str();
            }

            if (NameContains(family, "PackingDensity")) {
                auto metric_it = run.counters.find("metric_value");
                if (metric_it != run.counters.end()) {
                    if (NameContains(family, "Practical")) {
                        summary = FormatMetric(metric_it->second, "binary-bytes/t81-byte");
                    } else {
                        summary = FormatMetric(metric_it->second, "bits/trit");
                    }
                }
                latency_str = "informational-only (metric row)";
                latency = 0.0;
                throughput_recorded = false;
                bandwidth_recorded = false;
            }

            if (is_t81_classic) {
                if (arg_rank < final_results[family].t81_arg_rank) {
                    continue;
                }
                final_results[family].t81_arg_rank = arg_rank;
                final_results[family].t81_result_str = summary;
                final_results[family].t81_latency_seconds = latency;
                final_results[family].t81_latency_str = latency_str;
                if (!effective_label.empty()) final_results[family].t81_classic_note = effective_label;
                if (throughput_recorded) {
                    final_results[family].t81_result_val = gops;
                    final_results[family].has_t81_flow = true;
                }
                if (bandwidth_recorded) {
                    final_results[family].t81_bandwidth_val = bandwidth;
                    final_results[family].has_t81_flow = true;
                }
                final_results[family].t81_classic_inconsistent = inconsistent_counters;
                final_results[family].t81_work_defined = has_work;
                final_results[family].t81_work_per_iter = work_per_iter;
            } else if (is_t81_native) {
                if (arg_rank < final_results[family].t81_native_arg_rank) {
                    continue;
                }
                final_results[family].t81_native_arg_rank = arg_rank;
                final_results[family].t81_native_result_str = summary;
                final_results[family].t81_native_latency_seconds = latency;
                final_results[family].t81_native_latency_str = latency_str;
                if (!effective_label.empty()) final_results[family].t81_native_note = effective_label;
                if (throughput_recorded) {
                    final_results[family].t81_native_result_val = gops;
                    final_results[family].has_t81_native_flow = true;
                }
                if (bandwidth_recorded) {
                    final_results[family].t81_native_bandwidth_val = bandwidth;
                    final_results[family].has_t81_native_flow = true;
                }
                final_results[family].t81_native_inconsistent = inconsistent_counters;
                final_results[family].t81_native_work_defined = has_work;
                final_results[family].t81_native_work_per_iter = work_per_iter;
            } else if (is_binary) {
                if (arg_rank < final_results[family].binary_arg_rank) {
                    continue;
                }
                final_results[family].binary_arg_rank = arg_rank;
                final_results[family].binary_result_str = summary;
                final_results[family].binary_latency_seconds = latency;
                final_results[family].binary_latency_str = latency_str;
                if (!effective_label.empty()) final_results[family].binary_note = effective_label;
                if (throughput_recorded) {
                    final_results[family].binary_result_val = gops;
                    final_results[family].has_binary_flow = true;
                }
                if (bandwidth_recorded) {
                    final_results[family].binary_bandwidth_val = bandwidth;
                    final_results[family].has_binary_flow = true;
                }
                final_results[family].binary_inconsistent = inconsistent_counters;
                final_results[family].binary_work_defined = has_work;
                final_results[family].binary_work_per_iter = work_per_iter;
            } else {
                if (arg_rank < final_results[family].t81_arg_rank) {
                    continue;
                }
                final_results[family].t81_arg_rank = arg_rank;
                final_results[family].t81_result_str = summary;
                final_results[family].t81_latency_seconds = latency;
                final_results[family].t81_latency_str = latency_str;
                if (!effective_label.empty()) final_results[family].t81_classic_note = effective_label;
                if (throughput_recorded) {
                    final_results[family].t81_result_val = gops;
                    final_results[family].has_t81_flow = true;
                }
                if (bandwidth_recorded) final_results[family].t81_bandwidth_val = bandwidth;
                if (bandwidth_recorded) final_results[family].has_t81_flow = true;
                final_results[family].t81_classic_inconsistent = inconsistent_counters;
                final_results[family].t81_work_defined = has_work;
                final_results[family].t81_work_per_iter = work_per_iter;
            }
        }
    }
};

void GenerateMarkdownReport();

static void ConfigureBenchmarkTrapLogging() {
    if (std::getenv("T81_AXION_TRAP_STDERR") != nullptr) return;
#ifdef _WIN32
    _putenv_s("T81_AXION_TRAP_STDERR", "0");
#else
    setenv("T81_AXION_TRAP_STDERR", "0", 0);
#endif
}

int main(int argc, char** argv) {
    ConfigureBenchmarkTrapLogging();
    auto effective_args = BuildEffectiveBenchmarkArgs(argc, argv);
    std::vector<char*> effective_argv;
    effective_argv.reserve(effective_args.size() + 1U);
    for (auto& arg : effective_args) {
        effective_argv.push_back(arg.data());
    }
    effective_argv.push_back(nullptr);

    int effective_argc = static_cast<int>(effective_args.size());
    ::benchmark::Initialize(&effective_argc, effective_argv.data());
    argc = effective_argc;
    argv = effective_argv.data();
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;

    CustomReporter reporter;
    ::benchmark::RunSpecifiedBenchmarks(&reporter);
    ::benchmark::Shutdown();

    GenerateMarkdownReport();
    return 0;
}

std::string get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm buf;
#ifdef _WIN32
    localtime_s(&buf, &in_time_t);
#else
    localtime_r(&in_time_t, &buf);
#endif
    std::stringstream ss;
    ss << std::put_time(&buf, "%Y-%m-%d %X UTC");
    return ss.str();
}

std::string SIMDCapabilityLine() {
    std::ostringstream oss;
    oss << "SIMD capabilities (compile target): ";
#if defined(__AVX2__)
    oss << "AVX2=1, ";
#else
    oss << "AVX2=0, ";
#endif
#if defined(__SSE4_2__)
    oss << "SSE4.2=1, ";
#else
    oss << "SSE4.2=0, ";
#endif
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    oss << "NEON=1";
#else
    oss << "NEON=0";
#endif
    return oss.str();
}

void GenerateMarkdownReport() {
    std::lock_guard<std::mutex> guard(final_results_mutex);
    std::cout << "\nGenerating benchmark report...\n";

    auto DisplayValue = [](const std::string& value) -> std::string {
        return value.empty() ? "not-applicable (no annotation)" : value;
    };
    auto EscapePipes = [](const std::string& value) -> std::string {
        std::string result;
        result.reserve(value.size());
        for (char c : value) {
            if (c == '|') {
                result += "\\|";
            } else {
                result += c;
            }
        }
        return result;
    };

    auto ResolveT81ClassicResultCell = [&](const BenchmarkResult& r) -> std::string {
        if (!r.t81_result_str.empty()) return r.t81_result_str;
        if (r.t81_classic_skipped) return "skipped (no counters emitted)";
        if (T81ClassicSemanticallyUnavoidable(r)) return "not-applicable (semantic)";
        const bool has_native_only = IsNativeOnlyFamily(r);
        if (has_native_only) {
            if (!r.t81_native_result_str.empty()) return r.t81_native_result_str + " (native-only)";
            if (r.t81_native_skipped) return "skipped (native-only family)";
            return "not-applicable (native-only family)";
        }
        return "not-implemented (missing baseline)";
    };

    auto ResolveT81ClassicLatencyCell = [&](const BenchmarkResult& r) -> std::string {
        if (!r.t81_latency_str.empty()) return r.t81_latency_str;
        if (r.t81_classic_skipped) return "skipped (no counters emitted)";
        if (T81ClassicSemanticallyUnavoidable(r)) return "not-applicable (semantic)";
        const bool has_native_only = IsNativeOnlyFamily(r);
        if (has_native_only) {
            if (!r.t81_native_latency_str.empty()) return r.t81_native_latency_str + " (native-only)";
            if (r.t81_native_skipped) return "skipped (native-only family)";
            return "not-applicable (native-only family)";
        }
        return "not-implemented (missing baseline)";
    };

    auto ResolveBinaryResultCell = [&](const BenchmarkResult& r) -> std::string {
        if (!r.binary_result_str.empty()) return r.binary_result_str;
        if (r.binary_skipped) return "skipped (no counters emitted)";
        if (IsBinaryOnlyFamily(r)) return "not-applicable (binary-only family)";
        if (BinaryBaselineSemanticallyUnavoidable(r)) {
            return "not-applicable (semantic: no binary baseline)";
        }
        return "not-implemented (missing baseline)";
    };

    auto ResolveBinaryLatencyCell = [&](const BenchmarkResult& r) -> std::string {
        if (!r.binary_latency_str.empty()) return r.binary_latency_str;
        if (r.binary_skipped) return "skipped (no counters emitted)";
        if (IsBinaryOnlyFamily(r)) return "not-applicable (binary-only family)";
        if (BinaryBaselineSemanticallyUnavoidable(r)) {
            return "not-applicable (semantic: no binary baseline)";
        }
        return "not-implemented (missing baseline)";
    };

    auto ResolveThroughputRatioCell = [&](const BenchmarkResult& r) -> std::string {
        if (r.t81_classic_inconsistent || r.binary_inconsistent) {
            return "DEFECT: inconsistent work definition";
        }
        if (r.throughput_ratio_computed) return r.throughput_ratio_str;
        const bool t81_semantic_gap = T81ClassicSemanticallyUnavoidable(r) && !r.has_t81_flow;
        if (!r.has_binary_flow && BinaryBaselineSemanticallyUnavoidable(r)) {
            return "not-computable (semantic: no binary baseline)";
        }
        if (t81_semantic_gap) {
            return "not-computable (semantic)";
        }
        if (IsBelowResolutionMarker(r.t81_result_str) || IsBelowResolutionMarker(r.binary_result_str)) {
            return "below timer resolution";
        }
        if (r.t81_classic_skipped || r.binary_skipped) {
            return "skipped (no counters emitted)";
        }
        return "not-implemented (missing baseline)";
    };

    auto ResolveLatencySpeedupCell = [&](const BenchmarkResult& r) -> std::string {
        if (r.t81_classic_inconsistent || r.binary_inconsistent) {
            return "DEFECT: inconsistent work definition";
        }
        if (r.latency_speedup_computed) return r.latency_speedup_str;
        const bool t81_semantic_gap = T81ClassicSemanticallyUnavoidable(r) && !r.has_t81_flow;
        if (!r.has_binary_flow && BinaryBaselineSemanticallyUnavoidable(r)) {
            return "not-computable (semantic: no binary baseline)";
        }
        if (t81_semantic_gap) {
            return "not-computable (semantic)";
        }
        if (IsBelowResolutionMarker(r.t81_latency_str) || IsBelowResolutionMarker(r.binary_latency_str)) {
            return "below timer resolution";
        }
        if (r.t81_classic_skipped || r.binary_skipped) {
            return "skipped (no counters emitted)";
        }
        return "not-implemented (missing baseline)";
    };

    auto ResolveNativeThroughputRatioCell = [&](const BenchmarkResult& r) -> std::string {
        if (!(r.has_binary_flow || !r.binary_result_str.empty())) {
            if (IsNativeOnlyFamily(r)) return "not-computable (native-only family)";
        }
        if (r.t81_native_inconsistent || r.binary_inconsistent) {
            return "DEFECT: inconsistent work definition";
        }
        if (r.native_throughput_ratio_computed) return r.native_throughput_ratio_str;
        if (!(r.has_t81_native_flow || !r.t81_native_result_str.empty())) {
            return "not-applicable (no native variant)";
        }
        if (IsBelowResolutionMarker(r.t81_native_result_str) || IsBelowResolutionMarker(r.binary_result_str)) {
            return "below timer resolution";
        }
        if (r.t81_native_skipped || r.binary_skipped) {
            return "skipped (no counters emitted)";
        }
        return "not-implemented (missing baseline)";
    };

    auto ResolveNativeLatencySpeedupCell = [&](const BenchmarkResult& r) -> std::string {
        if (!(r.has_binary_flow || !r.binary_latency_str.empty())) {
            if (IsNativeOnlyFamily(r)) return "not-computable (native-only family)";
        }
        if (r.t81_native_inconsistent || r.binary_inconsistent) {
            return "DEFECT: inconsistent work definition";
        }
        if (r.native_latency_speedup_computed) return r.native_latency_speedup_str;
        if (!(r.has_t81_native_flow || !r.t81_native_latency_str.empty())) {
            return "not-applicable (no native variant)";
        }
        if (IsBelowResolutionMarker(r.t81_native_latency_str) || IsBelowResolutionMarker(r.binary_latency_str)) {
            return "below timer resolution";
        }
        if (r.t81_native_skipped || r.binary_skipped) {
            return "skipped (no counters emitted)";
        }
        return "not-implemented (missing baseline)";
    };

    for (auto& [name, r] : final_results) {
        const bool has_classic_and_binary = r.has_t81_flow && r.has_binary_flow;
        if (has_classic_and_binary && (!r.t81_work_defined || !r.binary_work_defined)) {
            r.t81_classic_inconsistent = true;
            r.binary_inconsistent = true;
        }
        if (r.t81_work_defined && r.binary_work_defined && r.binary_work_per_iter > 0.0) {
            const double rel = std::abs(r.t81_work_per_iter - r.binary_work_per_iter) / r.binary_work_per_iter;
            if (rel > 0.05) {
                r.t81_classic_inconsistent = true;
                r.binary_inconsistent = true;
            }
        }
        const bool has_native_and_binary = r.has_t81_native_flow && r.has_binary_flow;
        if (has_native_and_binary && (!r.t81_native_work_defined || !r.binary_work_defined)) {
            r.t81_native_inconsistent = true;
            r.binary_inconsistent = true;
        }
        if (r.t81_native_work_defined && r.binary_work_defined && r.binary_work_per_iter > 0.0) {
            const double rel = std::abs(r.t81_native_work_per_iter - r.binary_work_per_iter) / r.binary_work_per_iter;
            if (rel > 0.05) {
                r.t81_native_inconsistent = true;
                r.binary_inconsistent = true;
            }
        }
        if (has_classic_and_binary && r.t81_work_defined && r.binary_work_defined &&
            r.binary_work_per_iter > 0.0) {
            const double rel =
                std::abs(r.t81_work_per_iter - r.binary_work_per_iter) / r.binary_work_per_iter;
            if (rel <= 0.05) {
                r.t81_classic_inconsistent = false;
                r.binary_inconsistent = false;
            }
        }
        if (has_native_and_binary && r.t81_native_work_defined && r.binary_work_defined &&
            r.binary_work_per_iter > 0.0) {
            const double rel = std::abs(r.t81_native_work_per_iter - r.binary_work_per_iter) /
                               r.binary_work_per_iter;
            if (rel <= 0.05) {
                r.t81_native_inconsistent = false;
                r.binary_inconsistent = false;
            }
        }
        if (r.t81_classic_inconsistent || r.t81_native_inconsistent || r.binary_inconsistent) {
            r.throughput_ratio_computed = false;
            r.latency_speedup_computed = false;
            r.native_throughput_ratio_computed = false;
            r.native_latency_speedup_computed = false;
            r.analysis = BuildAnalysis(r);
            continue;
        }
        bool throughput_ratio_ready = r.has_t81_flow && r.has_binary_flow &&
                                      r.binary_result_val > 0.0 && r.t81_result_val > 0.0;
        throughput_ratio_ready = throughput_ratio_ready &&
            !IsBelowResolutionMarker(r.t81_result_str) &&
            !IsBelowResolutionMarker(r.binary_result_str);
        double throughput_ratio = 0.0;
        if (throughput_ratio_ready) {
            throughput_ratio = r.t81_result_val / r.binary_result_val;
        } else if (r.has_t81_flow && r.has_binary_flow &&
                   r.binary_bandwidth_val > 0.0 && r.t81_bandwidth_val > 0.0) {
            throughput_ratio = r.t81_bandwidth_val / r.binary_bandwidth_val;
            throughput_ratio_ready = true;
        }

        if (throughput_ratio_ready) {
            r.throughput_ratio_val = throughput_ratio;
            r.throughput_ratio_str = FormatRatio(throughput_ratio);
            r.throughput_ratio_computed = true;
        } else {
            r.throughput_ratio_str.clear();
            r.throughput_ratio_computed = false;
        }

        if (r.t81_latency_seconds >= 1e-9 && r.binary_latency_seconds >= 1e-9 &&
            !IsBelowResolutionMarker(r.t81_latency_str) &&
            !IsBelowResolutionMarker(r.binary_latency_str)) {
            r.latency_speedup_val = r.binary_latency_seconds / r.t81_latency_seconds;
            r.latency_speedup_str = FormatRatio(r.latency_speedup_val);
            r.latency_speedup_computed = true;
        } else {
            r.latency_speedup_str.clear();
            r.latency_speedup_computed = false;
        }

        if (r.has_t81_native_flow && r.has_binary_flow &&
            r.t81_native_result_val > 0.0 && r.binary_result_val > 0.0 &&
            !IsBelowResolutionMarker(r.t81_native_result_str) &&
            !IsBelowResolutionMarker(r.binary_result_str)) {
            r.native_throughput_ratio_val = r.t81_native_result_val / r.binary_result_val;
            r.native_throughput_ratio_str = FormatRatio(r.native_throughput_ratio_val);
            r.native_throughput_ratio_computed = true;
        } else {
            r.native_throughput_ratio_computed = false;
            r.native_throughput_ratio_str.clear();
        }

        if (r.t81_native_latency_seconds >= 1e-9 && r.binary_latency_seconds >= 1e-9 &&
            !IsBelowResolutionMarker(r.t81_native_latency_str) &&
            !IsBelowResolutionMarker(r.binary_latency_str)) {
            r.native_latency_speedup_val = r.binary_latency_seconds / r.t81_native_latency_seconds;
            r.native_latency_speedup_str = FormatRatio(r.native_latency_speedup_val);
            r.native_latency_speedup_computed = true;
        } else {
            r.native_latency_speedup_computed = false;
            r.native_latency_speedup_str.clear();
        }
        r.analysis = BuildAnalysis(r);
    }

    const bool verbose_console = []() {
        if (const char* v = std::getenv("T81_BENCHMARK_VERBOSE_CONSOLE")) {
            return std::strcmp(v, "0") != 0;
        }
        return false;
    }();
    if (verbose_console) {
        std::cout << std::left << std::setw(25) << "Benchmark"
                  << std::setw(20) << "T81 Result"
                  << std::setw(16) << "T81 Iter Time"
                  << std::setw(20) << "Binary Result"
                  << std::setw(16) << "Binary Iter"
                  << std::setw(18) << "Thru Ratio"
                  << std::setw(18) << "Lat Speedup"
                  << std::setw(18) << "NThru Ratio"
                  << std::setw(18) << "NLat Speedup"
                  << std::setw(25) << "T81 Advantage"
                  << "Notes\n";
        std::cout << std::string(176, '-') << "\n";
        for (auto const& [name, r] : final_results) {
            const std::string t81_result_cell = ResolveT81ClassicResultCell(r);
            const std::string t81_latency_cell = ResolveT81ClassicLatencyCell(r);
            const std::string binary_result_cell = ResolveBinaryResultCell(r);
            const std::string binary_latency_cell = ResolveBinaryLatencyCell(r);
            const std::string throughput_ratio_cell = ResolveThroughputRatioCell(r);
            const std::string latency_speedup_cell = ResolveLatencySpeedupCell(r);
            const std::string native_throughput_ratio_cell = ResolveNativeThroughputRatioCell(r);
            const std::string native_latency_speedup_cell = ResolveNativeLatencySpeedupCell(r);
            const std::string advantage_display = BuildT81AdvantageDisplay(r);
            const std::string notes_display = BuildNotesDisplay(r);
            std::cout << std::left << std::setw(25) << r.name
                      << std::setw(20) << DisplayValue(t81_result_cell)
                      << std::setw(16) << DisplayValue(t81_latency_cell)
                      << std::setw(20) << DisplayValue(binary_result_cell)
                      << std::setw(16) << DisplayValue(binary_latency_cell)
                      << std::setw(18) << DisplayValue(throughput_ratio_cell)
                      << std::setw(18) << DisplayValue(latency_speedup_cell)
                      << std::setw(18) << DisplayValue(native_throughput_ratio_cell)
                      << std::setw(18) << DisplayValue(native_latency_speedup_cell)
                      << std::setw(25) << DisplayValue(advantage_display)
                      << DisplayValue(notes_display) << "\n";
        }
    } else {
        std::cout << "Benchmark console table suppressed (set T81_BENCHMARK_VERBOSE_CONSOLE=1 to enable).\n";
    }

    const auto git_branch = RunCommand("git rev-parse --abbrev-ref HEAD");
    const auto git_sha = RunCommand("git rev-parse --short HEAD");
    const std::string timestamp = get_current_timestamp();

    struct UserRow {
        std::string name;
        std::string category;
        std::string t81_result;
        std::string t81_iter;
        std::string binary_result;
        std::string binary_iter;
        std::string ratio;
        double ratio_val = 0.0;
        bool ratio_computed = false;
        std::string verdict;
        std::string fairness;
        std::string confidence;
        std::string notes;
        double impact = 0.0;
    };

    auto CategoryFor = [&](const std::string& name) -> std::string {
        if (NameContains(name, "Llama")) return "Inference Kernels";
        if (NameContains(name, "CanonFS")) return "CanonFS I/O";
        if (NameContains(name, "T81LangCompile") || NameContains(name, "Lexer")) return "Compiler & Language";
        if (NameContains(name, "Limb") || NameContains(name, "Add_") || NameContains(name, "Arith") ||
            NameContains(name, "overflow") || NameContains(name, "Roundtrip") || NameContains(name, "Negation")) {
            return "Core Arithmetic";
        }
        if (NameContains(name, "Tensor")) return "Tensor Math";
        return "Runtime & Misc";
    };

    auto FairnessFor = [&](const BenchmarkResult& r, const std::string& notes_display) -> std::string {
        if (r.t81_classic_inconsistent || r.binary_inconsistent) return "defect: work mismatch";
        if (BinaryBaselineSemanticallyUnavoidable(r) || T81ClassicSemanticallyUnavoidable(r)) {
            return "semantic mismatch";
        }
        if (notes_display.find("comparison=pipeline-advantage") != std::string::npos) {
            return "systems-path (pipeline-advantage)";
        }
        if (notes_display.find("baseline=map-read") != std::string::npos) {
            return "baseline-pinned (map-read)";
        }
        if (r.throughput_ratio_computed) return "apples-to-apples";
        return "missing baseline";
    };

    auto ConfidenceFor = [&](const BenchmarkResult& r, const std::string& fairness) -> std::string {
        if (fairness.find("defect") != std::string::npos) return "low";
        if (fairness.find("semantic mismatch") != std::string::npos) return "low";
        if (fairness.find("missing baseline") != std::string::npos) return "low";
        if (fairness.find("systems-path") != std::string::npos) return "medium";
        if (r.throughput_ratio_computed && r.latency_speedup_computed) return "high";
        if (r.throughput_ratio_computed) return "medium";
        return "low";
    };

    std::vector<UserRow> rows;
    rows.reserve(final_results.size());

    double best_t81_ratio = 1.0;
    double best_binary_ratio = 1.0;
    std::string best_name;
    std::string worst_name;
    int missing_baseline = 0;
    int semantic_mismatch = 0;
    int defect_rows = 0;

    for (auto& [name, r] : final_results) {
        const std::string notes_display = BuildNotesDisplay(r);
        const std::string t81_result_cell = ResolveT81ClassicResultCell(r);
        const std::string t81_latency_cell = ResolveT81ClassicLatencyCell(r);
        const std::string binary_result_cell = ResolveBinaryResultCell(r);
        const std::string binary_latency_cell = ResolveBinaryLatencyCell(r);
        const std::string throughput_ratio_cell = ResolveThroughputRatioCell(r);
        const std::string fairness = FairnessFor(r, notes_display);
        const std::string confidence = ConfidenceFor(r, fairness);

        UserRow row;
        row.name = r.name;
        row.category = CategoryFor(r.name);
        row.t81_result = t81_result_cell;
        row.t81_iter = t81_latency_cell;
        row.binary_result = binary_result_cell;
        row.binary_iter = binary_latency_cell;
        row.ratio = throughput_ratio_cell;
        row.ratio_computed = r.throughput_ratio_computed;
        row.ratio_val = r.throughput_ratio_val;
        row.fairness = fairness;
        row.confidence = confidence;
        row.notes = notes_display.empty() ? "not-applicable (no annotation)" : notes_display;

        if (r.throughput_ratio_computed) {
            if (r.throughput_ratio_val > 1.05) {
                row.verdict = "T81 wins";
            } else if (r.throughput_ratio_val < 0.95) {
                row.verdict = "Binary wins";
            } else {
                row.verdict = "Comparable";
            }
            row.impact = std::abs(std::log(r.throughput_ratio_val));
            if (r.throughput_ratio_val > 1.0 && r.throughput_ratio_val > best_t81_ratio) {
                best_t81_ratio = r.throughput_ratio_val;
                best_name = r.name;
            }
            if (r.throughput_ratio_val < 1.0 && r.throughput_ratio_val < best_binary_ratio) {
                best_binary_ratio = r.throughput_ratio_val;
                worst_name = r.name;
            }
        } else {
            row.verdict = "No baseline";
            if (fairness.find("semantic mismatch") != std::string::npos) {
                ++semantic_mismatch;
            } else if (fairness.find("defect") != std::string::npos) {
                ++defect_rows;
            } else {
                ++missing_baseline;
            }
        }
        rows.push_back(std::move(row));
    }

    std::vector<UserRow> comparable_rows;
    for (const auto& row : rows) {
        if (row.ratio_computed) comparable_rows.push_back(row);
    }
    std::sort(comparable_rows.begin(), comparable_rows.end(),
              [](const UserRow& a, const UserRow& b) { return a.impact > b.impact; });

    auto MdCell = [&](const std::string& in) {
        std::string s = EscapePipes(in);
        for (char& ch : s) {
            if (ch == '\n' || ch == '\r') ch = ' ';
        }
        return DisplayValue(s);
    };

    auto category_counts_for = [](const std::vector<UserRow>& source) {
        std::map<std::string, std::array<int, 4>> counts_map;
        for (const auto& row : source) {
            auto& counts = counts_map[row.category];
            counts[0] += 1;
            if (row.verdict == "T81 wins") counts[1] += 1;
            else if (row.verdict == "Binary wins") counts[2] += 1;
            else counts[3] += 1;
        }
        return counts_map;
    };

    const auto full_category_counts = category_counts_for(comparable_rows);

    auto is_product_relevant = [](const UserRow& row) {
        return NameContains(row.name, "Llama") ||
               NameContains(row.name, "CanonFS") ||
               row.name == "BM_T81LangCompile" ||
               row.name == "BM_Lexer_AllTokens" ||
               row.name == "BM_TensorMatMul_Naive";
    };

    std::vector<UserRow> public_rows;
    for (const auto& row : comparable_rows) {
        if (is_product_relevant(row)) public_rows.push_back(row);
    }
    const auto public_category_counts = category_counts_for(public_rows);

    auto write_header_and_metadata = [&](std::ofstream& out, const std::string& title) {
        out << "# " << title << "\n\n";
        out << "This document is auto-generated by the `benchmark_runner`.\n\n";
        out << "*Last Updated: " << timestamp << "*  ";
        if (!git_branch.empty()) {
            out << "*Branch: " << git_branch << "*  ";
        }
        if (!git_sha.empty()) {
            out << "*Commit: " << git_sha << "*";
        }
        out << "\n\n";
        out << "## Run Metadata\n\n";
        out << "- " << SIMDCapabilityLine() << "\n";
        out << "- Native benchmark mode: SIMD acceleration may fall back to scalar when unsupported by target/features.\n";
        out << "- CanonFS in-memory binary read baseline is pinned to `baseline=map-read` for cross-run comparability.\n";
        out << "- CanonFS persistent rows are labeled `comparison=pipeline-advantage` (systems-path comparison, not strict microkernel fairness).\n\n";
    };

    auto write_decision_tables = [&](std::ofstream& out,
                                     const std::vector<UserRow>& report_rows,
                                     const std::map<std::string, std::array<int, 4>>& category_counts,
                                     size_t top_n,
                                     bool include_full_appendix,
                                     bool include_known_gaps) {
        int report_t81_wins = 0;
        int report_binary_wins = 0;
        int report_ties = 0;
        double report_best_t81_ratio = 1.0;
        double report_best_binary_ratio = 1.0;
        std::string report_best_name;
        std::string report_worst_name;
        for (const auto& row : report_rows) {
            if (row.verdict == "T81 wins") ++report_t81_wins;
            else if (row.verdict == "Binary wins") ++report_binary_wins;
            else ++report_ties;
            if (row.ratio_val > 1.0 && row.ratio_val > report_best_t81_ratio) {
                report_best_t81_ratio = row.ratio_val;
                report_best_name = row.name;
            }
            if (row.ratio_val < 1.0 && row.ratio_val < report_best_binary_ratio) {
                report_best_binary_ratio = row.ratio_val;
                report_worst_name = row.name;
            }
        }

        out << "## Executive Summary\n\n";
        out << "- Comparable benchmark rows (ratio computed): " << report_rows.size()
            << " / " << rows.size() << "\n";
        out << "- Verdict split (comparable rows): T81 wins " << report_t81_wins
            << ", Binary wins " << report_binary_wins << ", Comparable " << report_ties << "\n";
        out << "- Data quality: missing baseline " << missing_baseline
            << ", semantic mismatch " << semantic_mismatch
            << ", defect rows " << defect_rows << "\n";
        if (!report_best_name.empty()) {
            out << "- Largest T81 advantage: `" << report_best_name << "` (" << FormatRatio(report_best_t81_ratio) << ")\n";
        }
        if (!report_worst_name.empty() && report_best_binary_ratio < 1.0) {
            out << "- Largest binary advantage: `" << report_worst_name << "` (" << FormatRatio(report_best_binary_ratio) << ")\n";
        }
        out << "\n";

        out << "## What To Use Today\n\n";
        out << "| Category | Comparable Rows | T81 Wins | Binary Wins | Comparable | Recommendation |\n";
        out << "|---|---:|---:|---:|---:|---|\n";
        for (const auto& [category, counts] : category_counts) {
            std::string recommendation = "Mixed; benchmark workload path";
            if (counts[1] >= 2 && counts[1] > counts[2]) {
                recommendation = "Prefer T81 on this host";
            } else if (counts[2] >= 2 && counts[2] > counts[1]) {
                recommendation = "Prefer binary for throughput-critical path";
            }
            out << "| " << MdCell(category)
                << " | " << counts[0]
                << " | " << counts[1]
                << " | " << counts[2]
                << " | " << counts[3]
                << " | " << recommendation << " |\n";
        }
        out << "\n";

        out << "## Top Signal Rows\n\n";
        out << "| Benchmark | Category | T81 Iteration | Binary Iteration | Throughput Ratio | Verdict | Fairness | Confidence |\n";
        out << "|---|---|---|---|---:|---|---|---|\n";
        const size_t max_rows = std::min(top_n, report_rows.size());
        for (size_t i = 0; i < max_rows; ++i) {
            const auto& row = report_rows[i];
            out << "| " << MdCell(row.name)
                << " | " << MdCell(row.category)
                << " | " << MdCell(row.t81_iter)
                << " | " << MdCell(row.binary_iter)
                << " | " << MdCell(row.ratio)
                << " | " << MdCell(row.verdict)
                << " | " << MdCell(row.fairness)
                << " | " << MdCell(row.confidence) << " |\n";
        }

        out << "\n## Category Detail\n\n";
        for (const auto& [category, counts] : category_counts) {
            if (counts[0] == 0) continue;
            out << "### " << MdCell(category) << "\n\n";
            out << "| Benchmark | T81 Iteration | Binary Iteration | Throughput Ratio | Verdict | Fairness |\n";
            out << "|---|---|---|---:|---|---|\n";
            int emitted = 0;
            for (const auto& row : report_rows) {
                if (row.category != category) continue;
                out << "| " << MdCell(row.name)
                    << " | " << MdCell(row.t81_iter)
                    << " | " << MdCell(row.binary_iter)
                    << " | " << MdCell(row.ratio)
                    << " | " << MdCell(row.verdict)
                    << " | " << MdCell(row.fairness) << " |\n";
                emitted++;
                if (emitted >= 8) break;
            }
            out << "\n";
        }

        if (include_known_gaps) {
            out << "## Known Gaps\n\n";
            out << "- Rows hidden from this public view: " << (rows.size() - report_rows.size()) << "\n";
            out << "- Missing baseline rows: " << missing_baseline << "\n";
            out << "- Semantic mismatch rows: " << semantic_mismatch << "\n";
            out << "- Defect rows: " << defect_rows << "\n\n";
        }

        if (include_full_appendix) {
            out << "## Appendix: Full Raw Rows\n\n";
            out << "| Benchmark | Category | T81 Result | T81 Iteration | Binary Result | Binary Iteration | Throughput Ratio | Verdict | Fairness | Confidence | Notes |\n";
            out << "|---|---|---|---|---|---|---:|---|---|---|---|\n";
            std::vector<UserRow> sorted_rows = rows;
            std::sort(sorted_rows.begin(), sorted_rows.end(),
                      [](const UserRow& a, const UserRow& b) { return a.name < b.name; });
            for (const auto& row : sorted_rows) {
                out << "| " << MdCell(row.name)
                    << " | " << MdCell(row.category)
                    << " | " << MdCell(row.t81_result)
                    << " | " << MdCell(row.t81_iter)
                    << " | " << MdCell(row.binary_result)
                    << " | " << MdCell(row.binary_iter)
                    << " | " << MdCell(row.ratio)
                    << " | " << MdCell(row.verdict)
                    << " | " << MdCell(row.fairness)
                    << " | " << MdCell(row.confidence)
                    << " | " << MdCell(row.notes) << " |\n";
            }
        }
    };

    const char* write_reports_env = std::getenv("T81_BENCHMARK_WRITE_REPORTS");
    const bool write_reports = write_reports_env != nullptr && std::string(write_reports_env) == "1";
    if (!write_reports) {
        std::cout << "Skipping benchmark report generation (set T81_BENCHMARK_WRITE_REPORTS=1 to enable)\n";
        return;
    }

    std::ofstream public_file("docs/reference/benchmarks.md");
    if (!public_file.is_open()) {
        std::cerr << "Error: Could not open docs/reference/benchmarks.md for writing.\n";
        return;
    }
    write_header_and_metadata(public_file, "TCB-Core v0.1: Public T81 Benchmark Report");
    write_decision_tables(public_file, public_rows, public_category_counts, 10, false, true);
    public_file.close();

    std::ofstream engineering_file("docs/reference/benchmarks_engineering.md");
    if (!engineering_file.is_open()) {
        std::cerr << "Error: Could not open docs/reference/benchmarks_engineering.md for writing.\n";
        return;
    }
    write_header_and_metadata(engineering_file, "TCB-Core v0.1: Engineering Benchmark Report");
    write_decision_tables(engineering_file, comparable_rows, full_category_counts, 12, true, false);
    engineering_file.close();

    std::cout << "Successfully wrote report to docs/reference/benchmarks.md\n";
    std::cout << "Successfully wrote report to docs/reference/benchmarks_engineering.md\n";
}
