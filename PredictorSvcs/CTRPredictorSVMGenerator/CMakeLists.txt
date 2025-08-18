
add_library(CTRPredictorSVMGeneration
  FileGuard.cpp 
  Utils.cpp 
  FeatureContainer.cpp 
  OutputLibSvmStream.cpp 
  InputCsvStream.cpp 
)

add_executable(CTRPredictorSVMGenerator
  CampaignFolder.cpp 
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
  CTRPredictorSVMGeneration
)

add_executable(CTRPredictorSVMGenerateUtil
  CTRPredictorSVMGenerateUtil.cpp 
)

target_link_libraries(CTRPredictorSVMGenerateUtil
  Generics
  Commons
  CTRGeneratorLib
  CTRPredictorSVMGeneration
)
