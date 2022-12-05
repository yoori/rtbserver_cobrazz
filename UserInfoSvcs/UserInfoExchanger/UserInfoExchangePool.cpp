#include <vector>
#include <iostream>
#include <sstream>

#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>
#include <ProfilingCommons/PlainStorageAdapters.hpp>
#include "UserInfoExchangePool.hpp"

namespace AdServer{
namespace UserInfoSvcs{

  /** UserInfoExchangePool::ProviderStorage::UserInfoRef */
  void
  UserInfoExchangePool::
  ProviderStorage::UserInfoRef::read(
    UserProfile& user_profile) /*throw(eh::Exception)*/
  {
    storage_->read(user_id_.c_str(), user_profile);
  }

  /** UserInfoExchangePool::ProviderStorage */
  UserInfoExchangePool::
  ProviderStorage::ProviderStorage(
    Generics::ActiveObjectCallback* callback,
    const char* base_path,
    const char* base_prefix,
    const char* history_path,
    const char* history_prefix,
    const Generics::Time extend_time)
    /*throw(eh::Exception)*/
  {
    Generics::ActiveObject_var user_profiles_active_object;
    user_profiles_ = AdServer::ProfilingCommons::ProfileMapFactory::
      open_transaction_level_map<
        Generics::StringHashAdapter,
        AdServer::ProfilingCommons::StringSerializer>(
          user_profiles_active_object,
          callback,
          base_path,
          base_prefix,
          AdServer::ProfilingCommons::LevelMapTraits(
            AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
            10*1024*1024,
            100*1024*1024,
            250*1024*1024,
            20,
            extend_time));

    add_child_object(user_profiles_active_object);

    Generics::ActiveObject_var user_history_profiles_active_object;
    user_history_profiles_ = AdServer::ProfilingCommons::ProfileMapFactory::
      open_transaction_level_map<
        Generics::StringHashAdapter,
        AdServer::ProfilingCommons::StringSerializer>(
          user_history_profiles_active_object,
          callback,
          history_path,
          history_prefix,
          AdServer::ProfilingCommons::LevelMapTraits(
            AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
            10*1024*1024,
            100*1024*1024,
            250*1024*1024,
            20,
            extend_time));

    add_child_object(user_history_profiles_active_object);
  }

  UserInfoExchangePool::
  ProviderStorage::~ProviderStorage() noexcept
  {}

  UserInfoExchangePool::ProviderStorage::UserInfoRef*
  UserInfoExchangePool::
  ProviderStorage::insert_i(const UserProfile& user_profile)
    /*throw(eh::Exception)*/
  {
    const char* user_id = user_profile.user_id.c_str();

    UserInfoRef_var user_info_ref;

    UserRefMap::iterator user_ref_it = user_refs_.find(user_id);

    if(user_ref_it == user_refs_.end())
    {
      user_info_ref = new UserInfoRef(this, user_id);
    }
    else
    {
      user_info_ref = ReferenceCounting::add_ref(user_ref_it->second);
    }

    {
      /* write base profile */
      SmartMemBuf_var buf = new SmartMemBuf();
      buf->membuf().assign(
        user_profile.plain_user_info.get(),
        user_profile.plain_size);

      user_profiles_->save_profile(
        user_id,
        Generics::transfer_membuf(buf),
        Generics::Time::get_time_of_day());
    }

    {
      /* write history profile */
      SmartMemBuf_var buf = new SmartMemBuf();
      buf->membuf().assign(
        user_profile.plain_history_user_info.get(),
        user_profile.plain_history_size);

      user_history_profiles_->save_profile(
        user_id,
        Generics::transfer_membuf(buf),
        Generics::Time::get_time_of_day());
    }

    return ReferenceCounting::add_ref(user_info_ref);
  }

  void
  UserInfoExchangePool::ProviderStorage::erase_old_profiles(
    const Generics::Time expire_time)
    /*throw(eh::Exception)*/
  {
    Generics::Time tm = Generics::Time::get_time_of_day() - expire_time;

    user_profiles_->clear_expired(tm);
    user_history_profiles_->clear_expired(tm);
  }

  void
  UserInfoExchangePool::
  ProviderStorage::read(const char* user_id, UserProfile& user_profile)
    /*throw(eh::Exception)*/
  {
    ReadGuard_ lock(lock_);

    const char* p_uid = strchr(user_id, '_');
    user_profile.user_id = ++p_uid;

    {
      ConstSmartMemBuf_var up = user_profiles_->get_profile(user_id);

      if (up.in() != 0)
      {
        if (up->membuf().size() != 0)
        {
          user_profile.plain_size = up->membuf().size();
          user_profile.plain_user_info.reset(up->membuf().size());

          ::memcpy(user_profile.plain_user_info.get(),
                   up->membuf().data(),
                   up->membuf().size());
        }
        else
        {
          user_profile.plain_size = 0;
          user_profile.plain_user_info.reset(0);
        }
      }
      else
      {
        user_profile.plain_size = 0;
        user_profile.plain_user_info.reset(0);
      }
    }

    {
      /* read history profile */
      ConstSmartMemBuf_var up = user_history_profiles_->get_profile(user_id);

      if (up.in() != 0)
      {
        if (up->membuf().size() != 0)
        {
          user_profile.plain_history_size = up->membuf().size();
          user_profile.plain_history_user_info.reset(up->membuf().size());

          ::memcpy(user_profile.plain_history_user_info.get(),
                   up->membuf().data(),
                   up->membuf().size());
        }
        else
        {
          user_profile.plain_history_size = 0;
          user_profile.plain_history_user_info.reset(0);
        }
      }
      else
      {
        user_profile.plain_history_size = 0;
        user_profile.plain_history_user_info.reset(0);
      }
    }
  }

  void
  UserInfoExchangePool::
  ProviderStorage::erase_(const char* user_id) noexcept
  {
    WriteGuard_ lock(lock_);

    UserRefMap::iterator user_ref_it = user_refs_.find(user_id);

    if(user_ref_it != user_refs_.end())
    {
      user_refs_.erase(user_ref_it);
    }

    user_profiles_->remove_profile(user_id);
    user_history_profiles_->remove_profile(user_id);
  }

  /** UserInfoExchangePool::Customer */
  UserInfoExchangePool::
  Customer::Customer(const char* customer_id) noexcept
    : customer_id_(customer_id)
  {}

  UserInfoExchangePool::Customer::~Customer() noexcept
  {}

  void
  UserInfoExchangePool::Customer::receive_users(
    UserProfileList& user_profiles,
    const ReceiveCriteria& receive_criteria) /*throw(eh::Exception)*/
  {
    try
    {
      WriteGuard_ lock(lock_);

      size_t sum_size = 0;
      bool stop = false;

      UserInfoBlockList::iterator block_it = received_user_blocks_.begin();

      std::vector<bool> chunks_mask(receive_criteria.common_chunks_number, false);

      for(ReceiveCriteria::ChunkIdList::const_iterator it =
            receive_criteria.chunk_ids.begin();
          it != receive_criteria.chunk_ids.end(); ++it)
      {
        if(*it > receive_criteria.common_chunks_number)
        {
          Stream::Error ostr;
          ostr << "UserInfoExchangePool::Customer::receive_users(): "
            "Non correct chunk_id in sequence. It great then common chunks number: "
            << *it << " > " << receive_criteria.common_chunks_number << ".";
          throw Exception(ostr);
        }

        chunks_mask[*it] = true;
      }

      for(; block_it != received_user_blocks_.end() && !stop; )
      {
        UserInfoRefList& block_users = block_it->users;

        for(UserInfoRefList::iterator user_it =
              block_users.begin();
            user_it != block_users.end() && !stop; )
        {
          const char* p_prov = (*user_it)->user_id();
          const char* p_uid = strchr(p_prov, '_');

          AdServer::Commons::UserId user_id(++p_uid);

          if(chunks_mask[AdServer::Commons::uuid_distribution_hash(user_id) %
                         receive_criteria.common_chunks_number])
          {
            user_profiles.push_back(UserProfile());
            UserProfile& new_user_profile = user_profiles.back();

            (*user_it)->read(new_user_profile);
            std::string provider_id(p_prov, p_uid - p_prov);
            new_user_profile.provider_id = provider_id;

            sum_size += user_profiles.back().plain_size;

            if(sum_size > receive_criteria.max_response_plain_size)
            {
              user_profiles.pop_back();
              stop = true;
            }
            else
            {
              user_it = block_users.erase(user_it);
            }
          }
          else
          {
            ++user_it;
          }
        }

        if(block_users.empty())
        {
          block_it = received_user_blocks_.erase(block_it);
        }
        else
        {
          ++block_it;
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "UserInfoExchangePool::Customer::receive_users(): "
        "Caught eh::Exception. : "
        << ex.what();
      throw Exception(ostr);
    }
  }

  void
  UserInfoExchangePool::Customer::notify_users_receiving(
    const char* provider_id,
    const UserInfoRefList& users) /*throw(eh::Exception)*/
  {
    WriteGuard_ lock(lock_);

    UserInfoBlock user_info_block;
    user_info_block.provider_id = provider_id;
    user_info_block.users.assign(users.begin(), users.end());

    received_user_blocks_.push_back(user_info_block);
  }

  /** UserInfoExchangePool::Provider */
  UserInfoExchangePool::Provider::Provider(
    Generics::ActiveObjectCallback* callback,
    const char* plain_storage_path,
    const char* base_prefix,
    const char* plain_history_storage_path,
    const char* history_prefix,
    const Generics::Time extend_time)
    /*throw(eh::Exception)*/
    : storage_(
        new ProviderStorage(
          callback,
          plain_storage_path,
          base_prefix,
          plain_history_storage_path,
          history_prefix,
          extend_time))
  {
    add_child_object(storage_);
  }

  UserInfoExchangePool::Provider::~Provider() noexcept
  {
  }

  void
  UserInfoExchangePool::Provider::erase_old_profiles(
    const Generics::Time expire_time)
    /*throw(eh::Exception)*/
  {
    WriteGuard_ lock(lock_);

    storage_->erase_old_profiles(expire_time);
  }

  void
  UserInfoExchangePool::
  Provider::register_users_request(
    Customer* customer,
    const UserIdList& users) /*throw(eh::Exception)*/
  {
    WriteGuard_ lock(lock_);

    for(UserIdList::const_iterator it = users.begin();
        it != users.end(); ++it)
    {
      requested_users_[*it].customers.insert(ReferenceCounting::add_ref(customer));
    }
  }

  void
  UserInfoExchangePool::
  Provider::get_users_requests(UserIdList& users)
    /*throw(eh::Exception)*/
  {
    ReadGuard_ lock(lock_);

    for(UserInfoRequestMap::const_iterator it = requested_users_.begin();
        it != requested_users_.end(); ++it)
    {
      const char* uid = strchr(it->first.c_str(), '_');
      std::string user_id(++uid);

      users.push_back(user_id);
    }
  }

  void
  UserInfoExchangePool::
  Provider::fill_users(const UserProfileList& users)
    /*throw(eh::Exception)*/
  {
    bool error = false;
    std::ostringstream error_ostr;

    try
    {
      WriteGuard_ lock(lock_);

      typedef
        std::list<ProviderStorage::UserInfoRef_var>
        UserInfoRefList;

      typedef
        std::map<Customer_var, UserInfoRefList>
        CustomerUserInfoRefsMap;

      CustomerUserInfoRefsMap user_refs;

      for(UserProfileList::const_iterator it = users.begin();
          it != users.end(); ++it)
      {
        UserInfoRequestMap::iterator request_it =
          requested_users_.find(it->user_id);

        if(request_it != requested_users_.end())
        {
          /* save user profile to plain storage */
          ProviderStorage::UserInfoRef_var new_user_ref =
            storage_->insert_i(*it);

          CustomerSet& customers = request_it->second.customers;

          for(CustomerSet::const_iterator customer_it = customers.begin();
              customer_it != customers.end(); ++customer_it)
          {
            Customer_var customer(*customer_it);
            user_refs[customer].push_back(new_user_ref);
          }

          requested_users_.erase(request_it);
        }

        /* notify customers for requested users */
        for(CustomerUserInfoRefsMap::const_iterator it =
              user_refs.begin();
            it != user_refs.end(); ++it)
        {
          Customer* customer = it->first;

          try
          {
            customer->notify_users_receiving(
              provider_id_.c_str(),
              it->second);
          }
          catch(const eh::Exception& ex)
          {
            error = true;
            error_ostr
              << "Can't notify customer '" << customer->get_name() << "'. "
                 "Caught eh::Exception. : " << ex.what() << ' ';
          }
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "UserInfoExchangePool::Provider::fill_users(..): "
        "Caught eh::Exception. : " << ex.what();
      throw Exception(ostr);
    }

    if(error)
    {
      Stream::Error ostr;
      ostr << "UserInfoExchangePool::Provider::fill_users(..): "
        << error_ostr.str();
      throw Exception(ostr);
    }
  }

  /** UserInfoExchangePool */
  UserInfoExchangePool::UserInfoExchangePool(
    Generics::ActiveObjectCallback* callback,
    const char* providers_dir,
    const Generics::Time extend_time) /*throw(Exception)*/
    : providers_dir_(providers_dir)
  {
    try
    {
      std::ostringstream ps_file_ostr;
      ps_file_ostr << providers_dir_ << "/";

      std::ostringstream hist_file_ostr;
      hist_file_ostr << ps_file_ostr.str().c_str();

      providers_ = new UserInfoExchangePool::Provider(
        callback,
        ps_file_ostr.str().c_str(),
        ".users",
        hist_file_ostr.str().c_str(),
        ".husers",
        extend_time);

      add_child_object(providers_);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error err;
      err << __func__ << ": eh::Exception: " << e.what();
      throw Exception(err);
    }
  }

  UserInfoExchangePool::~UserInfoExchangePool() noexcept
  {
  }

  void
  UserInfoExchangePool::erase_old_profiles(
    const Generics::Time expire_time)
    /*throw(eh::Exception)*/
  {
    Provider_var provider = get_provider();

    provider->erase_old_profiles(expire_time);
  }

  void
  UserInfoExchangePool::register_users_request(
    const char* customer_id,
    const char* /*provider_id*/,
    const UserIdList& users) /*throw(Exception)*/
  {
    Provider_var provider = get_provider();
    Customer_var customer = get_customer(customer_id);

    provider->register_users_request(customer, users);
  }

  void
  UserInfoExchangePool::get_users_request(
    const char* /*provider_id*/,
    UserIdList& users) /*throw(Exception)*/
  {
    Provider_var provider = get_provider();
    provider->get_users_requests(users);
  }

  void
  UserInfoExchangePool::add_users(
    const char* /*provider_id*/,
    const UserProfileList& users) /*throw(Exception)*/
  {
    try
    {
      Provider_var provider = get_provider();
      provider->fill_users(users);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "UserInfoExchangePool::add_users(..): "
        << "Caught eh::Exception: : "
        << ex.what();

      throw Exception(ostr);
    }
  }

  void
  UserInfoExchangePool::receive_users(
    const char* customer_id,
    UserProfileList& users,
    const ReceiveCriteria& receive_criteria) /*throw(Exception)*/
  {
    try
    {
      Customer_var customer = get_customer(customer_id);
      customer->receive_users(users, receive_criteria);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "UserInfoExchangePool::receive_users(..): "
        << "Caught eh::Exception: : "
        << ex.what();

      throw Exception(ostr);
    }
  }

  UserInfoExchangePool::Provider*
  UserInfoExchangePool::get_provider()
    /*throw(Exception)*/
  {
    ReadGuard_ lock(lock_);
    return ReferenceCounting::add_ref(providers_);
  }

  UserInfoExchangePool::Customer*
  UserInfoExchangePool::get_customer(const char* customer_id)
    noexcept
  {
    {
      ReadGuard_ lock(lock_);

      CustomerMap::iterator it = customers_.find(customer_id);
      if(it != customers_.end())
      {
        return ReferenceCounting::add_ref(it->second);
      }
    }

    Customer_var new_customer = new Customer(customer_id);

    WriteGuard_ lock(lock_);

    CustomerMap::iterator it = customers_.find(customer_id);
    if(it == customers_.end())
    {
      customers_.insert(
        CustomerMap::value_type(customer_id, new_customer));
    }
    else
    {
      new_customer = it->second;
    }

    return ReferenceCounting::add_ref(new_customer);
  }

} /* UserInfoSvcs */
} /* AdServer */
