#include <RequestInfoSvcs/RequestInfoCommons/RequestOperationProfile.hpp>

#include "Compatibility/RequestOperationImpressionProfileAdapter.hpp"
#include "RequestOperationLoader.hpp"

namespace AdServer
{
namespace RequestInfoSvcs
{
  RequestOperationLoader::RequestOperationLoader(
    RequestOperationProcessor* request_operation_processor)
    noexcept
    : request_operation_processor_(ReferenceCounting::add_ref(request_operation_processor))
  {}

  bool
  RequestOperationLoader::process_file(
    unsigned long& processed_lines_count,
    const char* file,
    Generics::ActiveObject* interrupter)
    /*throw(Exception)*/
  {
    static const char* FUN = "RequestOperationLoader::process_file()";

    Generics::MemBuf request_operation_membuf;
    uint32_t op_index;
    unsigned long skip_records = processed_lines_count;
    bool terminated = false;

    ProfilingCommons::FileReader file_reader(file, 1024*1024);

    while(file_reader.read(&op_index, sizeof(op_index)) != 0)
    {
      if (interrupter && !interrupter->active())
      {
        terminated = true;
        break;
      }

      if(op_index == OP_CHANGE)
      {
        if(skip_records == 0)
        {
          read_change_request_user_id_(file_reader, request_operation_membuf);
        }
        else
        {
          skip_change_request_user_id_(file_reader);
        }
      }
      else if(op_index == OP_IMPRESSION)
      {
        if(skip_records == 0)
        {
          read_impression_(file_reader, request_operation_membuf);
        }
        else
        {
          skip_single_buffer_operation_(file_reader);
        }
      }
      else if(op_index == OP_ACTION)
      {
        if(skip_records == 0)
        {
          read_action_(file_reader, request_operation_membuf);
        }
        else
        {
          skip_single_buffer_operation_(file_reader);
        }
      }
      else if(op_index == OP_REQUEST_ACTION)
      {
        if(skip_records == 0)
        {
          read_request_action_(file_reader, request_operation_membuf);
        }
        else
        {
          skip_single_buffer_operation_(file_reader);
        }
      }
      else
      {
        Stream::Error ostr;
        ostr << FUN << ": unknown operation: " << op_index;
        throw Exception(ostr);
      }

      if(skip_records == 0)
      {
        ++processed_lines_count;
      }
      else
      {
        --skip_records;
      }
    }

    return !terminated;
  }

  void
  RequestOperationLoader::read_change_request_user_id_(
    ProfilingCommons::FileReader& file_reader,
    Generics::MemBuf& membuf)
    /*throw(Exception)*/
  {
    static const char* FUN = "RequestOperationLoader::read_change_request_user_id_()";

    uint32_t op_size;

    if(file_reader.read(&op_size, sizeof(op_size)) != sizeof(op_size))
    {
      Stream::Error ostr;
      ostr << FUN << ": unexpected end of file";
      throw Exception(ostr);
    }

    prepare_mem_buf_(membuf, op_size);

    if(file_reader.read(membuf.data(), op_size) != op_size)
    {
      Stream::Error ostr;
      ostr << FUN << ": unexpected end of file on 'change request' operation reading";
      throw Exception(ostr);
    }

    RequestOperationChangeUserReader op_reader(membuf.data(), op_size);

    uint32_t request_profile_size;

    if(file_reader.read(&request_profile_size, sizeof(request_profile_size)) !=
       sizeof(request_profile_size))
    {
      Stream::Error ostr;
      ostr << FUN << ": unexpected end of file on 'change request' "
        "request profile size reading";
      throw Exception(ostr);
    }

    Generics::SmartMemBuf_var request_profile_membuf(
      new Generics::SmartMemBuf(request_profile_size));

    if(file_reader.read(
         request_profile_membuf->membuf().data(), request_profile_size) !=
         request_profile_size)
    {
      Stream::Error ostr;
      ostr << FUN << ": unexpected end of file on 'change request' "
        "request profile reading";
      throw Exception(ostr);
    }

    try
    {
      request_operation_processor_->change_request_user_id(
        AdServer::Commons::UserId(op_reader.user_id()),
        AdServer::Commons::RequestId(op_reader.request_id()),
        Generics::transfer_membuf(request_profile_membuf));
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  RequestOperationLoader::skip_change_request_user_id_(
    ProfilingCommons::FileReader& file_reader)
    /*throw(Exception)*/
  {
    static const char* FUN = "RequestOperationLoader::skip_change_request_user_id_()";

    uint32_t op_size;

    if(file_reader.read(&op_size, sizeof(op_size)) != sizeof(op_size))
    {
      Stream::Error ostr;
      ostr << FUN << ": unexpected end of file";
      throw Exception(ostr);
    }

    if(file_reader.skip(op_size) != op_size)
    {
      Stream::Error ostr;
      ostr << FUN << ": unexpected end of file";
      throw Exception(ostr);
    }

    uint32_t request_profile_size;

    if(file_reader.read(&request_profile_size, sizeof(request_profile_size)) !=
       sizeof(request_profile_size))
    {
      Stream::Error ostr;
      ostr << FUN << ": unexpected end of file";
      throw Exception(ostr);
    }

    if(file_reader.skip(request_profile_size) != request_profile_size)
    {
      Stream::Error ostr;
      ostr << FUN << ": unexpected end of file";
      throw Exception(ostr);
    }
  }

  void
  RequestOperationLoader::read_impression_(
    ProfilingCommons::FileReader& file_reader,
    Generics::MemBuf& membuf)
    /*throw(Exception)*/
  {
    static const char* FUN = "RequestOperationLoader::read_impression_()";

    uint32_t op_size;

    if(file_reader.read(&op_size, sizeof(op_size)) != sizeof(op_size))
    {
      Stream::Error ostr;
      ostr << FUN << ": unexpected end of file";
      throw Exception(ostr);
    }

    prepare_mem_buf_(membuf, op_size);

    if(file_reader.read(membuf.data(), op_size) != op_size)
    {
      Stream::Error ostr;
      ostr << FUN << ": unexpected end of file";
      throw Exception(ostr);
    }

    try
    {
      RequestOperationImpressionProfileAdapter adapter;
      adapter(membuf);

      RequestOperationImpressionReader op_reader(membuf.data(), op_size);

      ImpressionInfo impression_info;
      impression_info.user_id = AdServer::Commons::UserId(op_reader.user_id());
      impression_info.request_id = AdServer::Commons::RequestId(op_reader.request_id());
      impression_info.time = Generics::Time(op_reader.time());
      impression_info.verify_impression = op_reader.verify_impression();
      if(op_reader.pub_revenue_type() != AdServer::CampaignSvcs::RT_NONE)
      {
        ImpressionInfo::PubRevenue pub_revenue;
        pub_revenue.revenue_type = static_cast<AdServer::CampaignSvcs::RevenueType>(
          op_reader.pub_revenue_type());
        pub_revenue.impression = RevenueDecimal(op_reader.pub_revenue());
        impression_info.pub_revenue = pub_revenue;
      }

      request_operation_processor_->process_impression(impression_info);    
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  RequestOperationLoader::read_action_(
    ProfilingCommons::FileReader& file_reader,
    Generics::MemBuf& membuf)
    /*throw(Exception)*/
  {
    static const char* FUN = "RequestOperationLoader::read_action_()";

    try
    {
      uint32_t op_size;

      if(file_reader.read(&op_size, sizeof(op_size)) != sizeof(op_size))
      {
        throw Exception("unexpected end of file");
      }

      prepare_mem_buf_(membuf, op_size);

      if(file_reader.read(membuf.data(), op_size) != op_size)
      {
        throw Exception("unexpected end of file");
      }

      RequestOperationActionReader op_reader(membuf.data(), op_size);

      Generics::Time time(op_reader.time());
      AdServer::Commons::RequestId request_id(op_reader.request_id());

      request_operation_processor_->process_action(
        op_reader.user_id()[0] ?
          AdServer::Commons::UserId(op_reader.user_id()) :
          AdServer::Commons::UserId(),
        static_cast<RequestContainerProcessor::ActionType>(
          op_reader.action_type()),
        time,
        request_id);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  RequestOperationLoader::read_request_action_(
    ProfilingCommons::FileReader& file_reader,
    Generics::MemBuf& membuf)
    /*throw(Exception)*/
  {
    static const char* FUN = "RequestOperationLoader::read_request_action_()";

    try
    {
      uint32_t op_size;

      if(file_reader.read(&op_size, sizeof(op_size)) != sizeof(op_size))
      {
        throw Exception("unexpected end of file");
      }

      prepare_mem_buf_(membuf, op_size);

      if(file_reader.read(membuf.data(), op_size) != op_size)
      {
        throw Exception("unexpected end of file");
      }

      RequestOperationActionReader op_reader(membuf.data(), op_size);

      Generics::Time time(op_reader.time());
      AdServer::Commons::RequestId request_id(op_reader.request_id());

      request_operation_processor_->process_impression_post_action(
        op_reader.user_id()[0] ?
          AdServer::Commons::UserId(op_reader.user_id()) :
          AdServer::Commons::UserId(),
        request_id,
        RequestPostActionInfo(time, String::SubString(op_reader.action_name())));
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  RequestOperationLoader::skip_single_buffer_operation_(
    ProfilingCommons::FileReader& file_reader)
    /*throw(Exception)*/
  {
    static const char* FUN = "RequestOperationLoader::skip_single_buffer_operation_()";

    uint32_t op_size;

    if(file_reader.read(&op_size, sizeof(op_size)) != sizeof(op_size))
    {
      Stream::Error ostr;
      ostr << FUN << ": unexpected end of file";
      throw Exception(ostr);
    }

    file_reader.skip(op_size);
  }

  void
  RequestOperationLoader::prepare_mem_buf_(
    Generics::MemBuf& membuf,
    unsigned long size)
    noexcept
  {
    if(size > membuf.capacity())
    {
      membuf.alloc(2*size);
    }
    else if(size > membuf.size())
    {
      membuf.resize(size);
    }
  }
}
}
