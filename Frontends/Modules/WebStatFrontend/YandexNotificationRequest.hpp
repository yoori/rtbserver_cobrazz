#pragma once

#include <string>
#include <vector>
#include <Commons/JsonParamProcessor.hpp>

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

  typedef AdServer::Commons::JsonParamProcessor<YandexNotificationProcessingContext>
    JsonYNParamProcessor;

  typedef ReferenceCounting::SmartPtr<JsonYNParamProcessor>
    JsonYNParamProcessor_var;
}
