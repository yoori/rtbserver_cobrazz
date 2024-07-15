#ifndef USERBINDOPERATIONDISTRIBUTOR_HPP_
#define USERBINDOPERATIONDISTRIBUTOR_HPP_

#include <list>

#include <Sync/SyncPolicy.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/TaskRunner.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

#include <UserInfoSvcs/UserBindServer/UserBindServer.hpp>
#include <UserInfoSvcs/UserBindController/UserBindController.hpp>

namespace AdServer
{
namespace UserInfoSvcs
{
  class UserBindOperationDistributor:
    public CORBACommons::ReferenceCounting::CorbaRefCountImpl<
      OBV_AdServer::UserInfoSvcs::UserBindMapperValueType>,
    public Generics::CompositeActiveObject
  {
  public:
    typedef CORBACommons::CorbaObjectRefList ControllerRef;
    typedef std::list<ControllerRef> ControllerRefList;

  public:
    UserBindOperationDistributor(
      Logging::Logger* logger,
      const ControllerRefList& controller_refs,
      const CORBACommons::CorbaClientAdapter* corba_client_adapter,
      const Generics::Time& pool_timeout = Generics::Time::ONE_SECOND)
      /*throw (eh::Exception)*/;

    // UserInfoMapper interface
    virtual
    AdServer::UserInfoSvcs::UserBindMapper::BindRequestInfo*
    get_bind_request(
      const char* id,
      const CORBACommons::TimestampInfo& timestamp)
      /*throw(AdServer::UserInfoSvcs::UserBindMapper::NotReady,
        AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound,
        AdServer::UserInfoSvcs::UserBindMapper::ImplementationException)*/;

    virtual
    void
    add_bind_request(
      const char* id,
      const AdServer::UserInfoSvcs::UserBindServer::BindRequestInfo& bind_request,
      const CORBACommons::TimestampInfo& timestamp)
      /*throw(AdServer::UserInfoSvcs::UserBindMapper::NotReady,
        AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound,
        AdServer::UserInfoSvcs::UserBindMapper::ImplementationException)*/;

    virtual AdServer::UserInfoSvcs::UserBindMapper::GetUserResponseInfo*
    get_user_id(
      const AdServer::UserInfoSvcs::UserBindMapper::GetUserRequestInfo&
        request_info)
      /*throw(::AdServer::UserInfoSvcs::UserBindMapper::NotReady,
        ::AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound,
        ::AdServer::UserInfoSvcs::UserBindMapper::ImplementationException)*/;

    virtual AdServer::UserInfoSvcs::UserBindMapper::AddUserResponseInfo*
    add_user_id(
      const AdServer::UserInfoSvcs::UserBindMapper::AddUserRequestInfo&
        request_info)
      /*throw(AdServer::UserInfoSvcs::UserBindMapper::NotReady,
        AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound,
        AdServer::UserInfoSvcs::UserBindMapper::ImplementationException)*/;

  protected:
    class ResolvePartitionTask:
      public Generics::Task,
      public ReferenceCounting::AtomicImpl
    {
    public:
      ResolvePartitionTask(
        UserBindOperationDistributor* distributor,
        unsigned int partition_num)
        noexcept;

      virtual void execute() noexcept;

    protected:
      virtual
      ~ResolvePartitionTask() noexcept = default;

      UserBindOperationDistributor* distributor_;
      const unsigned int partition_num_;
    };

    struct RefHolder: public ReferenceCounting::AtomicImpl
    {
    public:
      RefHolder(AdServer::UserInfoSvcs::UserBindServer* ref_val)
        : ref(AdServer::UserInfoSvcs::UserBindServer::_duplicate(ref_val)),
          marked_as_bad_(false)
      {}

      bool
      is_bad(const Generics::Time& timeout) const noexcept;

      void
      release_bad() noexcept;

      // return pointer without reference count increment
      AdServer::UserInfoSvcs::UserBindServer*
      ref_i() noexcept;

    protected:
      typedef Sync::Policy::PosixThread SyncPolicy;

    protected:
      virtual
      ~RefHolder() noexcept = default;

    protected:
      const AdServer::UserInfoSvcs::UserBindServer_var ref;

      mutable SyncPolicy::Mutex lock_;
      Generics::Time marked_as_bad_time_;
      mutable bool marked_as_bad_;
    };

    typedef ReferenceCounting::SmartPtr<RefHolder>
      RefHolder_var;

    class Partition: public ReferenceCounting::AtomicImpl
    {
    public:
      Partition()
        : max_chunk_number(0)
      {}

      unsigned long
      chunk_index(const char* id) const noexcept;

      std::map<unsigned long, RefHolder_var> chunks_ref_map;
      unsigned int max_chunk_number;

    protected:
      virtual
      ~Partition() noexcept = default;
    };

    typedef ReferenceCounting::ConstPtr<Partition>
      Partition_var;

    class PartitionHolder: public ReferenceCounting::AtomicImpl
    {
    public:
      typedef Sync::Policy::PosixThread SyncPolicy;

      PartitionHolder()
        : resolve_in_progress(false)
      {}

      mutable SyncPolicy::Mutex lock;
      Partition_var partition;
      Generics::Time last_try_to_resolve;
      bool resolve_in_progress;

    protected:
      virtual
      ~PartitionHolder() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<PartitionHolder>
      PartitionHolder_var;

    typedef std::vector<PartitionHolder_var>
      PartitionHolderArray;

  protected:
    virtual
    ~UserBindOperationDistributor() noexcept;

    unsigned long
    partition_index_(const char* user_id)
      const noexcept;

    Partition_var
    get_partition_(unsigned long partition_num)
      noexcept;

    void
    try_to_reresolve_partition_(unsigned int partition_num)
      noexcept;

    void
    resolve_partition_(unsigned int partition_num)
      noexcept;

  private:
    Generics::ActiveObjectCallback_var callback_;
    const unsigned long try_count_;
    const Generics::Time pool_timeout_;
    const ControllerRefList controller_refs_;
    CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
    Generics::FixedTaskRunner_var task_runner_;

    PartitionHolderArray partition_holders_;
  };

  typedef ReferenceCounting::SmartPtr<UserBindOperationDistributor>
    UserBindOperationDistributor_var;
}
}

#endif /*USERBINDOPERATIONDISTRIBUTOR_HPP_*/
