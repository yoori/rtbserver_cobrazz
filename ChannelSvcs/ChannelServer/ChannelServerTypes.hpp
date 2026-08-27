#pragma once

#include <cstdint>

#include <eh/Exception.hpp>

namespace AdServer::ChannelSvcs
{
  struct ChannelServerStats
  {
    enum StatParams
    {
      KW_COUNT = 0,
      KW_ID_COUNT,
      URL_COUNT,
      URL_ID_COUNT,
      UID_COUNT,
      UID_ID_COUNT,
      NS_KW_COUNT,
      NS_URL_COUNT,
      MATCHINGS_COUNT,
      EXCEPTIONS_COUNT,
      PARAMS_MAX
    };
    static const char* param_name[PARAMS_MAX];
    size_t params[PARAMS_MAX] = {};
    std::uint64_t total_requests = 0;
    std::string configuration;
    Generics::Time configuration_date;
  };

  namespace ChannelServerException
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(NotReady, Exception);
    DECLARE_EXCEPTION(TemporyUnavailable, eh::DescriptiveException);
  }

}
