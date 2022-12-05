#include <LogCommons/ChannelCountStat.hpp>
#include <LogCommons/LogHolder.hpp>

#include <Commons/Algs.hpp>

#include "UserInfoManagerLogger.hpp"

namespace AdServer
{
  namespace UserInfoSvcs
  {
    /** UserInfoManagerLogger::HistoryOptimizationStatLogger */
    class UserInfoManagerLogger::HistoryOptimizationStatLogger:
      public AdServer::LogProcessing::LogHolderPool<
        AdServer::LogProcessing::ChannelCountStatTraits>
    {
    public:
      DECLARE_EXCEPTION(Exception, UserInfoManagerLogger::Exception);

      HistoryOptimizationStatLogger(
        const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(Exception)*/
        : AdServer::LogProcessing::LogHolderPool<
            AdServer::LogProcessing::ChannelCountStatTraits>(
              flush_traits)
      {}

      void
      process_history_optimization(
        const HistoryOptimizationInfo& ho_info)
        /*throw(Exception)*/;

    protected:
      virtual
      ~HistoryOptimizationStatLogger() noexcept = default;

    private:
      unsigned long power2align_(unsigned long value);
    };

    void UserInfoManagerLogger::HistoryOptimizationStatLogger::
    process_history_optimization(
      const HistoryOptimizationInfo& ho_info)
      /*throw(Exception)*/
    {
      static const char* FUN =
        "UserInfoManagerLogger::HistoryOptimizationStatLogger::process_history_optimization()";

      try
      {
        LogProcessing::ChannelCountStatCollector::DataT ch_count_data;
        LogProcessing::ChannelCountStatCollector::DataT::KeyT
          discover_inner_key('D', power2align_(ho_info.discover_channel_count));
        LogProcessing::ChannelCountStatCollector::DataT::KeyT
          adv_inner_key('A', power2align_(ho_info.adv_channel_count));

        LogProcessing::ChannelCountStatCollector::DataT::DataT inner_data(1);

        ch_count_data.add(discover_inner_key, inner_data);
        ch_count_data.add(adv_inner_key, inner_data);

        LogProcessing::ChannelCountStatCollector::KeyT
          stat_key(ho_info.isp_date, ho_info.colo_id);

        add_record(stat_key, ch_count_data);
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught. : "
             << ex.what();
        throw Exception(ostr);
      }
    }

    unsigned long
    UserInfoManagerLogger::HistoryOptimizationStatLogger::power2align_(
      unsigned long value)
    {
      unsigned long res = value;

      if ((unsigned long)(value & (value - 1)) != 0)
      {
        for (res = 1; value > 1; value >>= 1, res <<= 1) {}
      }

      return res;
    }

    /** UserInfoManagerLogger */
    UserInfoManagerLogger::UserInfoManagerLogger(
      const AdServer::LogProcessing::LogFlushTraits& flush_traits)
      /*throw(Exception)*/
    {
      static const char* FUN = "UserInfoManagerLogger::UserInfoManagerLogger()";

      try
      {
        if(flush_traits.period != Generics::Time::ZERO)
        {
          history_optimization_stat_logger_ =
            new HistoryOptimizationStatLogger(flush_traits);
          add_child_log_holder(history_optimization_stat_logger_);
        }
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't init logger. Caught eh::Exception: " << ex.what();
        throw Exception(ostr);
      }
    }

    UserInfoManagerLogger::~UserInfoManagerLogger() noexcept
    {}

    void
    UserInfoManagerLogger::process_history_optimization(
      const HistoryOptimizationInfo& ho_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "UserInfoManagerLogger::process_history_optimization";
      try
      {
        history_optimization_stat_logger_->process_history_optimization(
          ho_info);
      }
      catch(const eh::Exception& e)
      {
        Stream::Error err;
        err << FUN << ": Catch eh::Exception: " << e.what();
        throw Exception(err);
      }
    }
  }
}

