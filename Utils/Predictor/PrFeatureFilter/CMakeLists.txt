add_executable(PrFeatureFilter
  Application.cpp

)
target_link_libraries(PrFeatureFilter
    
 Generics

 DTree
 CTRGeneratorConfigXSD
 CTRGeneratorDataConfigXSD
 CTRProvider

)
install(TARGETS PrFeatureFilter  DESTINATION ${INSTALL_DIR})


