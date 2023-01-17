add_executable(ZmqBalancer
  ZmqBalancer.cpp 
  ZmqStreamStats.cpp
 )

target_link_libraries(ZmqBalancer
 Generics
 Commons

 AdServerCommonsXSD

 CORBACommons
 ProcessControlVars
 LogCommons

# UIDGeneratorAdapterConfigXSD
# DMPProfilingInfoPB
 UIDGeneratorProtocolPB
 protobuf
 protoc
 DMPProfilingInfoPST
 zmq
 ProcessControlIDL
 StatsIDL
 ZmqBalancerConfigXSD
 XMLUtility
)

install(TARGETS ZmqBalancer  DESTINATION ${INSTALL_DIR})