add_executable(logProcessingLogIoTest
 LogIoTest.cpp
)

include_directories()

if(LINUX)
set(LINUXLINK "Generics")
endif()

target_link_libraries(logProcessingLogIoTest LogCommons)


