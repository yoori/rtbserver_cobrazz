add_executable(LogHolderPortionedTest
  LogHolderPortionedTest.cpp
)

target_link_libraries(LogHolderPortionedTest
  PRIVATE
    Generics
    LogCommons
    Logger
)
