

add_executable(UserIdUtil
  Application.cpp
)
target_link_libraries(UserIdUtil
  PRIVATE
    Generics
    Commons
)
