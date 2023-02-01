// STD
#include <optional>
#include <unordered_map>

// THIS
#include "ModelEvaluatorCtr.hpp"

namespace Aspect
{
const char* MODEL_EVALUATOR_CTR = "MODEL_EVALUATOR_CTR";
} // namespace Aspect

namespace PredictorSvcs
{
namespace BidCostPredictor
{

ModelEvaluatorCtrImpl::ModelEvaluatorCtrImpl(
  const Imps trust_imps,
  const Imps tag_imps,
  DataModelProvider* data_provider,
  ModelCtrFactory* model_factory,
  Logging::Logger* logger)
  : trust_imps_(trust_imps),
    tag_imps_(tag_imps),
    data_provider_(ReferenceCounting::add_ref(data_provider)),
    model_factory_(ReferenceCounting::add_ref(model_factory)),
    logger_(ReferenceCounting::add_ref(logger)),
    collector_(1, 1)
{
}

ModelCtr_var ModelEvaluatorCtrImpl::evaluate() noexcept
{
  if (is_stopped_.load(std::memory_order_relaxed))
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : ModelEvaluatorCtr already is stopped";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
    return {};
  }

  if (!data_provider_)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : data_provider is null";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
    return {};
  }

  if (!model_factory_)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : model_factory is null";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
    return {};
  }

  model_ = model_factory_->create();
  if (!model_)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : model is null";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
    return {};
  }

  if (!data_provider_->load(collector_))
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : data_provider load collector is failed";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
    return {};
  }

  if (collector_.empty())
  {
    logger_->info(
      std::string("ModelEvaluatorCtr: Collector is empty"),
      Aspect::MODEL_EVALUATOR_CTR);
    return model_;
  }

  try
  {
    logger_->info(
      std::string("ModelEvaluatorCtr started"),
      Aspect::MODEL_EVALUATOR_CTR);

    calculate();
    is_success_ = true;

    logger_->info(
      std::string("ModelEvaluatorCtr is successfully stopped"),
      Aspect::MODEL_EVALUATOR_CTR);
  }
  catch (const eh::Exception& exc)
  {
    is_success_ = false;
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : ModelEvaluatorCtr is failed"
           << " : Reason: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
    return {};
  }

  if (is_success_)
  {
    return std::move(model_);
  }
  else
  {
    model_.reset();
    return {};
  }
}

ModelEvaluatorCtrImpl::~ModelEvaluatorCtrImpl()
{
}

void ModelEvaluatorCtrImpl::calculate()
{
  using Data = std::pair<Clicks, Imps>;
  std::unordered_map<TagId, Data> helper_tag_hash;
  helper_tag_hash.reserve(100000);

  FixedNumber coef("0.5");
  const FixedNumber one(false, 1, 0);

  std::size_t records_reached_1000_imps = 0;
  std::size_t records_reached_10000_imps = 0;
  std::size_t records_reached_100000_imps = 0;

  auto it = collector_.begin();
  auto it_end = collector_.end();
  for (;it != it_end; ++it)
  {
    const auto total_clicks = it->second->total_clicks();
    const auto total_imps = it->second->total_imps();

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
    else if (total_imps > 0)
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

    const auto& tag_id = it->first.tag_id();
    const auto& url = it->first.url_var();
    if (total_imps > 0 && total_imps < tag_imps_)
    {
      auto it = helper_tag_hash.find(tag_id);
      if (it == helper_tag_hash.end())
      {
        helper_tag_hash.try_emplace(tag_id, total_clicks, total_imps);
      }
      else
      {
        it->second.first += total_clicks;
        it->second.second += total_imps;
      }
    }

    if (ctr && !ctr->is_zero())
    {
      model_->set_ctr(tag_id, url, *ctr);
    }
  }

  std::stringstream stream;
  stream << "\n"
         << "Records reached 1000 imps : "
         << records_reached_1000_imps
         << "\n"
         << "Records reached 10000 imps : "
         << records_reached_10000_imps
         << "\n"
         << "Records reached 100000 imps : "
         << records_reached_100000_imps;
  logger_->info(stream.str(), Aspect::MODEL_EVALUATOR_CTR);

  coef = FixedNumber("0.1");
  Imps tag_sum_imps = 0;
  Clicks tag_sum_clicks = 0;

  auto it_helper = helper_tag_hash.begin();
  auto it_helper_end = helper_tag_hash.end();
  Url_var url_replacement(new Url("?"));
  for (; it_helper != it_helper_end; ++it_helper)
  {
    const auto& tag_id = it_helper->first;
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
        model_->set_ctr(tag_id, url_replacement, ctr);
      }
    }
  }

  stream = std::stringstream();
  stream << "Global CTR: "
         << "imps = "
         << tag_sum_imps
         << ", clicks = "
         << tag_sum_clicks;
  logger_->info(stream.str(), Aspect::MODEL_EVALUATOR_CTR);

  if (tag_sum_imps)
  {
    const FixedNumber default_ctr = FixedNumber::mul(
      FixedNumber::div(
        FixedNumber(false, tag_sum_clicks, 0),
        FixedNumber(false, tag_sum_imps, 0)),
      FixedNumber("0.5"),
      Generics::DMR_FLOOR);
    model_->set_ctr(0, url_replacement, default_ctr);
  }
}

void ModelEvaluatorCtrImpl::stop() noexcept
{
  is_stopped_.store(true, std::memory_order_relaxed);
  logger_->info(
    std::string("ModelEvaluatorCtr was interrupted"),
    Aspect::MODEL_EVALUATOR_CTR);
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs
