#ifndef RTBSERVER_PREDICTOR_HPP
#define RTBSERVER_PREDICTOR_HPP

// BOOST
#include <boost/property_tree/ptree.hpp>

// CATBOOST
#include <catboost/wrapped_calcer.h>

// UNIXCOMMONS
#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>
#include <Logger/Logger.hpp>

// STD
#include <functional>
#include <memory>

namespace AdServer::PredictorML
{

class Predictor final : Generics::Uncopyable
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using Probabilities = std::vector<double>;
  using ProbabilitiesList = std::vector<Probabilities>;
  using CatFeatures = std::vector<std::string>;
  using CatFeaturesList = std::vector<CatFeatures>;
  using FloatFeatures = std::vector<float>;
  using FloatFeaturesList = std::vector<FloatFeatures>;
  using Features = std::vector<float>;
  using FeaturesList = std::vector<Features>;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  using Label = int;
  using Labels = std::vector<Label>;
  using ModelCalcerPtr = std::unique_ptr<ModelCalcerWrapper>;
  using Ptree = boost::property_tree::ptree;
  using Params = std::shared_ptr<Ptree>;

public:
  explicit Predictor(
    const std::string& model_path,
    Logger* logger);

  ~Predictor() = default;

  const Labels& labels() const noexcept;

  std::size_t cat_features_count() const noexcept;

  std::size_t float_features_count() const noexcept;

  Probabilities predict_proba(
    const CatFeatures& cat_features,
    const FloatFeatures& float_features) const;

  Label predict(
    const CatFeatures& cat_features,
    const FloatFeatures& float_features) const;

  ProbabilitiesList predict_proba(
    const CatFeaturesList& cat_features_list,
    const FloatFeaturesList& float_features_list) const;

  Labels predict(
    const CatFeaturesList& cat_features_list,
    const FloatFeaturesList& float_features_list) const;

  // 1. Сategorical features and numbers in one vector
  //    according to their order
  // 2. For categorical features, it is mandatory to use
  //    "convert_int" function (see unit tests)
  Probabilities flat_predict_proba(
    const Features& features) const;

  // 1. Сategorical features and numbers in one vector
  //    according to their order
  // 2. For categorical features, it is mandatory to use
  //    "convert_int" function (see unit tests)
  Label flat_predict(
    const Features& features) const;

  // **WARNING** currently supports only singleclass models.
  ProbabilitiesList flat_predict_proba(
    const FeaturesList& features_list) const;

  // **WARNING** currently supports only singleclass models.
  Labels flat_predict(
    const FeaturesList& features_list) const;

private:
  Params get_params(const ModelCalcerWrapper& model_calcer);

  Labels get_labels(const Params& params);

  bool is_multiclass(const Params& params);

private:
  Logger_var logger_;

  Labels labels_;

  ModelCalcerPtr model_;

  std::size_t cat_features_count_;

  std::size_t float_features_count_;

  bool is_multiclass_ = false;
};

inline float convert_int(const int val)
{
  const std::size_t buffer_size = 10;
  char buffer[buffer_size]{};
  std::to_chars_result result = std::to_chars(
    buffer,
    buffer + buffer_size,
    val);

  if (result.ec != std::errc())
  {
    std::ostringstream stream;
    stream << FNS
           << "Function to_chars is failed. Reason:"
           << std::make_error_code(result.ec).message();
    throw std::runtime_error(stream.str());
  }

  const int hash = GetStringCatFeatureHash(
    buffer,
    result.ptr - buffer);

  float ret = 0;
  std::memcpy(
    &ret,
    &hash,
    sizeof(float));
  return ret;
}

} // namespace AdServer::PredictorML

#endif //RTBSERVER_PREDICTOR_HPP