#include "ChannelUpdateStatLogger.hpp"
#include <LogCommons/GenericLogIoImpl.hpp>

namespace AdServer
{
namespace ChannelSvcs
{
  ChannelUpdateStatLogger::ChannelUpdateStatLogger(
    unsigned long size,
    unsigned long period,
    const char* out_dir)
    /*throw(eh::Exception)*/ :
      size_(size),
      period_(period),
      out_dir_(out_dir)

  {
  }

  ChannelUpdateStatLogger::~ChannelUpdateStatLogger() noexcept
  {
  }

  void ChannelUpdateStatLogger::flush_if_required() /*throw(Exception)*/
  {
    const char FUN[] = "ChannelUpdateStatLogger::flush_if_required()";

    try
    {
      if (out_dir_.empty())
      {
        return;
      }
      ColoUpdateCollector tmp_collector;

      {
        SyncPolicy::WriteGuard guard(colo_update_lock_);

        if (need_flush_i_())
        {
          colo_update_collector_.swap(tmp_collector);
          flush_time_ = Generics::Time::get_time_of_day();
        }
        else
        {
          return;
        }
      }

      ColoUpdateStatIoHelper(tmp_collector).save(out_dir_);
    }
    catch (const AdServer::LogProcessing::LogSaver::Exception &ex)
    {
      Stream::Error oss;
      oss << FUN << ": AdServer::LogProcessing::LogSaver::Exception caught: "
          << ex.what();
      throw Exception(oss);
    }
    catch (const eh::Exception &ex)
    {
      Stream::Error oss;
      oss << FUN << ": eh::Exception caught: " << ex.what();
      throw Exception(oss);
    }
  }

  bool ChannelUpdateStatLogger::need_flush_i_() const /*throw(eh::Exception)*/
  {
    return
      (size_ && colo_update_collector_.size() >= size_) ||
      (colo_update_collector_.size() &&
        flush_time_ + period_ < Generics::Time::get_time_of_day());
  }

  void ChannelUpdateStatLogger::process_config_update(
    unsigned long colo_id,
    const std::string& version)
    /*throw(Exception)*/
  {
    if (out_dir_.empty())
    {
      return;
    }
    else
    {
      const char* FN = "ChannelUpdateStatLogger::process_config_update";
      try
      {
        SyncPolicy::WriteGuard guard(colo_update_lock_);

        colo_update_collector_.add(
          ColoUpdateCollector::KeyT(colo_id),
          ColoUpdateCollector::DataT(
            Generics::Time::get_time_of_day(),
            LogProcessing::OptionalSecondsTimestamp(),
            version));
      }
      catch(const eh::Exception& e)
      {
        Stream::Error err;
        err << FN << ": caught eh::Exception. : " << e.what();
        throw Exception(err);
      }
    }
  }
}
}
