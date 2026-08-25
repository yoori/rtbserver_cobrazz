add_idl(LogGeneralizerIDL LogGeneralizer.idl ${IDL_DIRECTORY}/LogProcessing/LogGeneralizer)

target_link_libraries(LogGeneralizerIDL
  PUBLIC
    CorbaTypesIDL
)

add_library(LogGeneralizerLib STATIC
  LogGeneralizerImplInit.cpp
  LogGeneralizerImplInit2.cpp
  LogGeneralizerImpl.cpp
  LogDbSaverImpl.cpp
  DbConnectionFactory.cpp
  ProcessingContexts.cpp
)

target_link_libraries(LogGeneralizerLib
  PUBLIC
    LogGeneralizerIDL
    LogCommons
  PRIVATE
    Postgres
    Commons
    CORBACommons
    CorbaTypesIDL
    Generics
    HTTP
    Logger
    ProcessControlIDL
    Sync
)

add_executable(LogGeneralizer
  LogGeneralizerMain.cpp
)

target_link_libraries(LogGeneralizer
  PRIVATE
    LogGeneralizerLib
    Threads::Threads
    Generics
    Commons
    CORBAConfig
    CORBACommons
    Logger
    ProcessControlIDL
    StatsIDL
    LogGeneralizerConfigXSD
    SNMPAgent
    ProcessControlVars
)
