add_library(ChannelCommons SHARED
  ChannelUtils.cpp ChannelUpdateStatLogger.cpp TriggerParser.cpp
)

target_link_libraries(ChannelCommons
  PUBLIC
    Boost::headers
  PRIVATE
    Generics
    HTTP
    BLogic
    LogCommons
)
