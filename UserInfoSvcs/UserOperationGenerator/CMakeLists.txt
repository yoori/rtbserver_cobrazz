add_library(UidChannelSnapshot STATIC
  UidChannelSnapshot.cpp
)

add_dependencies(UidChannelSnapshot UserChannelBaseProfilePST)

add_executable(UserOperationGenerator
  UserOperationGeneratorImpl.cpp 
  UserOperationGeneratorMain.cpp 
)

target_link_libraries(UserOperationGenerator
  CORBACommons
  Commons
  Generics
  PlainStorage3
  ProcessControlVars
  ProfileMap
  SyncLogsLib
  UServerUtils
  UidChannelSnapshot
  UserChannelBaseProfilePST
  UserInfoContainer
  UserOperationGeneratorConfigXSD
  UserOperationProfilesPST
  XMLUtility
)