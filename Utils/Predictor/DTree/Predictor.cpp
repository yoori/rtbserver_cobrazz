#include "Predictor.hpp"
#include "DTree.hpp"

namespace Vanga
{
  // Predictor
  PredArrayHolder_var
  Predictor::predict(const RowArray& rows) const noexcept
  {
    PredArrayHolder_var res = new PredArrayHolder();
    res->values.resize(rows.size());

    unsigned long i = 0;
    for(auto it = rows.begin(); it != rows.end(); ++it, ++i)
    {
      res->values[i] = predict((*it)->features);
    }

    return res;
  }

  DTree_var
  Predictor::as_dtree() noexcept
  {
    return nullptr;
  }

  PredictorSet_var
  Predictor::as_predictor_set() noexcept
  {
    return nullptr;
  }

  // LogRegPredictor
  LogRegPredictor::LogRegPredictor(Predictor* predictor) noexcept
    : predictor_(ReferenceCounting::add_ref(predictor))
  {}

  double
  LogRegPredictor::predict(const FeatureArray& features) const noexcept
  {
    const double DOUBLE_ONE = 1.0;
    return DOUBLE_ONE / (DOUBLE_ONE + std::exp(-predictor_->predict(features)));
  }

  void
  LogRegPredictor::save(std::ostream& ostr) const
  {
    predictor_->save(ostr);
  }

  std::string
  LogRegPredictor::to_string(
    const char* prefix,
    const FeatureDictionary* dict,
    double base)
    const noexcept
  {
    return predictor_->to_string(prefix, dict, base);
  }

  DTree_var
  LogRegPredictor::as_dtree() noexcept
  {
    return predictor_->as_dtree();
  }

  PredictorSet_var
  LogRegPredictor::as_predictor_set() noexcept
  {
    return predictor_->as_predictor_set();
  }
}
