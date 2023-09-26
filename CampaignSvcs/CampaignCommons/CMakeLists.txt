add_idl(CampaignCommonsIDL CampaignCommons.idl ${IDL_DIRECTORY}/CampaignSvcs/CampaignCommons)

target_link_libraries(CampaignCommonsIDL CorbaTypesIDL)

add_library(CampaignTypes SHARED
  CampaignTypes.cpp
  )

add_library(ExpressionChannel SHARED
  ExpressionChannel.cpp
  )

target_link_libraries(ExpressionChannel 
  Generics
  CORBACommons
  Commons
  CampaignCommonsIDL
  CampaignManagerProto
)


add_library(ExpressionChannelIndex SHARED
  ExpressionChannelIndex.cpp
  )

target_link_libraries(ExpressionChannelIndex
  Generics
  Commons
  ExpressionChannel
  )

#install(TARGETS CampaignCommonsIDL CampaignTypes ExpressionChannel ExpressionChannelIndex  DESTINATION ${INSTALL_DIR})
