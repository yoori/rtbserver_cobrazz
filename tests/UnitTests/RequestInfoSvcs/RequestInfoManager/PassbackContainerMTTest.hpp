/**
 * @file PassbackContainerMTTest.hpp
 */

#ifndef PASSBACK_CONTAINER_TEST_HPP_INCLUDED
#define PASSBACK_CONTAINER_TEST_HPP_INCLUDED

#include <Sync/PosixLock.hpp>
#include <RequestInfoSvcs/RequestInfoManager/PassbackContainer.hpp>

class TestPassbackProcessorImpl :
  public AdServer::RequestInfoSvcs::PassbackProcessor,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  typedef std::list<PassbackInfo> PassbackInfoList;

public:
  virtual void
  process_passback(
    const AdServer::RequestInfoSvcs::PassbackProcessor::PassbackInfo& pi)
    /*throw(AdServer::RequestInfoSvcs::PassbackProcessor::Exception)*/
  {
    Sync::PosixGuard lock(map_lock_);
    result_.push_back(pi);
  }

  const PassbackInfoList& result() const
  {
    return result_;
  }

  void clear() noexcept
  {
    result_.clear();
  }

protected:
  virtual ~TestPassbackProcessorImpl() noexcept
  {}

private:
  Sync::PosixMutex map_lock_;
  PassbackInfoList result_;
};

typedef
  ReferenceCounting::SmartPtr<TestPassbackProcessorImpl>
  TestPassbackProcessorImpl_var;

struct TestIt
{
  AdServer::RequestInfoSvcs::PassbackContainer* passback_container;
  TestPassbackProcessorImpl* passback_processor;
};

/**
 * Multithreading test
 */
bool
multi_thread_test(const TestIt* test_it) noexcept;

#endif // PASSBACK_CONTAINER_TEST_HPP_INCLUDED
