#ifndef CLICKMODEL_BENCHMARK_HPP
#define CLICKMODEL_BENCHMARK_HPP

// STD
#include <string>

// THIS
#include <PredictorML/Predictor.hpp>

class Benchmark final
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

private:
  using Predictor = AdServer::PredictorML::Predictor;
  using PredictorPtr = std::unique_ptr<Predictor>;
  using CatFeaturesList = Predictor::CatFeaturesList;
  using FloatFeaturesList = Predictor::FloatFeaturesList;

  struct Data final
  {
    Data() = default;

    CatFeaturesList cat_features_list;
    FloatFeaturesList float_features_list;
  };

public:
  explicit Benchmark(
    Logger* logger,
    const std::string& model_path,
    const std::string& csv_path,
    const std::uint32_t number_line);

  int run() noexcept;

private:
  void model_process(const Data& data);

  void model_process_bunch(const Data& data);

  void prepare_date(Data& data);

private:
  const Logger_var logger_;

  const std::string model_path_;

  const std::string csv_path_;

  const std::uint32_t number_line_;

  PredictorPtr predictor_;
};

#endif //CLICKMODEL_BENCHMARK_HPP
