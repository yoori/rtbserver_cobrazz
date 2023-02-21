
add_executable(CTRPredictorSVMGenerator
  FileGuard.cpp 
  Utils.cpp 
  CampaignFolder.cpp 
  FeatureContainer.cpp 
  OutputLibSvmStream.cpp 
  InputCsvStream.cpp 
  CTRPredictorSVMGeneratorImpl.cpp 
  CTRPredictorSVMGeneratorMain.cpp

)
target_link_libraries(CTRPredictorSVMGenerator

 Generics
 Commons
 AdServerCommonsXSD
 XMLUtility

 CORBACommons
 ProcessControlVars
 CTRPredictorSVMGeneratorConfigXSD
 CTRGeneratorLib


)

#install(TARGETS CTRPredictorSVMGenerator  DESTINATION ${INSTALL_DIR})



