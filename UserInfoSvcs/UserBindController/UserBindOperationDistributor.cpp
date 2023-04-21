#include <Commons/UserInfoManip.hpp>
#include <Commons/CorbaAlgs.hpp>
#include "UserBindOperationDistributor.hpp"

namespace AdServer
{
namespace UserInfoSvcs
{
  // UserBindOperationDistributor::ResolvePartitionTask
  UserBindOperationDistributor::ResolvePartitionTask::ResolvePartitionTask(
    UserBindOperationDistributor* distributor,
    unsigned int partition_num)
    noexcept
    : distributor_(distributor),
      partition_num_(partition_num)
  {}

  void
  UserBindOperationDistributor::ResolvePartitionTask::execute()
    noexcept
  {
    distributor_->resolve_partition_(partition_num_);
  }

  // UserBindOperationDistributor::RefHolder
  bool
  UserBindOperationDistributor::
  RefHolder::is_bad(const Generics::Time& timeout) const
    noexcept
  {
    if(!marked_as_bad_)
    {
      return false;
    }

    Generics::Time now = Generics::Time::get_time_of_day();

    SyncPolicy::WriteGuard lock(lock_);
    if(marked_as_bad_ && now >= marked_as_bad_time_ + timeout)
    {
      marked_as_bad_ = false;
    }

    return marked_as_bad_;
  }

  void
  UserBindOperationDistributor::
  RefHolder::release_bad()
    noexcept
  {
    Generics::Time now = Generics::Time::get_time_of_day();

    SyncPolicy::WriteGuard guard(lock_);
    marked_as_bad_ = true;
    marked_as_bad_time_ = now;
  }

  AdServer::UserInfoSvcs::UserBindServer*
  UserBindOperationDistributor::
  RefHolder::ref_i() noexcept
  {
    return ref.in();
  }

  // UserBindOperationDistributor::Partition
  unsigned long
  UserBindOperationDistributor::
  Partition::chunk_index(const char* id)
    const noexcept
  {
    return AdServer::Commons::external_id_distribution_hash(
      String::SubString(id)) % max_chunk_number;
  }

  // UserBindOperationDistributor
  UserBindOperationDistributor::UserBindOperationDistributor(
    Logging::Logger* logger,
    const ControllerRefList& controller_refs,
    const CORBACommons::CorbaClientAdapter* corba_client_adapter,
    const Generics::Time& pool_timeout)
    noexcept
    : callback_(Generics::ActiveObjectCallback_var(
        new Logging::ActiveObjectCallbackImpl(
          logger,
          "UserBindOperationDistributor",
          "UserInfo"))),
      try_count_(controller_refs.size()),
      pool_timeout_(pool_timeout),
      controller_refs_(controller_refs),
      corba_client_adapter_(ReferenceCounting::add_ref(corba_client_adapter)),
      task_runner_(new Generics::TaskRunner(callback_, try_count_))
  {
    try
    {
      add_child_object(task_runner_);
    }
    catch(const Generics::CompositeActiveObject::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "UserBindOperationDistributor: CompositeActiveObject::Exception caught: " <<
        ex.what();
      throw Exception(ostr);
    }

    int partition_i = 0;
    for (ControllerRefList::const_iterator group_it =
           controller_refs_.begin();
         group_it != controller_refs_.end(); ++group_it)
    {
      partition_holders_.push_back(new PartitionHolder());
      resolve_partition_(partition_i++);
    }
  }

  UserBindOperationDistributor::~UserBindOperationDistributor()
    noexcept
  {}

  void
  UserBindOperationDistributor::try_to_reresolve_partition_(
    unsigned int partition_num)
    noexcept
  {
    Generics::Time now = Generics::Time::get_time_of_day();

    {
      PartitionHolder::SyncPolicy::WriteGuard lock(
        partition_holders_[partition_num]->lock);

      if(!partition_holders_[partition_num]->resolve_in_progress &&
         now >= partition_holders_[partition_num]->last_try_to_resolve +
           pool_timeout_)
      {
        partition_holders_[partition_num]->resolve_in_progress = true;
      }
      else
      {
        return;
      }
    }

    task_runner_->enqueue_task(
      Generics::Task_var(new ResolvePartitionTask(this, partition_num)));
  }

  void
  UserBindOperationDistributor::resolve_partition_(unsigned int partition_num)
    noexcept
  {
    Partition_var new_partition;

    ControllerRefList::const_iterator partition_controllers_it =
      controller_refs_.begin();
    std::advance(partition_controllers_it, partition_num);

    const CORBACommons::CorbaObjectRefList& controllers =
      *partition_controllers_it;

    for(CORBACommons::CorbaObjectRefList::const_iterator ref_it =
          controllers.begin();
        ref_it != controllers.end(); ++ref_it)
    {
      try
      {
        CORBA::Object_var obj = corba_client_adapter_->resolve_object(*ref_it);
        if(!CORBA::is_nil(obj.in()))
        {
          AdServer::UserInfoSvcs::UserBindController_var controller =
            AdServer::UserInfoSvcs::UserBindController::_narrow(obj.in());

          if(!CORBA::is_nil(controller.in()))
          {
            AdServer::UserInfoSvcs::UserBindDescriptionSeq_var user_bind_servers =
              controller->get_session_description();

            assert(user_bind_servers->length());

            ReferenceCounting::SmartPtr<Partition> fill_partition = new Partition();
            fill_partition->max_chunk_number = 0;

            assert(user_bind_servers->length());

            for(unsigned int i = 0; i < user_bind_servers->length(); ++i)
            {
              const AdServer::UserInfoSvcs::UserBindDescription&
                user_bind_descr = (*user_bind_servers)[i];

              assert(user_bind_descr.chunk_ids.length());

              assert(!CORBA::is_nil(user_bind_descr.user_bind_server));

              for(CORBA::ULong chunk_i = 0;
                  chunk_i < user_bind_descr.chunk_ids.length(); ++chunk_i)
              {
                const unsigned long chunk_id = user_bind_descr.chunk_ids[chunk_i];

                fill_partition->chunks_ref_map.insert(
                  std::make_pair(
                    chunk_id,
                    RefHolder_var(new RefHolder(user_bind_descr.user_bind_server))));

                if(chunk_id >= fill_partition->max_chunk_number)
                {
                  fill_partition->max_chunk_number = chunk_id + 1;
                }
              }
            }

            new_partition = fill_partition;
            break;
          }
        }
      }
      catch(const CORBA::SystemException& ex)
      {}
      catch(const AdServer::UserInfoSvcs::UserBindController::
        ImplementationException& ex)
      {}
      catch(const AdServer::UserInfoSvcs::UserBindController::
        NotReady&)
      {}
    }

    Generics::Time now = Generics::Time::get_time_of_day();

    PartitionHolder::SyncPolicy::WriteGuard lock(
      partition_holders_[partition_num]->lock);

    if (new_partition.in())
    {
      assert(!new_partition->chunks_ref_map.empty());
      partition_holders_[partition_num]->partition.swap(new_partition);
    }
    else
    {
      partition_holders_[partition_num]->last_try_to_resolve = now;
      partition_holders_[partition_num]->resolve_in_progress = false;
    }
  }

  UserBindOperationDistributor::Partition_var
  UserBindOperationDistributor::get_partition_(
    unsigned long partition_num)
    noexcept
  {
    Partition_var result;

    {
      PartitionHolder::SyncPolicy::WriteGuard lock(
        partition_holders_[partition_num]->lock);
      result = partition_holders_[partition_num]->partition;
    }

    if(!result)
    {
      try_to_reresolve_partition_(partition_num);
    }

    return result;
  }

#define CALL_MATCHER_(USER_ID, PRECALL_STMT, CALL_NAME, POST_CALL_STMT, THROW_COND, ...) \
  { \
    static const char* FUN = "UserBindOperationDistributor::" #CALL_NAME "()"; \
    unsigned i = 0; \
    unsigned long chunk_index = 0; \
    for(; i < try_count_; ++i) \
    { \
      unsigned long partition_num = ( \
        partition_index_(USER_ID) + i) % partition_holders_.size();     \
      try \
      { \
        Partition_var partition = get_partition_(partition_num); \
        if(!partition) \
        { \
          try_to_reresolve_partition_(partition_num); \
          continue; \
        } \
        \
        chunk_index = partition->chunk_index(USER_ID); \
        \
        auto iter = partition->chunks_ref_map.find(chunk_index); \
        if (iter == partition->chunks_ref_map.end()) \
        { \
          try_to_reresolve_partition_(partition_num); \
          continue; \
        } \
        assert(iter->second.in()); \
        if (iter->second->is_bad(pool_timeout_)) \
        { \
          continue; \
        } \
        \
        try \
        { \
          PRECALL_STMT iter->second->ref_i()->CALL_NAME( __VA_ARGS__ ); \
          POST_CALL_STMT; \
        } \
        catch(const AdServer::UserInfoSvcs::UserBindMapper::NotReady&) \
        { \
          iter->second->release_bad(); \
        } \
        catch(const AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound& e) \
        { \
          iter->second->release_bad(); \
          try_to_reresolve_partition_(partition_num); \
        } \
        catch(const CORBA::SystemException& ex) \
        { \
          iter->second->release_bad(); \
          try_to_reresolve_partition_(partition_num); \
        } \
      } \
      catch(const eh::Exception& ex)     \
      { \
        Stream::Error ostr; \
        ostr << FUN << ": caught eh::Exception: " << ex.what();    \
        CORBACommons::throw_desc< \
          AdServer::UserInfoSvcs::UserBindMapper::ImplementationException>( \
            ostr.str()); \
      } \
    } \
    THROW_COND \
    { \
      Stream::Error ostr; \
      ostr << "max tries reached for chunk #" << chunk_index; \
      CORBACommons::throw_desc< \
        AdServer::UserInfoSvcs::UserBindMapper::ImplementationException>(ostr.str()); \
    } \
  }

#define CALL_MATCHER(USER_ID, CALL_NAME, ...) \
  CALL_MATCHER_(USER_ID,, CALL_NAME, return,, ##__VA_ARGS__);

#define CALL_MATCHER_WITH_RETURN(USER_ID, CALL_NAME, ...) \
  CALL_MATCHER_(USER_ID, return, CALL_NAME,,, ##__VA_ARGS__);

  unsigned long
  UserBindOperationDistributor::partition_index_(
    const char* id)
    const
    noexcept
  {
    return (
      AdServer::Commons::external_id_distribution_hash(
        String::SubString(id)) >> 8) % try_count_;
  }

  AdServer::UserInfoSvcs::UserBindMapper::BindRequestInfo*
  UserBindOperationDistributor::get_bind_request(
    const char* id,
    const CORBACommons::TimestampInfo& timestamp)
    /*throw(AdServer::UserInfoSvcs::UserBindMapper::NotReady,
      AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound,
      AdServer::UserInfoSvcs::UserBindMapper::ImplementationException)*/
  {
    CALL_MATCHER_WITH_RETURN(
      id,
      get_bind_request,
      id,
      timestamp);
  }

  void
  UserBindOperationDistributor::add_bind_request(
    const char* id,
    const AdServer::UserInfoSvcs::UserBindServer::BindRequestInfo& bind_request,
    const CORBACommons::TimestampInfo& timestamp)
    /*throw(AdServer::UserInfoSvcs::UserBindMapper::NotReady,
      AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound,
      AdServer::UserInfoSvcs::UserBindMapper::ImplementationException)*/
  {
    CALL_MATCHER(
      id,
      add_bind_request,
      id,
      bind_request,
      timestamp);
  }

  AdServer::UserInfoSvcs::UserBindMapper::GetUserResponseInfo*
  UserBindOperationDistributor::get_user_id(
    const AdServer::UserInfoSvcs::UserBindMapper::GetUserRequestInfo&
      request_info)
    /*throw(::AdServer::UserInfoSvcs::UserBindMapper::NotReady,
      ::AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound,
      ::AdServer::UserInfoSvcs::UserBindMapper::ImplementationException)*/
  {
    CALL_MATCHER_WITH_RETURN(
      request_info.id,
      get_user_id,
      request_info);
  }

  AdServer::UserInfoSvcs::UserBindMapper::AddUserResponseInfo*
  UserBindOperationDistributor::add_user_id(
    const AdServer::UserInfoSvcs::UserBindMapper::AddUserRequestInfo&
      request_info)
    /*throw(::AdServer::UserInfoSvcs::UserBindMapper::NotReady,
      ::AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound,
      ::AdServer::UserInfoSvcs::UserBindMapper::ImplementationException)*/
  {
    CALL_MATCHER_WITH_RETURN(
      request_info.id,
      add_user_id,
      request_info);
  }
}
}
