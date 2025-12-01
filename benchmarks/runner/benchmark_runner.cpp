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

struct BenchmarkResult {
    std::string name;
    std::string t81_result_str;
    std::string binary_result_str;
    double t81_result_val = 0.0;
    double binary_result_val = 0.0;
    double t81_latency_seconds = 0.0;
    double binary_latency_seconds = 0.0;
    std::string t81_advantage;
    std::string notes;
    std::string t81_latency_str;
    std::string binary_latency_str;
    std::string analysis;
    bool has_t81_flow = false;
    bool has_binary_flow = false;
    bool ratio_computed = false;
    std::string ratio_str;
    double ratio_val = 0.0;
};

std::map<std::string, BenchmarkResult> final_results;
std::mutex final_results_mutex;

const std::map<std::string, std::string> T81_ADVANTAGES = {
    {"BM_ArithThroughput", "Exact rounding, no FP error"},
    {"BM_NegationSpeed", "Free negation (no borrow)"},
    {"BM_RoundtripAccuracy", "No sign-bit tax"},
    {"BM_OverflowDetection", "Deterministic, provable"},
    {"BM_PackingDensity_Theoretical", "Theoretical maximum"},
    {"BM_PackingDensity_Achieved", "Achieved bits/trit"},
    {"BM_PackingDensity_Practical", "Practical size ratio"}
};

std::string RunCommand(const std::string& command) {
    std::array<char, 128> buffer;
    std::string output;
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return {};
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }
    pclose(pipe);
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
    if (seconds <= 0.0) {
        return {};
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

std::string FormatThroughput(double items_per_second) {
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
    kT81,
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

static FlowKind DetermineFlowKind(const std::string& base_name, const std::string& suffix) {
    const std::string lower_base = ToLower(base_name);
    const std::string lower_suffix = ToLower(suffix);
    if (lower_base.find("t81") != std::string::npos ||
        lower_base.find("ternary") != std::string::npos ||
        lower_suffix.find("t81") != std::string::npos ||
        lower_suffix.find("packed") != std::string::npos) {
        return FlowKind::kT81;
    }
    if (lower_base.find("int64") != std::string::npos ||
        lower_base.find("int128") != std::string::npos ||
        lower_base.find("binary") != std::string::npos ||
        lower_suffix.find("int64") != std::string::npos ||
        lower_suffix.find("int128") != std::string::npos ||
        lower_suffix.find("binary") != std::string::npos) {
        return FlowKind::kBinary;
    }
    return FlowKind::kUnknown;
}

std::string BuildAnalysis(const BenchmarkResult& r) {
    std::ostringstream oss;
    if (!r.ratio_computed) {
        oss << "Throughput data unavailable";
        if (!r.has_t81_flow && !r.has_binary_flow) {
            oss << " (needs `items_per_second` counters from the runner)";
        } else if (!r.has_t81_flow) {
            oss << " (T81 throughput missing due to metadata-only run)";
        } else if (!r.has_binary_flow) {
            oss << " (binary throughput missing for this suite)";
        }
        return oss.str();
    }
    double ratio = r.ratio_val;
    oss << std::fixed << std::setprecision(2) << ratio << "x throughput ratio";
    if (ratio > 1.05) {
        oss << " — T81 leads";
        if (!r.t81_advantage.empty()) {
            oss << " (" << r.t81_advantage << ")";
        }
    } else if (ratio < 0.95) {
        oss << " — binary wins";
    } else {
        oss << " — throughputs comparable";
    }
    if (!r.t81_latency_str.empty() && !r.binary_latency_str.empty()) {
        oss << "; latencies " << r.t81_latency_str << " vs " << r.binary_latency_str;
    }
    return oss.str();
}

class CustomReporter : public ::benchmark::BenchmarkReporter {
public:
    CustomReporter() {}
    bool ReportContext(const Context&) override { return true; }

    void ReportRuns(const std::vector<Run>& reports) override {
        std::lock_guard<std::mutex> guard(final_results_mutex);
        for (const auto& run : reports) {
            std::string base_name = run.benchmark_name();
            const auto slash_pos = base_name.find('/');
            if (slash_pos != std::string::npos) {
                base_name = base_name.substr(0, slash_pos);
            }

            std::string family = base_name;
            std::string suffix;
            const auto last_underscore = base_name.find_last_of('_');
            if (last_underscore != std::string::npos) {
                family = base_name.substr(0, last_underscore);
                suffix = base_name.substr(last_underscore + 1);
            }
            if (family.empty()) {
                family = base_name;
            }

            const FlowKind flow_kind = DetermineFlowKind(base_name, suffix);
            const bool is_t81 = flow_kind == FlowKind::kT81;
            const bool is_binary = flow_kind == FlowKind::kBinary;

            if (final_results.find(family) == final_results.end()) {
                final_results[family].name = family;
                if(T81_ADVANTAGES.count(family))
                    final_results[family].t81_advantage = T81_ADVANTAGES.at(family);
            }
            if(is_t81 || final_results[family].notes.empty()) {
                final_results[family].notes = run.report_label;
            }

            std::string summary;
            double gops = 0.0;
            bool throughput_recorded = false;
            auto items_it = run.counters.find("items_per_second");
            if (items_it != run.counters.end()) {
                double items_per_second = items_it->second;
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

            double latency = ExtractLatency(run);
            std::string latency_str = FormatLatency(latency);

            if(is_t81) {
                final_results[family].t81_result_str = summary;
                final_results[family].t81_latency_seconds = latency;
                final_results[family].t81_latency_str = latency_str;
                if (throughput_recorded) {
                    final_results[family].t81_result_val = gops;
                    final_results[family].has_t81_flow = true;
                }
            } else if(is_binary) {
                final_results[family].binary_result_str = summary;
                final_results[family].binary_latency_seconds = latency;
                final_results[family].binary_latency_str = latency_str;
                if (throughput_recorded) {
                    final_results[family].binary_result_val = gops;
                    final_results[family].has_binary_flow = true;
                }
            } else if (final_results[family].t81_result_str.empty()) {
                final_results[family].t81_result_str = summary;
                final_results[family].t81_latency_seconds = latency;
                final_results[family].t81_latency_str = latency_str;
                final_results[family].has_t81_flow = throughput_recorded;
                final_results[family].t81_result_val = throughput_recorded ? gops : 0.0;
            }
        }
    }
};

void GenerateMarkdownReport();

int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
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
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X UTC");
    return ss.str();
}

void GenerateMarkdownReport() {
    std::lock_guard<std::mutex> guard(final_results_mutex);
    std::cout << "\nGenerating benchmark report...\n";

    auto DisplayValue = [](const std::string& value) -> std::string {
        return value.empty() ? "n/a" : value;
    };

    std::cout << std::left << std::setw(25) << "Benchmark"
              << std::setw(18) << "T81 Result"
              << std::setw(18) << "T81 Latency"
              << std::setw(18) << "Binary Result"
              << std::setw(18) << "Binary Latency"
              << std::setw(25) << "T81 Advantage"
              << "Notes\n";
    std::cout << std::string(110, '-') << "\n";
    for(auto const& [name, r] : final_results) {
        std::cout << std::left << std::setw(25) << r.name
                  << std::setw(18) << DisplayValue(r.t81_result_str)
                  << std::setw(18) << DisplayValue(r.t81_latency_str)
                  << std::setw(18) << DisplayValue(r.binary_result_str)
                  << std::setw(18) << DisplayValue(r.binary_latency_str)
                  << std::setw(25) << r.t81_advantage
                  << r.notes << "\n";
    }

    std::ofstream md_file("docs/benchmarks.md");
    if (!md_file.is_open()) {
        std::cerr << "Error: Could not open docs/benchmarks.md for writing.\n";
        return;
    }

    const auto git_branch = RunCommand("git rev-parse --abbrev-ref HEAD");
    const auto git_sha = RunCommand("git rev-parse --short HEAD");

    md_file << "# TCB-Core v0.1: Official T81 Foundation Core Benchmarks\n\n";
    md_file << "This document is auto-generated by the `benchmark_runner`.\n\n";
    md_file << "*Last Updated: " << get_current_timestamp() << "*  ";
    if (!git_branch.empty()) {
        md_file << "*Branch: " << git_branch << "*  ";
    }
    if (!git_sha.empty()) {
        md_file << "*Commit: " << git_sha << "*";
    }
    md_file << "\n\n";
    md_file << "## Summary\n\n";

    md_file << "| Benchmark               | T81 Result     | T81 Latency    | Binary Result  | Binary Latency | Ratio (T81/Binary) | T81 Advantage                   | Notes                               |\n";
    md_file << "|-------------------------|----------------|----------------|----------------|----------------|--------------------|---------------------------------|-------------------------------------|\n";

    double best_t81_ratio = 1.0;
    double best_binary_ratio = 1.0;
    std::string best_name;
    std::string worst_name;
    int t81_wins = 0;
    int binary_wins = 0;
    int ties = 0;

    for (auto& [name, r] : final_results) {
        const bool ratio_ready = r.has_t81_flow && r.has_binary_flow &&
                                 r.binary_result_val > 0.0 && r.t81_result_val > 0.0;
        double ratio = 0.0;
        if (ratio_ready) {
            ratio = r.t81_result_val / r.binary_result_val;
            r.ratio_val = ratio;
            std::ostringstream temp;
            temp << std::fixed << std::setprecision(2) << ratio << "x";
            r.ratio_str = temp.str();
            r.ratio_computed = true;
            if (ratio > 1.0 && ratio > best_t81_ratio) {
                best_t81_ratio = ratio;
                best_name = r.name;
            }
            if (ratio < 1.0 && ratio < best_binary_ratio) {
                best_binary_ratio = ratio;
                worst_name = r.name;
            }
            if (ratio > 1.05) {
                ++t81_wins;
            } else if (ratio < 0.95) {
                ++binary_wins;
            } else {
                ++ties;
            }
        } else {
            r.ratio_str = "n/a";
            r.ratio_computed = false;
        }
        r.analysis = BuildAnalysis(r);
        md_file << "| " << std::left << std::setw(23) << r.name
                << "| " << std::setw(14) << DisplayValue(r.t81_result_str)
                << "| " << std::setw(14) << DisplayValue(r.t81_latency_str)
                << "| " << std::setw(14) << DisplayValue(r.binary_result_str)
                << "| " << std::setw(14) << DisplayValue(r.binary_latency_str)
                << "| " << std::setw(14) << r.ratio_str
                << "| " << std::setw(31) << r.t81_advantage
                << "| " << std::setw(35) << r.notes << "|\n";
    }

    md_file << "\n## Analysis\n\n";
    for (auto const& [name, r] : final_results) {
        md_file << "- `" << r.name << "`: ";
        if (r.analysis.empty()) {
            md_file << "no throughput summary available yet.\n";
        } else {
            md_file << r.analysis << '\n';
        }
    }

    md_file << "\n## Highlights\n\n";
    if (!best_name.empty()) {
        md_file << "- Largest T81 advantage: `" << best_name << "` (" << std::fixed << std::setprecision(2)
                << best_t81_ratio << "x) using Gops/s throughput.\n";
    }
    if (!worst_name.empty() && best_binary_ratio < 1.0) {
        md_file << "- Largest binary advantage: `" << worst_name << "` (" << std::fixed << std::setprecision(2)
                << best_binary_ratio << "x) reflects where deterministic handling lags.\n";
    }
    md_file << "- T81 wins: " << t81_wins << ", Binary wins: " << binary_wins << ", Comparable: " << ties << ".\n";

    md_file.close();
    std::cout << "Successfully wrote report to docs/benchmarks.md\n";
}
