
add_idl(LogGeneralizerIDL LogGeneralizer.idl ${IDL_DIRECTORY}/LogProcessing/LogGeneralizer)
target_link_libraries(LogGeneralizerIDL 
    CorbaTypesIDL
)

add_library(LogGeneralizer STATIC
           LogGeneralizerImplInit.cpp  
           LogGeneralizerImplInit2.cpp 
           LogGeneralizerImpl.cpp  
           LogDbSaverImpl.cpp      
           DbConnectionFactory.cpp 
           ProcessingContexts.cpp

)
target_link_libraries(LogGeneralizer
    LogGeneralizerIDL
)
add_executable(LogGeneralizerMain

        LogGeneralizerMain.cpp  
)
target_link_libraries(LogGeneralizerMain
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
)

install(TARGETS LogGeneralizerIDL LogGeneralizerMain  DESTINATION ${INSTALL_DIR})

