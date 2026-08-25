add_library(ProfilingCommons SHARED
  FileController.cpp
  FileReader.cpp
  FileWriter.cpp
)

target_link_libraries(ProfilingCommons
  PRIVATE
    Generics
)

ADD_SUBDIRECTORY(ProfileMap)

add_library(MessageSaver SHARED
  MessageSaver.cpp
)

target_link_libraries(MessageSaver
  PRIVATE
    Generics
    ProfilingCommons
    LogCommons
)
