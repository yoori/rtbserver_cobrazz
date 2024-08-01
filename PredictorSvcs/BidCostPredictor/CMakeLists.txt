add_library(BidCostPredictorLib SHARED
  Aggregator.cpp
  AggregatorMultyThread.cpp
  Application.cpp
  BidCostCollector.cpp
  Configuration.cpp
  ContainerImpl.cpp
  CreativeProvider.cpp
  CtrCollector.cpp
  Daemon.cpp
  DaemonImpl.cpp
  DataModelProviderImpl.cpp
  LogHelper.cpp
  ModelBidCostImpl.cpp
  ModelCtrImpl.cpp
  ModelEvaluatorBidCost.cpp
  ModelEvaluatorCtr.cpp
  ModelProcessor.cpp
  Process.cpp
  Reaggregator.cpp
  ReaggregatorMultyThread.cpp
  Regenerator.cpp
  Storage.cpp
  Utils.cpp
)

target_link_libraries(BidCostPredictorLib
  Commons
  Generics
  LogCommons
  Postgres
)

add_executable(BidCostPredictor
  Main.cpp
)

target_link_libraries(BidCostPredictor
  BidCostPredictorLib
)

add_executable(BidCostPredictorTest
  Test.cpp
)

target_link_libraries(BidCostPredictorTest
  BidCostPredictorLib
)

install(
  TARGETS
    BidCostPredictor
    BidCostPredictorLib
    BidCostPredictorTest
  DESTINATION
    ${INSTALL_DIR}
)