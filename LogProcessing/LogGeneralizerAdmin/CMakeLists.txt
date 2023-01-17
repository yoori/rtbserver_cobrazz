

add_executable(LogGeneralizerAdminMain




    Application.cpp
)
target_link_libraries(LogGeneralizerAdminMain
    LogGeneralizer
    LogGeneralizerIDL
    pthread
    Generics
    Commons
    CORBACommons
    ProcessControlIDL
    StatsIDL
    Postgres
    LogCommons
    LogGeneralizerConfigXSD
    SNMPAgent
    ProcessControlVars
    UtilCommons
    String

    
)


install(TARGETS LogGeneralizerAdminMain  DESTINATION ${INSTALL_DIR})