// THIS
#include "LogHelper.hpp"
#include "ModelBidCostImpl.hpp"
#include "BidCostCollector.hpp"

namespace Aspect
{
const char* MODEL_BID_COST = "MODEL_BID_COST";
}

namespace PredictorSvcs
{
namespace BidCostPredictor
{

ModelBidCostImpl::ModelBidCostImpl(Logging::Logger* logger)
  : logger_(ReferenceCounting::add_ref(logger))
{
  collector_.prepare_adding(10000000);
}

ModelBidCostImpl::FixedNumber ModelBidCostImpl::get_cost(
  const TagId& tag_id,
  const Url& url,
  const WinRate& win_rate,
  const Cost& cur_cost) const
{
  Cost min_cost(false, 100000, 0);
  Cost max_cost(false, 0, 0);

  const auto it =
    win_rates_.lower_bound(std::tuple(url, tag_id, win_rate));
  const auto it_end = std::end(win_rates_);
  while (it != it_end
    && std::get<1>(it->first) == tag_id
    && std::get<0>(it->first) == url)
  {
    const auto& win_rate_found = std::get<2>(it->first);
    const auto& data = it->second;
    const auto& cost_win = data.cost();
    const auto& max_cost_win = data.maxCost();

    min_cost = std::min(min_cost, cost_win);
    max_cost = std::min(max_cost, max_cost_win);
  }

  if (cur_cost >= max_cost)
  {
    return cur_cost;
  }

  return std::min(cur_cost, min_cost);
}

void ModelBidCostImpl::set_cost(
  const TagId& tag_id,
  const Url_var& url,
  const WinRate& win_rate,
  const Cost& cost,
  const Cost& max_cost)
{
  const LogProcessing::BidCostKey key(
    tag_id,
    url,
    win_rate);
  const LogProcessing::BidCostData data(
    cost,
    max_cost);

  collector_.add(key, data);
  win_rates_.try_emplace({
    *url,
    tag_id,
    win_rate},
    cost,
    max_cost);
}

void ModelBidCostImpl::clear() noexcept
{
  win_rates_.clear();
  collector_.clear();
}

void ModelBidCostImpl::save(const std::string& path) const
{
  logger_->info(
    "ModelBidCost save started, file_path=" + path,
    Aspect::MODEL_BID_COST);

  try
  {
    LogHelper<LogProcessing::BidCostTraits>::save(path, collector_);
    logger_->info(
      std::string("ModelBidCost save is success"),
      Aspect::MODEL_BID_COST);
  }
  catch (const eh::Exception& exc)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : ModelBidCost save is failed. Reason: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::MODEL_BID_COST);
    throw;
  }
}

void ModelBidCostImpl::load(const std::string& path)
{
  try
  {
    logger_->info(
      "ModelBidCost load started, path=" + path,
      Aspect::MODEL_BID_COST);
    clear();
    LogHelper<LogProcessing::BidCostTraits>::load(path, collector_);
    for (const auto& [k, d]: collector_)
    {
      win_rates_.try_emplace({
        k.url(),
        k.tag_id(),
        k.win_rate()},
        d.cost(),
        d.maxCost());
    }
    logger_->info(
      std::string("ModelBidCost load is success"),
      Aspect::MODEL_BID_COST);
  }
  catch (const eh::Exception& exc)
  {
    clear();
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : ModelBidCost load is failed. Reason: "
           << exc.what();
    logger_->error(stream.str(), Aspect::MODEL_BID_COST);
    throw;
  }
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs