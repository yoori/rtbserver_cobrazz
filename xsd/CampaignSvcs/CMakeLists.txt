
add_xsd(BillingServerConfigXSD BillingServerConfig.xsd ${XSD_DIRECTORY}/xsd/CampaignSvcs)
add_xsd(CampaignManagerConfigXSD CampaignManagerConfig.xsd ${XSD_DIRECTORY}/xsd/CampaignSvcs)
add_xsd(CampaignServerConfigXSD CampaignServerConfig.xsd ${XSD_DIRECTORY}/xsd/CampaignSvcs)
add_xsd(DomainConfigXSD DomainConfig.xsd ${XSD_DIRECTORY}/xsd/CampaignSvcs)
#add_xsd(HostDistributionFileXSD HostDistributionFile.xsd ${XSD_DIRECTORY}/xsd/AdServerCommons)


add_dependencies(CampaignManagerConfigXSD AdServerCommonsXSD)


