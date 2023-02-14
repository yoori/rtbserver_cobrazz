add_library(ProfileMap SHARED

  ProfileMapFactory.cpp
  RocksDBProfileMap.cpp
)

target_link_libraries(ProfileMap

 Generics
 Commons
 rocksdb
)

#install(TARGETS ProfileMap  DESTINATION ${INSTALL_DIR})