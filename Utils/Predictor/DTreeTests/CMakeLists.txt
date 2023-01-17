
add_executable(DTreeUtilsTest
DTreeUtilsTest.cpp
)
target_link_libraries(DTreeUtilsTest

 Generics
    DTree
)

install(TARGETS DTreeUtilsTest DESTINATION ${INSTALL_DIR})

