add_idl(BillingServerIDL BillingServer.idl ${IDL_DIRECTORY}/CampaignSvcs/BillingServer)

target_link_libraries(BillingServerIDL 
    ProcessControlIDL  CORBACommons StatsIDL
    CorbaTypesIDL
)

add_library(BillingContainer  STATIC
    BillingContainer.cpp 
    BillingContainerState.cpp

)
add_library(CTROptimizer  STATIC
    CTROptimizer.cpp

)


add_executable(BillingServer 
    BillingServerMain.cpp 
    BillingServerImpl.cpp

)

target_link_libraries(BillingServer   Commons  BillStatServerSource BillingContainer
    Generics ACE Logger Sync String  CORBACommons     xerces-c 
     CTROptimizer

    ProcessControlIDL
    BillingServerIDL
    BillStatInfoIDL
    CorbaTypesIDL
    CampaignServerIDL 
    CampaignCommonsIDL 
    StatInfoIDL
    
    AdServerCommonsXSD 
    BillingServerConfigXSD 
    CampaignManagerConfigXSD 
    CampaignServerConfigXSD 
    DomainConfigXSD
    HostDistributionFileXSD 
    
    pcre ssl crypto z event  pthread 
    TAO TAO_AnyTypeCode  TAO_CodecFactory TAO_CosEvent TAO_CosNaming TAO_CosNotification TAO_DynamicAny TAO_EndpointPolicy TAO_FaultTolerance
    TAO_FT_ClientORB TAO_FT_ServerORB TAO_FTORB_Utils TAO_IORManip TAO_IORTable 
    TAO_Messaging TAO_PI TAO_PI_Server TAO_PortableGroup TAO_PortableServer TAO_Security TAO_SSLIOP TAO_TC TAO_TC_IIOP TAO_Valuetype ACE_SSL 
    )

install(TARGETS BillingContainer BillingServerIDL BillingServer DESTINATION ${INSTALL_DIR})


