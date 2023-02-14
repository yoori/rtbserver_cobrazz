add_library(PlainCode SHARED
  Element.cpp 
  Declarator.cpp

)
target_link_libraries( PlainCode
PlainDeclaration
)

#install(TARGETS PlainCode  DESTINATION ${INSTALL_DIR})