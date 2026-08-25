
add_executable(GeoUtil
  Application.cpp
)
target_link_libraries(GeoUtil
  PRIVATE
    Generics
    Geoip
)
