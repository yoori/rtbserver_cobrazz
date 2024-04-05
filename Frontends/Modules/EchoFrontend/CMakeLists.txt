add_library(EchoFrontend SHARED
  EchoFrontend.cpp
)

target_link_libraries(EchoFrontend
  Generics
  UServerUtils
)