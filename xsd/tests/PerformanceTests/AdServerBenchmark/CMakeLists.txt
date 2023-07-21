add_xsd(AdServerBenchmarkConfigXSD AdServerBenchmarkConfig.xsd ${XSD_DIRECTORY}/xsd/tests/PerformanceTests/AdServerBenchmark)

target_link_libraries(AdServerBenchmarkConfigXSD
  AdServerTestCommonConfigXSD
)
