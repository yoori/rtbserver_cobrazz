#ifndef _COMPOSITE_REQUEST_ACTION_PROCESSOR_HPP_
#define _COMPOSITE_REQUEST_ACTION_PROCESSOR_HPP_

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include "RequestActionProcessor.hpp"

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    /**
     * CompositeRequestActionProcessor
     * delegate request processing to child processors
     */
    class CompositeRequestActionProcessor:
      public virtual RequestActionProcessor,
      public virtual ReferenceCounting::AtomicImpl
    {
    public:
      DECLARE_EXCEPTION(Exception, RequestActionProcessor::Exception);

    public:
      CompositeRequestActionProcessor(
        RequestActionProcessor* child_processor = nullptr)
        noexcept;

      void
      add_child_processor(RequestActionProcessor* child_processor)
        noexcept;

      virtual void
      process_request(
        const RequestInfo&,
        const ProcessingState& processing_state)
        /*throw(RequestActionProcessor::Exception)*/;

      virtual void
      process_impression(
        const RequestInfo&,
        const ImpressionInfo& imp_info,
        const ProcessingState& processing_state)
        /*throw(RequestActionProcessor::Exception)*/;

      virtual void
      process_click(
        const RequestInfo&,
        const ProcessingState& processing_state)
        /*throw(RequestActionProcessor::Exception)*/;

      virtual void
      process_action(const RequestInfo&)
        /*throw(RequestActionProcessor::Exception)*/;

      virtual void
      process_custom_action(
        const RequestInfo&, const AdvCustomActionInfo&)
        /*throw(RequestActionProcessor::Exception)*/;

      virtual void
      process_request_post_action(
        const RequestInfo&, const RequestPostActionInfo&)
        /*throw(RequestActionProcessor::Exception)*/;

    protected:
      virtual ~CompositeRequestActionProcessor() noexcept {}

    private:
      typedef std::list<RequestActionProcessor_var> RequestActionProcessorList;
      RequestActionProcessorList child_processors_;
    };

    typedef ReferenceCounting::SmartPtr<CompositeRequestActionProcessor>
      CompositeRequestActionProcessor_var;

    class FilterRequestActionProcessor:
      public virtual RequestActionProcessor,
      public virtual ReferenceCounting::AtomicImpl
    {
    public:
      FilterRequestActionProcessor(
        RequestActionProcessor* delegate_processor,
        RequestInfo::RequestState min_request_state,
        RequestInfo::RequestState max_request_state)
        noexcept;

      virtual void
      process_request(
        const RequestInfo&,
        const ProcessingState& processing_state)
        /*throw(RequestActionProcessor::Exception)*/;

      virtual void
      process_impression(
        const RequestInfo&,
        const ImpressionInfo& imp_info,
        const ProcessingState& processing_state)
        /*throw(RequestActionProcessor::Exception)*/;

      virtual void
      process_click(
        const RequestInfo&,
        const ProcessingState& processing_state)
        /*throw(RequestActionProcessor::Exception)*/;

      virtual void
      process_action(const RequestInfo&)
        /*throw(RequestActionProcessor::Exception)*/;

      virtual void
      process_custom_action(
        const RequestInfo&, const AdvCustomActionInfo&)
        /*throw(RequestActionProcessor::Exception)*/;

      virtual void
      process_request_post_action(
        const RequestInfo&, const RequestPostActionInfo&)
        /*throw(RequestActionProcessor::Exception)*/;

    protected:
      RequestActionProcessor_var delegate_processor_;
      RequestInfo::RequestState min_request_state_;
      RequestInfo::RequestState max_request_state_;
    };

    typedef ReferenceCounting::SmartPtr<FilterRequestActionProcessor>
      FilterRequestActionProcessor_var;
  }
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  // CompositeRequestActionProcessor
  inline
  CompositeRequestActionProcessor::CompositeRequestActionProcessor(
    RequestActionProcessor* child_processor)
    noexcept
  {
    if(child_processor)
    {
      add_child_processor(child_processor);
    }
  }

  inline
  void
  CompositeRequestActionProcessor::add_child_processor(
    RequestActionProcessor* child_processor)
    noexcept
  {
    RequestActionProcessor_var add_processor(
      ReferenceCounting::add_ref(child_processor));
    child_processors_.push_back(add_processor);
    //cmprim->set_child_processors(child_processors_.size());

  }

  inline void
  CompositeRequestActionProcessor::process_request(
    const RequestInfo& request_info,
    const ProcessingState& processing_state)
    /*throw(RequestActionProcessor::Exception)*/
  {
    for(RequestActionProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      (*it)->process_request(request_info, processing_state);
    }
  }

  inline void
  CompositeRequestActionProcessor::process_impression(
    const RequestInfo& request_info,
    const ImpressionInfo& imp_info,
    const ProcessingState& processing_state)
    /*throw(RequestActionProcessor::Exception)*/
  {
    for(RequestActionProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      (*it)->process_impression(request_info, imp_info, processing_state);
    }
  }

  inline void
  CompositeRequestActionProcessor::process_click(
    const RequestInfo& request_info,
    const ProcessingState& processing_state)
    /*throw(RequestActionProcessor::Exception)*/
  {
    for(RequestActionProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      (*it)->process_click(request_info, processing_state);
    }
  }

  inline void
  CompositeRequestActionProcessor::process_action(
    const RequestInfo& request_info)
    /*throw(RequestActionProcessor::Exception)*/
  {
    for(RequestActionProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      (*it)->process_action(request_info);
    }
  }

  inline
  void CompositeRequestActionProcessor::process_custom_action(
    const RequestInfo& request_info,
    const AdvCustomActionInfo& adv_custom_action_info)
    /*throw(RequestActionProcessor::Exception)*/
  {
    for(RequestActionProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      (*it)->process_custom_action(request_info, adv_custom_action_info);
    }
  }

  inline void
  CompositeRequestActionProcessor::process_request_post_action(
    const RequestInfo& request_info,
    const RequestPostActionInfo& request_post_action_info)
    /*throw(RequestActionProcessor::Exception)*/
  {
    for(RequestActionProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      (*it)->process_request_post_action(request_info, request_post_action_info);
    }
  }

  // FilterRequestActionProcessor
  inline
  FilterRequestActionProcessor::FilterRequestActionProcessor(
    RequestActionProcessor* delegate_processor,
    RequestInfo::RequestState min_request_state,
    RequestInfo::RequestState max_request_state)
    noexcept
      : delegate_processor_(ReferenceCounting::add_ref(delegate_processor)),
        min_request_state_(min_request_state),
        max_request_state_(max_request_state)
  {}

  inline void
  FilterRequestActionProcessor::process_request(
    const RequestInfo& request_info,
    const ProcessingState& processing_state)
    /*throw(RequestActionProcessor::Exception)*/
  {
    if(processing_state.state >= min_request_state_ &&
      processing_state.state <= max_request_state_)
    {
      delegate_processor_->process_request(request_info, processing_state);
    }
  }

  inline void
  FilterRequestActionProcessor::process_impression(
    const RequestInfo& request_info,
    const ImpressionInfo& imp_info,
    const ProcessingState& processing_state)
    /*throw(RequestActionProcessor::Exception)*/
  {
    if(processing_state.state >= min_request_state_ &&
      processing_state.state <= max_request_state_)
    {
      delegate_processor_->process_impression(request_info, imp_info, processing_state);
    }
  }

  inline void
  FilterRequestActionProcessor::process_click(
    const RequestInfo& request_info,
    const ProcessingState& processing_state)
    /*throw(RequestActionProcessor::Exception)*/
  {
    if(processing_state.state >= min_request_state_ &&
      processing_state.state <= max_request_state_)
    {
      delegate_processor_->process_click(request_info, processing_state);
    }
  }

  inline void
  FilterRequestActionProcessor::process_action(
    const RequestInfo& request_info)
    /*throw(RequestActionProcessor::Exception)*/
  {
    delegate_processor_->process_action(request_info);
  }

  inline void
  FilterRequestActionProcessor::process_custom_action(
    const RequestInfo& request_info,
    const AdvCustomActionInfo& adv_custom_action_info)
    /*throw(RequestActionProcessor::Exception)*/
  {
    delegate_processor_->process_custom_action(request_info, adv_custom_action_info);
  }

  inline void
  FilterRequestActionProcessor::process_request_post_action(
    const RequestInfo& request_info,
    const RequestPostActionInfo& request_post_action_info)
    /*throw(RequestActionProcessor::Exception)*/
  {
    delegate_processor_->process_request_post_action(request_info, request_post_action_info);
  }
}
}

#endif /*_COMPOSITE_REQUEST_ACTION_PROCESSOR_HPP_*/
