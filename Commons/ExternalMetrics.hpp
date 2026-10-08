#pragma once

#include <cstdint>

namespace AdServer::Commons
{
  // Stable IDs from the PostgreSQL ExternalMetricsConnectionType dictionary.
  constexpr std::uint64_t INTERNAL_METRICS = 0;
  constexpr std::uint64_t YANDEX_METRIKA = 1;
  constexpr std::uint64_t MAX_METRICS_CONNECTION_TYPE = 32767;
}
