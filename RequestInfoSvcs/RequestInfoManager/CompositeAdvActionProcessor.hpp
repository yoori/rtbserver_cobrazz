#ifndef COMPOSITE_ADV_ACTION_PROCESSOR_HPP
#define COMPOSITE_ADV_ACTION_PROCESSOR_HPP

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include "RequestActionProcessor.hpp"
#include "CompositeMetricsProviderRIM.hpp"

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    /**
     * CompositeAdvActionProcessor
     * delegate request processing to child processors
     */
    class CompositeAdvActionProcessor:
      public virtual AdvActionProcessor,
      public virtual ReferenceCounting::AtomicImpl
    {
    public:
      DECLARE_EXCEPTION(Exception, AdvActionProcessor::Exception);

      CompositeAdvActionProcessor(CompositeMetricsProviderRIM_var cmprim): cmprim_(cmprim){}

      void add_child_processor(AdvActionProcessor* child_processor)
        /*throw(Exception)*/;

      /** AdvActionProcessor interface */
      virtual void process_adv_action(
        const AdvActionInfo& adv_action_info)
        /*throw(AdvActionProcessor::Exception)*/;

      virtual void process_custom_action(
        const AdvExActionInfo& adv_custom_action_info)
        /*throw(AdvActionProcessor::Exception)*/;

    protected:
      virtual ~CompositeAdvActionProcessor() noexcept {
          cmprim_->add_container(typeid(child_processors_).name(),"child_processors_", - child_processors_.size());
      }

    private:
      typedef std::list<AdvActionProcessor_var> AdvActionProcessorList;
      AdvActionProcessorList child_processors_;

      CompositeMetricsProviderRIM_var cmprim_;
    };

    typedef
      ReferenceCounting::SmartPtr<CompositeAdvActionProcessor>
      CompositeAdvActionProcessor_var;
  }
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  /* CompositeAdvActionProcessor */
  inline
  void
  CompositeAdvActionProcessor::add_child_processor(
    AdvActionProcessor* child_processor
    //,CompositeMetricsProviderRIM * cmprim
    ) /*throw(Exception)*/
  {
    AdvActionProcessor_var add_processor(
      ReferenceCounting::add_ref(child_processor));
    child_processors_.push_back(add_processor);
    cmprim_->add_container(typeid(child_processors_).name(),"child_processors_",1);
    
  }

  inline
  void CompositeAdvActionProcessor::process_adv_action(
    const AdvActionInfo& adv_action_info)
    /*throw(AdvActionProcessor::Exception)*/
  {
    for(AdvActionProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      (*it)->process_adv_action(adv_action_info);
    }
  }

  inline
  void CompositeAdvActionProcessor::process_custom_action(
    const AdvExActionInfo& adv_custom_action_info)
    /*throw(AdvActionProcessor::Exception)*/
  {
    for(AdvActionProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      (*it)->process_custom_action(adv_custom_action_info);
    }
  }
}
}

#endif /*COMPOSITE_ADV_ACTION_PROCESSOR_HPP*/
