add_library(PlainStorage2 SHARED

 File.cpp 
  FileLayer.cpp 
  MemLayer.cpp 
  FragmentLayer.cpp 
  DefaultAllocatorLayer.cpp
)

target_link_libraries(PlainStorage2

 Generics
 Commons
)

#install(TARGETS PlainStorage2  DESTINATION ${INSTALL_DIR})