include_directories(${IDL_DIRECTORY}/Plain)
add_yy(YYParser_yy YYParser.yy ${IDL_DIRECTORY}/Plain/Parsing)
add_ll(YYScanner_ll YYScanner.ll  ${IDL_DIRECTORY}/Plain/Parsing)


target_link_libraries(YYScanner_ll YYParser_yy)

add_library(PlainParser SHARED
  Processor.cpp 
  Parser.cpp

)
target_link_libraries(PlainParser 
YYParser_yy YYScanner_ll
PlainDeclaration
PlainCode
)

install(TARGETS PlainParser  DESTINATION ${INSTALL_DIR})