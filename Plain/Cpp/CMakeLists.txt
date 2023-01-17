add_library(PlainCpp SHARED
  DescriptorGenerator.cpp 
  ReaderGenerator.cpp 
  WriterGenerator.cpp 
  Generator.cpp
)

target_link_libraries(PlainCpp
 Generics
 PlainDeclaration
 PlainCode
)
install(TARGETS PlainCpp DESTINATION ${INSTALL_DIR})