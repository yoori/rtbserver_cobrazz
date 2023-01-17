add_pst(UserCampaignReachProfile_v24PST  UserCampaignReachProfile_v24.pst ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoManager/Compatibility)
add_pst(RequestProfile_v351PST  RequestProfile_v351.pst ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoManager/Compatibility)
add_pst(RequestProfile_v356PST  RequestProfile_v356.pst ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoManager/Compatibility)
add_pst(RequestProfile_v360PST  RequestProfile_v360.pst ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoManager/Compatibility)
add_pst(RequestProfile_v361PST  RequestProfile_v361.pst ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoManager/Compatibility)

add_pst(PassbackProfile_v32PST  PassbackProfile_v32.pst ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoManager/Compatibility)
add_pst(UserActionProfile_v321PST  UserActionProfile_v321.pst ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoManager/Compatibility)

add_pst(UserSiteReachProfile_v24PST  UserSiteReachProfile_v24.pst ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoManager/Compatibility)
add_pst(RequestOperationImpressionProfile_v0PST  RequestOperationImpressionProfile_v0.pst ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoManager/Compatibility)

add_library(RequestInfoManagerCompatibility SHARED
  UserCampaignReachProfileAdapter.cpp 
  RequestProfileAdapter.cpp 
  PassbackProfileAdapter.cpp 
  UserActionProfileAdapter.cpp 
  UserSiteReachProfileAdapter.cpp 
  RequestOperationImpressionProfileAdapter.cpp


)

target_link_libraries(RequestInfoManagerCompatibility
  UserCampaignReachProfile_v24PST
  RequestProfile_v351PST
  RequestProfile_v356PST
  RequestProfile_v360PST
  RequestProfile_v361PST
  PassbackProfile_v32PST
  
  
  UserActionProfile_v321PST
  UserSiteReachProfile_v24PST
  RequestOperationImpressionProfile_v0PST
  Commons
RequestProfilePST

RequestOperationProfilePST 
UserChannelInventoryProfilePST
UserCampaignReachProfilePST 
UserActionProfilePST  
UserSiteReachProfilePST
UserFraudProtectionProfilePST
PassbackProfilePST  
UserTagRequestMergeProfilePST  
UserTriggerMatchProfilePST  
RequestTriggerMatchProfilePST 
ConsiderMessagesPST  

)