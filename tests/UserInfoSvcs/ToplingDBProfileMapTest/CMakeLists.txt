add_executable(ToplingDBProfileMapTest
  ToplingDBProfileMapTest.cpp)

target_link_libraries(ToplingDBProfileMapTest
  PRIVATE
    Generics
    Commons
    ProfileMap
    ProfileMapToplingDB
)

target_include_directories(ToplingDBProfileMapTest
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)
