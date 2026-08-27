#pragma once

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include "TagRequestProcessor.hpp"

namespace AdServer::RequestInfoSvcs
{
  /**
   * CompositeTagRequestProcessor
   * delegate request processing to child processors
   */
  class CompositeTagRequestProcessor:
    public virtual TagRequestProcessor,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, TagRequestProcessor::Exception);

    void add_child_processor(TagRequestProcessor* child_processor)
      /*throw(Exception)*/;

    void process_tag_request(const TagRequestInfo& tag_request_info)
      /*throw(TagRequestProcessor::Exception)*/;

    AdServer::Commons::Awaitable<void>
    co_process_tag_request(const TagRequestInfo& tag_request_info) override;

  protected:
    virtual ~CompositeTagRequestProcessor() noexcept {}

  private:
    typedef std::list<TagRequestProcessor_var> TagRequestProcessorList;
    TagRequestProcessorList child_processors_;
  };

  typedef
    ReferenceCounting::SmartPtr<CompositeTagRequestProcessor>
    CompositeTagRequestProcessor_var;
}

namespace AdServer::RequestInfoSvcs
{
  /* CompositeTagRequestProcessor */
  inline
  void
  CompositeTagRequestProcessor::add_child_processor(
    TagRequestProcessor* child_processor) /*throw(Exception)*/
  {
    TagRequestProcessor_var add_processor(ReferenceCounting::add_ref(child_processor));
    child_processors_.push_back(add_processor);
  }

  inline
  void
  CompositeTagRequestProcessor::process_tag_request(const TagRequestInfo& tag_request_info)
    /*throw(TagRequestProcessor::Exception)*/
  {
    for (TagRequestProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      (*it)->process_tag_request(tag_request_info);
    }
  }

  inline AdServer::Commons::Awaitable<void>
  CompositeTagRequestProcessor::co_process_tag_request(const TagRequestInfo& tag_request_info)
  {
    for (TagRequestProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      co_await (*it)->co_process_tag_request(tag_request_info);
    }
  }
}
