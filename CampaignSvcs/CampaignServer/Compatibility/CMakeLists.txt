add_idl(BillStatInfo_v350IDL BillStatInfo_v350.idl ${IDL_DIRECTORY}/CampaignSvcs/CampaignServer/Compatibility NO_SERVANT)
target_link_libraries(BillStatInfo_v350IDL
  PUBLIC
    CorbaTypesIDL
)

add_idl(CampaignCommons_v350IDL CampaignCommons_v350.idl ${IDL_DIRECTORY}/CampaignSvcs/CampaignServer/Compatibility NO_SERVANT)
target_link_libraries(CampaignCommons_v350IDL
  PUBLIC
    CorbaTypesIDL
)

add_idl(CampaignServer_v350IDL CampaignServer_v350.idl ${IDL_DIRECTORY}/CampaignSvcs/CampaignServer/Compatibility)
target_link_libraries(CampaignServer_v350IDL
  PUBLIC
    CampaignCommons_v350IDL
    BillStatInfo_v350IDL
)
add_dependencies(CampaignServer_v350IDL StatInfoIDL)

add_library(CampaignServerStubs_v350 SHARED
  CampaignServerImpl_v350.cpp
)
target_link_libraries(CampaignServerStubs_v350
  PRIVATE
    CampaignTypes
    CampaignServer_v350IDL
    CORBACommons
    CampaignServerIDL
)
