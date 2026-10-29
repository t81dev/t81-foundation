#pragma once
#include <cstdint>
#include <stdexcept>
#if defined(__unix__) || defined(__APPLE__)
#include <sys/resource.h>
inline void check_resource_budget() {
  rusage usage{};
  if (getrusage(RUSAGE_SELF, &usage) != 0)
    throw std::runtime_error("RESOURCE_ACCOUNTING_FAILURE");
  const auto user =
      static_cast<std::uint64_t>(usage.ru_utime.tv_sec) * 1000000ULL +
      static_cast<std::uint64_t>(usage.ru_utime.tv_usec);
  const auto system =
      static_cast<std::uint64_t>(usage.ru_stime.tv_sec) * 1000000ULL +
      static_cast<std::uint64_t>(usage.ru_stime.tv_usec);
#if defined(__APPLE__)
  const auto rss = static_cast<std::uint64_t>(usage.ru_maxrss);
#else
  const auto rss = static_cast<std::uint64_t>(usage.ru_maxrss) * 1024ULL;
#endif
  if (user + system > 600000000ULL || rss > 500000000ULL)
    throw std::runtime_error("BUDGET_EXCEEDED");
}
#else
inline void check_resource_budget() {
  throw std::runtime_error("RESOURCE_ACCOUNTING_UNAVAILABLE");
}
#endif
