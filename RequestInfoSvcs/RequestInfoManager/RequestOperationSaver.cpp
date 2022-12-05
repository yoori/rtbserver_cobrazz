#include <LogCommons/LogCommons.hpp>

#include <RequestInfoSvcs/RequestInfoCommons/RequestOperationProfile.hpp>

#include "Compatibility/RequestOperationImpressionProfileAdapter.hpp"
#include "RequestOperationLoader.hpp"
#include "RequestOperationSaver.hpp"

namespace Aspect
{
  const char REQUEST_OPERATION_SAVER[] = "RequestOperationSaver";
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  // RequestOperationSaver
  RequestOperationSaver::RequestOperationSaver(
    Logging::Logger* logger,
    const char* output_dir,
    const char* output_file_prefix,
    unsigned long chunks_count,
    const Generics::Time& flush_period)
    noexcept
    : MessageSaver(logger, output_dir, output_file_prefix, chunks_count, flush_period)
  {}

  void
  RequestOperationSaver::process_impression(
    const ImpressionInfo& impression_info)
    /*throw(RequestOperationProcessor::Exception)*/
  {
    RequestOperationImpressionWriter operation_writer;
    operation_writer.version() = CURRENT_REQUESTOPERATIONIMPRESSION_PROFILE_VERSION;
    operation_writer.user_id() = impression_info.user_id.to_string();
    operation_writer.time() = impression_info.time.tv_sec;
    operation_writer.request_id() = impression_info.request_id.to_string();
    operation_writer.verify_impression() = impression_info.verify_impression;
    if(impression_info.pub_revenue.present())
    {
      operation_writer.pub_revenue_type() = impression_info.pub_revenue->revenue_type;
      operation_writer.pub_revenue() = impression_info.pub_revenue->impression.str();
      operation_writer.pub_sys_revenue() = "0"; // unused on load
    }
    else
    {
      operation_writer.pub_revenue_type() = AdServer::CampaignSvcs::RT_NONE;
    }

    Generics::SmartMemBuf_var op_mem_buf(new Generics::SmartMemBuf(operation_writer.size()));
    operation_writer.save(op_mem_buf->membuf().data(), op_mem_buf->membuf().size());

    write_operation_(
      impression_info.user_id,
      RequestOperationLoader::OP_IMPRESSION,
      op_mem_buf->membuf());
  }

  void
  RequestOperationSaver::process_action(
    const AdServer::Commons::UserId& user_id,
    RequestContainerProcessor::ActionType action_type,
    const Generics::Time& time,
    const AdServer::Commons::RequestId& request_id)
    /*throw(RequestOperationProcessor::Exception)*/
  {
    RequestOperationActionWriter operation_writer;
    operation_writer.version() = 0;
    operation_writer.action_type() = action_type;
    operation_writer.time() = time.tv_sec;
    operation_writer.request_id() = request_id.to_string();
    operation_writer.user_id() = user_id.to_string();
    Generics::SmartMemBuf_var op_mem_buf(new Generics::SmartMemBuf(operation_writer.size()));
    operation_writer.save(op_mem_buf->membuf().data(), op_mem_buf->membuf().size());

    write_operation_(
      user_id,
      RequestOperationLoader::OP_ACTION,
      op_mem_buf->membuf());
  }

  void
  RequestOperationSaver::process_impression_post_action(
    const AdServer::Commons::UserId& user_id,
    const AdServer::Commons::RequestId& request_id,
    const RequestPostActionInfo& request_post_action_info)
    /*throw(RequestOperationProcessor::Exception)*/
  {
    RequestOperationActionWriter operation_writer;
    operation_writer.version() = 0;
    operation_writer.action_type() = 0;
    operation_writer.time() = request_post_action_info.time.tv_sec;
    operation_writer.request_id() = request_id.to_string();
    operation_writer.user_id() = user_id.to_string();
    operation_writer.action_name() = request_post_action_info.action_name;
    Generics::SmartMemBuf_var op_mem_buf(new Generics::SmartMemBuf(operation_writer.size()));
    operation_writer.save(op_mem_buf->membuf().data(), op_mem_buf->membuf().size());

    write_operation_(
      user_id,
      RequestOperationLoader::OP_REQUEST_ACTION,
      op_mem_buf->membuf());
  }

  void
  RequestOperationSaver::change_request_user_id(
    const AdServer::Commons::UserId& new_user_id,
    const AdServer::Commons::RequestId& request_id,
    const Generics::ConstSmartMemBuf* request_profile)
    /*throw(RequestOperationProcessor::Exception)*/
  {
    RequestOperationChangeUserWriter operation_writer;
    operation_writer.version() = 0;
    operation_writer.user_id() = new_user_id.to_string();
    operation_writer.request_id() = request_id.to_string();
    Generics::SmartMemBuf_var op_mem_buf(new Generics::SmartMemBuf(operation_writer.size()));
    operation_writer.save(op_mem_buf->membuf().data(), op_mem_buf->membuf().size());

    FileHolderGuard_var file_holder_guard = get_file_holder_(new_user_id);
    uint32_t op_index = RequestOperationLoader::OP_CHANGE;
    file_holder_guard->write(&op_index, sizeof(op_index));
    file_holder_guard->write(op_mem_buf->membuf());
    file_holder_guard->write(request_profile->membuf());
  }

  void
  RequestOperationSaver::write_operation_(
    const AdServer::Commons::UserId& user_id,
    unsigned long op,
    const Generics::MemBuf& mem_buf)
    /*throw(eh::Exception)*/
  {
    write_operation(
      AdServer::LogProcessing::user_id_distribution_hash(user_id),
      op,
      mem_buf);
  }

  RequestOperationSaver::FileHolderGuard_var
  RequestOperationSaver::get_file_holder_(
    const AdServer::Commons::UserId& user_id)
    /*throw(eh::Exception)*/
  {
    return MessageSaver::get_file_holder_(
      AdServer::LogProcessing::user_id_distribution_hash(user_id));
  }
}
}
