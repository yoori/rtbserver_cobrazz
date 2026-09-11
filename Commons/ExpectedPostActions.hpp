#pragma once

#include <memory>
#include <string>
#include <vector>

namespace AdServer
{
  inline const std::shared_ptr<const std::vector<std::string>>&
  empty_expected_post_actions()
  {
    static const auto result = std::make_shared<const std::vector<std::string>>();
    return result;
  }
}
