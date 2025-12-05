#pragma once

#include <functional>
#include <string>
#include <vector>

#include "t81/canonfs/canon_driver.hpp"

namespace t81::canonfs {

std::function<AxionVerdict(OpKind, const CanonRef&)> make_axion_policy_hook(
    std::string policy_text);

const std::vector<std::string>& axion_trace();
void reset_axion_trace();

}  // namespace t81::canonfs
