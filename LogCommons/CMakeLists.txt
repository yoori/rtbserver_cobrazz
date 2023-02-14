ADD_SUBDIRECTORY(Compatibility)


add_library(LogCommons SHARED
ActionRequest.cpp
ActionStat.cpp
AdRequestLogger.cpp
BidCostStat.cpp
CampaignReferrerStat.cpp
CampaignUserStat.cpp
CcUserStat.cpp
CcgKeywordStat.cpp
CcgSelectionFailureStat.cpp
CcgStat.cpp
CcgUserStat.cpp
ChannelCountStat.cpp
ChannelHitStat.cpp
ChannelImpInventory.cpp
ChannelInventory.cpp
ChannelInventoryEstimationStat.cpp
ChannelOverlapUserStat.cpp
ChannelPerformance.cpp
ChannelPriceRange.cpp
ChannelTriggerImpStat.cpp
ChannelTriggerStat.cpp
CmpStat.cpp
ColoUpdateStat.cpp
ColoUserStat.cpp
ColoUsers.cpp
ConvLogger.cpp
CreativeStat.cpp
DeviceChannelCountStat.cpp
DiscoverChannelState.cpp
DiscoverFeedState.cpp
ExpressionPerformance.cpp
FileReceiver.cpp
HistoryMatch.cpp
ImpNotify.cpp
KeywordHitStat.cpp
LogCommons.cpp
PageLoadsDailyStat.cpp
PassbackStat.cpp
Request.cpp
RequestBasicChannels.cpp
Request_Base.cpp
#Request_v354.cpp
#Request_v360.cpp
ResearchLogs.cpp
ResearchProfStat.cpp
ResearchWebStat.cpp
SearchEngineStat.cpp
SearchTermStat.cpp
SiteChannelStat.cpp
SiteReferrerStat.cpp
SiteStat.cpp
SiteUserStat.cpp
TagAuctionStat.cpp
TagPositionStat.cpp
TagRequest.cpp
UserAgentStat.cpp
UserProperties.cpp
WebStat.cpp
WebwiseDiscoverItemStat.cpp
WebwiseDiscoverItems.cpp
WebwiseDiscoverTagStat.cpp

)

target_link_libraries(LogCommons
 Generics
# ExceptionHandling
 Commons
# LogCommons
 CorbaTypesIDL
)

#install(TARGETS LogCommons  DESTINATION ${INSTALL_DIR})





















