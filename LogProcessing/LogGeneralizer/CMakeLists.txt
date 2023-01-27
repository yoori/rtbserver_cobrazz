
add_idl(LogGeneralizerIDL LogGeneralizer.idl ${IDL_DIRECTORY}/LogProcessing/LogGeneralizer)

target_link_libraries(LogGeneralizerIDL 
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
  LogGeneralizerIDL
)

add_executable(LogGeneralizer
  LogGeneralizerMain.cpp  
)

target_link_libraries(LogGeneralizer
  LogGeneralizerLib
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

install(TARGETS LogGeneralizerIDL LogGeneralizer DESTINATION ${INSTALL_DIR})

