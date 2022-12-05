#include "UserBindOperationSaver.hpp"
#include "UserBindOperationProfile.hpp"

namespace AdServer
{
namespace UserInfoSvcs
{
  UserBindOperationSaver::UserBindOperationSaver(
    Logging::Logger* logger,
    const char* output_dir,
    const char* output_file_prefix,
    unsigned long chunks_count,
    const Generics::Time& flush_period,
    UserBindProcessor* next_processor)
    : MessageSaver(logger, output_dir, output_file_prefix, chunks_count, flush_period),
      next_processor_(ReferenceCounting::add_ref(next_processor))
  {}

  UserBindProcessor::UserInfo
  UserBindOperationSaver::add_user_id(
    const String::SubString& external_id,
    const Commons::UserId& user_id,
    const Generics::Time& now,
    bool resave_if_exists,
    bool ignore_bad_event)
    /*throw(ChunkNotFound, UserBindProcessor::Exception)*/
  {
    UserInfo res = next_processor_->add_user_id(
      external_id,
      user_id,
      now,
      resave_if_exists,
      ignore_bad_event);

    // don't mirror invalid operations (slave increase bad events only on mirrored operations)
    if(!res.invalid_operation)
    {
      // create add operation with low priority
      UserBindAddOperationWriter op_writer;
      op_writer.version() = 0;
      op_writer.external_id() = external_id.str();
      op_writer.user_id() = user_id.to_string();
      op_writer.time() = now.tv_sec;
      op_writer.resave_if_exists() = resave_if_exists;

      Generics::MemBuf op_mem_buf(op_writer.size());
      op_writer.save(op_mem_buf.data(), op_mem_buf.size());
      write_operation(
        AdServer::Commons::external_id_distribution_hash(external_id),
        OP_ADD_USER_ID,
        op_mem_buf);
    }

    return res;
  }

  UserBindProcessor::UserInfo
  UserBindOperationSaver::get_user_id(
    const String::SubString& external_id,
    const Commons::UserId& current_user_id,
    const Generics::Time& now,
    bool silent,
    const Generics::Time& create_time,
    bool for_set_cookie)
    /*throw(ChunkNotFound, UserBindProcessor::Exception)*/
  {
    UserInfo res = next_processor_->get_user_id(
      external_id,
      current_user_id,
      now,
      silent,
      create_time,
      for_set_cookie);

    if(res.user_id_generated)
    {
      // create add operation with low priority
      UserBindAddOperationWriter op_writer;
      op_writer.version() = 0;
      op_writer.external_id() = external_id.str();
      op_writer.user_id() = res.user_id.to_string();
      op_writer.time() = now.tv_sec;
      op_writer.resave_if_exists() = 0;

      Generics::MemBuf op_mem_buf(op_writer.size());
      op_writer.save(op_mem_buf.data(), op_mem_buf.size());
      write_operation(
        AdServer::Commons::external_id_distribution_hash(external_id),
        OP_ADD_USER_ID,
        op_mem_buf);
    }
    else if(res.created)
    {
      UserBindGetOperationWriter op_writer;
      op_writer.version() = 0;
      op_writer.external_id() = external_id.str();
      op_writer.current_user_id() = current_user_id.to_string();

      // for create non bound record on other server will be used create_time
      // if defined
      if(create_time == Generics::Time::ZERO)
      {
        op_writer.time() = now.tv_sec;
      }
      else
      {
        op_writer.time() = std::min(create_time, now).tv_sec;
      }

      Generics::MemBuf op_mem_buf(op_writer.size());
      op_writer.save(op_mem_buf.data(), op_mem_buf.size());
      write_operation(
        AdServer::Commons::external_id_distribution_hash(external_id),
        OP_GET_USER_ID,
        op_mem_buf);
    }

    return res;
  }

  void
  UserBindOperationSaver::clear_expired(
    const Generics::Time& unbound_expire_time,
    const Generics::Time& bound_expire_time)
    /*throw(UserBindProcessor::Exception)*/
  {
    next_processor_->clear_expired(
      unbound_expire_time,
      bound_expire_time);
  }

  void
  UserBindOperationSaver::dump()
    /*throw(UserBindProcessor::Exception)*/
  {
    // TODO: dump operations

    next_processor_->dump();
  }
}
}
