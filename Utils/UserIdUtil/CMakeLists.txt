

add_executable(UserIdUtil
  Application.cpp
)
target_link_libraries(UserIdUtil
  Generics
  PlainStorage2

  UtilCommons


)

#install(TARGETS UserIdUtil DESTINATION ${INSTALL_DIR})
