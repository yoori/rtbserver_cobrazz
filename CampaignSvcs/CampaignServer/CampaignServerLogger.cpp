#include <LogCommons/GenericLogIoImpl.hpp>
#include "CampaignServerLogger.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  CampaignServerLogger::CampaignServerLogger(
    const LogFlushTraits& colo_update_flush_traits)
    /*throw(Exception)*/
    : colo_update_flush_traits_(colo_update_flush_traits)
  {
  }

  void
  CampaignServerLogger::flush_if_required() /*throw(Exception)*/
  {
    static const char* FUN = "CampaignServerLogger::flush_if_required()";

    try
    {
      ColoUpdateCollector tmp_collector;
      bool flush;

      {
        SyncPolicy::WriteGuard guard(colo_update_lock_);

        if ((flush = need_flush_i()))
        {
          colo_update_collector_.swap(tmp_collector);
          flush_time_ = Generics::Time::get_time_of_day();
          if (colo_update_flush_traits_.out_dir.empty())
          {
            return;
          }
        }
      }

      if (flush)
      {
        ColoUpdateStatIoHelper(tmp_collector).save(
          colo_update_flush_traits_.out_dir,
          colo_update_flush_traits_.archive_params);
      }
    }
    catch (const AdServer::LogProcessing::LogSaver::Exception &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": AdServer::LogProcessing::LogSaver::Exception caught: " <<
        ex.what();
      throw Exception(ostr);
    }
    catch (const eh::Exception &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
  }

  bool CampaignServerLogger::need_flush_i() const /*throw(eh::Exception)*/
  {
    return
      (colo_update_flush_traits_.size && colo_update_collector_.size() >=
        colo_update_flush_traits_.size) ||
      (colo_update_collector_.size() &&
        flush_time_ + colo_update_flush_traits_.period <
          Generics::Time::get_time_of_day());
  }

  void
  CampaignServerLogger::process_config_update(
    const ConfigUpdateInfo& colo_update_info)
    /*throw(Exception)*/
  {
     SyncPolicy::WriteGuard guard(colo_update_lock_);

     colo_update_collector_.add(
       ColoUpdateCollector::KeyT(
         colo_update_info.colo_id),
       ColoUpdateCollector::DataT(
         LogProcessing::OptionalSecondsTimestamp(),
         colo_update_info.time,
         colo_update_info.version));
  }
}
}
