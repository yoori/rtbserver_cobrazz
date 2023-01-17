add_executable(PlainCppCompiler
    CppCompiler.cpp
)

target_link_libraries(PlainCppCompiler
    PlainParser
    PlainCpp
)
install(TARGETS PlainCppCompiler  DESTINATION ${INSTALL_DIR})