#pragma once

#include <memory>
#include <string>
#include <vector>

#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>

#include "UserInfoConfigSource.hpp"

namespace AdServer::UserInfoSvcs
{
  class UserInfoConfigCampaignServerSource: public UserInfoConfigSource
  {
  public:
    UserInfoConfigCampaignServerSource(
      Logging::Logger* logger,
      const std::vector<std::string>& campaign_server_refs,
      unsigned long service_index,
      const Generics::Time& confirm_timeout);

    ~UserInfoConfigCampaignServerSource() noexcept override;

    Config get_config() override;

  private:
    struct State;
    using StatePtr = std::unique_ptr<State>;

    Logging::Logger_var logger_;
    StatePtr state_;
    unsigned long service_index_;
    Generics::Time confirm_timeout_;
  };
}
