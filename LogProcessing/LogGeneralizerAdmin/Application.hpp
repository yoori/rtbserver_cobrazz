#ifndef AD_SERVER_LOG_PROCESSING_LOG_GENERALIZER_ADMIN_APPLICATION_HPP
#define AD_SERVER_LOG_PROCESSING_LOG_GENERALIZER_ADMIN_APPLICATION_HPP


#include <eh/Exception.hpp>
#include <Generics/Singleton.hpp>
#include <LogProcessing/LogGeneralizer/LogGeneralizer.hpp>

class Application
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

  Application() /*throw(Exception, eh::Exception)*/;

  virtual ~Application() noexcept;

  int run(int &argc, char **argv)
    /*throw(InvalidArgument, Exception, eh::Exception, CORBA::Exception)*/;

  static Application* instance() /*throw(eh::Exception)*/;

private:
  typedef AdServer::LogProcessing::LogGeneralizer::NotSupported 
    LogGeneralizerNotSupported;

  typedef AdServer::LogProcessing::LogGeneralizer::ImplementationException 
    LogGeneralizerImplementationException;

  typedef AdServer::LogProcessing::LogGeneralizer::CollectionNotStarted 
    LogGeneralizerCollectionNotStarted;

  typedef AdServer::LogProcessing::StatInfo_var StatInfo_var;

  typedef AdServer::LogProcessing::StatInfo StatInfo;

  typedef AdServer::LogProcessing::CreativeStatInfo CreativeStatInfo;
  typedef AdServer::LogProcessing::CreativeStatSeq CreativeStatSeq;
  typedef AdServer::LogProcessing::PublisherAmountInfo PublisherAmountInfo;
  typedef AdServer::LogProcessing::PublisherAmountSeq PublisherAmountSeq;
  typedef AdServer::LogProcessing::TagAmountInfo TagAmountInfo;
  typedef AdServer::LogProcessing::TagAmountSeq TagAmountSeq;

  int help()
    /*throw(InvalidArgument, Exception, eh::Exception, CORBA::Exception)*/;

  void print_stat_info(const StatInfo &info)
    /*throw(eh::Exception, CORBA::Exception)*/;

  void
  output_creative_stats(
    std::ostream &os,
    const CreativeStatSeq &creative_stats
  ) /*throw(eh::Exception, CORBA::Exception)*/;

  void
  output_publisher_amounts(
    std::ostream &os,
    const PublisherAmountSeq &publisher_amounts
  ) /*throw(eh::Exception, CORBA::Exception)*/;

  void
  output_tag_amounts(
    std::ostream &os,
    const TagAmountSeq &tag_amounts
  ) /*throw(eh::Exception, CORBA::Exception)*/;

  CORBA::ORB_var orb_;
  AdServer::LogProcessing::LogGeneralizer_var log_generalizer_;
};

typedef Generics::Singleton<Application> AppSingleton;

inline
Application*
Application::instance() /*throw(eh::Exception)*/
{
  return &AppSingleton::instance();
}

#endif // AD_SERVER_LOG_PROCESSING_LOG_GENERALIZER_ADMIN_APPLICATION_HPP

