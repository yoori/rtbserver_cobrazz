add_library(CampaignTypes SHARED
  CampaignTypes.cpp
)

target_link_libraries(CampaignTypes
  PUBLIC
    Generics
)

add_library(ExpressionChannel SHARED
  ExpressionChannel.cpp
)

target_link_libraries(ExpressionChannel
  PUBLIC
    Generics
)

add_library(ExpressionChannelIndex SHARED
  ExpressionChannelIndex.cpp
)

target_link_libraries(ExpressionChannelIndex
  PRIVATE
    ExpressionChannel
)
