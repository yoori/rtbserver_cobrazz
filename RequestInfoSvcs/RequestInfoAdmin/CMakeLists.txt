add_executable(RequestInfoAdmin
  Application.cpp
)

target_link_libraries(RequestInfoAdmin
  CORBACommons
  ExpressionMatcherIDL
  Generics
  PlainStorage2
  PlainStorage3
  ProcessControlIDL
  RequestInfoManagerCompatibility
  RequestInfoManagerIDL
  String
  UServerUtils
  UserActionInfoContainer
  UserActionProfilePST
  UserActionProfile_v321PST
  UtilCommons
  pcre
  pthread
)