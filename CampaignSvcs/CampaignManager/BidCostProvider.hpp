#ifndef CAMPAIGNMANAGER_BIDCOSTPROVIDER_HPP
#define CAMPAIGNMANAGER_BIDCOSTPROVIDER_HPP

#include <optional>

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/Hash.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/Singleton.hpp>
#include <Generics/TaskRunner.hpp>
#include <Sync/SyncPolicy.hpp>

#include <Commons/Containers.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include "CampaignSelectParams.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  class BidCostProvider: public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(InvalidConfig, Exception);
    DECLARE_EXCEPTION(Overflow, Exception);

    struct RequestParams
    {
      unsigned long tag_id;
      std::string domain;
    };

  protected:
    class CostMapping: public ReferenceCounting::AtomicImpl
    {
    public:
      struct Key
      {
        const unsigned long tag_id;
        const std::string domain;

      public:
        Key(unsigned long tag_id_val, const String::SubString& domain_val);

        bool
        operator==(const Key& right) const;

        size_t hash() const;

      protected:
        void calc_hash_();

      protected:
        size_t hash_;
      };

      struct Cell
      {
        Cell(const RevenueDecimal& win_rate_reduction_val,
          const RevenueDecimal& sys_impression_val,
          const RevenueDecimal& max_sys_impression_val);

        bool
        operator<(const Cell& right) const;

        bool
        operator==(const Cell& right) const;

        RevenueDecimal win_rate_reduction;
        RevenueDecimal sys_impression;
        RevenueDecimal max_sys_impression;
      };

      struct Value: public ReferenceCounting::AtomicImpl
      {
        std::vector<Cell> cells;
      };

      typedef ReferenceCounting::SmartPtr<Value> Value_var;
      typedef Generics::GnuHashTable<Key, Value_var> CostMap;

    public:
      CostMapping() noexcept;

      void
      load(const String::SubString& file);

      CostMap cost_map;

    protected:
      virtual ~CostMapping() noexcept
      {}
    };

    typedef ReferenceCounting::SmartPtr<CostMapping> CostMapping_var;

  public:
    BidCostProvider(
      const String::SubString& directory,
      const Generics::Time& config_timestamp,
      Generics::TaskRunner* task_runner)
      /*throw(Exception)*/;

    std::optional<RevenueDecimal>
    get_bid_cost(
      const RequestParams& request_params,
      const RevenueDecimal& allowable_lose_win_percentage,
      const RevenueDecimal& orig_bid_cost)
      const;

    Generics::Time
    config_timestamp() const noexcept
    {
      return config_timestamp_;
    }

    static
    Generics::Time
    check_config_appearance(
      std::string& config_root,
      const String::SubString& check_root)
      /*throw(Exception)*/;

    void
    remove_config_files_at_destroy(bool val)
      const noexcept;

  protected:
    typedef Sync::Policy::PosixThreadRW RemoveFilesSyncPolicy;
    typedef std::list<std::string> FileList;

  protected:
    virtual
    ~BidCostProvider() noexcept;

    void
    load_(
      const String::SubString& directory,
      const String::SubString& file)
      /*throw(InvalidConfig, Exception)*/;

    void
    load_cost_map_(
      BidCostProvider::CostMapping::CostMap& res,
      const String::SubString& cost_file_path);

    void
    clear_config_files_() /*throw(Exception)*/;

  protected:
    Generics::TaskRunner_var task_runner_;
    const Generics::Time config_timestamp_;
    CostMapping_var cost_mapping_;

    FileList config_files_;
    FileList config_directories_;

    mutable RemoveFilesSyncPolicy::Mutex remove_config_files_lock_;
    mutable bool remove_config_files_at_destroy_;
  };

  typedef ReferenceCounting::SmartPtr<BidCostProvider>
    BidCostProvider_var;
  typedef ReferenceCounting::SmartPtr<const BidCostProvider>
    ConstBidCostProvider_var;
}
}

namespace AdServer
{
namespace CampaignSvcs
{
  // BidCostProvider::CostMapping::Key
  inline
  BidCostProvider::CostMapping::Key::Key(
    unsigned long tag_id_val,
    const String::SubString& domain_val)
    : tag_id(tag_id_val),
      domain(domain_val.str())
  {
    calc_hash_();
  }

  inline bool
  BidCostProvider::CostMapping::Key::operator==(const Key& right) const
  {
    return tag_id == right.tag_id && domain == right.domain;
  }

  inline
  size_t
  BidCostProvider::CostMapping::Key::hash() const
  {
    return hash_;
  }

  inline
  void
  BidCostProvider::CostMapping::Key::calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, tag_id);
    hash_add(hasher, domain);
  }

  // BidCostProvider::CostMapping::Cell
  inline
  BidCostProvider::CostMapping::Cell::Cell(
    const RevenueDecimal& win_rate_reduction_val,
    const RevenueDecimal& sys_impression_val,
    const RevenueDecimal& max_sys_impression_val)
    : win_rate_reduction(win_rate_reduction_val),
      sys_impression(sys_impression_val),
      max_sys_impression(max_sys_impression_val)
  {}

  inline
  bool
  BidCostProvider::CostMapping::Cell::operator<(const Cell& right) const
  {
    return win_rate_reduction < right.win_rate_reduction;
  }

  inline
  bool
  BidCostProvider::CostMapping::Cell::operator==(const Cell& right) const
  {
    return win_rate_reduction == right.win_rate_reduction;
  }

  // BidCostProvider::CostMapping
  inline
  BidCostProvider::CostMapping::CostMapping() noexcept
  {}
}
}

#endif /*CAMPAIGNMANAGER_BIDCOSTPROVIDER_HPP*/
