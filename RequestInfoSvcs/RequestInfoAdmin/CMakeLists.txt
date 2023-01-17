
add_executable(RequestInfoAdmin
Application.cpp
)
target_link_libraries(RequestInfoAdmin
 String
 Generics
 CORBACommons
 UtilCommons

 ExpressionMatcherIDL
 pthread
 ProcessControlIDL
 UserActionProfilePST
UserActionProfile_v321PST
RequestInfoManagerIDL
UserActionInfoContainer
pcre
PlainStorage2
PlainStorage3
RequestInfoManagerCompatibility


)
install(TARGETS RequestInfoAdmin  DESTINATION ${INSTALL_DIR})



