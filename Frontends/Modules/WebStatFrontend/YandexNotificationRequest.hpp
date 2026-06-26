#pragma once

#include <string>
#include <vector>

namespace AdServer::WebStat
{
  struct YandexNotificationProcessingElementContext
  {
    std::string cr_id;
    std::string request_id;
    std::string imp_id;
    int status;
    std::vector<int> reasons;
    std::string payload;
  };

  struct YandexNotificationProcessingContext
  {
    std::vector<YandexNotificationProcessingElementContext> elements;
  };
}
