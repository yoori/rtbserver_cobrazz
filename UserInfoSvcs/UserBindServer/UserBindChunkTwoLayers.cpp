// THIS
#include <Commons/Algs.hpp>
#include <UserInfoSvcs/UserBindServer/UserBindChunkTwoLayers.hpp>

namespace AdServer::UserInfoSvcs
{

UserBindTwoLayersChunk::UserBindTwoLayersChunk(
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
  const std::uint32_t rocksdb_ttl)
  : logger_(ReferenceCounting::add_ref(logger)),
    sources_expire_times_(
      new SourcesExpireTime(
        seen_default_expire_time,
        bound_default_expire_time,
        bound_list_source_expire_time)),
    directory_(directory),
    seen_name_(seen_name),
    bound_name_(bound_name),
    min_age_(min_age),
    bind_at_min_age_(bind_at_min_age),
    max_bad_event_(max_bad_event)
{
  auto load_filter = [
    load_slave = load_slave,
    partitions_number = partitions_number,
    partition_index = partition_index] (const std::size_t hash)
  {
    if (load_slave)
    {
      return true;
    }

    return (hash >> 8) % partitions_number == partition_index;
  };

  portions_ = std::make_shared<Portions>(
    logger_.in(),
    portions_number,
    directory,
    seen_name,
    bound_name,
    directory + "/rocksdb",
    rocksdb_number_threads,
    rocksdb_compaction_style,
    rocksdb_block_сache_size_mb,
    rocksdb_ttl,
    100,
    1000,
    std::move(load_filter));
}

void UserBindTwoLayersChunk::dump()
{
  FilterSourceDate filter(sources_expire_times_);
  portions_->save(filter);
}

void UserBindTwoLayersChunk::clear_expired(
  const Generics::Time& /*unbound_expire_time*/,
  const Generics::Time& /*bound_expire_time*/)
{
}

UserBindTwoLayersChunk::UserInfo
UserBindTwoLayersChunk::add_user_id(
  const String::SubString& external_id,
  const Commons::UserId& user_id,
  const Generics::Time& now,
  const bool resave_if_exists,
  const bool ignore_bad_event)
{
  String::SubString external_id_prefix;
  String::SubString external_id_suffix;
  divide_id(
    external_id_prefix,
    external_id_suffix,
    external_id);

  StringDefHashAdapter external_id_hash(
    external_id,
    portions_->hash(external_id));
  auto portion = portions_->portion(external_id_hash.hash());
  portion->erase_seen(external_id_hash);

  StringDefHashAdapter external_id_prefix_hash(
    external_id_prefix,
    portions_->hash(external_id_prefix));
  ExternalIdHashAdapter external_id_suffix_hash(
    external_id_suffix,
    portions_->hash(external_id_suffix));

  BoundUserInfoHolder previous_user_info;
  const auto container = portion->get_bound(
    previous_user_info,
    external_id_prefix_hash,
    external_id_suffix_hash);
  if (container)
  {
    previous_user_info.reset_init_day();

    ResultHolder result_holder(
      false, // invalid_operation
      true); // user_found
    UserInfo result_user_info = adapt_user_info(
      previous_user_info,
      false,
      result_holder);

    if (resave_if_exists)
    {
      save_user_id(
        previous_user_info,
        result_user_info,
        user_id,
        ignore_bad_event,
        now);
      container->set(
        external_id_suffix_hash,
        previous_user_info);
    }

    return result_user_info;
  }
  else
  {
    ResultHolder result_holder(
      false, // invalid_operation
      false); // user_found
    BoundUserInfoHolder user_info;

    UserInfo result_user_info = adapt_user_info(
      user_info,
      false,
      result_holder);

    save_user_id(
      user_info,
      result_user_info,
      user_id,
      ignore_bad_event,
      now);
    portion->set_bound(
      external_id_prefix_hash,
      external_id_suffix_hash,
      user_info);

    return result_user_info;
  }
}

UserBindTwoLayersChunk::UserInfo
UserBindTwoLayersChunk::get_user_id(
  const String::SubString& external_id,
  const Commons::UserId& current_user_id,
  const Generics::Time& now,
  const bool silent,
  const Generics::Time& create_time,
  const bool for_set_cookie)
{
  static const Generics::Time base_time(
    String::SubString("2000-01-01"), "%Y-%m-%d");

  String::SubString external_id_prefix;
  String::SubString external_id_suffix;
  divide_id(
    external_id_prefix,
    external_id_suffix,
    external_id);

  StringDefHashAdapter external_id_hash(
    external_id,
    portions_->hash(external_id));
  auto portion = portions_->portion(external_id_hash.hash());

  StringDefHashAdapter external_id_prefix_hash(
    external_id_prefix,
    portions_->hash(external_id_prefix));
  ExternalIdHashAdapter external_id_suffix_hash(
    external_id_suffix,
    portions_->hash(external_id_suffix));

  bool insert_into_bound = false;
  bool inserted = false;
  ResultHolder result_holder(
    false, // invalid_operation
    false // user_found
  );

  BoundUserInfoHolder bound_user_info_holder;

  if (get_user_id_bound(
    bound_user_info_holder,
    insert_into_bound,
    inserted,
    result_holder,
    portion,
    external_id_prefix_hash,
    external_id_suffix_hash,
    current_user_id,
    now,
    Generics::Time::ZERO, // create time not affect bound users
    false, // don't insert
    silent,
    for_set_cookie))
  {
    return adapt_user_info(bound_user_info_holder, false, result_holder);
  }

  SeenUserInfoHolder seen_user_info_holder;
  get_user_id_seen(
    seen_user_info_holder,
    insert_into_bound,
    inserted,
    result_holder,
    portion,
    external_id_hash,
    current_user_id,
    now,
    create_time,
    true, // insert if not found
    silent,
    for_set_cookie,
    base_time);

  if (insert_into_bound)
  {
    bound_user_info_holder.user_id =
      AdServer::Commons::UserId::create_random_based();
    if (for_set_cookie)
    {
      bound_user_info_holder.flags |= BoundUserInfoHolder::BF_SETCOOKIE;
    }

    portion->set_bound(
      std::move(external_id_prefix_hash),
      std::move(external_id_suffix_hash),
      bound_user_info_holder);

    return adapt_user_info(
      bound_user_info_holder,
      true,
      result_holder);
  }

  return adapt_user_info(
    seen_user_info_holder,
    inserted,
    result_holder,
    now,
    base_time);
}

void UserBindTwoLayersChunk::divide_id(
  String::SubString& id_prefix,
  String::SubString& id_suffix,
  const String::SubString& id) noexcept
{
  const auto pos = id.find('/');
  if (pos != String::SubString::NPOS)
  {
    id_prefix = id.substr(0, pos);
    id_suffix = id.substr(pos + 1, id.size() - pos - 1);
  }
  else
  {
    id_prefix = String::SubString();
    id_suffix = id;
  }
}

UserBindTwoLayersChunk::UserInfo
UserBindTwoLayersChunk::adapt_user_info(
  const SeenUserInfoHolder& user_info,
  const bool inserted,
  const ResultHolder& result_holder,
  const Generics::Time& now,
  const Generics::Time& base_time) const noexcept
{
  UserInfo result_user_info;
  result_user_info.min_age_reached = (
    user_info.is_init() &&
    user_info.get_time(base_time) + min_age_ < now);
  result_user_info.created = inserted;
  result_user_info.invalid_operation = result_holder.invalid_operation;
  result_user_info.user_found = result_holder.user_found;

  return result_user_info;
}

UserBindTwoLayersChunk::UserInfo
UserBindTwoLayersChunk::adapt_user_info(
  const BoundUserInfoHolder& user_info,
  const bool user_id_generated,
  const ResultHolder& result_holder) const noexcept
{
  UserInfo result_user_info;
  result_user_info.user_id = user_info.user_id;
  result_user_info.min_age_reached = true;
  result_user_info.user_id_generated = user_id_generated;
  result_user_info.invalid_operation = result_holder.invalid_operation;
  result_user_info.user_found = result_holder.user_found;

  return result_user_info;
}

bool UserBindTwoLayersChunk::modify_at_get(
  SeenUserInfoHolder& user_info,
  const Commons::UserId& /*current_user_id*/,
  const Generics::Time& now,
  const Generics::Time& base_time,
  const std::optional<Generics::Time> create_time,
  const bool /*for_set_cookie*/,
  const bool silent) const noexcept
{
  if (silent)
  {
    return false;
  }

  if (!user_info.is_init() ||
    (create_time ? *create_time : now) < user_info.get_time(base_time))
  {
    user_info.set_time((create_time ? *create_time : now), base_time);
  }

  // return min age reached
  return bind_at_min_age_ && (user_info.get_time(base_time) + min_age_ < now);
}

bool UserBindTwoLayersChunk::modify_at_get(
  BoundUserInfoHolder& user_info,
  const Commons::UserId& current_user_id,
  const Generics::Time& now,
  const bool for_set_cookie,
  const bool /*silent*/) const noexcept
{
  if (for_set_cookie)
  {
    if (user_info.flags & BoundUserInfoHolder::BF_SETCOOKIE)
    {
      if (user_info.user_id != current_user_id)
      {
        rotate_bad_event_count(user_info, now);
        user_info.bad_event_count += 1;
      }
    }
    else
    {
      user_info.flags |= BoundUserInfoHolder::BF_SETCOOKIE;
    }
  }

  return false;
}

void UserBindTwoLayersChunk::rotate_bad_event_count(
  BoundUserInfoHolder& user_info,
  const Generics::Time& now) const noexcept
{
  const unsigned long current_day =
    Algs::round_to_day(now).tv_sec / Generics::Time::ONE_DAY.tv_sec;

  if (current_day != user_info.last_bad_event_day)
  {
    user_info.last_bad_event_day = current_day;
    user_info.bad_event_count = 0;
  }
}

bool UserBindTwoLayersChunk::prepare_result_at_get(
  SeenUserInfoHolder& /*result*/,
  ResultHolder& /*result_holder*/,
  const bool /*for_set_cookie*/) noexcept
{
  return true;
}

bool UserBindTwoLayersChunk::prepare_result_at_get(
  BoundUserInfoHolder& result,
  ResultHolder& result_holder,
  const bool for_set_cookie) noexcept
{
  // check max event number
  if (for_set_cookie &&
    (result.flags & BoundUserInfoHolder::BF_SETCOOKIE) &&
    result.bad_event_count > max_bad_event_)
  {
    result_holder.invalid_operation = true;
  }
  result_holder.user_found = true;

  return true;
}

void UserBindTwoLayersChunk::save_user_id(
  BoundUserInfoHolder& user_info,
  UserInfo& result_user_info,
  const Commons::UserId& user_id,
  const bool ignore_bad_event,
  const Generics::Time& now) const noexcept
{
  if (user_info.flags & BoundUserInfoHolder::BF_SETCOOKIE)
  {
    if (user_id != user_info.user_id)
    {
      rotate_bad_event_count(user_info, now);

      if (user_info.bad_event_count < max_bad_event_)
      {
        user_info.user_id = user_id;
      }
      else
      {
        // skip operation
        result_user_info.invalid_operation = true;
      }
      user_info.bad_event_count += 1;
    }
  }
  else
  {
    user_info.flags |= BoundUserInfoHolder::BF_SETCOOKIE;
    user_info.user_id = user_id;
  }

  if (ignore_bad_event)
  {
    user_info.user_id = user_id;
  }
}

bool UserBindTwoLayersChunk::get_user_id_bound(
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
  const bool for_set_cookie)
{
  insert_into_bound = false;

  // find user
  bool found = false;
  bool insert_after = insert_if_not_found;

  const auto container = portion->get_bound(
    result_user_info,
    external_id_prefix,
    external_id_suffix);
  if (container)
  {
    result_user_info.reset_init_day();

    found = true;
    insert_into_bound = modify_at_get(
      result_user_info,
      current_user_id,
      now,
      for_set_cookie,
      silent);
    insert_after = !insert_into_bound;

    if (silent || !insert_into_bound)
    {
      container->set(external_id_suffix, result_user_info);
      return prepare_result_at_get(
        result_user_info,
        result_holder,
        for_set_cookie);
    }

    container->erase(external_id_suffix);
  }

  if (silent || !insert_after)
  {
    return false;
  }

  if ((insert_if_not_found || found) &&
    modify_at_get(
      result_user_info,
      current_user_id,
      now,
      for_set_cookie,
      silent))
  {
    insert_into_bound = true;
    return false;
  }

  if (!found)
  {
    inserted = true;
  }

  portion->set_bound(
    external_id_prefix,
    external_id_suffix,
    result_user_info);

  return prepare_result_at_get(
    result_user_info,
    result_holder,
    for_set_cookie);
}

bool UserBindTwoLayersChunk::get_user_id_seen(
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
  const Generics::Time& base_time)
{
  insert_into_bound = false;

  // find user
  bool found = false;
  bool insert_after = insert_if_not_found;

  const auto container = portion->get_seen(
    result_user_info,
    external_id);
  if (container)
  {
    result_user_info.reset_init_day();

    found = true;
    insert_into_bound = modify_at_get(
      result_user_info,
      current_user_id,
      now,
      base_time,
      {}, // create_time
      for_set_cookie,
      silent);
    insert_after = !insert_into_bound;

    if (silent || !insert_into_bound)
    {
      container->set(external_id, result_user_info);
      return prepare_result_at_get(
        result_user_info,
        result_holder,
        for_set_cookie);
    }

    container->erase(external_id);
  }

  if (silent || !insert_after)
  {
    return false;
  }

  const std::optional<Generics::Time> first_seen_time =
    create_time == Generics::Time::ZERO ?
      std::optional<Generics::Time>{} : create_time;

  if ((insert_if_not_found || found) && modify_at_get(
    result_user_info,
    current_user_id,
    now,
    base_time,
    first_seen_time,
    for_set_cookie,
    silent))
  {
    insert_into_bound = true;
    return false;
  }

  if (!found)
  {
    inserted = true;
  }

  portion->set_seen(external_id, result_user_info);

  return prepare_result_at_get(
    result_user_info,
    result_holder,
    for_set_cookie);
}

} // namespace AdServer::UserInfoSvcs