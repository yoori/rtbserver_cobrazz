
add_executable(ExpressionMatcherAdmin
Application.cpp
)
target_link_libraries(ExpressionMatcherAdmin
 String
 Generics
 CORBACommons
 UtilCommons

 ExpressionMatcherIDL
 pthread
 ProcessControlIDL
 UserActionProfilePST
UserActionProfile_v321PST

)



install(TARGETS ExpressionMatcherAdmin  DESTINATION ${INSTALL_DIR})