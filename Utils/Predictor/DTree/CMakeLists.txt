
add_library(DTree SHARED


 Utils.cpp 
  Predictor.cpp 
  SVM.cpp 
  DTree.cpp 
  TreeLearner.cpp
)
target_link_libraries(DTree

 Generics
 Commons

)


#install(TARGETS DTree DESTINATION ${INSTALL_DIR})