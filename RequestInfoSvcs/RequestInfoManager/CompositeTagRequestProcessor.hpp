#ifndef COMPOSITE_TAG_REQUEST_PROCESSOR_HPP
#define COMPOSITE_TAG_REQUEST_PROCESSOR_HPP

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include "TagRequestProcessor.hpp"
#include "CompositeMetricsProviderRIM.hpp"

namespace AdServer
{
namespace RequestInfoSvcs
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

    CompositeTagRequestProcessor(CompositeMetricsProviderRIM_var cmprim):cmprim_(cmprim){}
    void add_child_processor(TagRequestProcessor* child_processor)
      /*throw(Exception)*/;

    void process_tag_request(const TagRequestInfo& tag_request_info)
      /*throw(TagRequestProcessor::Exception)*/;

  protected:
    virtual ~CompositeTagRequestProcessor() noexcept {
        cmprim_->sub_container(typeid (child_processors_).name(),"child_processors_",child_processors_.size());
    }

  private:
    typedef std::list<TagRequestProcessor_var> TagRequestProcessorList;
    TagRequestProcessorList child_processors_;
    CompositeMetricsProviderRIM_var cmprim_;
  };

  typedef
    ReferenceCounting::SmartPtr<CompositeTagRequestProcessor>
    CompositeTagRequestProcessor_var;
}
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  /* CompositeTagRequestProcessor */
  inline
  void
  CompositeTagRequestProcessor::add_child_processor(
    TagRequestProcessor* child_processor) /*throw(Exception)*/
  {
    TagRequestProcessor_var add_processor(
      ReferenceCounting::add_ref(child_processor));
    child_processors_.push_back(add_processor);
    cmprim_->add_container(typeid (child_processors_).name(),"child_processors_", 1);

  }

  inline
  void
  CompositeTagRequestProcessor::process_tag_request(
    const TagRequestInfo& tag_request_info)
    /*throw(TagRequestProcessor::Exception)*/
  {
    for(TagRequestProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      (*it)->process_tag_request(tag_request_info);
    }
  }
}
}

#endif /*COMPOSITE_TAG_REQUEST_PROCESSOR_HPP*/
