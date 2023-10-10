#ifndef _COMPOSITE_REQUEST_CONTAINER_PROCESSOR_HPP_
#define _COMPOSITE_REQUEST_CONTAINER_PROCESSOR_HPP_

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include "RequestActionProcessor.hpp"
#include "CompositeMetricsProviderRIM.hpp"

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    /**
     * CompositeRequestContainerProcessor
     * delegate request processing to child RequestContainerProcessor's
     */
    class CompositeRequestContainerProcessor:
      public virtual RequestContainerProcessor,
      public virtual ReferenceCounting::AtomicImpl
    {
    public:
      DECLARE_EXCEPTION(Exception, RequestContainerProcessor::Exception);

      CompositeRequestContainerProcessor(CompositeMetricsProviderRIM_var cmprim):cmprim_(cmprim) {}

      void
      add_child_processor(RequestContainerProcessor* child_processor) /*throw(Exception)*/;

      virtual void
      process_request(const RequestInfo& request_info)
        /*throw(RequestContainerProcessor::Exception)*/;

      virtual void
      process_impression(const ImpressionInfo& impression_info)
        /*throw(RequestContainerProcessor::Exception)*/;

      virtual void
      process_action(
        ActionType action_type,
        const Generics::Time& time,
        const AdServer::Commons::RequestId& request_id)
        /*throw(RequestContainerProcessor::Exception)*/;

      virtual void
      process_custom_action(
        const AdServer::Commons::RequestId& request_id,
        const AdvCustomActionInfo& adv_custom_action_info)
        /*throw(RequestContainerProcessor::Exception)*/;

      virtual void
      process_impression_post_action(
        const AdServer::Commons::RequestId& request_id,
        const RequestPostActionInfo& request_post_action_info)
        /*throw(RequestContainerProcessor::Exception)*/;

    protected:
      virtual ~CompositeRequestContainerProcessor() noexcept
      {
          cmprim_->sub_container(typeid (child_processors_).name(),"child_processors_",child_processors_.size());
      }

    private:
      typedef std::list<RequestContainerProcessor_var> RequestContainerProcessorList;
      RequestContainerProcessorList child_processors_;
      CompositeMetricsProviderRIM_var cmprim_;
    };

    typedef ReferenceCounting::SmartPtr<CompositeRequestContainerProcessor>
      CompositeRequestContainerProcessor_var;
  }
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  /* CompositeRequestContainerProcessor */
  inline
  void
  CompositeRequestContainerProcessor::add_child_processor(
    RequestContainerProcessor* child_processor) /*throw(Exception)*/
  {
    RequestContainerProcessor_var add_processor(
    ReferenceCounting::add_ref(child_processor));
    child_processors_.push_back(add_processor);
    cmprim_->add_container(typeid (child_processors_).name(),"child_processors_",1);

  }

  inline
  void CompositeRequestContainerProcessor::process_request(
    const RequestInfo& request_info)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    for(RequestContainerProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      (*it)->process_request(request_info);
    }
  }

  inline
  void CompositeRequestContainerProcessor::process_impression(
    const ImpressionInfo& impression_info)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    for(RequestContainerProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      (*it)->process_impression(impression_info);
    }
  }

  inline
  void CompositeRequestContainerProcessor::process_action(
    ActionType action_type,
    const Generics::Time& time,
    const AdServer::Commons::RequestId& request_id)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    for(RequestContainerProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      (*it)->process_action(action_type, time, request_id);
    }
  }

  inline
  void CompositeRequestContainerProcessor::process_custom_action(
    const AdServer::Commons::RequestId& request_id,
    const AdvCustomActionInfo& adv_custom_action_info)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    for(RequestContainerProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      (*it)->process_custom_action(request_id, adv_custom_action_info);
    }
  }

  inline void
  CompositeRequestContainerProcessor::process_impression_post_action(
    const AdServer::Commons::RequestId& request_id,
    const RequestPostActionInfo& request_post_action_info)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    for(RequestContainerProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      (*it)->process_impression_post_action(request_id, request_post_action_info);
    }
  }
}
}

#endif
