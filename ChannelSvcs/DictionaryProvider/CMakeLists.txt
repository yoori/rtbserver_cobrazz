add_idl(DictionaryProviderIDL DictionaryProvider.idl ${IDL_DIRECTORY}/ChannelSvcs/DictionaryProvider)


target_link_libraries(DictionaryProviderIDL
 Generics
 HTTP
  Commons
 BLogic
 LogCommons
CORBACommons
CorbaTypesIDL
ChannelUpdateBaseIDL
ChannelServerIDL
)




add_executable(DictionaryProvider

DictionaryProviderMain.cpp DictionaryProviderImpl.cpp DictionaryContainer.cpp
)
target_link_libraries(DictionaryProvider
  Generics
  xerces-c
  XMLUtility
  CORBACommons
  ProcessControlVars
  AdServerCommonsXSD
  DictionaryProviderConfigXSD
  Commons
  DictionaryProviderIDL
  ChannelCommons
)

#install(TARGETS DictionaryProvider DictionaryProviderIDL  DESTINATION ${INSTALL_DIR})