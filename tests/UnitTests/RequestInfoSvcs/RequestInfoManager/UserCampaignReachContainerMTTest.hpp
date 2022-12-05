/**
 * @file UserCampaignReachContainerMTTest.hpp
 */
#ifndef USER_CAMPAIGN_REACH_CONTAINER_MT_TEST_HPP_INCLUDED
#define USER_CAMPAIGN_REACH_CONTAINER_MT_TEST_HPP_INCLUDED

#include <list>

#include <Sync/PosixLock.hpp>
#include <RequestInfoSvcs/RequestInfoManager/UserCampaignReachContainer.hpp>


typedef std::list<
  AdServer::RequestInfoSvcs::CampaignReachProcessor::ReachInfo> ReachInfoList;

struct CollReachTestProcessor :
  public AdServer::RequestInfoSvcs::CampaignReachProcessor,
  public virtual ReferenceCounting::AtomicImpl
{
  typedef AdServer::RequestInfoSvcs::CampaignReachProcessor::Exception
    Exception;

  void
  process_reach(
    const AdServer::RequestInfoSvcs::CampaignReachProcessor::ReachInfo&
      reach_info)
    /*throw(Exception)*/
  {
    Sync::PosixGuard lock(mutex_);
    result_.push_back(reach_info);
  }

  const ReachInfoList&
  result() const
  {
    return result_;
  }

  void
  clear()
  {
    result_.clear();
  }

protected:
  virtual ~CollReachTestProcessor() noexcept
  {}

private:
  Sync::PosixMutex mutex_;
  ReachInfoList result_;
};

struct TestIt
{
  AdServer::RequestInfoSvcs::UserCampaignReachContainer* reach_container;
  CollReachTestProcessor* reach_processor;
};

/**
 * Multithreading test
 */
bool
multi_thread_test(const TestIt* test_it) noexcept;


#endif // USER_CAMPAIGN_REACH_CONTAINER_MT_TEST_HPP_INCLUDED
