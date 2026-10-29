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
#include <utility>
#include <vector>

namespace t81::s02 {

struct SimplePRNG {
    uint64_t state;
    explicit SimplePRNG(uint64_t seed) : state(seed ^ 0x9E3779B97F4A7C15ULL) {}

    uint64_t next() {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return state;
    }

    double uniform_real(double min_val, double max_val) {
        double u = static_cast<double>(next() >> 11) / 9007199254740992.0;
        return min_val + u * (max_val - min_val);
    }

    int uniform_int(int min_val, int max_val) {
        return min_val + static_cast<int>(next() % static_cast<uint64_t>(max_val - min_val + 1));
    }
};

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
    double effective_sampling_efficiency{0.0}; // Ground state hits / total steps
};

// Generate deterministic Ising spin-glass problem instances given a seed
inline ProblemInstance generate_problem_instance(std::size_t N, double coupling_scale, uint64_t seed) {
    ProblemInstance prob;
    prob.num_nodes = N;
    prob.h_fields.resize(N, 0.0);
    prob.J_couplings.assign(N, std::vector<double>(N, 0.0));

    SimplePRNG prng(seed);

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

// Cached ground state calculation
inline double compute_exact_ground_state(const ProblemInstance& prob, ElementType elem) {
    static std::map<std::tuple<std::size_t, int>, double> cache;
    std::tuple<std::size_t, int> key = {prob.num_nodes, static_cast<int>(elem)};

    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second;
    }

    std::size_t N = prob.num_nodes;
    double min_energy = std::numeric_limits<double>::infinity();

    if (elem == ElementType::Binary) {
        uint64_t total_states = 1ULL << N;
        for (uint64_t mask = 0; mask < total_states; ++mask) {
            double E = 0.0;
            for (std::size_t i = 0; i < N; ++i) {
                int s_i = ((mask >> i) & 1) ? 1 : -1;
                E -= prob.h_fields[i] * s_i;
                for (std::size_t j = i + 1; j < N; ++j) {
                    if (std::abs(prob.J_couplings[i][j]) > 1e-9) {
                        int s_j = ((mask >> j) & 1) ? 1 : -1;
                        E -= prob.J_couplings[i][j] * s_i * s_j;
                    }
                }
            }
            if (E < min_energy) min_energy = E;
        }
    } else { // Ternary
        uint64_t total_states = 1ULL;
        for (std::size_t i = 0; i < N; ++i) total_states *= 3;

        std::vector<int> s(N);
        for (uint64_t state = 0; state < total_states; ++state) {
            uint64_t tmp = state;
            for (std::size_t i = 0; i < N; ++i) {
                int digit = static_cast<int>(tmp % 3);
                s[i] = (digit == 0) ? -1 : ((digit == 1) ? 0 : 1);
                tmp /= 3;
            }
            double E = 0.0;
            for (std::size_t i = 0; i < N; ++i) {
                E -= prob.h_fields[i] * s[i];
                for (std::size_t j = i + 1; j < N; ++j) {
                    if (std::abs(prob.J_couplings[i][j]) > 1e-9) {
                        E -= prob.J_couplings[i][j] * s[i] * s[j];
                    }
                }
            }
            if (E < min_energy) min_energy = E;
        }
    }

    cache[key] = min_energy;
    return min_energy;
}

class NetworkSimulator {
public:
    NetworkSimulator(const ProblemInstance& problem, const SimulationConfig& config)
        : prob_(problem), cfg_(config), prng_(config.seed) {

        N_ = prob_.num_nodes;
        if (cfg_.matched_state_capacity && cfg_.element_type == ElementType::Binary) {
            N_ = static_cast<std::size_t>(std::ceil(static_cast<double>(N_) * 1.584962500721156));
        }

        states_.resize(N_, 1);
        active_adj_.assign(N_, std::vector<int>(N_, 0));

        for (std::size_t i = 0; i < prob_.num_nodes && i < N_; ++i) {
            for (std::size_t j = 0; j < prob_.num_nodes && j < N_; ++j) {
                if (std::abs(prob_.J_couplings[i][j]) > 1e-9) {
                    active_adj_[i][j] = 1;
                }
            }
        }

        if (cfg_.element_type == ElementType::Binary) {
            for (std::size_t i = 0; i < N_; ++i) {
                states_[i] = prng_.uniform_int(0, 1) ? 1 : -1;
            }
        } else {
            for (std::size_t i = 0; i < N_; ++i) {
                int r = prng_.uniform_int(0, 2);
                states_[i] = (r == 0) ? -1 : ((r == 1) ? 0 : 1);
            }
        }

        ground_state_energy_ = compute_exact_ground_state(prob_, cfg_.element_type);
    }

    double calculate_current_energy() const {
        double E = 0.0;
        for (std::size_t i = 0; i < N_ && i < prob_.num_nodes; ++i) {
            E -= prob_.h_fields[i] * states_[i];
            for (std::size_t j = i + 1; j < N_ && j < prob_.num_nodes; ++j) {
                if (active_adj_[i][j]) {
                    double J = prob_.J_couplings[i][j];
                    if (std::abs(J) < 1e-9) J = cfg_.coupling_scale; // Fallback interaction for rewired edges
                    E -= J * states_[i] * states_[j];
                }
            }
        }
        return E;
    }

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

            double current_E = calculate_current_energy();
            states_[target_node] = new_s;
            double next_E = calculate_current_energy();
            double dE = next_E - current_E;

            metrics.spin_update_ops++;

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
                    accept = (prng_.uniform_real(0.0, 1.0) < prob);
                }
            }

            if (!accept) {
                states_[target_node] = old_s;
            }

            if (step > 0 && (step % 10 == 0)) {
                perform_topology_adaptation(metrics);
            }

            double step_E = calculate_current_energy();
            total_energy += step_E;

            if (std::abs(step_E - ground_state_energy_) < 1e-6) {
                gs_hits++;
                if (!metrics.reached_ground_state) {
                    metrics.reached_ground_state = true;
                    metrics.steps_to_ground_state = step + 1;
                }
            }
        }

        metrics.final_energy = calculate_current_energy();
        metrics.energy_error = std::abs(metrics.final_energy - ground_state_energy_);
        metrics.mean_energy = total_energy / static_cast<double>(cfg_.num_steps);
        metrics.effective_sampling_efficiency = static_cast<double>(gs_hits) / static_cast<double>(cfg_.num_steps);

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
            std::size_t u = static_cast<std::size_t>(prng_.uniform_int(0, static_cast<int>(N_ - 1)));
            std::size_t v = static_cast<std::size_t>(prng_.uniform_int(0, static_cast<int>(N_ - 1)));

            if (u != v && active_adj_[u][v]) {
                double J_uv = prob_.J_couplings[u][v];
                if (std::abs(J_uv) < 1e-9) J_uv = cfg_.coupling_scale;
                double frustration = J_uv * states_[u] * states_[v];
                if (frustration < 0.0) {
                    std::size_t w = static_cast<std::size_t>(prng_.uniform_int(0, static_cast<int>(N_ - 1)));
                    if (w != u && w != v && !active_adj_[u][w]) {
                        active_adj_[u][v] = 0;
                        active_adj_[v][u] = 0;
                        active_adj_[u][w] = 1;
                        active_adj_[w][u] = 1;
                        if (std::abs(prob_.J_couplings[u][w]) < 1e-9) {
                            double new_J = (prng_.uniform_int(0, 1) == 1 ? 1.0 : -1.0) * cfg_.coupling_scale;
                            prob_.J_couplings[u][w] = new_J;
                            prob_.J_couplings[w][u] = new_J;
                        }
                        metrics.topology_update_ops += 2;
                    }
                }
            }
        } else if (cfg_.topology_type == TopologyType::RandomAdaptive) {
            std::size_t u = static_cast<std::size_t>(prng_.uniform_int(0, static_cast<int>(N_ - 1)));
            std::size_t v = static_cast<std::size_t>(prng_.uniform_int(0, static_cast<int>(N_ - 1)));
            std::size_t w = static_cast<std::size_t>(prng_.uniform_int(0, static_cast<int>(N_ - 1)));

            if (u != v && active_adj_[u][v] && w != u && !active_adj_[u][w]) {
                active_adj_[u][v] = 0;
                active_adj_[v][u] = 0;
                active_adj_[u][w] = 1;
                active_adj_[w][u] = 1;
                if (std::abs(prob_.J_couplings[u][w]) < 1e-9) {
                    double new_J = (prng_.uniform_int(0, 1) == 1 ? 1.0 : -1.0) * cfg_.coupling_scale;
                    prob_.J_couplings[u][w] = new_J;
                    prob_.J_couplings[w][u] = new_J;
                }
                metrics.topology_update_ops += 2;
            }
        } else if (cfg_.topology_type == TopologyType::CentralizedAdaptive) {
            std::vector<std::pair<double, std::pair<std::size_t, std::size_t>>> potential_edges;
            for (std::size_t i = 0; i < N_ && i < prob_.num_nodes; ++i) {
                for (std::size_t j = i + 1; j < N_ && j < prob_.num_nodes; ++j) {
                    double J_ij = prob_.J_couplings[i][j];
                    if (std::abs(J_ij) < 1e-9) J_ij = cfg_.coupling_scale;
                    double align_benefit = J_ij * states_[i] * states_[j];
                    potential_edges.push_back({align_benefit, {i, j}});
                }
            }
            std::sort(potential_edges.rbegin(), potential_edges.rend());

            std::size_t target_edges = N_;
            active_adj_.assign(N_, std::vector<int>(N_, 0));
            std::size_t added = 0;
            for (const auto& item : potential_edges) {
                if (added >= target_edges) break;
                std::size_t u = item.second.first;
                std::size_t v = item.second.second;
                active_adj_[u][v] = 1;
                active_adj_[v][u] = 1;
                added++;
                metrics.topology_update_ops += 1;
            }
        }
    }

    ProblemInstance prob_;
    SimulationConfig cfg_;
    SimplePRNG prng_;
    std::size_t N_{0};
    std::vector<int> states_;
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
