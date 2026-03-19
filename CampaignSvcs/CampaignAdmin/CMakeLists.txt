add_executable(CampaignAdmin
  Application.cpp
  CampaignServerUtils.cpp
)

target_link_libraries(CampaignAdmin
  CampaignServerStubs_v350
  CampaignManagerIDL
  CampaignCommonsIDL
  CampaignCommons_v350IDL
  BillStatInfo_v350IDL

  CampaignTypes
  ExpressionChannel
  BidRateProvider
  XslTransformer
  UtilCommons
  Commons

  Generics
)
