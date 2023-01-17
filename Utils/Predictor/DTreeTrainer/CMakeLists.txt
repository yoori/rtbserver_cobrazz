add_executable(DTreeTrainer
    Application.cpp
)
target_link_libraries(DTreeTrainer
    Generics
    DTree
)


install(TARGETS DTreeTrainer  DESTINATION ${INSTALL_DIR})