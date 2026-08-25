add_idl(CampaignCommonsIDL CampaignCommons.idl ${IDL_DIRECTORY}/CampaignSvcs/CampaignCommons NO_SERVANT)

target_link_libraries(CampaignCommonsIDL
  PUBLIC
    CorbaTypesIDL
)

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
