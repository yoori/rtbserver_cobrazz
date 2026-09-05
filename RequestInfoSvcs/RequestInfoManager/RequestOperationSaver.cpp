#include <LogCommons/LogCommons.hpp>

#include <RequestInfoSvcs/RequestInfoCommons/RequestOperationProfile.hpp>

#include <cstring>
#include <utility>

#include "Compatibility/RequestOperationImpressionProfileAdapter.hpp"
#include "RequestOperationLoader.hpp"
#include "RequestOperationSaver.hpp"

namespace Aspect
{
  const char REQUEST_OPERATION_SAVER[] = "RequestOperationSaver";
}

namespace AdServer::RequestInfoSvcs
{
  // RequestOperationSaver
  RequestOperationSaver::RequestOperationSaver(
    Logging::Logger* logger,
    const char* output_dir,
    const char* output_file_prefix,
    unsigned long chunks_count,
    const Generics::Time& flush_period,
    unsigned long threads_count)
    noexcept
    : MessageSaver(
        logger,
        output_dir,
        output_file_prefix,
        chunks_count,
        flush_period,
        threads_count)
  {}

  void
  RequestOperationSaver::process_impression(const ImpressionInfo& impression_info)
    /*throw(RequestOperationProcessor::Exception)*/
  {
    RequestOperationImpressionWriter operation_writer;
    operation_writer.version() = CURRENT_REQUESTOPERATIONIMPRESSION_PROFILE_VERSION;
    operation_writer.user_id() = impression_info.user_id.to_string();
    operation_writer.time() = impression_info.time.tv_sec;
    operation_writer.request_id() = impression_info.request_id.to_string();
    operation_writer.verify_impression() = impression_info.verify_impression;
    if (impression_info.pub_revenue.present())
    {
      operation_writer.pub_revenue_type() = impression_info.pub_revenue->revenue_type;
      operation_writer.pub_revenue() = impression_info.pub_revenue->impression.str();
      operation_writer.pub_sys_revenue() = "0"; // unused on load
    }
    else
    {
      operation_writer.pub_revenue_type() = AdServer::CampaignSvcs::RT_NONE;
    }

    Generics::MemBuf op_mem_buf(operation_writer.size());
    operation_writer.save(op_mem_buf.data(), op_mem_buf.size());

    write_operation_(
      impression_info.user_id,
      RequestOperationLoader::OP_IMPRESSION,
      std::move(op_mem_buf));
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
    Generics::MemBuf op_mem_buf(operation_writer.size());
    operation_writer.save(op_mem_buf.data(), op_mem_buf.size());

    write_operation_(user_id, RequestOperationLoader::OP_ACTION, std::move(op_mem_buf));
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
    Generics::MemBuf op_mem_buf(operation_writer.size());
    operation_writer.save(op_mem_buf.data(), op_mem_buf.size());

    write_operation_(user_id, RequestOperationLoader::OP_REQUEST_ACTION, std::move(op_mem_buf));
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
    const uint32_t change_profile_size = operation_writer.size();
    const uint32_t request_profile_size = request_profile->membuf().size();
    const std::size_t operation_size =
      sizeof(change_profile_size) + change_profile_size +
      sizeof(request_profile_size) + request_profile_size;
    Generics::MemBuf op_mem_buf(operation_size);
    char* current = static_cast<char*>(op_mem_buf.data());

    std::memcpy(current, &change_profile_size, sizeof(change_profile_size));
    current += sizeof(change_profile_size);
    operation_writer.save(current, change_profile_size);
    current += change_profile_size;
    std::memcpy(current, &request_profile_size, sizeof(request_profile_size));
    current += sizeof(request_profile_size);
    std::memcpy(current, request_profile->membuf().data(), request_profile_size);

    write_operation_(
      new_user_id,
      RequestOperationLoader::OP_CHANGE,
      std::move(op_mem_buf));
  }

  void
  RequestOperationSaver::write_operation_(
    const AdServer::Commons::UserId& user_id,
    unsigned long op,
    Generics::MemBuf&& mem_buf)
    /*throw(eh::Exception)*/
  {
    write_operation(
      AdServer::LogProcessing::user_id_distribution_hash(user_id),
      op,
      std::move(mem_buf));
  }
}
