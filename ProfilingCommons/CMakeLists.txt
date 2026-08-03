add_library(ProfilingCommons SHARED
  FileController.cpp
  FileReader.cpp
  FileWriter.cpp
)

target_link_libraries(ProfilingCommons
  Generics
  Commons
)

ADD_SUBDIRECTORY(ProfileMap)

add_library(MessageSaver SHARED
  MessageSaver.cpp
)

target_link_libraries(MessageSaver
  Generics
  ProfilingCommons
  PrivacyFilter
  Commons
  LogCommons
)
