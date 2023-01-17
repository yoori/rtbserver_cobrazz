
add_executable(SVMGenerator
Application.cpp
)
target_link_libraries(SVMGenerator
 Generics

 Commons
# IPMap
#Geoip
SVMGeneratorConfigXSD

)
install(TARGETS SVMGenerator  DESTINATION ${INSTALL_DIR})


