
add_executable(SVMGenerator
  Application.cpp
)
target_link_libraries(SVMGenerator
  PRIVATE
    Generics
    Commons
    SVMGeneratorConfigXSD
)
#install(TARGETS SVMGenerator  DESTINATION ${INSTALL_DIR})
