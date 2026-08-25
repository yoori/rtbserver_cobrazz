add_idl(BillStatInfoIDL BillStatInfo.idl ${IDL_DIRECTORY}/CampaignSvcs/CampaignServer)
add_idl(StatInfoIDL StatInfo.idl ${IDL_DIRECTORY}/CampaignSvcs/CampaignServer)
add_idl(CampaignServerIDL CampaignServer.idl ${IDL_DIRECTORY}/CampaignSvcs/CampaignServer)

add_dependencies(StatInfoIDL CampaignCommonsIDL)
target_link_libraries(BillStatInfoIDL PRIVATE CorbaTypesIDL)

target_link_libraries(CampaignServerIDL
  PRIVATE
    CampaignCommonsIDL
    BillStatInfoIDL
    StatInfoIDL
)

add_library(BillStatServerSource STATIC
  BillStatServerSource.cpp
  BillStatSource.cpp
)

target_link_libraries(BillStatServerSource
  PRIVATE
    Generics
    Commons
    CORBACommons
    BillStatInfoIDL
    CampaignServerIDL
)

add_library(CampaignConfigModifier STATIC
  CampaignConfigModifier.cpp
  StatLogGeneralizerSource.cpp
  StatDBSource.cpp
)

target_link_libraries(CampaignConfigModifier
  PRIVATE
    Generics
    Commons
    CORBACommons
    Postgres
    LogGeneralizerIDL
)

add_executable(CampaignServer
  ModifyConfigSource.cpp
  CampaignConfigDBSource.cpp
  CampaignConfigServerSource.cpp
  CampaignServerImpl.cpp
  CampaignServerLogger.cpp
  CampaignServerMain.cpp
  CampaignServerStatValues.cpp
  BillStatDBSource.cpp
)

target_link_libraries(CampaignServer
  PRIVATE
    TAO::PortableServer
    TAO::Valuetype
    LogCommons
    Commons
    CORBAConfig
    Postgres
    BillStatServerSource
    CampaignCommonsIDL
    CampaignConfigModifier
    CORBACommons
    CorbaTypesIDL
    CampaignServerStubs_v350
    CampaignServerIDL
    StatInfoIDL
    AdServerCommonsXSD
    Generics
    HTTP
    Logger
    Sync
    CampaignServerConfigXSD
    DomainConfigXSD
    HostDistributionFileXSD
    ProcessControlIDL
)

ADD_SUBDIRECTORY(Compatibility)
