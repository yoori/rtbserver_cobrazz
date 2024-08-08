// THIS
#include "LogHelper.hpp"
#include "ModelBidCostImpl.hpp"
#include "BidCostCollector.hpp"

namespace Aspect
{

inline constexpr char MODEL_BID_COST[] = "MODEL_BID_COST";

} // namespace Aspect

namespace PredictorSvcs::BidCostPredictor
{

ModelBidCostImpl::ModelBidCostImpl(Logger* logger)
  : logger_(ReferenceCounting::add_ref(logger))
{
}

ModelBidCostImpl::Cost ModelBidCostImpl::get_cost(
  const TagId& tag_id,
  const Url& url,
  const WinRate& win_rate,
  const Cost& cur_cost) const
{
  Cost min_cost = Cost::MAXIMUM;
  Cost max_cost = Cost::ZERO;

  auto it = win_rates_.lower_bound(
    std::tuple{tag_id, url, win_rate});
  const auto it_end = std::end(win_rates_);
  while (it != it_end
    && std::get<0>(it->first) == tag_id
    && std::get<1>(it->first) == url)
  {
    const auto& data = it->second;
    const auto& cost_win = data.first;
    const auto& max_cost_win = data.second;

    min_cost = std::min(min_cost, cost_win);
    max_cost = std::max(max_cost, max_cost_win);
    ++it;
  }

  if (cur_cost >= max_cost)
  {
    return cur_cost;
  }

  return std::min(cur_cost, min_cost);
}

void ModelBidCostImpl::set_cost(
  const TagId& tag_id,
  const Url& url,
  const WinRate& win_rate,
  const Cost& cost,
  const Cost& max_cost)
{
  const auto& data = collector_.add(
    tag_id,
    url,
    win_rate,
    cost,
    max_cost);
  win_rates_.try_emplace({
    data.tag_id,
    data.url,
    data.win_rate},
    data.cost,
    data.max_cost);
}

void ModelBidCostImpl::clear() noexcept
{
  win_rates_.clear();
  collector_.clear();
}

void ModelBidCostImpl::save(const std::string& path) const
{
  {
    std::ostringstream stream;
    stream << FNS
           << "ModelBidCost save started, file_path="
           << path;
    logger_->info(stream.str(), Aspect::MODEL_BID_COST);
  }

  try
  {
    collector_.save(path);

    std::ostringstream stream;
    stream << FNS
           << "ModelBidCost save is success";
    logger_->info(stream.str(), Aspect::MODEL_BID_COST);
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "ModelBidCost save is failed. Reason: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::MODEL_BID_COST);
    throw;
  }
  catch (...)
  {
    Stream::Error stream;
    stream << FNS
           << "ModelBidCost save is failed. Reason: Unknown error";
    logger_->critical(stream.str(), Aspect::MODEL_BID_COST);
    throw;
  }
}

void ModelBidCostImpl::load(const std::string& path)
{
  try
  {
    {
      std::ostringstream stream;
      stream << FNS
             << "ModelBidCost load started, path="
             << path;
      logger_->info(stream.str(), Aspect::MODEL_BID_COST);
    }

    clear();
    collector_.load(path);

    for (const auto& data: collector_)
    {
      win_rates_.try_emplace({
        data.tag_id,
        data.url,
        data.win_rate},
        data.cost,
        data.max_cost);
    }

    {
      std::ostringstream stream;
      stream << FNS
             << "ModelBidCost load is success";
      logger_->info(stream.str(), Aspect::MODEL_BID_COST);
    }
  }
  catch (const eh::Exception& exc)
  {
    clear();

    Stream::Error stream;
    stream << FNS
           << "ModelBidCost load is failed. Reason: "
           << exc.what();
    logger_->error(stream.str(), Aspect::MODEL_BID_COST);
    throw;
  }
  catch (...)
  {
    clear();

    Stream::Error stream;
    stream << FNS
           << "ModelBidCost load is failed. Reason: Unknown error";
    logger_->error(stream.str(), Aspect::MODEL_BID_COST);
    throw;
  }
}

} // namespace PredictorSvcs::BidCostPredictor