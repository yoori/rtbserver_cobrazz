add_executable(BillingAdmin
  Application.cpp
)

target_link_libraries(BillingAdmin
  BillingServerIDL
  CampaignCommonsIDL

  Commons
  CORBACommons
  Generics
)
