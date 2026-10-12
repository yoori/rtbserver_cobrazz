add_library(CommonModule SHARED
  CommonModule.cpp
)

target_link_libraries(CommonModule
  PRIVATE
    CampaignServerGrpc
    Commons
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
