#pragma once

#include <Generics/ActiveObject.hpp>
#include <ProfilingCommons/FileReader.hpp>
#include <Commons/Coro/Awaitable.hpp>

#include "RequestOperationProcessor.hpp"

namespace AdServer
{
namespace RequestInfoSvcs
{
  class RequestOperationLoader:
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    enum Operation
    {
      OP_CHANGE = 0,
      OP_IMPRESSION,
      OP_ACTION,
      OP_REQUEST_ACTION
    };

    RequestOperationLoader(
      RequestOperationProcessor* request_operation_processor)
      noexcept;

    bool
    process_file(
      unsigned long& processed_lines_count,
      const char* file,
      Generics::ActiveObject* interrupter)
      /*throw(Exception)*/;

    Commons::Awaitable<bool>
    co_process_file(
      unsigned long& processed_lines_count,
      const char* file,
      Generics::ActiveObject* interrupter);

  protected:
    virtual
    ~RequestOperationLoader() noexcept
    {}

  private:
    void
    read_change_request_user_id_(
      ProfilingCommons::FileReader& file_reader,
      Generics::MemBuf& membuf)
      /*throw(Exception)*/;

    Commons::Awaitable<void>
    co_read_change_request_user_id_(
      ProfilingCommons::FileReader& file_reader,
      Generics::MemBuf& membuf);

    void
    skip_change_request_user_id_(
      ProfilingCommons::FileReader& file_reader)
      /*throw(Exception)*/;

    void
    read_impression_(
      ProfilingCommons::FileReader& file_reader,
      Generics::MemBuf& membuf)
      /*throw(Exception)*/;

    Commons::Awaitable<void>
    co_read_impression_(
      ProfilingCommons::FileReader& file_reader,
      Generics::MemBuf& membuf);

    void
    read_action_(
      ProfilingCommons::FileReader& file_reader,
      Generics::MemBuf& membuf)
      /*throw(Exception)*/;

    Commons::Awaitable<void>
    co_read_action_(
      ProfilingCommons::FileReader& file_reader,
      Generics::MemBuf& membuf);

    void
    read_request_action_(
      ProfilingCommons::FileReader& file_reader,
      Generics::MemBuf& membuf)
      /*throw(Exception)*/;

    Commons::Awaitable<void>
    co_read_request_action_(
      ProfilingCommons::FileReader& file_reader,
      Generics::MemBuf& membuf);

    void
    skip_single_buffer_operation_(
      ProfilingCommons::FileReader& file_reader)
      /*throw(Exception)*/;

    void
    prepare_mem_buf_(
      Generics::MemBuf& membuf,
      unsigned long size)
      noexcept;

  private:
    RequestOperationProcessor_var request_operation_processor_;
  };

  typedef ReferenceCounting::SmartPtr<RequestOperationLoader>
    RequestOperationLoader_var;
}
}
