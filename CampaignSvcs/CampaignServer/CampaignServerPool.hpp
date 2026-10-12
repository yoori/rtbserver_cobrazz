#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <vector>
#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>
#include <CampaignSvcs/CampaignServerClient/CampaignServerClient.hpp>

namespace AdServer::CampaignSvcs
{
  template<class Refs>
  std::vector<std::string> campaign_server_endpoints(const Refs& refs)
  {
    std::vector<std::string> result;
    for (const auto& ref : refs.Ref())
    {
      result.push_back(ref.endpoint());
    }

    return result;
  }

  struct CampaignServerPoolConfig
  {
    std::vector<std::string> endpoints;
    Generics::Time timeout = Generics::Time(10);
  };

  class CampaignServerPool
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    class ObjectHandlerType
    {
    public:
      CampaignServerClient* operator->() const noexcept;
      void release_bad(const String::SubString& reason = String::SubString()) noexcept;

    private:
      friend class CampaignServerPool;
      ObjectHandlerType(CampaignServerPool& pool, std::size_t index) noexcept;
      CampaignServerPool* pool_;
      std::size_t index_;
    };

    explicit CampaignServerPool(const CampaignServerPoolConfig& config);
    explicit CampaignServerPool(
      const std::vector<std::string>& endpoints,
      const Generics::Time& bad_period = Generics::Time(10));

    ObjectHandlerType get_object(unsigned long key = 0);

    template<class UserException>
    ObjectHandlerType get_object(
      Logging::Logger* logger, unsigned long severity, const char* aspect, const char* code,
      unsigned long first_key = 0, unsigned long next_key = 0);

  private:
    void mark_bad_(std::size_t index, const String::SubString& reason) noexcept;

    std::vector<std::unique_ptr<CampaignServerClient>> clients_;
    std::vector<std::chrono::steady_clock::time_point> retry_at_;
    std::vector<std::string> errors_;
    const std::chrono::microseconds bad_period_;
    std::mutex mutex_;
  };

  using CampaignServerPoolPtr = std::unique_ptr<CampaignServerPool>;

  template<class UserException>
  CampaignServerPool::ObjectHandlerType CampaignServerPool::get_object(
    Logging::Logger* logger, unsigned long severity, const char* aspect, const char* code,
    unsigned long first_key, unsigned long /*next_key*/)
  {
    try
    {
      return get_object(first_key);
    }
    catch (const Exception& e)
    {
      if (logger)
      {
        logger->log(String::SubString(e.what()), severity, aspect, code);
      }

      throw UserException(e.what());
    }
  }
}
