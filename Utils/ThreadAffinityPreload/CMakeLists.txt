add_library(ThreadAffinityPreload SHARED
  ThreadAffinityPreload.c
)

target_link_libraries(ThreadAffinityPreload
  PRIVATE
    ${CMAKE_DL_LIBS}
    Threads::Threads
)
