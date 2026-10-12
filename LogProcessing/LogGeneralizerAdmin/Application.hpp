#pragma once


#include <eh/Exception.hpp>
#include <Generics/Singleton.hpp>
#include <LogProcessing/LogGeneralizer/LogGeneralizerGrpc.hpp>

class Application
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

  Application() /*throw(Exception, eh::Exception)*/;

  virtual ~Application() noexcept;

  int run(int &argc, char **argv)
    /*throw(InvalidArgument, Exception, eh::Exception)*/;

  static Application* instance() /*throw(eh::Exception)*/;

private:
  using StatInfo = AdServer::LogProcessing::StatInfo;
  using CreativeStatInfo = AdServer::LogProcessing::CreativeStatInfo;
  using CreativeStatSeq = AdServer::LogProcessing::CreativeStatSeq;
  using PublisherAmountInfo = AdServer::LogProcessing::PublisherAmountInfo;
  using PublisherAmountSeq = AdServer::LogProcessing::PublisherAmountSeq;
  using TagAmountInfo = AdServer::LogProcessing::TagAmountInfo;
  using TagAmountSeq = AdServer::LogProcessing::TagAmountSeq;

  int help()
    /*throw(InvalidArgument, Exception, eh::Exception)*/;

  void print_stat_info(const StatInfo &info)
    /*throw(eh::Exception)*/;

  void output_creative_stats(std::ostream &os, const CreativeStatSeq &creative_stats)
    /*throw(eh::Exception)*/;

  void output_publisher_amounts(std::ostream &os, const PublisherAmountSeq &publisher_amounts)
    /*throw(eh::Exception)*/;

  void output_tag_amounts(std::ostream &os, const TagAmountSeq &tag_amounts)
    /*throw(eh::Exception)*/;

  std::unique_ptr<AdServer::LogProcessing::LogGeneralizerClient> log_generalizer_;
};

using AppSingleton = Generics::Singleton<Application>;

inline
Application*
Application::instance() /*throw(eh::Exception)*/
{
  return &AppSingleton::instance();
}
