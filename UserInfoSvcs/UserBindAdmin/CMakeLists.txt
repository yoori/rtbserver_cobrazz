add_executable(UserBindAdmin
    Application.cpp
)
target_link_libraries(UserBindAdmin
 Generics
 CORBACommons

 Commons

 UserBindControllerStubs
 UserBindServerIDL
 UserBindControllerIDL
)
#install(TARGETS UserBindAdmin  DESTINATION ${INSTALL_DIR})

