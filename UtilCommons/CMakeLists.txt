add_library(UtilCommons SHARED
Table.cpp
)
target_link_libraries(UtilCommons
String


)

install(TARGETS UtilCommons DESTINATION ${INSTALL_DIR})



