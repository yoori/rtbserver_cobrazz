add_library(PlainStorage3 SHARED
  FileController.cpp 
  RandomAccessFile.cpp 
  FileReader.cpp 
  FileWriter.cpp 
  LoadingProgressCallbackBase.cpp 
  LoadingProgressCallback.cpp
)

target_link_libraries(PlainStorage3

 Generics
 Commons
)
#install(TARGETS PlainStorage3  DESTINATION ${INSTALL_DIR})
