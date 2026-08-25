add_executable(DTreeTrainer
  Application.cpp
)
target_link_libraries(DTreeTrainer
  PRIVATE
    Commons
    Generics
    Sync
    DTree
)


#install(TARGETS DTreeTrainer  DESTINATION ${INSTALL_DIR})
