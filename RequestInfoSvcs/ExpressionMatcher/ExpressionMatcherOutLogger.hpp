#ifndef EXPRESSIONMATCHER_EXPRESSIONMATCHEROUTLOGGER_HPP
#define EXPRESSIONMATCHER_EXPRESSIONMATCHEROUTLOGGER_HPP

#include <set>
#include <string>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Logger/Logger.hpp>

#include <Commons/UserInfoManip.hpp>

#include <LogCommons/ColoUserStat.hpp>
#include <LogCommons/LogHolder.hpp>

#include "InventoryActionProcessor.hpp"
#include "TriggerActionProcessor.hpp"
#include "ColoReachProcessor.hpp"

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    DECLARE_EXCEPTION(LoggerException, eh::DescriptiveException);

    typedef std::set<unsigned long> ChannelIdSet;

    class ExpressionMatcherOutLogger:
      public virtual AdServer::LogProcessing::CompositeLogHolder,
      public virtual MatchRequestProcessor,
      public virtual InventoryActionProcessor,
      public virtual TriggerActionProcessor,
      public virtual ColoReachProcessor
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      ExpressionMatcherOutLogger(
        Logging::Logger* logger,
        unsigned long colo_id,
        const RevenueDecimal& simplify_factor,
        const AdServer::LogProcessing::LogFlushTraits& channel_inventory_flush,
        const AdServer::LogProcessing::LogFlushTraits& channel_imp_inventory_flush,
        const AdServer::LogProcessing::LogFlushTraits& channel_price_range_flush,
        const AdServer::LogProcessing::LogFlushTraits& channel_activity_flush,
        const AdServer::LogProcessing::LogFlushTraits& channel_performance_flush,
        const AdServer::LogProcessing::LogFlushTraits& channel_trigger_imp_flush,
        const AdServer::LogProcessing::LogFlushTraits& global_colo_user_stat_flush,
        const AdServer::LogProcessing::LogFlushTraits& colo_user_stat_flush)
        /*throw(Exception)*/;

      void
      process_match_request(
        const MatchRequestProcessor::MatchInfo& request_info)
        /*throw(MatchRequestProcessor::Exception)*/;

      virtual void
      process_request(const InventoryInfo&)
        /*throw(InventoryActionProcessor::Exception)*/;

      virtual void
      process_user(const InventoryUserInfo&)
        /*throw(InventoryActionProcessor::Exception)*/;

      virtual void
      process_channel_activity(
        const Generics::Time& date,
        unsigned long colo_id,
        const ChannelIdSet& channels)
        /*throw(InventoryActionProcessor::Exception)*/;

      // TriggerActionProcessor
      virtual void
      process_triggers_impression(
        const TriggersMatchInfo& match_info)
        /*throw(TriggerActionProcessor::Exception)*/;

      virtual void
      process_triggers_click(
        const TriggersMatchInfo& match_info)
        /*throw(TriggerActionProcessor::Exception)*/;

      virtual void
      process_gmt_colo_reach(
        const ColoReachInfo& request_info)
        /*throw(ColoReachProcessor::Exception)*/;

      virtual void
      process_isp_colo_reach(
        const ColoReachInfo& request_info)
        /*throw(ColoReachProcessor::Exception)*/;

    protected:
      virtual
      ~ExpressionMatcherOutLogger() noexcept;

    private:
      class ChannelInventoryLogger;
      typedef ReferenceCounting::SmartPtr<ChannelInventoryLogger>
        ChannelInventoryLogger_var;

      class IProcessRequest : public virtual ReferenceCounting::Interface
      {
      public:
        virtual void
        process_request(
          const InventoryActionProcessor::InventoryInfo& inv_info)
          /*throw(eh::Exception)*/ = 0;

      protected:
        virtual
        ~IProcessRequest() noexcept = default;
      };

      template<bool SamplingFlag>
      class ChannelImpInventoryLogger;
      typedef ReferenceCounting::AssertPtr<IProcessRequest>::Ptr
        IProcessRequest_var;

      class ChannelActivityLogger;
      typedef ReferenceCounting::AssertPtr<ChannelActivityLogger>::Ptr
        ChannelActivityLogger_var;

      class ChannelPriceRangeLogger;
      typedef ReferenceCounting::AssertPtr<ChannelPriceRangeLogger>::Ptr
        ChannelPriceRangeLogger_var;

      class ChannelPerformanceLogger;
      typedef ReferenceCounting::AssertPtr<ChannelPerformanceLogger>::Ptr
        ChannelPerformanceLogger_var;

      class ChannelTriggerImpLogger;
      typedef ReferenceCounting::AssertPtr<ChannelTriggerImpLogger>::Ptr
        ChannelTriggerImpLogger_var;

      template <typename LogProcessingTraits>
      class ColoUserStatLoggerT:
        public virtual ColoReachProcessor,
        public virtual AdServer::LogProcessing::LogHolderPool<
          LogProcessingTraits>
      {
      protected:
        typedef AdServer::LogProcessing::LogHolderPool<
          LogProcessingTraits> Base;
        typedef typename Base::CollectorT CollectorT;
        typedef typename CollectorT::DataT DataT;
        typedef typename DataT::KeyT KeyT;

      public:
        ColoUserStatLoggerT(
          const AdServer::LogProcessing::LogFlushTraits& flush_traits)
          /*throw(LoggerException)*/
          : AdServer::LogProcessing::LogHolderPool<
              LogProcessingTraits>(flush_traits)
        {}

      protected:
        virtual
        ~ColoUserStatLoggerT() noexcept = default;

        void
        process_colo_reach(
          const ColoReachInfo& reach_info)
          /*throw(ColoReachProcessor::Exception)*/
        {
          if(reach_info.household)
          {
            for(IdAppearanceList::const_iterator it = reach_info.colocations.begin();
                it != reach_info.colocations.end(); ++it)
            {
              typename CollectorT::DataT::KeyT inner_key(
                reach_info.create_time, it->last_appearance_date);
              typename CollectorT::DataT data;
              data.add(inner_key,
                typename CollectorT::DataT::DataT(0, 0, 0, it->counter));
              this->add_record(typename CollectorT::KeyT(it->date, it->id), data);
            }
          }
          else
          {
            for(IdAppearanceList::const_iterator it = reach_info.colocations.begin();
                it != reach_info.colocations.end(); ++it)
            {
              typename CollectorT::DataT::KeyT inner_key(
                reach_info.create_time, it->last_appearance_date);
              typename CollectorT::DataT data;
              data.add(inner_key,
                typename CollectorT::DataT::DataT(it->counter, 0, 0, 0));
              this->add_record(typename CollectorT::KeyT(it->date, it->id), data);
            }

            for(IdAppearanceList::const_iterator it = reach_info.ad_colocations.begin();
                it != reach_info.ad_colocations.end(); ++it)
            {
              typename CollectorT::DataT::KeyT inner_key(
                reach_info.create_time, it->last_appearance_date);
              typename CollectorT::DataT data;
              data.add(inner_key,
                typename CollectorT::DataT::DataT(0, it->counter, 0, 0));
              this->add_record(typename CollectorT::KeyT(it->date, it->id), data);
            }

            for(IdAppearanceList::const_iterator it =
                  reach_info.merge_colocations.begin();
                it != reach_info.merge_colocations.end(); ++it)
            {
              typename CollectorT::DataT::KeyT inner_key(
                reach_info.create_time, it->last_appearance_date);
              typename CollectorT::DataT data;
              data.add(inner_key,
                typename CollectorT::DataT::DataT(0, 0, it->counter, 0));
              this->add_record(typename CollectorT::KeyT(it->date, it->id), data);
            }
          }
        }
      public:

        // ColoReachProcessor interface
        virtual void process_gmt_colo_reach(
          const ColoReachInfo& request_info) /*throw(Exception)*/
        {
          process_colo_reach(request_info);
        }

        virtual void process_isp_colo_reach(
          const ColoReachInfo& request_info) /*throw(Exception)*/
        {
          process_colo_reach(request_info);
        }
      };

      typedef ColoUserStatLoggerT<AdServer::LogProcessing::GlobalColoUserStatTraits>
        GlobalColoUserStatLogger;
      typedef ReferenceCounting::SmartPtr<GlobalColoUserStatLogger>
        GlobalColoUserStatLogger_var;

      typedef ColoUserStatLoggerT<AdServer::LogProcessing::ColoUserStatTraits>
        ColoUserStatLogger;
      typedef ReferenceCounting::SmartPtr<ColoUserStatLogger>
        ColoUserStatLogger_var;

    private:
      Logging::Logger_var logger_;
      RevenueDecimal simplify_factor_;
      ChannelInventoryLogger_var channel_inventory_logger_;
      IProcessRequest_var channel_imp_inventory_logger_;
      ChannelActivityLogger_var channel_activity_logger_;
      ChannelPriceRangeLogger_var channel_price_range_logger_;
      ChannelPerformanceLogger_var channel_performance_logger_;
      ChannelTriggerImpLogger_var channel_trigger_imp_logger_;
      GlobalColoUserStatLogger_var global_colo_user_stat_logger_;
      ColoUserStatLogger_var colo_user_stat_logger_;
    };

    typedef ReferenceCounting::SmartPtr<ExpressionMatcherOutLogger>
      ExpressionMatcherOutLogger_var;
  }
}

#endif /*EXPRESSIONMATCHER_EXPRESSIONMATCHEROUTLOGGER_HPP*/
