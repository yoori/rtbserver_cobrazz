add_executable(LogGeneralizerAdmin
  Application.cpp
)

target_link_libraries(LogGeneralizerAdmin
  PRIVATE
    LogGeneralizerIDL
    ProcessControlIDL
    Threads::Threads
    Generics
    CORBACommons
    UtilCommons
)
