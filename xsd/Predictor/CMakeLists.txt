
add_xsd(CTRPredictorSVMGeneratorConfigXSD CTRPredictorSVMGeneratorConfig.xsd ${XSD_DIRECTORY}/xsd/Predictor)
add_xsd(PredictorMergerConfigXSD PredictorMergerConfig.xsd ${XSD_DIRECTORY}/xsd/Predictor)


add_dependencies(CTRPredictorSVMGeneratorConfigXSD CTRGeneratorConfigXSD)