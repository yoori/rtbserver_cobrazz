add_idl(CampaignCommonsIDL CampaignCommons.idl ${IDL_DIRECTORY}/CampaignSvcs/CampaignCommons NO_SERVANT)

target_link_libraries(CampaignCommonsIDL
  PRIVATE
    CorbaTypesIDL
)

add_library(CampaignTypes SHARED
  CampaignTypes.cpp
)

add_library(ExpressionChannel SHARED
  ExpressionChannel.cpp
)

add_library(ExpressionChannelIndex SHARED
  ExpressionChannelIndex.cpp
)

target_link_libraries(ExpressionChannelIndex
  PRIVATE
    Generics
    ExpressionChannel
)
