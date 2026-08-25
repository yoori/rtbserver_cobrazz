
add_executable(RequestInfoAdmin
  Application.cpp
)
target_link_libraries(RequestInfoAdmin
  PRIVATE
    Generics
    Commons
    Logger
    UtilCommons

    pthread
    UserActionProfilePST
    UserActionProfile_v321PST
    RequestInfoManagerGrpcClient
    UserActionInfoContainer


)
#install(TARGETS RequestInfoAdmin  DESTINATION ${INSTALL_DIR})
