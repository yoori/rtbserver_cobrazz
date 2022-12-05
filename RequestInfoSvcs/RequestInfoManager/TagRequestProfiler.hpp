#ifndef TAGREQUESTPROFILER_HPP
#define TAGREQUESTPROFILER_HPP

#include <list>
#include <ReferenceCounting/DefaultImpl.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Sync/Condition.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <HTTP/UrlAddress.hpp>

#include "TagRequestProcessor.hpp"

namespace AdServer
{
namespace RequestInfoSvcs
{
  struct RequestPool;

  /**
   * TagRequestProfiler
   */
  class TagRequestProfiler:
    public TagRequestProcessor,
    public Generics::CompositeActiveObject,
    public ReferenceCounting::AtomicImpl
  {
  public:
    typedef std::list<std::string> AddressList;

    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    TagRequestProfiler(
      Logging::Logger* logger,
      Generics::ActiveObjectCallback* callback,
      unsigned long thread_count,
      const Generics::Time& window_size,
      unsigned long max_request_pool_size,
      const char* uid_private_key,
      const AddressList& addresses,
      const Generics::Time& repeat_trigger_timeout)
      /*throw(Exception)*/;

    virtual void
    process_tag_request(const TagRequestInfo&)
      /*throw(TagRequestProcessor::Exception)*/;

  protected:
    typedef ReferenceCounting::SmartPtr<RequestPool>
      RequestPool_var;

  protected:
    virtual ~TagRequestProfiler() noexcept;

  private:
    Logging::Logger_var logger_;
    const unsigned long max_request_pool_size_;
    const Generics::Time repeat_trigger_timeout_;
    RequestPool_var request_pool_;
  };

  typedef ReferenceCounting::SmartPtr<TagRequestProfiler>
    TagRequestProfiler_var;
}
}

#endif /*TAGREQUESTPROFILER_HPP*/
