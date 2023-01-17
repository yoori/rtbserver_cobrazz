
add_executable(GeoUtil
Application.cpp
)
target_link_libraries(GeoUtil
 Generics

 Commons
Geoip

)



install(TARGETS GeoUtil DESTINATION ${INSTALL_DIR})