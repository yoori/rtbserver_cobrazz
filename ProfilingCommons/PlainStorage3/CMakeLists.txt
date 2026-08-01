add_library(PlainStorage3 SHARED
  RandomAccessFile.cpp
)

target_link_libraries(PlainStorage3
  ProfilingCommons
  Generics
  Commons
)
#install(TARGETS PlainStorage3  DESTINATION ${INSTALL_DIR})
