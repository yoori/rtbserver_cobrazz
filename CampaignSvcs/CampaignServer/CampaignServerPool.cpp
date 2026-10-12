#include "CampaignServerPool.hpp"

namespace AdServer::CampaignSvcs
{
  CampaignServerPool::CampaignServerPool(const CampaignServerPoolConfig& config)
    : CampaignServerPool(config.endpoints, config.timeout)
  {}

  CampaignServerPool::CampaignServerPool(
    const std::vector<std::string>& endpoints, const Generics::Time& bad_period)
    : bad_period_(bad_period.tv_sec * 1000000LL + bad_period.tv_usec)
  {
    if (endpoints.empty() || bad_period_.count() <= 0)
    {
      throw Exception("CampaignServer pool requires endpoints and a positive bad period");
    }

    for (const auto& endpoint : endpoints)
    {
      clients_.push_back(std::make_unique<CampaignServerClient>(endpoint));
    }

    retry_at_.resize(clients_.size());
    errors_.resize(clients_.size());
  }

  CampaignServerPool::ObjectHandlerType::ObjectHandlerType(
    CampaignServerPool& pool, std::size_t index) noexcept
    : pool_(&pool), index_(index)
  {}

  CampaignServerClient* CampaignServerPool::ObjectHandlerType::operator->() const noexcept
  {
    return pool_->clients_[index_].get();
  }

  void CampaignServerPool::ObjectHandlerType::release_bad(const String::SubString& reason) noexcept
  {
    pool_->mark_bad_(index_, reason);
  }

  void CampaignServerPool::mark_bad_(std::size_t index, const String::SubString& reason) noexcept
  {
    std::lock_guard<std::mutex> lock(mutex_);
    retry_at_[index] = std::chrono::steady_clock::now() + bad_period_;
    errors_[index] = reason.str();
  }

  CampaignServerPool::ObjectHandlerType CampaignServerPool::get_object(unsigned long key)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto now = std::chrono::steady_clock::now();
    for (std::size_t offset = 0; offset < clients_.size(); ++offset)
    {
      const auto index = (key % clients_.size() + offset) % clients_.size();
      if (retry_at_[index] <= now)
      {
        return ObjectHandlerType(*this, index);
      }
    }

    std::string message = "No available CampaignServer gRPC endpoints";
    for (const auto& error : errors_)
    {
      message += "; " + error;
    }

    throw Exception(message);
  }
}
