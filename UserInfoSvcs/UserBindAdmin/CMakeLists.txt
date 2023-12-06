add_executable(UserBindAdmin
  Application.cpp
  Main.cpp
)

target_link_libraries(UserBindAdmin
 Commons
 CORBACommons
 Generics
 UserBindControllerStubs
 UserBindServerIDL
 UserBindControllerIDL
 UserBindServerProto
 UServerUtilsMetrics
)


