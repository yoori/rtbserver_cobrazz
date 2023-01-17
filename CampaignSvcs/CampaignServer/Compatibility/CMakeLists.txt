add_idl(BillStatInfo_v350IDL BillStatInfo_v350.idl ${IDL_DIRECTORY}/CampaignSvcs/CampaignServer/Compatibility)
target_link_libraries(BillStatInfo_v350IDL CorbaTypesIDL)

add_idl(CampaignCommons_v350IDL CampaignCommons_v350.idl ${IDL_DIRECTORY}/CampaignSvcs/CampaignServer/Compatibility)
target_link_libraries(CampaignCommons_v350IDL CorbaTypesIDL)


add_idl(CampaignServer_v350IDL CampaignServer_v350.idl ${IDL_DIRECTORY}/CampaignSvcs/CampaignServer/Compatibility)
target_link_libraries(CampaignServer_v350IDL CorbaTypesIDL CampaignCommons_v350IDL BillStatInfo_v350IDL)

add_library(CampaignServerStubs_v350 SHARED
        CampaignServerImpl_v350.cpp
)
target_link_libraries(CampaignServerStubs_v350
	Commons
        CampaignServer_v350IDL
	CORBACommons
	CorbaTypesIDL
	CampaignCommons_v350IDL
	BillStatInfo_v350IDL
    BillStatInfoIDL
    CampaignCommonsIDL
    CampaignServerIDL
)



install(TARGETS BillStatInfo_v350IDL CampaignCommons_v350IDL CampaignServer_v350IDL CampaignServerStubs_v350  DESTINATION ${INSTALL_DIR})