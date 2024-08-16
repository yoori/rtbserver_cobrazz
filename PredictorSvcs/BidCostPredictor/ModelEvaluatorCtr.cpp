// STD
#include <optional>
#include <unordered_map>

// THIS
#include "ModelEvaluatorCtr.hpp"

namespace Aspect
{

inline constexpr char MODEL_EVALUATOR_CTR[] = "MODEL_EVALUATOR_CTR";

} // namespace Aspect

namespace std
{

template<>
struct hash<std::pair<unsigned long, unsigned long>>
{
  std::size_t operator()(const std::pair<unsigned long, unsigned long>& value) const noexcept
  {
    std::size_t seed = 0;
    boost::hash_combine(seed, value.first);
    boost::hash_combine(seed, value.second);
    return seed;
  }
};

} // namespace std

namespace PredictorSvcs::BidCostPredictor
{

ModelEvaluatorCtrImpl::ModelEvaluatorCtrImpl(
  const Imps trust_imps,
  const Imps tag_imps,
  DataModelProvider* data_provider,
  ModelCtrFactory* model_factory,
  Logger* logger)
  : trust_imps_(trust_imps),
    tag_imps_(tag_imps),
    data_provider_(ReferenceCounting::add_ref(data_provider)),
    model_factory_(ReferenceCounting::add_ref(model_factory)),
    logger_(ReferenceCounting::add_ref(logger))
{
  if (!data_provider_)
  {
    Stream::Error stream;
    stream << FNS
           << "data_provider is null";
    throw Exception(stream);
  }

  if (!model_factory_)
  {
    Stream::Error stream;
    stream << FNS
           << "model_factory is null";
    throw Exception(stream);
  }
}

ModelCtr_var ModelEvaluatorCtrImpl::evaluate() noexcept
{
  try
  {
    if (is_stopped_.load())
    {
      std::ostringstream stream;
      stream << FNS
             << "ModelEvaluatorCtr already is stopped";
      logger_->info(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
      return {};
    }

    ModelCtr_var model = model_factory_->create();
    if (!model)
    {
      std::ostringstream stream;
      stream << FNS
             << "model is null";
      logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
      return {};
    }

    CtrHelpCollector collector(1);
    if (!data_provider_->load(collector))
    {
      Stream::Error stream;
      stream << FNS
             << "data_provider load of collector is failed";
      logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
      return {};
    }

    if (collector.empty())
    {
      std::ostringstream stream;
      stream << FNS
             << "Collector is empty";
      logger_->info(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
      return model;
    }

    {
      std::ostringstream stream;
      stream << FNS
             << "ModelEvaluatorCtr started";
      logger_->info(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
    }

    calculate(collector, *model);

    {
      std::ostringstream stream;
      stream << FNS
             << "ModelEvaluatorCtr is successfully stopped";
      logger_->info(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
    }

    return model;
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "ModelEvaluatorCtr is failed"
           << " : Reason: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
  }
  catch (...)
  {
    Stream::Error stream;
    stream << FNS
           << "ModelEvaluatorCtr is failed"
           << " : Reason: Unknown error";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
  }

  return {};
}

void ModelEvaluatorCtrImpl::calculate(
  const CtrHelpCollector& collector,
  ModelCtr& model)
{
  using Data = std::pair<Clicks, Imps>;
  using HelperTagHash = std::unordered_map<
    std::pair<TagId, CreativeCategoryId>,
    Data>;

  HelperTagHash helper_tag_hash;
  helper_tag_hash.reserve(1000000);

  FixedNumber coef("0.5");
  const FixedNumber one(false, 1, 0);

  std::size_t records_reached_1000_imps = 0;
  std::size_t records_reached_10000_imps = 0;
  std::size_t records_reached_100000_imps = 0;

  auto it = collector.begin();
  const auto it_end = collector.end();
  for (; it != it_end; ++it)
  {
    const auto& key = it->first;
    const auto& creative_category_id = key.creative_category_id();
    const auto& tag_id = key.tag_id();
    const auto& url = key.url_ptr();
    const auto& data = it->second;
    const auto& total_clicks = data.total_clicks;
    const auto& total_imps = data.total_imps;

    if (total_imps >= 1000)
    {
      records_reached_1000_imps += 1;
    }
    if (records_reached_10000_imps >= 10000)
    {
      records_reached_10000_imps += 1;
    }
    if (records_reached_100000_imps >= 100000)
    {
      records_reached_100000_imps += 1;
    }

    std::optional<FixedNumber> ctr;
    if (total_imps >= trust_imps_)
    {
      ctr = FixedNumber::div(
        FixedNumber(false, total_clicks, 0),
        FixedNumber(false, total_imps, 0));
    }
    else if (total_imps > 0 &&
      (*url == "?" || creative_category_id == 0))
    {
      const FixedNumber corr_coef = coef +
        FixedNumber::div(
          FixedNumber::mul(
            one - coef,
            FixedNumber(false, total_imps, 0),
            Generics::DMR_CEIL),
          FixedNumber(false, trust_imps_, 0));

      const FixedNumber base_ctr = FixedNumber::div(
        FixedNumber(false, total_clicks, 0),
        FixedNumber(false, total_imps, 0));

      ctr = FixedNumber::mul(base_ctr, corr_coef, Generics::DMR_CEIL);
    }

    if (total_imps > 0 && total_imps < tag_imps_)
    {
      auto it = helper_tag_hash.find(std::pair{tag_id, creative_category_id});
      if (it == helper_tag_hash.end())
      {
        helper_tag_hash.try_emplace(
          std::pair{tag_id, creative_category_id},
          total_clicks,
          total_imps);
      }
      else
      {
        auto& data = it->second;
        data.first += total_clicks;
        data.second += total_imps;
      }
    }

    if (ctr && !ctr->is_zero())
    {
      model.set_ctr(tag_id, *url, creative_category_id, *ctr);
    }
  }

  {
    std::ostringstream stream;
    stream << '\n'
           << "Records reached 1000 imps : "
           << records_reached_1000_imps
           << '\n'
           << "Records reached 10000 imps : "
           << records_reached_10000_imps
           << '\n'
           << "Records reached 100000 imps : "
           << records_reached_100000_imps;
    logger_->info(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
  }

  coef = FixedNumber("0.1");
  Imps tag_sum_imps = 0;
  Clicks tag_sum_clicks = 0;

  auto it_helper = helper_tag_hash.begin();
  const auto it_helper_end = helper_tag_hash.end();
  Url url_replacement("?");
  for (; it_helper != it_helper_end; ++it_helper)
  {
    const auto& key = it_helper->first;
    const auto& tag_id = key.first;
    const auto& creative_category_id = key.second;
    const auto& clicks = it_helper->second.first;
    const auto& imps = it_helper->second.second;

    if (tag_id > 0 && imps > 0)
    {
      const FixedNumber ctr = FixedNumber::mul(
        FixedNumber::div(
          FixedNumber(false, imps >= 1000 ? clicks : (
            clicks > 0 ? clicks - 1 : 0), 0),
          FixedNumber(false, imps, 0)),
        coef,
        Generics::DMR_FLOOR);

      tag_sum_imps += imps;
      tag_sum_clicks += clicks;

      if (!ctr.is_zero())
      {
        model.set_ctr(tag_id, url_replacement, creative_category_id, ctr);
      }
    }
  }

  {
    std::ostringstream stream;
    stream << "Global CTR: "
           << "imps = "
           << tag_sum_imps
           << ", clicks = "
           << tag_sum_clicks;
    logger_->info(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
  }

  if (tag_sum_imps)
  {
    const FixedNumber default_ctr = FixedNumber::mul(
      FixedNumber::div(
        FixedNumber(false, tag_sum_clicks, 0),
        FixedNumber(false, tag_sum_imps, 0)),
      FixedNumber("0.5"),
      Generics::DMR_FLOOR);
    model.set_ctr(0, url_replacement, 0, default_ctr);
  }
}

void ModelEvaluatorCtrImpl::stop() noexcept
{
  is_stopped_.store(true);
  try
  {
    std::ostringstream stream;
    stream << FNS
           << "ModelEvaluatorCtr was interrupted";
    logger_->info(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
  }
  catch (...)
  {
  }
}

} // namespace PredictorSvcs::BidCostPredictor