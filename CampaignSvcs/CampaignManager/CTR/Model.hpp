#ifndef CAMPAIGNMANAGER_CTR_MODEL_HPP_
#define CAMPAIGNMANAGER_CTR_MODEL_HPP_

#include <optional>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#include "CTREvaluator.hpp"
#include "Feature.hpp"
#include "ModelTraits.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  enum FeatureType
  {
    FT_REQUEST = 0,
    FT_AUCTION,
    FT_CANDIDATE,
    FT_MAX
  };

  enum ModelMethodType
  {
    MM_FTRL,
    MM_XGBOOST,
    MM_VANGA,
    MM_TRIVIAL,
    MM_CATBOOST
  };

  // Model
  struct Model:
    public ReferenceCounting::AtomicImpl,
    public ModelTraits
  {
  public:
    Model(unsigned long model_id) noexcept;

    void
    load_feature_weights(
      const String::SubString& file)
      /*throw(InvalidConfig)*/;

  public:
    const unsigned long model_id;
    ModelMethodType method;
    std::string method_name;
    RevenueDecimal weight;

    // FT_REQUEST: features can be calculated once for request (
    //   at CTRProvider::create_calculation)
    // FT_AUCTION: features can be calculated only when know context (tag size)
    //   but once for all campaigns/creatives
    // FT_CANDIDATE: features can be calculated only when known candidate - campaign/creative (
    //   at CalculationContext::get_ctr
    FeatureArray features[FT_MAX];

    // TODO !!! Fill
    uint64_t feature_set_indexes[FT_MAX];
    std::optional<FeatureType> max_feature_type;

    unsigned long features_size = 16 * 1024;
    std::shared_ptr<CTREvaluator> ctr_evaluator;

  protected:
    virtual ~Model() noexcept
    {}
  };

  using Model_var = ReferenceCounting::SmartPtr<Model>;
  using ModelList = std::list<Model_var>;
}

#endif /*CAMPAIGNMANAGER_CTR_MODEL_HPP_*/
