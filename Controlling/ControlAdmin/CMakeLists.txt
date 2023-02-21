add_executable(ControlAdmin
    Application.cpp
)
target_link_libraries(ControlAdmin
    ProcessControlIDL  CORBACommons StatsIDL StatsCollectorIDL Generics pthread UtilCommons
    CorbaTypesIDL
)


#install(TARGETS ControlAdmin  DESTINATION ${INSTALL_DIR})