// BOOST
#include <boost/exception/diagnostic_information.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/iostreams/stream.hpp>

// STD
#include <iostream>
#include <sstream>

// THIS
#include <Generics/Function.hpp>
#include <PredictorML/Predictor.hpp>

namespace AdServer::PredictorML
{

namespace Aspect
{

inline constexpr char PREDICTOR_ML[] = "PREDICTOR_ML";

} // namespace Aspect

namespace
{

inline double calc_probability(const double raw) noexcept
{
  return 1.0 / (1.0 + std::exp(-raw));
}

inline void calc_probability(std::vector<double>& raw_vector) noexcept
{
  double sum = 0.0;
  for (auto& r : raw_vector)
  {
    r = std::exp(r);
    sum += r;
  }

  for (auto& r : raw_vector)
  {
    r = r / sum;
  }
}

inline std::size_t get_max_index(
  const std::vector<double>& data)
{
  std::size_t max_index = 0;
  double max = data[max_index];
  const std::size_t size = data.size();
  for (std::size_t index = 0; index < size; index += 1)
  {
    if (data[index] > max)
    {
      max = data[index];
      max_index = index;
    }
  }

  return max_index;
}

} // namespace

Predictor::Predictor(
  const std::string& model_path,
  Logger* logger)
  : logger_(ReferenceCounting::add_ref(logger))
{
  model_ = std::make_unique<ModelCalcerWrapper>(model_path);
  auto params = get_params(*model_);
  labels_ = get_labels(params);
  is_multiclass_ = is_multiclass(params);

  cat_features_count_ = model_->GetCatFeaturesCount();
  const auto cat_features_indexes = model_->GetCatFeatureIndices();
  {
    std::ostringstream stream;
    stream << "MLPredictor: Category features count="
           << cat_features_count_
           << "\n";
    stream << "Category indices=[";
    std::copy(
      std::begin(cat_features_indexes),
      std::end(cat_features_indexes),
      std::ostream_iterator<std::size_t>(stream, " "));
    stream << "]";
    logger_->info(stream.str(), Aspect::PREDICTOR_ML);
  }

  float_features_count_ = model_->GetFloatFeaturesCount();
  const auto float_features_indexes = model_->GetFloatFeatureIndices();
  {
    std::ostringstream stream;
    stream << "MLPredictor: Float features count="
           << float_features_count_
           << "\n";
    stream << "Float indices=[";
    std::copy(
      std::begin(float_features_indexes),
      std::end(float_features_indexes),
      std::ostream_iterator<std::size_t>(stream, " "));
    stream << "]";
    logger_->info(stream.str(), Aspect::PREDICTOR_ML);
  }
}

Predictor::Params Predictor::get_params(
  const ModelCalcerWrapper& model_calcer)
{
  const std::string params_key = "params";
  const bool exist = model_calcer.CheckMetadataHasKey(
    params_key);
  if (!exist)
  {
    std::ostringstream stream;
    stream << FNS
           << "Not correct metadata, not exist "
              "key=params in metadata";
    throw Exception(stream.str());
  }

  const std::string params = model_calcer.GetMetadataKeyValue(
    params_key);
  {
    std::ostringstream stream;
    stream << "Predictor: Applying model trained with params: "
           << params;
    logger_->info(stream.str(), Aspect::PREDICTOR_ML);
  }

  auto ptree = std::make_shared<Ptree>();
  try
  {
    boost::iostreams::stream<
      boost::iostreams::array_source> stream(
        params.data(),
        params.size());
    boost::property_tree::read_json(
      stream,
      *ptree);
  }
  catch(const boost::exception& exc)
  {
    std::ostringstream stream;
    stream << FNS
           << "Error parsing json, reason: "
           << boost::diagnostic_information(exc);
    throw Exception(stream.str());
  }

  return ptree;
}

Predictor::Labels Predictor::get_labels(
  const Params& params)
{
  Labels labels;
  labels.reserve(100);
  try
  {
    const auto& class_names = params->get_child(
      "data_processing_options.class_names");
    for (auto& item : class_names)
    {
      labels.emplace_back(
        item.second.get_value<Label>());
    }
  }
  catch (const boost::exception& exc)
  {
    std::ostringstream stream;
    stream << FNS
           << "Error params of json, reason: "
           << boost::diagnostic_information(exc);
    throw Exception(stream.str());
  }
  catch (const std::exception& exc)
  {
    std::ostringstream stream;
    stream << FNS
           << "Error params of json, reason: "
           << exc.what();
    throw Exception(stream.str());
  }

  return labels;
}

bool Predictor::is_multiclass(const Params& params)
{
  try
  {
    const std::string loss_function =
      params->get<std::string>("loss_function.type");
    return loss_function == "MultiClass";
  }
  catch (const boost::exception& exc)
  {
    std::ostringstream stream;
    stream << FNS
           << "Error params of json, reason: "
           << boost::diagnostic_information(exc);
    throw Exception(stream.str());
  }
  catch (const std::exception& exc)
  {
    std::ostringstream stream;
    stream << FNS
           << "Error params of json, reason: "
           << exc.what();
    throw Exception(stream.str());
  }
}

const Predictor::Labels& Predictor::labels() const noexcept
{
  return labels_;
}

std::size_t Predictor::cat_features_count() const noexcept
{
  return cat_features_count_;
}

std::size_t Predictor::float_features_count() const noexcept
{
  return float_features_count_;
}

Predictor::Probabilities Predictor::predict_proba(
  const CatFeatures& cat_features,
  const FloatFeatures& float_features) const
{
  if (cat_features.size() != cat_features_count_)
  {
    Stream::Error stream;
    stream << FNS
           << "size of cat_features must be equal "
           << cat_features_count_;
    throw Exception(stream);
  }

  if (float_features.size() != float_features_count_)
  {
    Stream::Error stream;
    stream << FNS
           << "size of float_features must be equal "
           << float_features_count_;
    throw Exception(stream);
  }

  if (is_multiclass_)
  {
    auto raw = model_->CalcMulti(
      float_features,
      cat_features);
    calc_probability(raw);
    return raw;
  }
  else
  {
    const double raw = model_->Calc(
      float_features,
      cat_features);
    const double probability = calc_probability(raw);
    return {std::abs(1.0 - probability), probability};
  }
}

Predictor::Label Predictor::predict(
  const CatFeatures& cat_features,
  const FloatFeatures& float_features) const
{
  if (cat_features.size() != cat_features_count_)
  {
    Stream::Error stream;
    stream << FNS
           << "size of cat_features must be equal "
           << cat_features_count_;
    throw Exception(stream);
  }

  if (float_features.size() != float_features_count_)
  {
    Stream::Error stream;
    stream << FNS
           << "size of float_features must be equal "
           << float_features_count_;
    throw Exception(stream);
  }

  if (is_multiclass_)
  {
    const auto raw = model_->CalcMulti(
      float_features,
      cat_features);
    const auto index = get_max_index(raw);
    return labels_[index];
  }
  else
  {
    const double raw = model_->Calc(
      float_features,
      cat_features);
    if (raw >= 0.0)
    {
      return labels_[1];
    }
    else
    {
      return labels_[0];
    }
  }
}

Predictor::ProbabilitiesList Predictor::predict_proba(
  const CatFeaturesList& cat_features_list,
  const FloatFeaturesList& float_features_list) const
{
  if (is_multiclass_)
  {
    Stream::Error stream;
    stream << FNS
           << "Supported only for singleclass model";
    throw Exception(stream);
  }

  for (const auto& cat_features : cat_features_list)
  {
    if (cat_features.size() != cat_features_count_)
    {
      Stream::Error stream;
      stream << FNS
             << "cat_features size must be equal = "
             << cat_features_count_;
      throw Exception(stream);
    }
  }

  for (const auto& float_features : float_features_list)
  {
    if (float_features.size() != float_features_count_)
    {
      Stream::Error stream;
      stream << FNS
             << "float_features size must be equal = "
             << float_features_count_;
      throw Exception(stream);
    }
  }

  const auto raw_list = model_->Calc(
    float_features_list,
    cat_features_list);

  ProbabilitiesList probabilities_list;
  probabilities_list.reserve(raw_list.size());
  for (const auto& raw : raw_list)
  {
    const auto p = calc_probability(raw);
    probabilities_list.emplace_back(
      Probabilities{std::abs(1.0 - p), p});
  }

  return probabilities_list;
}

Predictor::Labels Predictor::predict(
  const CatFeaturesList& cat_features_list,
  const FloatFeaturesList& float_features_list) const
{
  if (is_multiclass_)
  {
    Stream::Error stream;
    stream << FNS
           << "Supported only for singleclass model";
    throw Exception(stream);
  }

  if (cat_features_list.size() != float_features_list.size())
  {
    Stream::Error stream;
    stream << FNS
           << "cat_features_list and float_features_list"
              " must be the same size";
    throw Exception(stream);
  }

  for (const auto& cat_features : cat_features_list)
  {
    if (cat_features.size() != cat_features_count_)
    {
      Stream::Error stream;
      stream << FNS
             << "cat_features size must be equal = "
             << cat_features_count_;
      throw Exception(stream);
    }
  }

  for (const auto& float_features : float_features_list)
  {
    if (float_features.size() != float_features_count_)
    {
      Stream::Error stream;
      stream << FNS
             << "float_features size must be equal = "
             << float_features_count_;
      throw Exception(stream);
    }
  }

  const auto raw_list = model_->Calc(
    float_features_list,
    cat_features_list);
  Labels labels;
  labels.reserve(raw_list.size());
  for (const auto& raw : raw_list)
  {
    if (raw >= 0.0)
    {
      labels.emplace_back(labels_[1]);
    }
    else
    {
      labels.emplace_back(labels_[0]);
    }
  }

  return labels;
}

Predictor::Probabilities Predictor::flat_predict_proba(
  const Features& features) const
{
  const std::size_t number_features =
    cat_features_count_ + float_features_count_;
  if (features.size() != number_features)
  {
    Stream::Error stream;
    stream << FNS
           << "size of features must be equal "
           << number_features;
    throw Exception(stream);
  }

  if (is_multiclass_)
  {
    auto raw = model_->CalcFlatMulti(
      features);
    calc_probability(raw);
    return raw;
  }
  else
  {
    const double raw = model_->CalcFlat(
      features);
    const double probability = calc_probability(raw);
    return {std::abs(1.0 - probability), probability};
  }
}

Predictor::Label Predictor::flat_predict(
  const Features& features) const
{
  const std::size_t number_features =
    cat_features_count_ + float_features_count_;
  if (features.size() != number_features)
  {
    Stream::Error stream;
    stream << FNS
           << "size of features must be equal "
           << number_features;
    throw Exception(stream);
  }

  if (is_multiclass_)
  {
    auto raw = model_->CalcFlatMulti(
      features);
    const auto index = get_max_index(raw);
    return labels_[index];
  }
  else
  {
    const double raw = model_->CalcFlat(
      features);
    if (raw >= 0.0)
    {
      return labels_[1];
    }
    else
    {
      return labels_[0];
    }
  }
}

Predictor::ProbabilitiesList
Predictor::flat_predict_proba(
  const FeaturesList& features_list) const
{
  if (is_multiclass_)
  {
    Stream::Error stream;
    stream << FNS
           << "Supported only for singleclass model";
    throw Exception(stream);
  }

  auto raw_list = model_->CalcFlat(features_list);

  ProbabilitiesList probabilities_list;
  probabilities_list.reserve(raw_list.size());
  for (const auto raw : raw_list)
  {
    const auto p = calc_probability(raw);
    probabilities_list.emplace_back(
      Probabilities{std::abs(1.0 - p), p});
  }

  return probabilities_list;
}

Predictor::Labels Predictor::flat_predict(
  const FeaturesList& features_list) const
{
  if (is_multiclass_)
  {
    Stream::Error stream;
    stream << FNS
           << "Supported only for singleclass model";
    throw Exception(stream);
  }

  auto raw_list = model_->CalcFlat(features_list);
  Labels labels;
  labels.reserve(raw_list.size());
  for (const auto raw : raw_list)
  {
    if (raw >= 0.0)
    {
      labels.emplace_back(labels_[1]);
    }
    else
    {
      labels.emplace_back(labels_[0]);
    }
  }

  return labels;
}

} // namespace AdServer::PredictorML