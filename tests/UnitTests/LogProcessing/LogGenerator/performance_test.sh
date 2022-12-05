#!/bin/sh

if test -n "${1}" ; then
  tmp_folder="-p ${1}"
fi
records_count="-c 10000"
if test -n "${2}" ; then
  records_count="-c ${2}"
fi
valgrind_prefix="${3}"

echo "FRONTEND fill logs results:"
$valgrind_prefix LogGenerator $tmp_folder $records_count -t -d -gd \
ActionRequest,AdvertiserAction,ChannelHitStat,\
ChannelTriggerStat,Click,ColoUsers,CreativeStat,Impression,\
OptOutStat,PassbackImpression,PassbackOpportunity,PublisherInventory,Request,\
RequestBasicChannels,SiteReferrerStat,TagRequest,UserProperties

echo "BACKEND fill logs results:"
$valgrind_prefix LogGenerator $tmp_folder $records_count -t -d -gd \
ActionOpportunity,ActionStat,CCGKeywordStat,CCGStat,\
CCStat,ChannelImpInventory,ChannelInventory,ChannelInventoryEstimationStat,\
ChannelPerformance,ChannelPriceRange,CMPStat,ColoUpdateStat,\
ExpressionPerformance,HistoryMatch,PassbackStat,SiteChannelStat,SiteStat
