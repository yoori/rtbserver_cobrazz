#pragma once

#include <memory>

#include <UserInfoSvcs/UserInfoCommons/ChannelDictionary.hpp>
#include <UserInfoSvcs/UserInfoCommons/FreqCapConfig.hpp>

namespace AdServer::UserInfoSvcs
{
  class UserInfoConfigSource
  {
  public:
    struct Config
    {
      ChannelDictionary_var channels_config;
      FreqCapConfig_var freq_cap_config;
    };

    virtual Config get_config() = 0;

    virtual ~UserInfoConfigSource() noexcept = default;
  };

  using UserInfoConfigSourcePtr = std::shared_ptr<UserInfoConfigSource>;
}
