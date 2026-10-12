add_executable(LogGeneralizerAdmin
  Application.cpp
)

target_link_libraries(LogGeneralizerAdmin
  PRIVATE
    LogGeneralizerGrpc
    Threads::Threads
    Generics
    UtilCommons
)
