#pragma once

#include <cstdint>
#include <vector>
#include <eh/Exception.hpp>

namespace AdServer::ChannelSvcs
{
  using ChannelIdList = std::vector<std::uint32_t>;

  inline constexpr unsigned int COUNT_SPECIAL = 4;
  inline constexpr unsigned int CT_URL = 0;
  inline constexpr unsigned int CT_PAGE = 1;
  inline constexpr unsigned int CT_SEARCH = 2;
  inline constexpr unsigned int CT_URL_KEYWORDS = 3;
  inline constexpr unsigned int CT_MAX = 4;

  DECLARE_EXCEPTION(NotConfigured, eh::DescriptiveException);
  DECLARE_EXCEPTION(ImplementationException, eh::DescriptiveException);
}
