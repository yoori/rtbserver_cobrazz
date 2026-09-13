add_executable(CTRConfigFolderTest
  CTRConfigFolderTest.cpp
)

target_link_libraries(CTRConfigFolderTest
  PRIVATE
    CTRConfigSelector
    Generics
)

target_include_directories(CTRConfigFolderTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)

add_custom_target(CTRConfigFolderCompatibilityTest
  COMMAND $<TARGET_FILE:CTRConfigFolderTest>
  DEPENDS CTRConfigFolderTest
)
