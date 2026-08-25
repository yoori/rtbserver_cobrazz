add_library(CommonModule SHARED
  CommonModule.cpp
)

target_link_libraries(CommonModule
  PRIVATE
    CampaignServerIDL
    Commons
    CORBAConfig
    RequestMatchers
    DomainParser
    Geoip
    Polyglot
    UserIdController
    GenericSegmentor
    FeConfigXSD
    DomainConfigXSD
    Generics
)
