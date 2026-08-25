

add_executable(TrivialCTRConfigGenerator
  Application.cpp
)
target_link_libraries(TrivialCTRConfigGenerator
  PRIVATE
    Generics

)

#install(TARGETS TrivialCTRConfigGenerator  DESTINATION ${INSTALL_DIR})
