#include "internal/tier_limits.hpp"
#include "t81/vm/state.hpp"

namespace t81::vm::internal {

int tier_rank(t81::cog::TierId tier) {
  switch (tier) {
    case t81::cog::TierId::Tier0:
      return 0;
    case t81::cog::TierId::Tier1:
      return 1;
    case t81::cog::TierId::Tier2:
      return 2;
    case t81::cog::TierId::Tier3:
      return 3;
    case t81::cog::TierId::Tier4:
      return 4;
    case t81::cog::TierId::Tier5:
      return 5;
    case t81::cog::TierId::Tier6:
      return 6;
  }
  return 0;
}

std::size_t recursion_limit_for_tier(t81::cog::TierId tier) {
  switch (tier) {
    case t81::cog::TierId::Tier0:
      return 0;
    case t81::cog::TierId::Tier1:
      return 1;
    case t81::cog::TierId::Tier2:
      return 10;
    case t81::cog::TierId::Tier3:
      return 81;
    case t81::cog::TierId::Tier4:
      return 243;
    case t81::cog::TierId::Tier5:
      return 729;
    case t81::cog::TierId::Tier6:
      return 2187;  // 3^7
  }
  return 0;
}

t81::cog::TierId tier_from_rank(int rank) {
  switch (rank) {
    case 0:
      return t81::cog::TierId::Tier0;
    case 1:
      return t81::cog::TierId::Tier1;
    case 2:
      return t81::cog::TierId::Tier2;
    case 3:
      return t81::cog::TierId::Tier3;
    case 4:
      return t81::cog::TierId::Tier4;
    case 5:
      return t81::cog::TierId::Tier5;
    default:
      return rank < 1 ? t81::cog::TierId::Tier0 : t81::cog::TierId::Tier5;
  }
}

std::size_t max_shape_complexity_for_tier(t81::cog::TierId tier) {
  switch (tier) {
    case t81::cog::TierId::Tier0:
    case t81::cog::TierId::Tier1:
      return 81;
    case t81::cog::TierId::Tier2:
      return 81 * 81 * 3;
    case t81::cog::TierId::Tier3:
      return 81 * 81 * 5;
    case t81::cog::TierId::Tier4:
      return 81 * 81 * 7;
    case t81::cog::TierId::Tier5:
      return 81 * 81 * 9;
    case t81::cog::TierId::Tier6:
      return 81 * 81 * 11;
  }
  return 81;
}

int max_tensor_rank_for_tier(t81::cog::TierId tier) {
  switch (tier) {
    case t81::cog::TierId::Tier0:
    case t81::cog::TierId::Tier1:
      return 1;
    case t81::cog::TierId::Tier2:
      return 3;
    case t81::cog::TierId::Tier3:
      return 5;
    case t81::cog::TierId::Tier4:
      return 7;
    case t81::cog::TierId::Tier5:
      return 9;
    case t81::cog::TierId::Tier6:
      return 11;
  }
  return 1;
}

std::size_t max_symbolic_complexity_for_tier(t81::cog::TierId tier) {
  switch (tier) {
    case t81::cog::TierId::Tier0:
    case t81::cog::TierId::Tier1:
      return 81;
    case t81::cog::TierId::Tier2:
      return 243;
    case t81::cog::TierId::Tier3:
      return 729;
    case t81::cog::TierId::Tier4:
      return 2187;
    case t81::cog::TierId::Tier5:
      return 6561;
    case t81::cog::TierId::Tier6:
      return 19683;  // 3^9
  }
  return 81;
}

double max_branch_entropy_for_tier(t81::cog::TierId tier) {
  switch (tier) {
    case t81::cog::TierId::Tier0:
    case t81::cog::TierId::Tier1:
      return 16.0;
    case t81::cog::TierId::Tier2:
      return 64.0;
    case t81::cog::TierId::Tier3:
      return 162.0;
    case t81::cog::TierId::Tier4:
      return 243.0;
    case t81::cog::TierId::Tier5:
      return 324.0;
    case t81::cog::TierId::Tier6:
      return 486.0;  // 6 * 81
  }
  return 16.0;
}

}  // namespace t81::vm::internal
