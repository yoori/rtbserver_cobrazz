
add_executable(RequestInfoAdmin
  Application.cpp
)
target_link_libraries(RequestInfoAdmin
  String
  Generics
  CORBACommons
  UtilCommons

  pthread
  ProcessControlIDL
  UserActionProfilePST
  UserActionProfile_v321PST
  RequestInfoManagerIDL
  UserActionInfoContainer
  pcre
  RequestInfoManagerCompatibility


)
#install(TARGETS RequestInfoAdmin  DESTINATION ${INSTALL_DIR})
