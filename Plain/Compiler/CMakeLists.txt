add_executable(PlainCppCompiler
  CppCompiler.cpp
)

target_link_libraries(PlainCppCompiler
  PRIVATE
    PlainParser
    PlainCpp
    PlainCode
    PlainDeclaration
    Generics
)
