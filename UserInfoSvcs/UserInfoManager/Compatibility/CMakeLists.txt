# UserBaseProfileAdapter
add_pst(UserChannelBaseProfile_v1PST  UserChannelBaseProfile_v1.pst ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)
add_pst(UserChannelBaseProfile_v2PST  UserChannelBaseProfile_v2.pst ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)
add_pst(UserChannelBaseProfile_v3PST  UserChannelBaseProfile_v3.pst ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)
add_pst(UserChannelBaseProfile_v4PST  UserChannelBaseProfile_v4.pst ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)
add_pst(UserChannelBaseProfile_v5PST  UserChannelBaseProfile_v5.pst ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)
add_pst(UserChannelBaseProfile_v26PST  UserChannelBaseProfile_v26.pst ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)
add_pst(UserChannelBaseProfile_v290PST  UserChannelBaseProfile_v290.pst ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)
add_pst(UserChannelBaseProfile_v300PST  UserChannelBaseProfile_v300.pst ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)
add_pst(UserChannelBaseProfile_v301PST  UserChannelBaseProfile_v301.pst ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)
add_pst(UserChannelBaseProfile_v320PST  UserChannelBaseProfile_v320.pst ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)

add_library(UserBaseProfileAdapter SHARED
  UserBaseProfileAdapter.cpp
)

target_link_libraries(UserBaseProfileAdapter
  Commons
  ChannelMatcher
  UserChannelBaseProfile_v1PST
  UserChannelBaseProfile_v2PST
  UserChannelBaseProfile_v3PST
  UserChannelBaseProfile_v4PST
  UserChannelBaseProfile_v5PST
  UserChannelBaseProfile_v26PST
  UserChannelBaseProfile_v290PST
  UserChannelBaseProfile_v300PST
  UserChannelBaseProfile_v301PST
  UserChannelBaseProfile_v320PST
)

# UserFreqCapProfileAdapter
add_pst(UserFreqCapProfile_v290PST  UserFreqCapProfile_v290.pst  ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)
add_pst(UserFreqCapProfile_v330PST  UserFreqCapProfile_v330.pst  ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)
add_pst(UserFreqCapProfile_v340PST  UserFreqCapProfile_v340.pst  ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)

add_library(UserFreqCapProfileAdapter SHARED
  UserFreqCapProfileAdapter.cpp
)

target_link_libraries(UserFreqCapProfileAdapter
  Commons
  UserFreqCapProfile_v290PST
  UserFreqCapProfile_v330PST
  UserFreqCapProfile_v340PST
  UserFreqCapProfileDescriptionPST
)

# UserHistoryProfileAdapter
add_pst(UserChannelHistoryProfile_v1PST  UserChannelHistoryProfile_v1.pst  ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)
add_pst(UserChannelHistoryProfile_v320PST  UserChannelHistoryProfile_v320.pst  ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)

add_library(UserHistoryProfileAdapter SHARED
  UserHistoryProfileAdapter.cpp
)

target_link_libraries(UserHistoryProfileAdapter
  Commons
  ChannelMatcher
  UserChannelHistoryProfile_v1PST
  UserChannelHistoryProfile_v320PST
)

# UserOperationProfilesAdapter
add_pst(UserOperationProfiles_v24PST  UserOperationProfiles_v24.pst  ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)
add_pst(UserOperationProfiles_v25PST  UserOperationProfiles_v25.pst  ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)
add_pst(UserOperationProfiles_v26PST  UserOperationProfiles_v26.pst  ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)
add_pst(UserOperationProfiles_v290PST  UserOperationProfiles_v290.pst  ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)
add_pst(UserOperationProfiles_v300PST  UserOperationProfiles_v300.pst  ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)
add_pst(UserOperationProfiles_v301PST  UserOperationProfiles_v301.pst  ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)
add_pst(UserOperationProfiles_v310PST  UserOperationProfiles_v310.pst  ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)
add_pst(UserOperationProfiles_v320PST  UserOperationProfiles_v320.pst  ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)
add_pst(UserOperationProfiles_v340PST  UserOperationProfiles_v340.pst  ${PST_DIRECTORY}/UserInfoSvcs/UserInfoManager/Compatibility)

add_library(UserOperationProfilesAdapter SHARED
  UserOperationProfilesAdapter.cpp
)

target_link_libraries(UserOperationProfilesAdapter
  Commons
  ChannelMatcher
  UserOperationProfiles_v24PST
  UserOperationProfiles_v25PST
  UserOperationProfiles_v26PST
  UserOperationProfiles_v290PST
  UserOperationProfiles_v300PST
  UserOperationProfiles_v301PST
  UserOperationProfiles_v310PST
  UserOperationProfiles_v320PST
  UserOperationProfiles_v340PST
  UserOperationProfilesPST
)
