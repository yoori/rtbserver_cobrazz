

add_executable(TrivialCTRConfigGenerator
 Application.cpp
)
target_link_libraries(TrivialCTRConfigGenerator
 Generics
 Commons

)

#install(TARGETS TrivialCTRConfigGenerator  DESTINATION ${INSTALL_DIR})