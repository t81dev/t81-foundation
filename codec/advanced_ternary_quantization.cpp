#include "t81/codec/advanced_ternary_quantization.hpp"
#include <algorithm>
#include <chrono>
#include <numeric>
#include <queue>
#include <random>
#include <unordered_set>

namespace t81::codec {

// AdvancedT3KQuantizer implementation
AdvancedT3KQuantizer::AdvancedT3KQuantizer(const T3KConfig& config) : config_(config) {}

std::vector<int8_t> AdvancedT3KQuantizer::quantize(const std::vector<float>& input) {
  if (input.empty()) {
    return {};
  }

  std::vector<int8_t> result;

  switch (config_.strategy) {
    case QuantizationStrategy::THRESHOLD:
      result = quantize_threshold(input);
      break;
    case QuantizationStrategy::PERCENTILE:
      result = quantize_percentile(input);
      break;
    case QuantizationStrategy::K_MEANS:
      result = quantize_kmeans(input);
      break;
    case QuantizationStrategy::ADAPTIVE:
      result = quantize_adaptive(input);
      break;
    case QuantizationStrategy::ENTROPY_MIN:
      result = quantize_entropy_min(input);
      break;
    default:
      result = quantize_adaptive(input);
      break;
  }

  // Calculate statistics
  last_stats_.total_elements = input.size();
  last_stats_.positive_count = std::count(result.begin(), result.end(), 1);
  last_stats_.negative_count = std::count(result.begin(), result.end(), -1);
  last_stats_.zero_count = std::count(result.begin(), result.end(), 0);
  last_stats_.sparsity_ratio = static_cast<float>(last_stats_.zero_count) / input.size();

  // Calculate MSE and SNR
  auto dequantized = dequantize(result);
  last_stats_.mean_squared_error = calculate_mse(input, dequantized);
  last_stats_.signal_to_noise_ratio = calculate_snr(input, dequantized);
  last_stats_.compression_ratio = 4.0f;  // float32 to int8
  last_stats_.entropy_bits = calculate_entropy(result);

  return result;
}

std::vector<float> AdvancedT3KQuantizer::dequantize(const std::vector<int8_t>& input) {
  std::vector<float> result;
  result.reserve(input.size());

  for (int8_t val : input) {
    switch (val) {
      case -1:
        result.push_back(-1.0f);
        break;
      case 0:
        result.push_back(0.0f);
        break;
      case 1:
        result.push_back(1.0f);
        break;
      default:
        result.push_back(0.0f);
        break;  // Handle invalid values
    }
  }

  return result;
}

std::vector<std::vector<int8_t>> AdvancedT3KQuantizer::quantize_batch(
    const std::vector<std::vector<float>>& inputs) {
  std::vector<std::vector<int8_t>> results;
  results.reserve(inputs.size());

  for (const auto& input : inputs) {
    results.push_back(quantize(input));
  }

  return results;
}

std::vector<std::vector<float>> AdvancedT3KQuantizer::dequantize_batch(
    const std::vector<std::vector<int8_t>>& inputs) {
  std::vector<std::vector<float>> results;
  results.reserve(inputs.size());

  for (const auto& input : inputs) {
    results.push_back(dequantize(input));
  }

  return results;
}

void AdvancedT3KQuantizer::learn_from_data(const std::vector<float>& training_data) {
  if (training_data.empty()) {
    return;
  }

  analyze_distribution(training_data);
  calculate_adaptive_thresholds(training_data);
  thresholds_calculated_ = true;
}

void AdvancedT3KQuantizer::learn_from_batch(const std::vector<std::vector<float>>& training_batch) {
  std::vector<float> all_data;
  for (const auto& batch : training_batch) {
    all_data.insert(all_data.end(), batch.begin(), batch.end());
  }

  learn_from_data(all_data);
}

void AdvancedT3KQuantizer::analyze_distribution(const std::vector<float>& data) {
  // Calculate basic statistics
  float mean = std::accumulate(data.begin(), data.end(), 0.0f) / data.size();

  std::vector<float> squared_diffs;
  squared_diffs.reserve(data.size());
  for (float val : data) {
    squared_diffs.push_back((val - mean) * (val - mean));
  }
  float variance = std::accumulate(squared_diffs.begin(), squared_diffs.end(), 0.0f) / data.size();
  float std_dev = std::sqrt(variance);

  // Update config based on data distribution
  if (config_.strategy == QuantizationStrategy::ADAPTIVE) {
    config_.threshold_positive = mean + 0.5f * std_dev;
    config_.threshold_negative = mean - 0.5f * std_dev;
  }
}

float AdvancedT3KQuantizer::calculate_mse(const std::vector<float>& original,
                                          const std::vector<float>& quantized) {
  if (original.size() != quantized.size()) {
    return std::numeric_limits<float>::infinity();
  }

  float mse = 0.0f;
  for (size_t i = 0; i < original.size(); ++i) {
    float diff = original[i] - quantized[i];
    mse += diff * diff;
  }

  return mse / original.size();
}

float AdvancedT3KQuantizer::calculate_snr(const std::vector<float>& original,
                                          const std::vector<float>& quantized) {
  float signal_power = 0.0f;
  float noise_power = 0.0f;

  for (size_t i = 0; i < original.size(); ++i) {
    signal_power += original[i] * original[i];
    float noise = original[i] - quantized[i];
    noise_power += noise * noise;
  }

  if (noise_power == 0.0f) {
    return std::numeric_limits<float>::infinity();
  }

  return 10.0f * std::log10(signal_power / noise_power);
}

float AdvancedT3KQuantizer::calculate_entropy(const std::vector<int8_t>& data) {
  if (data.empty()) {
    return 0.0f;
  }

  // Count occurrences of each value
  std::unordered_map<int8_t, int> counts;
  for (int8_t val : data) {
    counts[val]++;
  }

  // Calculate entropy
  float entropy = 0.0f;
  float total = static_cast<float>(data.size());

  for (const auto& [value, count] : counts) {
    float probability = count / total;
    if (probability > 0.0f) {
      entropy -= probability * std::log2(probability);
    }
  }

  return entropy;
}

// Private implementation methods
std::vector<int8_t> AdvancedT3KQuantizer::quantize_threshold(const std::vector<float>& input) {
  std::vector<int8_t> result;
  result.reserve(input.size());

  for (float val : input) {
    if (val > config_.threshold_positive) {
      result.push_back(1);
    } else if (val < config_.threshold_negative) {
      result.push_back(-1);
    } else {
      result.push_back(0);
    }
  }

  return result;
}

std::vector<int8_t> AdvancedT3KQuantizer::quantize_percentile(const std::vector<float>& input) {
  std::vector<float> sorted_input = input;
  std::sort(sorted_input.begin(), sorted_input.end());

  size_t low_idx = static_cast<size_t>(input.size() * config_.percentile_low / 100.0f);
  size_t high_idx = static_cast<size_t>(input.size() * config_.percentile_high / 100.0f);

  float low_threshold = sorted_input[low_idx];
  float high_threshold = sorted_input[high_idx];

  std::vector<int8_t> result;
  result.reserve(input.size());

  for (float val : input) {
    if (val > high_threshold) {
      result.push_back(1);
    } else if (val < low_threshold) {
      result.push_back(-1);
    } else {
      result.push_back(0);
    }
  }

  return result;
}

std::vector<int8_t> AdvancedT3KQuantizer::quantize_kmeans(const std::vector<float>& input) {
  auto clusters = kmeans_cluster(input, 3);

  // Sort clusters by centroid to maintain consistent ordering
  std::sort(clusters.begin(), clusters.end(),
            [](const KMeansCluster& a, const KMeansCluster& b) { return a.centroid < b.centroid; });

  std::vector<int8_t> result;
  result.reserve(input.size());

  for (float val : input) {
    // Find nearest cluster
    size_t nearest_cluster = 0;
    float min_distance = std::abs(val - clusters[0].centroid);

    for (size_t i = 1; i < clusters.size(); ++i) {
      float distance = std::abs(val - clusters[i].centroid);
      if (distance < min_distance) {
        min_distance = distance;
        nearest_cluster = i;
      }
    }

    // Map cluster index to ternary value
    if (nearest_cluster == 0) {
      result.push_back(-1);
    } else if (nearest_cluster == clusters.size() - 1) {
      result.push_back(1);
    } else {
      result.push_back(0);
    }
  }

  return result;
}

std::vector<int8_t> AdvancedT3KQuantizer::quantize_adaptive(const std::vector<float>& input) {
  if (!thresholds_calculated_) {
    calculate_adaptive_thresholds(input);
  }

  return quantize_threshold(input);
}

std::vector<int8_t> AdvancedT3KQuantizer::quantize_entropy_min(const std::vector<float>& input) {
  auto optimal_thresholds = find_optimal_thresholds_entropy(input);

  std::vector<int8_t> result;
  result.reserve(input.size());

  for (float val : input) {
    if (val > optimal_thresholds[0]) {
      result.push_back(1);
    } else if (val < optimal_thresholds[1]) {
      result.push_back(-1);
    } else {
      result.push_back(0);
    }
  }

  return result;
}

std::vector<AdvancedT3KQuantizer::KMeansCluster> AdvancedT3KQuantizer::kmeans_cluster(
    const std::vector<float>& data, int k) {
  // Initialize centroids using k-means++ algorithm
  std::vector<KMeansCluster> clusters(k);
  std::vector<float> centroids(k);

  // Choose first centroid randomly
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, data.size() - 1);
  centroids[0] = data[dis(gen)];

  // Choose remaining centroids
  for (int i = 1; i < k; ++i) {
    std::vector<float> distances(data.size());
    std::vector<float> cumulative_distances(data.size());

    // Calculate distances to nearest existing centroid
    for (size_t j = 0; j < data.size(); ++j) {
      float min_dist = std::numeric_limits<float>::infinity();
      for (int l = 0; l < i; ++l) {
        float dist = std::abs(data[j] - centroids[l]);
        min_dist = std::min(min_dist, dist);
      }
      distances[j] = min_dist * min_dist;  // Square for probability
    }

    // Calculate cumulative distribution
    cumulative_distances[0] = distances[0];
    for (size_t j = 1; j < data.size(); ++j) {
      cumulative_distances[j] = cumulative_distances[j - 1] + distances[j];
    }

    // Choose next centroid
    std::uniform_real_distribution<> prob_dis(0.0f, cumulative_distances.back());
    float random_val = prob_dis(gen);

    for (size_t j = 0; j < data.size(); ++j) {
      if (random_val <= cumulative_distances[j]) {
        centroids[i] = data[j];
        break;
      }
    }
  }

  // K-means iterations
  for (int iter = 0; iter < config_.max_iterations; ++iter) {
    // Assign points to clusters
    for (auto& cluster : clusters) {
      cluster.points.clear();
    }

    for (float point : data) {
      int nearest_cluster = 0;
      float min_distance = std::abs(point - centroids[0]);

      for (int i = 1; i < k; ++i) {
        float distance = std::abs(point - centroids[i]);
        if (distance < min_distance) {
          min_distance = distance;
          nearest_cluster = i;
        }
      }

      clusters[nearest_cluster].points.push_back(point);
    }

    // Update centroids
    bool converged = true;
    for (int i = 0; i < k; ++i) {
      if (!clusters[i].points.empty()) {
        float new_centroid =
            std::accumulate(clusters[i].points.begin(), clusters[i].points.end(), 0.0f) /
            clusters[i].points.size();

        if (std::abs(new_centroid - centroids[i]) > config_.convergence_tolerance) {
          converged = false;
        }

        centroids[i] = new_centroid;
        clusters[i].centroid = new_centroid;
      }
    }

    if (converged) {
      break;
    }
  }

  return clusters;
}

void AdvancedT3KQuantizer::calculate_adaptive_thresholds(const std::vector<float>& data) {
  // Calculate percentiles for adaptive thresholds
  auto percentiles = calculate_percentiles(data, {25.0f, 50.0f, 75.0f});

  float q1 = percentiles[0];
  float median = percentiles[1];
  float q3 = percentiles[2];

  // Set thresholds based on data distribution
  config_.threshold_negative = q1;
  config_.threshold_positive = q3;

  thresholds_calculated_ = true;
}

std::vector<float> AdvancedT3KQuantizer::find_optimal_thresholds_entropy(
    const std::vector<float>& data) {
  // Grid search for optimal thresholds that minimize entropy
  std::vector<float> sorted_data = data;
  std::sort(sorted_data.begin(), sorted_data.end());

  float best_entropy = std::numeric_limits<float>::infinity();
  std::vector<float> best_thresholds = {0.5f, -0.5f};

  // Search over reasonable threshold ranges
  float min_val = sorted_data.front();
  float max_val = sorted_data.back();

  for (float low = min_val; low <= max_val; low += (max_val - min_val) / 50.0f) {
    for (float high = low; high <= max_val; high += (max_val - min_val) / 50.0f) {
      // Quantize with current thresholds
      std::vector<int8_t> quantized;
      quantized.reserve(data.size());

      for (float val : data) {
        if (val > high) {
          quantized.push_back(1);
        } else if (val < low) {
          quantized.push_back(-1);
        } else {
          quantized.push_back(0);
        }
      }

      // Calculate entropy
      float entropy = calculate_entropy(quantized);

      if (entropy < best_entropy) {
        best_entropy = entropy;
        best_thresholds = {high, low};
      }
    }
  }

  return best_thresholds;
}

std::vector<float> AdvancedT3KQuantizer::calculate_percentiles(
    const std::vector<float>& data, const std::vector<float>& percentiles) {
  std::vector<float> sorted_data = data;
  std::sort(sorted_data.begin(), sorted_data.end());

  std::vector<float> results;
  results.reserve(percentiles.size());

  for (float percentile : percentiles) {
    size_t index = static_cast<size_t>((percentile / 100.0f) * (data.size() - 1));
    results.push_back(sorted_data[index]);
  }

  return results;
}

// TernaryTensorOps implementation
std::vector<int8_t> TernaryTensorOps::add(const std::vector<int8_t>& a,
                                          const std::vector<int8_t>& b) {
  if (a.size() != b.size()) {
    return {};
  }

  std::vector<int8_t> result;
  result.reserve(a.size());

  for (size_t i = 0; i < a.size(); ++i) {
    int sum = a[i] + b[i];
    // Clamp to ternary range
    if (sum > 1)
      result.push_back(1);
    else if (sum < -1)
      result.push_back(-1);
    else
      result.push_back(static_cast<int8_t>(sum));
  }

  return result;
}

std::vector<int8_t> TernaryTensorOps::multiply(const std::vector<int8_t>& a,
                                               const std::vector<int8_t>& b) {
  if (a.size() != b.size()) {
    return {};
  }

  std::vector<int8_t> result;
  result.reserve(a.size());

  for (size_t i = 0; i < a.size(); ++i) {
    result.push_back(static_cast<int8_t>(a[i] * b[i]));
  }

  return result;
}

std::vector<int8_t> TernaryTensorOps::scale(const std::vector<int8_t>& input, int8_t scalar) {
  std::vector<int8_t> result;
  result.reserve(input.size());

  for (int8_t val : input) {
    int product = val * scalar;
    // Clamp to ternary range
    if (product > 1)
      result.push_back(1);
    else if (product < -1)
      result.push_back(-1);
    else
      result.push_back(static_cast<int8_t>(product));
  }

  return result;
}

size_t TernaryTensorOps::count_ones(const std::vector<int8_t>& input) {
  return std::count(input.begin(), input.end(), 1);
}

size_t TernaryTensorOps::count_neg_ones(const std::vector<int8_t>& input) {
  return std::count(input.begin(), input.end(), -1);
}

size_t TernaryTensorOps::count_zeros(const std::vector<int8_t>& input) {
  return std::count(input.begin(), input.end(), 0);
}

float TernaryTensorOps::calculate_density(const std::vector<int8_t>& input) {
  if (input.empty()) {
    return 0.0f;
  }

  size_t non_zeros = input.size() - count_zeros(input);
  return static_cast<float>(non_zeros) / input.size();
}

// PackedTernaryStorage implementation
std::vector<uint8_t> PackedTernaryStorage::pack_ternary(const std::vector<int8_t>& input) {
  std::vector<uint8_t> packed;
  packed.reserve(packed_size(input.size()));

  for (size_t i = 0; i < input.size(); i += 3) {
    int8_t v1 = (i < input.size()) ? input[i] : 0;
    int8_t v2 = (i + 1 < input.size()) ? input[i + 1] : 0;
    int8_t v3 = (i + 2 < input.size()) ? input[i + 2] : 0;

    uint8_t packed_byte = pack_three_values(v1, v2, v3);
    packed.push_back(packed_byte);
  }

  return packed;
}

std::vector<int8_t> PackedTernaryStorage::unpack_ternary(const std::vector<uint8_t>& packed) {
  std::vector<int8_t> unpacked;
  unpacked.reserve(packed.size() * 3);

  for (uint8_t byte : packed) {
    auto [v1, v2, v3] = unpack_three_values(byte);
    unpacked.push_back(v1);
    unpacked.push_back(v2);
    unpacked.push_back(v3);
  }

  return unpacked;
}

uint8_t PackedTernaryStorage::pack_three_values(int8_t v1, int8_t v2, int8_t v3) {
  // Convert to 0, 1, 2 representation for packing
  uint8_t encoded_v1 = (v1 == -1) ? 0 : (v1 == 0) ? 1 : 2;
  uint8_t encoded_v2 = (v2 == -1) ? 0 : (v2 == 0) ? 1 : 2;
  uint8_t encoded_v3 = (v3 == -1) ? 0 : (v3 == 0) ? 1 : 2;

  // Pack: v1 (bits 0-1), v2 (bits 2-3), v3 (bits 4-5)
  return encoded_v1 | (encoded_v2 << 2) | (encoded_v3 << 4);
}

std::tuple<int8_t, int8_t, int8_t> PackedTernaryStorage::unpack_three_values(uint8_t packed) {
  // Unpack: v1 (bits 0-1), v2 (bits 2-3), v3 (bits 4-5)
  uint8_t encoded_v1 = packed & 0x03;
  uint8_t encoded_v2 = (packed >> 2) & 0x03;
  uint8_t encoded_v3 = (packed >> 4) & 0x03;

  // Convert back to -1, 0, 1 representation
  auto decode = [](uint8_t encoded) -> int8_t {
    return (encoded == 0) ? -1 : (encoded == 1) ? 0 : 1;
  };

  return {decode(encoded_v1), decode(encoded_v2), decode(encoded_v3)};
}

// TernaryQuantizationCache implementation
TernaryQuantizationCache::TernaryQuantizationCache(size_t max_size) : max_size_(max_size) {}

void TernaryQuantizationCache::put(const std::string& key,
                                   const std::vector<int8_t>& quantized_data) {
  evict_lru_if_needed();

  CacheEntry entry;
  entry.data = quantized_data;
  entry.last_access = std::chrono::steady_clock::now();
  entry.access_count = 1;

  cache_[key] = std::move(entry);
}

std::optional<std::vector<int8_t>> TernaryQuantizationCache::get(const std::string& key) {
  total_accesses_++;

  auto it = cache_.find(key);
  if (it != cache_.end()) {
    cache_hits_++;
    it->second.last_access = std::chrono::steady_clock::now();
    it->second.access_count++;
    return it->second.data;
  }

  return std::nullopt;
}

void TernaryQuantizationCache::remove(const std::string& key) { cache_.erase(key); }

void TernaryQuantizationCache::clear() {
  cache_.clear();
  total_accesses_ = 0;
  cache_hits_ = 0;
}

size_t TernaryQuantizationCache::size() const { return cache_.size(); }

size_t TernaryQuantizationCache::memory_usage() const {
  size_t total = 0;
  for (const auto& [key, entry] : cache_) {
    total += entry.data.size() * sizeof(int8_t);
  }
  return total;
}

float TernaryQuantizationCache::hit_rate() const {
  if (total_accesses_ == 0) {
    return 0.0f;
  }
  return static_cast<float>(cache_hits_) / total_accesses_;
}

void TernaryQuantizationCache::evict_lru_if_needed() {
  while (cache_.size() >= max_size_) {
    auto oldest_it = std::min_element(
        cache_.begin(), cache_.end(),
        [](const auto& a, const auto& b) { return a.second.last_access < b.second.last_access; });

    if (oldest_it != cache_.end()) {
      cache_.erase(oldest_it);
    } else {
      break;
    }
  }
}

// Utility functions implementation
namespace ternary_utils {

std::vector<TernaryType> to_ternary_type(const std::vector<int8_t>& input) {
  std::vector<TernaryType> result;
  result.reserve(input.size());

  for (int8_t val : input) {
    if (val == -1) {
      result.push_back(TernaryType::NEGATIVE);
    } else if (val == 0) {
      result.push_back(TernaryType::ZERO);
    } else if (val == 1) {
      result.push_back(TernaryType::POSITIVE);
    } else {
      result.push_back(TernaryType::ZERO);  // Default for invalid values
    }
  }

  return result;
}

std::vector<int8_t> from_ternary_type(const std::vector<TernaryType>& input) {
  std::vector<int8_t> result;
  result.reserve(input.size());

  for (TernaryType val : input) {
    result.push_back(static_cast<int8_t>(val));
  }

  return result;
}

bool is_valid_ternary(const std::vector<int8_t>& input) {
  return std::all_of(input.begin(), input.end(),
                     [](int8_t val) { return val == -1 || val == 0 || val == 1; });
}

bool is_valid_ternary(const std::vector<TernaryType>& input) {
  return std::all_of(input.begin(), input.end(), [](TernaryType val) {
    return val == TernaryType::NEGATIVE || val == TernaryType::ZERO || val == TernaryType::POSITIVE;
  });
}

float calculate_ternary_balance(const std::vector<int8_t>& input) {
  if (input.empty()) {
    return 0.0f;
  }

  size_t pos_count = std::count(input.begin(), input.end(), 1);
  size_t neg_count = std::count(input.begin(), input.end(), -1);

  return std::abs(static_cast<float>(pos_count - neg_count)) / input.size();
}

std::vector<float> calculate_transition_probabilities(const std::vector<int8_t>& input) {
  if (input.size() < 2) {
    return {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
  }

  // Transition matrix: from {-1,0,1} to {-1,0,1}
  std::vector<size_t> transitions(9, 0);  // 3x3 matrix flattened

  for (size_t i = 1; i < input.size(); ++i) {
    int8_t from_val = input[i - 1] + 1;  // Map to {0,1,2}
    int8_t to_val = input[i] + 1;        // Map to {0,1,2}

    if (from_val >= 0 && from_val < 3 && to_val >= 0 && to_val < 3) {
      transitions[from_val * 3 + to_val]++;
    }
  }

  // Convert to probabilities
  std::vector<float> probabilities(9);
  for (int i = 0; i < 3; ++i) {
    size_t row_sum = transitions[i * 3] + transitions[i * 3 + 1] + transitions[i * 3 + 2];
    if (row_sum > 0) {
      for (int j = 0; j < 3; ++j) {
        probabilities[i * 3 + j] = static_cast<float>(transitions[i * 3 + j]) / row_sum;
      }
    }
  }

  return probabilities;
}

}  // namespace ternary_utils

}  // namespace t81::codec
