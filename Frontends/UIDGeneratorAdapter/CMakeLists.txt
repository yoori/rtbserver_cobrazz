add_pb(UIDGeneratorProtocolPB UIDGeneratorProtocol.proto ${PB_DIRECTORY}/Frontends/UIDGeneratorAdapter)

add_executable(UIDGeneratorAdapter 
KeywordsProcessor.cpp 
  DescriptorHandlerPoller.cpp 
  AcceptorDescriptorHandler.cpp 
  UIDGeneratorAdapterLogger.cpp 
  UIDGeneratorAdapterImpl.cpp 
  UIDGeneratorAdapterMain.cpp
)
target_link_libraries(UIDGeneratorAdapter
 Generics
 Commons

 AdServerCommonsXSD

 CORBACommons
 ProcessControlVars
 LogCommons

 UIDGeneratorAdapterConfigXSD
# DMPProfilingInfoPB
 UIDGeneratorProtocolPB
 protobuf
 protoc
 DMPProfilingInfoPST

)

#install(TARGETS UIDGeneratorAdapter  DESTINATION ${INSTALL_DIR})