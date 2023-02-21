

add_executable(UserIdUtil
 Application.cpp
)
target_link_libraries(UserIdUtil
 Generics
 PlainStorage2
 PlainStorage3

 UtilCommons


)

#install(TARGETS UserIdUtil DESTINATION ${INSTALL_DIR})