add_executable(TrivialCTRConfigGenerator
  Application.cpp
)
target_link_libraries(TrivialCTRConfigGenerator
  PRIVATE
    Generics
)
