#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace AdServer::Commons
{
  using FixedTimestampInfo = std::string;

  using OctSeq = std::string;

  using TimestampInfo = std::string;

  using TimestampSeq = std::vector<TimestampInfo>;

  using RequestIdInfo = OctSeq;

  using UserIdInfo = OctSeq;

  using DecimalInfo = OctSeq;

  using OptionalDecimalInfo = OctSeq;

  using StringSeq = std::vector<std::string>;

  using RequestIdInfoSeq = std::vector<RequestIdInfo>;

  using DecimalInfoSeq = std::vector<DecimalInfo>;

  using IdSeq = std::vector<std::uint32_t>;

  struct PartlyMatchResult
  {
    std::uint32_t channel_id{};
    std::uint32_t visits{};
    std::uint32_t minimum_visits{};
  };

}
