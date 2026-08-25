add_library(PlainCode SHARED
  Element.cpp
  Declarator.cpp

)
target_link_libraries(PlainCode
  PRIVATE
    PlainDeclaration
)
