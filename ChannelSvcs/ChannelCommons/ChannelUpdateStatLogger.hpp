#ifndef AD_SERVER_CHANNEL_UPDATE_STAT_LOGGER_HPP
#define AD_SERVER_CHANNEL_UPDATE_STAT_LOGGER_HPP

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Sync/SyncPolicy.hpp>

#include <LogCommons/ColoUpdateStat.hpp>

namespace AdServer
{
  namespace ChannelSvcs
  {
    class ChannelUpdateStatLogger:
      public virtual ReferenceCounting::AtomicImpl
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      ChannelUpdateStatLogger(
        unsigned long size,
        unsigned long period, 
        const char* out_dir)
        /*throw(eh::Exception)*/;
    protected:
      virtual ~ChannelUpdateStatLogger() noexcept;
    public:

      void process_config_update(
        unsigned long colo_id,
        const std::string& version)
        /*throw(Exception)*/;

      void flush_if_required() /*throw(Exception)*/;

    protected:
      bool need_flush_i_() const /*throw(eh::Exception)*/;

    private:
      typedef Sync::Policy::PosixThread SyncPolicy;
      typedef
        AdServer::LogProcessing::ColoUpdateStatTraits::CollectorType
        ColoUpdateCollector;
      typedef
        AdServer::LogProcessing::GenericLogIoHelperImpl<
          AdServer::LogProcessing::ColoUpdateStatTraits>
        ColoUpdateStatIoHelper;

      SyncPolicy::Mutex colo_update_lock_;
      ColoUpdateCollector colo_update_collector_;
      Generics::Time flush_time_;
      unsigned long size_;
      unsigned long period_;
      std::string out_dir_;
    };

    typedef
      ReferenceCounting::SmartPtr<ChannelUpdateStatLogger>
      ChannelUpdateStatLogger_var;
    
  }
}

#endif//AD_SERVER_CHANNEL_UPDATE_STAT_LOGGER_HPP
