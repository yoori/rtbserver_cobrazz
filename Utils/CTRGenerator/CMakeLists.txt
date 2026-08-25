include_directories(/opt/foros/vanga/include)

add_library(CTRGeneratorLib SHARED
  CalculateParamsFilter.cpp
  CTRGenerator.cpp
  FeatureConfig.cpp
)

target_link_libraries(CTRGeneratorLib
  PRIVATE
  Commons
  Generics
  CTRGeneratorDataConfigXSD
  SVMGeneratorConfigXSD
  CTRProvider
)

add_executable(CTRGenerator
  Application.cpp
)

target_link_libraries(CTRGenerator
  PRIVATE
    CTRGeneratorLib
    CTRGeneratorDataConfigXSD
    CTRProvider
    CTRProviderImplLib
    CampaignConfig
    Commons
    Generics
    HTTP
    ProfilingCommons
)
