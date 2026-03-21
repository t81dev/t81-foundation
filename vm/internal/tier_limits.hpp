#pragma once

#include <cstddef>

namespace t81::cog {
enum class TierId;
}

namespace t81::vm::internal {

int tier_rank(t81::cog::TierId tier);
std::size_t recursion_limit_for_tier(t81::cog::TierId tier);
t81::cog::TierId tier_from_rank(int rank);
std::size_t max_shape_complexity_for_tier(t81::cog::TierId tier);
int max_tensor_rank_for_tier(t81::cog::TierId tier);
std::size_t max_symbolic_complexity_for_tier(t81::cog::TierId tier);
double max_branch_entropy_for_tier(t81::cog::TierId tier);

}  // namespace t81::vm::internal
