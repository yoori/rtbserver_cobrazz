add_pst(RequestProfilePST  RequestProfile.pst ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoCommons)

add_pst(RequestOperationProfilePST  RequestOperationProfile.pst ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoCommons)
add_pst(UserChannelInventoryProfilePST  UserChannelInventoryProfile.pst ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoCommons)
add_pst(UserCampaignReachProfilePST  UserCampaignReachProfile.pst ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoCommons)
add_pst(UserActionProfilePST  UserActionProfile.pst ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoCommons)
add_pst(UserSiteReachProfilePST  UserSiteReachProfile.pst ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoCommons)
add_pst(UserFraudProtectionProfilePST  UserFraudProtectionProfile.pst ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoCommons)
add_pst(PassbackProfilePST  PassbackProfile.pst ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoCommons)
add_pst(UserTagRequestMergeProfilePST  UserTagRequestMergeProfile.pst ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoCommons)
add_pst(UserTriggerMatchProfilePST  UserTriggerMatchProfile.pst ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoCommons)
add_pst(RequestTriggerMatchProfilePST  RequestTriggerMatchProfile.pst ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoCommons)
add_pst(ConsiderMessagesPST  ConsiderMessages.pst ${PST_DIRECTORY}/RequestInfoSvcs/RequestInfoCommons)

add_library(RequestInfoStatistics SHARED
  Statistics.cpp
)

target_link_libraries(RequestInfoStatistics
  UServerUtils
)