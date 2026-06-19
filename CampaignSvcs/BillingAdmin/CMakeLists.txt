add_executable(BillingAdmin
  Application.cpp
)

target_link_libraries(BillingAdmin
  BillingServerGrpcClient
  CampaignCommonsIDL

  Commons
  Generics
)
