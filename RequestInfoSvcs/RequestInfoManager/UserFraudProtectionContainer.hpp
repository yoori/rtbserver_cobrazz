/**
 * @file UserFraudProtectionContainer.hpp
 */
#ifndef _REQUEST_INFO_SVCS_USER_FRAUD_PROTECTION_CONTAINER_IMPL_HPP_
#define _REQUEST_INFO_SVCS_USER_FRAUD_PROTECTION_CONTAINER_IMPL_HPP_

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/PtrHolder.hpp>

#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>

#include <Generics/MemBuf.hpp>

#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>
#include <ProfilingCommons/PlainStorageAdapters.hpp>

#include "CompositeRequestContainerProcessor.hpp"

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    const Generics::Time DEFAULT_FRAUD_PROFILE_EXPIRE_TIME =
      Generics::Time::ONE_DAY * 180; // 180 days

    /**
     * UserFraudProtectionContainer
     */
    class UserFraudProtectionContainer:
      public virtual ReferenceCounting::AtomicImpl,
      public virtual RequestActionProcessor
    {
    public:
      DECLARE_EXCEPTION(Exception, RequestActionProcessor::Exception);

      struct Callback: public virtual ReferenceCounting::Interface
      {
        virtual void
        detected_fraud_user(
          const AdServer::Commons::UserId& user_id,
          const Generics::Time& deactivate_time)
          noexcept = 0;
      };

      typedef ReferenceCounting::SmartPtr<Callback> Callback_var;
      
      struct Config: public ReferenceCounting::AtomicImpl
      {
        struct FraudRule
        {
          unsigned long limit;
          Generics::Time period;
        };

        typedef std::list<FraudRule> FraudRuleList;

        class FraudRuleSet
        {
        public:
          void add_rule(const FraudRule& rule) noexcept;

          const FraudRuleList& rules() const noexcept;

          const Generics::Time& max_period() const noexcept;

        private:
          Generics::Time max_period_;
          // FraudRuleList sorted in period descending order
          FraudRuleList fraud_rules_;
        };

        Generics::Time deactivate_period;
        FraudRuleSet imp_rules;
        FraudRuleSet click_rules;

      protected:
        virtual ~Config() noexcept
        {}
      };

      using RocksdbManagerPool = UServerUtils::Grpc::RocksDB::DataBaseManagerPool;
      using RocksdbManagerPoolPtr = std::shared_ptr<RocksdbManagerPool>;
      using RocksDBParams = AdServer::ProfilingCommons::RocksDB::RocksDBParams;
      using Config_var = ReferenceCounting::SmartPtr<Config>;
      using RequestIdList = std::list<AdServer::Commons::RequestId>;

    public:
      UserFraudProtectionContainer(
        Logging::Logger* logger,
        RequestContainerProcessor* request_container_processor,
        Callback* callback,
        const char* file_base_path,
        const char* file_prefix,
        const bool is_rocksdb_enable,
        const RocksdbManagerPoolPtr& rocksdb_manager_pool,
        const RocksDBParams& rocksdb_params,
        ProfilingCommons::ProfileMapFactory::Cache* cache,
        const Generics::Time& expire_time =
          Generics::Time(DEFAULT_FRAUD_PROFILE_EXPIRE_TIME),
        const Generics::Time& extend_time_period = Generics::Time::ZERO)
        /*throw(Exception)*/;

      Generics::ConstSmartMemBuf_var
      get_profile(const AdServer::Commons::UserId& user_id)
        /*throw(Exception)*/;

      void clear_expired() /*throw(Exception)*/;

      bool config_initialized() const noexcept;

      void config(Config* new_config) noexcept;

      void request_container_processor(
        RequestContainerProcessor* request_container_processor)
        noexcept;

      /** RequestActionProcessor interface */
      virtual void
      process_request(
        const RequestInfo&,
        const ProcessingState&)
        /*throw(RequestActionProcessor::Exception)*/
      {};

      virtual void
      process_impression(
        const RequestInfo&,
        const ImpressionInfo&,
        const ProcessingState&)
        /*throw(RequestActionProcessor::Exception)*/;

      virtual void
      process_click(
        const RequestInfo&,
        const ProcessingState&)
        /*throw(RequestActionProcessor::Exception)*/;

      virtual void
      process_action(const RequestInfo&)
        /*throw(RequestActionProcessor::Exception)*/
      {};

    private:
      typedef ProfilingCommons::TransactionProfileMap<
        AdServer::Commons::UserId>
        ProfileMap;

      typedef ReferenceCounting::SmartPtr<ProfileMap>
        ProfileMap_var;

      typedef Sync::Policy::PosixThread SyncPolicy;

    protected:
      virtual ~UserFraudProtectionContainer() noexcept;

    private:
      void
      process_click_trans_(
        RequestIdList& fraud_impressions,
        Generics::Time& user_deactivate_time,
        const RequestInfo& request_info)
        /*throw(RequestActionProcessor::Exception)*/;

      void
      process_impression_trans_(
        RequestIdList& fraud_impressions,
        Generics::Time& user_deactivate_time,
        const RequestInfo& request_info)
        /*throw(RequestActionProcessor::Exception)*/;

      void
      process_click_(
        const RequestInfo& request_id)
        /*throw(AdvActionProcessor::Exception)*/;

      void
      process_impression_(
        const RequestInfo& request_id)
        /*throw(AdvActionProcessor::Exception)*/;

      Config_var
      get_config_() const noexcept;

    private:
      Logging::Logger_var logger_;
      Generics::Time expire_time_;
      ProfileMap_var user_map_;
      RequestContainerProcessor_var request_container_processor_;
      Callback_var callback_;

      ReferenceCounting::PtrHolder<Config_var> config_;
    };

    typedef ReferenceCounting::SmartPtr<UserFraudProtectionContainer>
      UserFraudProtectionContainer_var;

  } // RequestInfoSvcs
} // AdServer

namespace AdServer
{
namespace RequestInfoSvcs
{
  inline
  bool
  UserFraudProtectionContainer::config_initialized() const noexcept
  {
    return config_.get() != Config_var();
  }

  inline
  void
  UserFraudProtectionContainer::config(
    UserFraudProtectionContainer::Config* new_config)
    noexcept
  {
    config_ = ReferenceCounting::add_ref(new_config);
  }

  inline
  void
  UserFraudProtectionContainer::request_container_processor(
    RequestContainerProcessor* request_container_processor)
    noexcept
  {
    request_container_processor_ = ReferenceCounting::add_ref(
      request_container_processor);
  }

  inline
  UserFraudProtectionContainer::Config_var
  UserFraudProtectionContainer::get_config_() const
    noexcept
  {
    return config_.get();
  }

  inline
  void
  UserFraudProtectionContainer::Config::FraudRuleSet::add_rule(
    const FraudRule& rule)
    noexcept
  {
    FraudRuleList::iterator rit = fraud_rules_.begin();

    for(; rit != fraud_rules_.end(); ++rit)
    {
      if(rule.period > rit->period)
      {
        break;
      }
    }

    fraud_rules_.insert(rit, rule);
    max_period_ = std::max(max_period_, rule.period);
  }

  inline
  const UserFraudProtectionContainer::Config::FraudRuleList&
  UserFraudProtectionContainer::Config::FraudRuleSet::rules() const noexcept
  {
    return fraud_rules_;
  }

  inline
  const Generics::Time&
  UserFraudProtectionContainer::Config::FraudRuleSet::max_period() const noexcept
  {
    return max_period_;
  }
}
}

#endif // _REQUEST_INFO_SVCS_USER_FRAUD_PROTECTION_CONTAINER_IMPL_HPP_
