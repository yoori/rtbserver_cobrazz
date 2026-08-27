#pragma once

#include <mutex>
#include <shared_mutex>
#include <string>

#include <eh/Exception.hpp>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/HashTableAdapters.hpp>
#include <Generics/Hash.hpp>

#include <Commons/UserInfoManip.hpp>
#include <Commons/LockMap.hpp>
#include <Commons/Containers.hpp>

#include "BindRequestProcessor.hpp"

namespace AdServer::UserInfoSvcs
{
  class BindRequestChunk:
    public BindRequestProcessor,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    BindRequestChunk(
      Logging::Logger* logger,
      const char* file_root,
      const char* file_prefix,
      const Generics::Time& extend_time_period,
      unsigned long portions_number)
      /*throw(Exception)*/;

    // return previous state
    void
    add_bind_request(
      const String::SubString& id,
      const BindRequest& bind_request,
      const Generics::Time& now)
      noexcept;

    AdServer::Commons::StartableAwaitable<BindRequest>
    co_get_bind_request(const String::SubString& external_id, const Generics::Time& now)
      noexcept override;

    BindRequest
    get_bind_request(const String::SubString& external_id, const Generics::Time& now)
      noexcept override;

    void
    clear_expired(const Generics::Time& bound_expire_time)
      noexcept;

    virtual void
    dump() /*throw(Exception)*/;

  protected:
    using SyncMutex = std::shared_mutex;
    using SyncReadGuard = std::shared_lock<SyncMutex>;
    using SyncWriteGuard = std::unique_lock<SyncMutex>;

    using FlushMutex = std::mutex;
    using FlushWriteGuard = std::lock_guard<FlushMutex>;

    using ExtendMutex = std::mutex;
    using ExtendWriteGuard = std::lock_guard<ExtendMutex>;

    struct UserLockPolicy
    {
      using Mutex = std::mutex;
      using ReadGuard = std::lock_guard<Mutex>;
      using WriteGuard = std::lock_guard<Mutex>;
    };

    class BindRequestHolder
    {
    public:
      BindRequestHolder();

      BindRequestHolder(const std::vector<std::string>& bind_user_ids);

      void
      save(std::ostream& out) const noexcept;

      void
      load(std::istream& input) /*throw(Exception)*/;

      const std::vector<std::string>&
      bind_user_ids() const;

    private:
      std::vector<std::string> bind_user_ids_;
    };

    class HashHashAdapter
    {
    public:
      HashHashAdapter(size_t hash_val = 0) noexcept;

      template<typename HashAdapterType>
      HashHashAdapter(const HashAdapterType& init);

      bool
      operator==(const HashHashAdapter& right) const noexcept;

      size_t
      hash() const noexcept;

    protected:
      size_t hash_;
    };

    template<typename HashAdapterType, typename HolderType>
    struct HolderContainer: public ReferenceCounting::AtomicCopyImpl
    {
      using HolderMap = Generics::GnuHashTable<
        HashAdapterType, HolderType>;

      using KeyType = HashAdapterType;
      using MappedType = HolderType;

      struct TimePeriodHolder: public ReferenceCounting::AtomicImpl
      {
        TimePeriodHolder(const Generics::Time& min_time_val, const Generics::Time& max_time_val)
          noexcept;

        const Generics::Time min_time;
        const Generics::Time max_time;

        mutable SyncMutex lock;
        HolderMap holders;

      protected:
        virtual ~TimePeriodHolder() noexcept
        {}
      };

      using TimePeriodHolder_var = ReferenceCounting::SmartPtr<TimePeriodHolder>;

      using TimePeriodHolderArray = std::vector<TimePeriodHolder_var>;

      // sorted in max_time descending order
      TimePeriodHolderArray time_holders;

    protected:
      virtual ~HolderContainer() noexcept
      {}
    };

    template<typename HolderContainerType>
    class HolderContainerGuard
    {
    public:
      using HolderContainer_var = ReferenceCounting::SmartPtr<HolderContainerType>;

    public:
      mutable ExtendMutex extend_lock;

    public:
      HolderContainerGuard() noexcept;

      HolderContainer_var
      holder_container() const noexcept;

      void
      swap_holder_container(HolderContainer_var& new_holder_container)
        noexcept;

    protected:
      mutable SyncMutex holder_container_lock_;
      HolderContainer_var holder_container_;
    };

    //
    using BindRequestHolderContainer = HolderContainer<HashHashAdapter, BindRequestHolder>;

    using BindRequestHolderContainer_var = ReferenceCounting::SmartPtr<BindRequestHolderContainer>;

    using UserLockMap = AdServer::Commons::NoAllocLockMap<
      HashHashAdapter,
      UserLockPolicy>;

    struct Portion: public ReferenceCounting::AtomicImpl
    {
    public:
      UserLockMap holders_lock;

      HolderContainerGuard<BindRequestHolderContainer> holder_container_guard;

    protected:
      virtual ~Portion() noexcept
      {}
    };

    using Portion_var = ReferenceCounting::SmartPtr<Portion>;

    using PortionArray = std::vector<Portion_var>;

    using TempFilePathMap = std::map<std::string, std::string>;

    using FileNameSet = std::set<std::string>;

  protected:
    virtual
    ~BindRequestChunk() noexcept;

    HashHashAdapter
    get_external_id_hash_(
      unsigned long& portion,
      const String::SubString& external_id) const noexcept;

    unsigned long
    get_external_id_portion_(unsigned long full_hash) const
      noexcept;

    template<typename HolderContainerType>
    typename HolderContainerType::TimePeriodHolder_var
    get_holder_(
      HolderContainerGuard<HolderContainerType>& holder_container_guard,
      const HolderContainerType* holder_container,
      const Generics::Time& time,
      const Generics::Time& extend_time_period)
      noexcept;

    template<typename HolderContainerType>
    void
    get_holder_i_(
      typename HolderContainerType::TimePeriodHolder_var& res_holder,
      const HolderContainerType* holder_container,
      const Generics::Time& time)
      noexcept;

    template<typename HolderContainerType>
    typename HolderContainerType::TimePeriodHolderArray::iterator
    get_holder_i_(
      typename HolderContainerType::TimePeriodHolder_var& res_holder,
      HolderContainerType* holder_container,
      const Generics::Time& time)
      noexcept;

    template<typename HolderContainerType>
    void
    clear_expired_(
      HolderContainerGuard<HolderContainerType>& holder_container_guard,
      const Generics::Time& time)
      noexcept;

    template<typename HolderContainerType>
    void
    save_holders_i_(
      TempFilePathMap& result_files,
      HolderContainerGuard<HolderContainerType> Portion::* holder_container_guard_field,
      const FileNameSet& used_file_names)
      /*throw(Exception)*/;

    template<typename HolderContainerType>
    void
    load_holders_(
      HolderContainerGuard<HolderContainerType> Portion::* holder_container_guard_field,
      const Generics::Time& extend_time_period)
      /*throw(Exception)*/;

    void
    load_holders_() /*throw(Exception)*/;

    void
    save_holders_() /*throw(Exception)*/;

    void
    save_key_(std::ostream& out, const HashHashAdapter& key);

    void
    load_key_(HashHashAdapter& res, unsigned long& portion, std::istream& in) const
      noexcept;

    void
    generate_file_name_(
      std::string& persistent_file_name,
      std::string& tmp_file_name,
      const Generics::Time& max_time,
      const std::set<std::string>& used_file_names)
      const
      noexcept;

  private:
    const Logging::Logger_var logger_;
    const std::string file_root_;
    const std::string file_prefix_;
    const Generics::Time extend_time_period_;

    PortionArray portions_;

    // serialize saving to files
    mutable FlushMutex flush_lock_;
  };

  using BindRequestChunk_var = ReferenceCounting::SmartPtr<BindRequestChunk>;

} /* AdServer::UserInfoSvcs */

namespace AdServer::UserInfoSvcs
{
  // HashHashAdapter
  inline
  BindRequestChunk::HashHashAdapter::HashHashAdapter(size_t hash_val) noexcept
    : hash_(hash_val)
  {}

  template<typename HashAdapterType>
  BindRequestChunk::HashHashAdapter::HashHashAdapter(const HashAdapterType& init)
    : hash_(init.hash())
  {}

  inline
  bool
  BindRequestChunk::HashHashAdapter::operator==(const HashHashAdapter& right) const noexcept
  {
    return hash_ == right.hash_;
  }

  inline
  size_t
  BindRequestChunk::HashHashAdapter::hash() const noexcept
  {
    return hash_;
  }
}
