add_library(UidChannelSnapshot STATIC
  UidChannelSnapshot.cpp
)

add_dependencies(UidChannelSnapshot UserChannelBaseProfilePST)

add_executable(UserOperationGenerator
  UserOperationGeneratorImpl.cpp 
  UserOperationGeneratorMain.cpp 
)
target_link_libraries(UserOperationGenerator
  UidChannelSnapshot
  Generics
  Commons
  PlainStorage3
  SyncLogsLib
  CORBACommons
  ProcessControlVars
  UserOperationGeneratorConfigXSD
  UserInfoContainer
  UserOperationProfilesPST
  XMLUtility
  UserChannelBaseProfilePST
)

#install(TARGETS UserOperationGenerator DESTINATION ${INSTALL_DIR})
