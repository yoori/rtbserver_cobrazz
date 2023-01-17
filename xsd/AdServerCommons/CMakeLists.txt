
add_xsd(AdServerCommonsXSD AdServerCommons.xsd ${XSD_DIRECTORY}/xsd/AdServerCommons)
add_xsd(HostDistributionFileXSD HostDistributionFile.xsd ${XSD_DIRECTORY}/xsd/AdServerCommons)

add_dependencies(HostDistributionFileXSD AdServerCommonsXSD)
