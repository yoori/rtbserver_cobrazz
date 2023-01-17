add_executable(CampaignAdmin
    Application.cpp CampaignServerUtils.cpp
)

target_link_libraries(CampaignAdmin
Generics 
Commons 
UtilCommons 
CampaignCommonsIDL 
CampaignTypes
ExpressionChannel
CampaignCommons_v350IDL
CampaignServerStubs_v350
BillStatInfo_v350IDL
CampaignManagerIDL
BidRateProvider
XslTransformer
#XslTransformer 
#CampaignCommonsStubs 
#CampaignServerStubs 
#CampaignManagerStubs 
)


install(TARGETS CampaignAdmin DESTINATION ${INSTALL_DIR})
