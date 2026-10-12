include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)
add_yy(YYParser_yy YYParser.yy ${CMAKE_CURRENT_BINARY_DIR})
add_ll(YYScanner_ll YYScanner.ll ${CMAKE_CURRENT_BINARY_DIR})

add_dependencies(YYScanner_ll YYParser_yy)

add_library(PlainParser SHARED
  Processor.cpp
  Parser.cpp

)
target_link_libraries(PlainParser
  PRIVATE
    YYParser_yy
    YYScanner_ll
    PlainDeclaration
    PlainCode
    Generics
)
