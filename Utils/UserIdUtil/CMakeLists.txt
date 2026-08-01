

add_executable(UserIdUtil
  Application.cpp
)
target_link_libraries(UserIdUtil
  Generics
  Commons

  UtilCommons


)

#install(TARGETS UserIdUtil DESTINATION ${INSTALL_DIR})
