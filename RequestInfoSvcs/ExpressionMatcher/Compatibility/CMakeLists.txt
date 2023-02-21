
add_pst(UserChannelInventoryProfile_v281PST  UserChannelInventoryProfile_v281.pst ${PST_DIRECTORY}/RequestInfoSvcs/ExpressionMatcher/Compatibility)
add_pst(UserTriggerMatchProfile_v0PST  UserTriggerMatchProfile_v0.pst ${PST_DIRECTORY}/RequestInfoSvcs/ExpressionMatcher/Compatibility)
add_pst(RequestTriggerMatchProfile_v330PST  RequestTriggerMatchProfile_v330.pst ${PST_DIRECTORY}/RequestInfoSvcs/ExpressionMatcher/Compatibility)

add_library(ExpressionMatcherCompatibility SHARED
  UserChannelInventoryProfileAdapter.cpp 
  UserTriggerMatchProfileAdapter.cpp 
  UserColoReachProfileAdapter.cpp 
  RequestTriggerMatchProfileAdapter.cpp
)
target_link_libraries(ExpressionMatcherCompatibility
  RequestTriggerMatchProfile_v330PST
  UserTriggerMatchProfile_v0PST
  UserChannelInventoryProfile_v281PST
  UserChannelInventoryProfilePST
  UserTriggerMatchProfilePST
  RequestTriggerMatchProfilePST

Sync
Generics
String
pthread
)

#install(TARGETS ExpressionMatcherCompatibility  DESTINATION ${INSTALL_DIR})