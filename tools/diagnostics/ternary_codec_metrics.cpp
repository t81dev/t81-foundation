// tools/diagnostics/ternary_codec_metrics.cpp
//
// Off-core diagnostic quality metrics for the T81 ternary codec.
// Gated behind T81_BUILD_DIAGNOSTICS (OFF by default in production builds).
//
// This translation unit is the ONLY permitted location for float/double arithmetic
// derived from ternary codec operation. It MUST NOT be linked against by any
// target in the deterministic core build (RFC-0032 §6.1).
//

#include "t81/math/quantization/ternary_codec.hpp"

#include <cmath>
#include <cstddef>
#include <limits>
#include <iostream>
#include <vector>

namespace t81::diagnostics {

using t81::math::quantization::TritValue;

/// Floating-point quality metrics for offline codec evaluation.
/// NOT for use in the deterministic core — diagnostic use only.
struct CodecMetrics {
  double mse{0.0};                  ///< Mean squared error (original vs reconstructed)
  double psnr{0.0};                 ///< Peak signal-to-noise ratio (dB)
  double compression_ratio{16.0};   ///< Fixed: ternary ~2 bits vs 32-bit float = 16:1
  double accuracy_preservation{0.0};///< 100 * (1 − sqrt(MSE)); diagnostic approximation
};

/// Compute MSE and PSNR between original float weights and their ternary-reconstructed
/// counterparts. `scale` is the fixed-point scale used for dequantization.
///
/// NOTE: This function uses hardware floating-point. It is intentionally excluded from the
/// deterministic core and MUST be guarded by T81_BUILD_DIAGNOSTICS in any build that
/// includes it (RFC-0002 §3, RFC-0032 §6.1).
CodecMetrics compute_codec_metrics(const std::vector<float>& original_weights,
                                   const std::vector<TritValue>& quantized,
                                   float scale) {
  if (original_weights.empty() || original_weights.size() != quantized.size()) {
    return {};
  }

  double mse = 0.0;
  for (std::size_t i = 0; i < original_weights.size(); ++i) {
    float reconstructed = 0.0f;
    switch (quantized[i]) {
      case TritValue::Neg:  reconstructed = -scale; break;
      case TritValue::Pos:  reconstructed =  scale; break;
      default:              reconstructed =  0.0f;  break;
    }
    const double error = static_cast<double>(original_weights[i]) - reconstructed;
    mse += error * error;
  }
  mse /= static_cast<double>(original_weights.size());

  CodecMetrics m;
  m.mse = mse;
  m.compression_ratio = 16.0;

  if (mse > 0.0) {
    m.psnr = 10.0 * std::log10(1.0 / mse);
  } else {
    m.psnr = std::numeric_limits<double>::infinity();
  }

  m.accuracy_preservation = 100.0 * (1.0 - std::sqrt(mse));

  return m;
}

/// Print a human-readable metrics summary to stdout.
void print_metrics(const CodecMetrics& m, const char* scheme_name) {
  std::cout << "[diagnostics] codec=" << scheme_name << '\n'
            << "  MSE:                  " << m.mse << '\n'
            << "  PSNR (dB):            " << m.psnr << '\n'
            << "  compression ratio:    " << m.compression_ratio << ":1\n"
            << "  accuracy_preservation " << m.accuracy_preservation << "%\n";
}

}  // namespace t81::diagnostics
