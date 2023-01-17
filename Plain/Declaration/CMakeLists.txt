add_library(PlainDeclaration SHARED
  BaseType.cpp 
  BaseDescriptor.cpp 
  BaseReader.cpp 
  BaseWriter.cpp 
  BaseTemplate.cpp 
  CompleteTemplateDescriptor.cpp 
  Namespace.cpp 
  ListTemplate.cpp

)


target_link_libraries(PlainDeclaration
 Generics
)

install(TARGETS PlainDeclaration DESTINATION ${INSTALL_DIR})