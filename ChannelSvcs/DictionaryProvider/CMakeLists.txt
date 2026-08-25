add_idl(DictionaryProviderIDL DictionaryProvider.idl ${IDL_DIRECTORY}/ChannelSvcs/DictionaryProvider)


target_link_libraries(DictionaryProviderIDL
  PRIVATE
    CorbaTypesIDL
    ChannelCommonsIDL
)




add_executable(DictionaryProvider

  DictionaryProviderMain.cpp DictionaryProviderImpl.cpp DictionaryContainer.cpp
)
target_link_libraries(DictionaryProvider
  PRIVATE
    TAO::PortableServer
    TAO::Valuetype
    Generics
    CORBACommons
    AdServerCommonsXSD
    DictionaryProviderConfigXSD
    Commons
    CORBAConfig
    CorbaTypesIDL
    DictionaryProviderIDL
    ChannelCommons
    ChannelCommonsIDL
    Logger
    ProcessControlIDL
    Sync
)
