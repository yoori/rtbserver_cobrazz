add_executable(CTRConfigFolderTest
  CTRConfigFolderTest.cpp
)

target_link_libraries(CTRConfigFolderTest
  PRIVATE
    CTRConfigSelector
    Generics
)

add_custom_target(CTRConfigFolderCompatibilityTest
  COMMAND $<TARGET_FILE:CTRConfigFolderTest>
  DEPENDS CTRConfigFolderTest
)
