#ifndef USERINFOSVCS_USERBINDCHUNKTWOLAYERS_HPP
#define USERINFOSVCS_USERBINDCHUNKTWOLAYERS_HPP

// UNIXCOMMONS
#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

// THIS
#include <UserInfoSvcs/UserBindServer/UserBindChunkTypes.hpp>
#include <UserInfoSvcs/UserBindServer/UserBindProcessor.hpp>

namespace AdServer::UserInfoSvcs
{

class UserBindTwoLayersChunk final :
  public UserBindProcessor,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  using ExpireTime = std::uint16_t;
  using ListSourceExpireTime = SourcesExpireTime::ListSourceExpireTime;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  struct ResultHolder
  {
    ResultHolder(
      const bool invalid_operation,
      const bool user_found)
      : invalid_operation(invalid_operation),
        user_found(user_found)
    {
    }

    bool invalid_operation;
    bool user_found;
  };

public:
  UserBindTwoLayersChunk(
    Logging::Logger* logger,
    const ExpireTime seen_default_expire_time,
    const ExpireTime bound_default_expire_time,
    const ListSourceExpireTime& bound_list_source_expire_time,
    const std::string& directory,
    const std::string& seen_name,
    const std::string& bound_name,
    const Generics::Time& min_age,
    const bool bind_at_min_age,
    const std::size_t max_bad_event,
    const std::size_t portions_number,
    const bool load_slave,
    const std::size_t partition_index, // instance partition number (first or second part of cluster)
    const std::size_t partitions_number,
    const std::size_t rocksdb_number_threads,
    const rocksdb::CompactionStyle rocksdb_compaction_style,
    const std::uint32_t rocksdb_block_сache_size_mb,
    const std::uint32_t rocksdb_ttl);

  // return previous state
  UserInfo add_user_id(
    const String::SubString& external_id,
    const Commons::UserId& user_id,
    const Generics::Time& now,
    const bool resave_if_exists,
    const bool ignore_bad_event) override;

  UserInfo get_user_id(
    const String::SubString& external_id,
    const Commons::UserId& current_user_id,
    const Generics::Time& now,
    const bool silent,
    const Generics::Time& create_time,
    const bool for_set_cookie) override;

  void clear_expired(
    const Generics::Time& /*unbound_expire_time*/,
    const Generics::Time& /*bound_expire_time*/) override;

  void dump() override;

protected:
  ~UserBindTwoLayersChunk() override = default;

private:
  void divide_id(
    String::SubString& id_prefix,
    String::SubString& id_suffix,
    const String::SubString& id) noexcept;

  UserInfo adapt_user_info(
    const SeenUserInfoHolder& user_info,
    const bool inserted,
    const ResultHolder& result_holder,
    const Generics::Time& now,
    const Generics::Time& base_time) const noexcept;

  UserInfo adapt_user_info(
    const BoundUserInfoHolder& user_info,
    const bool user_id_generated,
    const ResultHolder& result_holder) const noexcept;

  void save_user_id(
    BoundUserInfoHolder& user_info,
    UserInfo& result_user_info,
    const Commons::UserId& user_id,
    const bool ignore_bad_event,
    const Generics::Time& now) const noexcept;

  bool modify_at_get(
    SeenUserInfoHolder& user_info,
    const Commons::UserId& current_user_id,
    const Generics::Time& now,
    const Generics::Time& base_time,
    const std::optional<Generics::Time> create_time,
    const bool for_set_cookie,
    const bool silent) const noexcept;

  bool modify_at_get(
    BoundUserInfoHolder& user_info,
    const Commons::UserId& current_user_id,
    const Generics::Time& now,
    const bool for_set_cookie,
    const bool silent) const noexcept;

  void rotate_bad_event_count(
    BoundUserInfoHolder& user_info,
    const Generics::Time& now) const noexcept;

  bool prepare_result_at_get(
    SeenUserInfoHolder& result,
    ResultHolder& result_holder,
    const bool for_set_cookie) noexcept;

  bool prepare_result_at_get(
    BoundUserInfoHolder& result,
    ResultHolder& result_holder,
    const bool for_set_cookie) noexcept;

  bool get_user_id_bound(
    BoundUserInfoHolder& result_user_info,
    bool& insert_into_bound,
    bool& inserted,
    ResultHolder& result_holder,
    const PortionPtr& portion,
    const StringDefHashAdapter& external_id_prefix,
    const ExternalIdHashAdapter& external_id_suffix,
    const Commons::UserId& current_user_id,
    const Generics::Time& now,
    const Generics::Time& create_time,
    const bool insert_if_not_found,
    const bool silent,
    const bool for_set_cookie);

  bool get_user_id_seen(
    SeenUserInfoHolder& result_user_info,
    bool& insert_into_bound,
    bool& inserted,
    ResultHolder& result_holder,
    const PortionPtr& portion,
    const HashHashAdapter& external_id,
    const Commons::UserId& current_user_id,
    const Generics::Time& now,
    const Generics::Time& create_time,
    const bool insert_if_not_found,
    const bool silent,
    const bool for_set_cookie,
    const Generics::Time& base_time);

private:
  const Logging::Logger_var logger_;

  const SourcesExpireTimePtr sources_expire_times_;

  const std::string directory_;

  const std::string seen_name_;

  const std::string bound_name_;

  const Generics::Time min_age_;

  const bool bind_at_min_age_;

  const unsigned long max_bad_event_;

  PortionsPtr portions_;
};

using UserBindTwoLayersChunk_var = ReferenceCounting::SmartPtr<UserBindTwoLayersChunk>;

} // namespace AdServer::UserInfoSvcs

#endif /*USERINFOSVCS_USERBINDCHUNKTWOLAYERS_HPP*/