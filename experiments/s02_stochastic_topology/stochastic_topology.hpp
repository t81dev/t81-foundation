#ifndef T81_EXPERIMENTS_S02_STOCHASTIC_TOPOLOGY_HPP
#define T81_EXPERIMENTS_S02_STOCHASTIC_TOPOLOGY_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace t81::s02 {

// SplitMix64 (Sebastiano Vigna, public-domain reference at
// https://prng.di.unimi.it/splitmix64.c). Rejection removes range-reduction
// bias; this reproducible PRNG is not a physical IID or cryptographic source.
template <class Source>
inline uint64_t sample_uniform_below(uint64_t bound, Source&& source) {
    if (bound == 0) throw std::invalid_argument("empty integer sampling range");
    const uint64_t threshold = (uint64_t{0} - bound) % bound;
    for (;;) {
        const uint64_t word = source();
        if (word >= threshold) return word % bound;
    }
}
struct SimplePRNG {
    uint64_t state;
    uint64_t draws{0};
    explicit SimplePRNG(uint64_t seed) : state(seed) {}
    uint64_t next() {
        ++draws;
        uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }
    double uniform_real(double min_val, double max_val) {
        const double u = static_cast<double>(next() >> 11) / 9007199254740992.0;
        return min_val + u * (max_val - min_val);
    }
    int uniform_int(int min_val, int max_val) {
        if (max_val < min_val) throw std::invalid_argument("reversed integer range");
        const uint64_t span = static_cast<uint64_t>(static_cast<int64_t>(max_val) - min_val) + 1;
        return static_cast<int>(static_cast<int64_t>(min_val) +
            static_cast<int64_t>(sample_uniform_below(span, [this] { return next(); })));
    }
};
inline constexpr uint64_t problem_stream = 0x243f6a8885a308d3ULL;
inline constexpr uint64_t proposal_stream = 0x13198a2e03707344ULL;
inline constexpr uint64_t acceptance_stream = 0xa4093822299f31d0ULL;
inline constexpr uint64_t topology_stream = 0x082efa98ec4e6c89ULL;
inline constexpr uint64_t initialization_stream = 0x452821e638d01377ULL;

enum class ElementType { Binary, Ternary };
enum class TopologyType { Fixed, AdaptiveLocal, CentralizedAdaptive, RandomAdaptive };
enum class DynamicsType { Stochastic, Deterministic };

struct ProblemInstance {
    std::size_t num_nodes{0};
    std::vector<double> h_fields; // local magnetic fields h_i
    std::vector<std::vector<double>> J_couplings; // symmetric matrix of target coupling weights
};

struct SimulationConfig {
    ElementType element_type{ElementType::Binary};
    TopologyType topology_type{TopologyType::Fixed};
    DynamicsType dynamics_type{DynamicsType::Stochastic};

    std::size_t num_nodes{12};
    std::size_t num_steps{200};
    uint64_t seed{42};

    double temperature{1.0};
    double coupling_scale{1.0};
    double cost_spin_update{1.0};
    double cost_comm_per_degree{1.0};
    double cost_topology_update{1.0};

    // Capacity matching flag
    bool matched_state_capacity{false};
};

struct ExecutionMetrics {
    ElementType element_type{ElementType::Binary};
    TopologyType topology_type{TopologyType::Fixed};
    DynamicsType dynamics_type{DynamicsType::Stochastic};

    std::size_t num_nodes{0};
    uint64_t seed{0};
    double temperature{0.0};
    double cost_topology_update{0.0};

    double final_energy{0.0};
    double ground_state_energy{0.0};
    double energy_error{0.0};
    double mean_energy{0.0};

    uint64_t spin_update_ops{0};
    uint64_t comm_ops{0};
    uint64_t topology_update_ops{0};
    double total_cost{0.0};

    std::size_t steps_to_ground_state{0};
    bool reached_ground_state{false};
    uint64_t ground_state_hits{0};
    double ground_state_occupancy_fraction{0.0}; // Not ESS or independent samples.
    double final_network_energy{0.0};
    uint64_t num_steps{0};
    double coupling_scale{0.0};
    uint64_t network_pair_checks{0};
    uint64_t topology_pair_checks{0};
    uint64_t topology_sort_comparisons{0};
    uint64_t random_words{0};
};

// Generate deterministic Ising spin-glass problem instances given a seed
inline ProblemInstance generate_problem_instance(std::size_t N, double coupling_scale, uint64_t seed) {
    ProblemInstance prob;
    prob.num_nodes = N;
    prob.h_fields.resize(N, 0.0);
    prob.J_couplings.assign(N, std::vector<double>(N, 0.0));

    SimplePRNG prng(seed ^ problem_stream);

    for (std::size_t i = 0; i < N; ++i) {
        prob.h_fields[i] = prng.uniform_real(-0.5, 0.5);
    }

    std::size_t side = static_cast<std::size_t>(std::ceil(std::sqrt(static_cast<double>(N))));

    for (std::size_t i = 0; i < N; ++i) {
        std::size_t r = i / side;
        std::size_t c = i % side;

        if (c + 1 < side) {
            std::size_t j = r * side + (c + 1);
            if (j < N) {
                double val = (prng.uniform_int(0, 1) == 1 ? 1.0 : -1.0) * coupling_scale;
                prob.J_couplings[i][j] = val;
                prob.J_couplings[j][i] = val;
            }
        }
        if (r + 1 < side) {
            std::size_t j = (r + 1) * side + c;
            if (j < N) {
                double val = (prng.uniform_int(0, 1) == 1 ? 1.0 : -1.0) * coupling_scale;
                prob.J_couplings[i][j] = val;
                prob.J_couplings[j][i] = val;
            }
        }
    }

    return prob;
}

// Immutable task Hamiltonian. Computational edges/couplings never enter here.
inline double task_energy(const ProblemInstance& prob, const std::vector<int>& states) {
    if (states.size() != prob.num_nodes) throw std::invalid_argument("state dimension");
    double energy = 0.0;
    for (std::size_t i = 0; i < prob.num_nodes; ++i) {
        energy -= prob.h_fields[i] * states[i];
        for (std::size_t j = i + 1; j < prob.num_nodes; ++j)
            energy -= prob.J_couplings[i][j] * states[i] * states[j];
    }
    return energy;
}
inline void validate_problem(const ProblemInstance& prob) {
    const auto n = prob.num_nodes;
    if (n == 0 || n > 16 || prob.h_fields.size() != n || prob.J_couplings.size() != n)
        throw std::invalid_argument("exact reference supports well-formed N=1..16 only");
    for (std::size_t i = 0; i < n; ++i) {
        if (!std::isfinite(prob.h_fields[i]) || prob.J_couplings[i].size() != n)
            throw std::invalid_argument("nonfinite or malformed task");
    }
    for (std::size_t i = 0; i < n; ++i) {
        if (prob.J_couplings[i][i] != 0) throw std::invalid_argument("self coupling unsupported");
        for (std::size_t j = 0; j < n; ++j)
            if (!std::isfinite(prob.J_couplings[i][j]) || prob.J_couplings[i][j] != prob.J_couplings[j][i])
                throw std::invalid_argument("task couplings must be finite and symmetric");
    }
}
// Complete numeric instance identity, including every field/coupling and alphabet.
inline double compute_exact_ground_state(const ProblemInstance& prob, ElementType elem) {
    validate_problem(prob);
    using Key = std::tuple<std::size_t, int, std::vector<double>, std::vector<std::vector<double>>>;
    static thread_local std::map<Key, double> cache;
    const Key key{prob.num_nodes, static_cast<int>(elem), prob.h_fields, prob.J_couplings};
    const auto found = cache.find(key);
    if (found != cache.end()) return found->second;
    // E is affine in each coordinate with all others fixed: replacing a zero
    // trit by a suitable endpoint cannot increase E. Thus the ternary minimum
    // equals the binary minimum for THIS Hamiltonian (no onsite s_i^2 term).
    double best = std::numeric_limits<double>::infinity();
    std::vector<int> spins(prob.num_nodes);
    for (uint64_t mask = 0; mask < (uint64_t{1} << prob.num_nodes); ++mask) {
        for (std::size_t i = 0; i < prob.num_nodes; ++i) spins[i] = (mask >> i) & 1 ? 1 : -1;
        best = std::min(best, task_energy(prob, spins));
    }
    cache.emplace(key, best);
    return best;
}

class NetworkSimulator {
    friend struct IntegrityAccess; // experiment-only white-box fixtures
public:
    NetworkSimulator(const ProblemInstance& problem, const SimulationConfig& config)
        : prob_(problem), cfg_(config), prng_(config.seed ^ proposal_stream),
          acceptance_prng_(config.seed ^ acceptance_stream),
          topology_prng_(config.seed ^ topology_stream),
          initialization_prng_(config.seed ^ initialization_stream),
          network_couplings_(problem.J_couplings) {

        N_ = prob_.num_nodes;
        validate_problem(prob_);
        if (cfg_.matched_state_capacity)
            throw std::invalid_argument("H2 capacity matching is unimplemented; extra unused nodes are not a matched task");
        if (cfg_.num_nodes != N_ || cfg_.num_steps == 0 ||
            !std::isfinite(cfg_.temperature) ||
            (cfg_.dynamics_type == DynamicsType::Stochastic && cfg_.temperature <= 0) ||
            !std::isfinite(cfg_.coupling_scale) || cfg_.coupling_scale < 0)
            throw std::invalid_argument("invalid simulation configuration");
        for (double cost : {cfg_.cost_spin_update, cfg_.cost_comm_per_degree, cfg_.cost_topology_update})
            if (!std::isfinite(cost) || cost < 0) throw std::invalid_argument("invalid cost");

        states_.resize(N_, 1);
        active_adj_.assign(N_, std::vector<int>(N_, 0));

        for (std::size_t i = 0; i < prob_.num_nodes && i < N_; ++i) {
            for (std::size_t j = 0; j < prob_.num_nodes && j < N_; ++j) {
                if (prob_.J_couplings[i][j] != 0.0) {
                    active_adj_[i][j] = 1;
                }
            }
        }

        if (cfg_.element_type == ElementType::Binary) {
            for (std::size_t i = 0; i < N_; ++i) {
                states_[i] = initialization_prng_.uniform_int(0, 1) ? 1 : -1;
            }
        } else {
            for (std::size_t i = 0; i < N_; ++i) {
                int r = initialization_prng_.uniform_int(0, 2);
                states_[i] = (r == 0) ? -1 : ((r == 1) ? 0 : 1);
            }
        }

        ground_state_energy_ = compute_exact_ground_state(prob_, cfg_.element_type);
    }

    double calculate_current_energy() const { return task_energy(prob_, states_); }

    double calculate_network_energy() const {
        double E = 0.0;
        for (std::size_t i = 0; i < N_ && i < prob_.num_nodes; ++i) {
            E -= prob_.h_fields[i] * states_[i];
            for (std::size_t j = i + 1; j < N_ && j < prob_.num_nodes; ++j) {
                if (active_adj_[i][j]) {
                    double J = network_couplings_[i][j];
                    if (J == 0.0) J = cfg_.coupling_scale; // Fallback interaction for rewired edges
                    E -= J * states_[i] * states_[j];
                }
            }
        }
        return E;
    }

    const std::vector<std::vector<int>>& trajectory() const { return trajectory_; }

    ExecutionMetrics run() {
        ExecutionMetrics metrics;
        metrics.element_type = cfg_.element_type;
        metrics.topology_type = cfg_.topology_type;
        metrics.dynamics_type = cfg_.dynamics_type;
        metrics.num_nodes = N_;
        metrics.seed = cfg_.seed;
        metrics.temperature = cfg_.temperature;
        metrics.cost_topology_update = cfg_.cost_topology_update;
        metrics.ground_state_energy = ground_state_energy_;
        metrics.num_steps = cfg_.num_steps;
        metrics.coupling_scale = cfg_.coupling_scale;
        if (std::abs(calculate_current_energy() - ground_state_energy_) < 1e-9)
            metrics.reached_ground_state = true; // first hit at initial step 0

        trajectory_.clear();
        trajectory_.push_back(states_);
        double total_energy = 0.0;
        uint64_t gs_hits = 0;

        for (std::size_t step = 0; step < cfg_.num_steps; ++step) {
            std::size_t target_node = static_cast<std::size_t>(prng_.uniform_int(0, static_cast<int>(N_ - 1)));

            int old_s = states_[target_node];
            int new_s = old_s;

            if (cfg_.element_type == ElementType::Binary) {
                new_s = (old_s == 1) ? -1 : 1;
            } else {
                double u = prng_.uniform_real(0.0, 1.0);
                if (old_s == 0) {
                    new_s = u < 0.5 ? -1 : 1;
                } else if (old_s == 1) {
                    new_s = u < 0.5 ? -1 : 0;
                } else {
                    new_s = u < 0.5 ? 0 : 1;
                }
            }

            double current_E = calculate_network_energy();
            states_[target_node] = new_s;
            double next_E = calculate_network_energy();
            double dE = next_E - current_E;

            metrics.spin_update_ops++;
            metrics.network_pair_checks += N_ * (N_ - 1); // two full network-energy scans

            std::size_t degree = 0;
            for (std::size_t j = 0; j < N_; ++j) {
                if (active_adj_[target_node][j]) degree++;
            }
            metrics.comm_ops += degree;

            bool accept = false;
            if (cfg_.dynamics_type == DynamicsType::Deterministic) {
                accept = (dE <= 0.0);
            } else {
                if (dE <= 0.0) {
                    accept = true;
                } else {
                    double prob = std::exp(-dE / cfg_.temperature);
                    accept = (acceptance_prng_.uniform_real(0.0, 1.0) < prob);
                }
            }

            if (!accept) {
                states_[target_node] = old_s;
            }

            if (step > 0 && (step % 10 == 0)) {
                perform_topology_adaptation(metrics);
            }

            trajectory_.push_back(states_);
            double step_E = calculate_current_energy();
            total_energy += step_E;

            if (std::abs(step_E - ground_state_energy_) < 1e-9) {
                gs_hits++;
                if (!metrics.reached_ground_state) {
                    metrics.reached_ground_state = true;
                    metrics.steps_to_ground_state = step + 1;
                }
            }
        }

        metrics.final_energy = calculate_current_energy();
        const double gap = metrics.final_energy - ground_state_energy_;
        if (gap < -1e-9) throw std::runtime_error("task energy below exact reference");
        metrics.energy_error = std::max(0.0, gap);
        metrics.final_network_energy = calculate_network_energy();
        metrics.mean_energy = total_energy / static_cast<double>(cfg_.num_steps);
        metrics.ground_state_hits = gs_hits;
        metrics.ground_state_occupancy_fraction = static_cast<double>(gs_hits) / static_cast<double>(cfg_.num_steps);
        metrics.random_words = prng_.draws + acceptance_prng_.draws + topology_prng_.draws + initialization_prng_.draws;

        metrics.total_cost = (metrics.spin_update_ops * cfg_.cost_spin_update) +
                             (metrics.comm_ops * cfg_.cost_comm_per_degree) +
                             (metrics.topology_update_ops * cfg_.cost_topology_update);

        return metrics;
    }

private:
    void perform_topology_adaptation(ExecutionMetrics& metrics) {
        if (cfg_.topology_type == TopologyType::Fixed) {
            return;
        }

        if (cfg_.topology_type == TopologyType::AdaptiveLocal) {
            std::size_t u = static_cast<std::size_t>(topology_prng_.uniform_int(0, static_cast<int>(N_ - 1)));
            std::size_t v = static_cast<std::size_t>(topology_prng_.uniform_int(0, static_cast<int>(N_ - 1)));

            ++metrics.topology_pair_checks;
            if (u != v && active_adj_[u][v]) {
                double J_uv = network_couplings_[u][v];
                if (J_uv == 0.0) J_uv = cfg_.coupling_scale;
                double frustration = J_uv * states_[u] * states_[v];
                if (frustration < 0.0) {
                    std::size_t w = static_cast<std::size_t>(topology_prng_.uniform_int(0, static_cast<int>(N_ - 1)));
                    if (w != u && w != v && !active_adj_[u][w]) {
                        active_adj_[u][v] = 0;
                        active_adj_[v][u] = 0;
                        active_adj_[u][w] = 1;
                        active_adj_[w][u] = 1;
                        if (network_couplings_[u][w] == 0.0) {
                            double new_J = (topology_prng_.uniform_int(0, 1) == 1 ? 1.0 : -1.0) * cfg_.coupling_scale;
                            network_couplings_[u][w] = new_J;
                            network_couplings_[w][u] = new_J;
                        }
                        metrics.topology_update_ops += 2;
                    }
                }
            }
        } else if (cfg_.topology_type == TopologyType::RandomAdaptive) {
            std::size_t u = static_cast<std::size_t>(topology_prng_.uniform_int(0, static_cast<int>(N_ - 1)));
            std::size_t v = static_cast<std::size_t>(topology_prng_.uniform_int(0, static_cast<int>(N_ - 1)));
            std::size_t w = static_cast<std::size_t>(topology_prng_.uniform_int(0, static_cast<int>(N_ - 1)));

            ++metrics.topology_pair_checks;
            if (u != v && active_adj_[u][v] && w != u && !active_adj_[u][w]) {
                active_adj_[u][v] = 0;
                active_adj_[v][u] = 0;
                active_adj_[u][w] = 1;
                active_adj_[w][u] = 1;
                if (network_couplings_[u][w] == 0.0) {
                    double new_J = (topology_prng_.uniform_int(0, 1) == 1 ? 1.0 : -1.0) * cfg_.coupling_scale;
                    network_couplings_[u][w] = new_J;
                    network_couplings_[w][u] = new_J;
                }
                metrics.topology_update_ops += 2;
            }
        } else if (cfg_.topology_type == TopologyType::CentralizedAdaptive) {
            std::vector<std::pair<double, std::pair<std::size_t, std::size_t>>> potential_edges;
            for (std::size_t i = 0; i < N_ && i < prob_.num_nodes; ++i) {
                for (std::size_t j = i + 1; j < N_ && j < prob_.num_nodes; ++j) {
                    double J_ij = network_couplings_[i][j];
                    if (J_ij == 0.0) J_ij = cfg_.coupling_scale;
                    double align_benefit = J_ij * states_[i] * states_[j];
                    potential_edges.push_back({align_benefit, {i, j}});
                }
            }
            metrics.topology_pair_checks += potential_edges.size();
            std::sort(potential_edges.begin(), potential_edges.end(), [&metrics](const auto& a, const auto& b) {
                ++metrics.topology_sort_comparisons;
                return a > b;
            });

            std::size_t target_edges = N_;
            const auto previous_adj = active_adj_;
            active_adj_.assign(N_, std::vector<int>(N_, 0));
            std::size_t added = 0;
            for (const auto& item : potential_edges) {
                if (added >= target_edges) break;
                std::size_t u = item.second.first;
                std::size_t v = item.second.second;
                active_adj_[u][v] = 1;
                active_adj_[v][u] = 1;
                added++;
            }
            for (std::size_t i = 0; i < N_; ++i)
                for (std::size_t j = i + 1; j < N_; ++j)
                    metrics.topology_update_ops += previous_adj[i][j] != active_adj_[i][j];
        }
    }

    const ProblemInstance prob_;
    SimulationConfig cfg_;
    SimplePRNG prng_;
    SimplePRNG acceptance_prng_;
    SimplePRNG topology_prng_;
    SimplePRNG initialization_prng_;
    std::vector<std::vector<double>> network_couplings_;
    std::size_t N_{0};
    std::vector<int> states_;
    std::vector<std::vector<int>> trajectory_;
    std::vector<std::vector<int>> active_adj_;
    double ground_state_energy_{0.0};
};

inline std::string element_type_to_string(ElementType t) {
    return (t == ElementType::Binary) ? "Binary" : "Ternary";
}

inline std::string topology_type_to_string(TopologyType t) {
    switch (t) {
        case TopologyType::Fixed: return "Fixed";
        case TopologyType::AdaptiveLocal: return "AdaptiveLocal";
        case TopologyType::CentralizedAdaptive: return "CentralizedAdaptive";
        case TopologyType::RandomAdaptive: return "RandomAdaptive";
    }
    return "Unknown";
}

inline std::string dynamics_type_to_string(DynamicsType d) {
    return (d == DynamicsType::Stochastic) ? "Stochastic" : "Deterministic";
}

} // namespace t81::s02

#endif // T81_EXPERIMENTS_S02_STOCHASTIC_TOPOLOGY_HPP
