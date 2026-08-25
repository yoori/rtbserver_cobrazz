add_executable(LogGeneralizerAdmin
  Application.cpp
)

target_link_libraries(LogGeneralizerAdmin
  PRIVATE
    TAO::PortableServer
    TAO::Valuetype
    LogGeneralizerIDL
    ProcessControlIDL
    pthread
    Generics
    CORBACommons
    UtilCommons
)
