// @file ProfilingCommons/ExpireProfileMapTest.cpp
#include <map>
#include <ReferenceCounting/AtomicImpl.hpp>

#include <Logger/StreamLogger.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Generics/AppUtils.hpp>
#include <Generics/TaskRunner.hpp>
#include <TestCommons/MTTester.hpp>
#include <Commons/Algs.hpp>
#include "ProfileMapUtils.hpp"

/*
 * Calculate serialization coefficient:
 * ratio: execution time without common locks /
 *   execution time with using one object (common locks)
 */
using namespace AdServer::ProfilingCommons;

namespace
{
  const char USAGE[] =
    "ExpireProfileMapSerializeTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -p, --path : path to folder with temporary files.\n";

  const char DEFAULT_ROOT_PATH[] = "./";
  const char IND_MAP_PATH[] = "/ExpireProfileMapSerializeTestCont/Ind";
  const char DEP_MAP_PATH[] = "/ExpireProfileMapSerializeTestCont/Dep";
  const char SEQ_MAP_PATH[] = "/ExpireProfileMapSerializeTestCont/Seq";
  const Generics::Time EXTEND_TIME(1000);
}

typedef AdServer::ProfilingCommons::ExpireProfileMap<
  std::string,
  PlainStorage::StringSerializer>
  StringMap;

struct TestTraits
{
  std::string path;
  unsigned long size_start;
  unsigned long size_step;
  unsigned long iters;
  unsigned long thread_count;
};

struct TestShare
{
  TestShare(): profile_i_(0) {}

  volatile _Atomic_word profile_i_;
};

template<typename MapType>
class SaveProfileFunctor
{
public:
  typedef StringMap MapT;

  SaveProfileFunctor(
    ProfileMap<typename MapT::KeyTypeT>& map,
    const TestTraits& traits,
    TestShare* test_share)
    : map_(map),
      traits_(traits),
      test_share_(test_share)
  {
    buffer_ = new Generics::SmartMemBuf(
      traits.size_start + traits.iters * traits.size_step);
    memset(buffer_->membuf().data(),
      0, traits.size_start + traits.iters * traits.size_step);
  }

  void operator()(bool /*init_call*/ = false)
  {
    _Atomic_word val = __gnu_cxx::__exchange_and_add(
      &test_share_->profile_i_, 1);

    std::ostringstream key;
    key << "TEST" << val;
    map_.save_profile(key.str(), Generics::transfer_membuf(Algs::copy_membuf(buffer_)));
  }

private:
  ProfileMap<typename MapT::KeyTypeT>& map_;
  TestTraits traits_;
  TestShare* test_share_;
  Generics::SmartMemBuf_var buffer_;
};

template<typename MapType>
class ReadProfileFunctor
{
public:
  typedef StringMap MapT;

  ReadProfileFunctor(
    ProfileMap<typename MapT::KeyTypeT>& map,
    const TestTraits& traits,
    TestShare* test_share)
    : map_(map),
      traits_(traits),
      test_share_(test_share)
  {}

  void operator()(bool /*init_call*/ = false)
  {
    _Atomic_word val = __gnu_cxx::__exchange_and_add(
      &test_share_->profile_i_, 1);

    std::ostringstream key;
    key << "TEST" << val;
    Generics::SmartMemBuf_var buf = map_.get_profile(key.str());
  }

private:
  ProfileMap<typename MapT::KeyTypeT>& map_;
  TestTraits traits_;
  TestShare* test_share_;
};

/*
template<typename MapType>
class SaveFileLayerFunctor
{
public:
  SaveFileLayerFunctor(MapType& map, const TestTraits& traits, TestShare* test_share)
    : map_(map),
      traits_(traits),
      test_share_(test_share)
  {
    buffer_ = new Generics::SmartMemBuf(
      traits.size_start + traits.iters * traits.size_step);
    memset(buffer_->membuf().data(),
      0, traits.size_start + traits.iters * traits.size_step);
  }

  void operator()(bool init_call = false)
  {
    _Atomic_word val = __gnu_cxx::__exchange_and_add(
      &test_share_->profile_i_, -1);

    PlainStorage::WriteBlock_var wb = map_.get_write_block(val);
    wb->write(buffer_->membuf().data(), buffer_->membuf().size());
  }

private:
  MapType& map_;
  TestTraits traits_;
  TestShare* test_share_;
  Generics::SmartMemBuf_var buffer_;
};
*/

template<typename FunType>
class IndTask:
  public virtual Generics::Task,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  IndTask(
    Sync::Semaphore& sema,
    volatile _Atomic_word& count,
    unsigned long i,
    const TestTraits& traits,
    TestShare* test_share)
    /*throw(eh::Exception)*/
    : sema_(sema),
      count_(count),
      i_(i),
      traits_(traits),
      test_share_(test_share)
  {
    std::ostringstream ns;
    ns << traits.path << "/" << IND_MAP_PATH << "/" << i << "/";
    map_ = new typename FunType::MapT(
      ns.str().c_str(), "Chunk_", EXTEND_TIME);
    system((std::string("rm -r ") + ns.str() +
      " 2>/dev/null; mkdir -p " + ns.str()).c_str());

    FunType fun(*map_, traits_, test_share_);
    fun(true);
  }

  virtual void execute() noexcept
  {
    FunType fun(*map_, traits_, test_share_);

    while(count_ > 0)
    {
//    std::cout << ">#" << i_ << std::endl;
      _Atomic_word val = __gnu_cxx::__exchange_and_add(&count_, -1);
      if(val <= 0)
      {
        break;
      }

      fun(false);
//    std::cout << "<#" << i_ << std::endl;
    }

    sema_.release();
  }

protected:
  virtual ~IndTask() noexcept {}
  
private:
  Sync::Semaphore& sema_;
  volatile _Atomic_word& count_;
  unsigned long i_;
  TestTraits traits_;
  TestShare* test_share_;
  ReferenceCounting::SmartPtr<typename FunType::MapT> map_;
};

template<typename FunType>
Generics::Time calc_dependent_exec_time(const TestTraits& traits)
{
//static const char* FUN = "calc_dependent_exec_time()";

  typedef typename FunType::MapT MapT;
  typedef ProfileMap<typename MapT::KeyTypeT> UseMapType;
  typedef ReferenceCounting::SmartPtr<UseMapType> UseMap_var;

  UseMap_var map = init_map<MapT>(
    traits.path.c_str(), DEP_MAP_PATH, EXTEND_TIME, false);

  TestShare test_share;
  FunType fun(*map, traits, &test_share);
  fun(true);

  Generics::Timer timer;

  {
    TestCommons::MTTester<FunType&> mt_tester(fun, traits.thread_count);

    timer.start();
    mt_tester.run(traits.iters, 0, traits.iters);
    timer.stop();
  }

  return timer.elapsed_time();
}

template<typename FunType>
Generics::Time calc_seq_exec_time(const TestTraits& traits)
{
//static const char* FUN = "calc_independent_exec_time()";

  typedef typename FunType::MapT MapT;
  typedef ProfileMap<typename MapT::KeyTypeT> UseMapType;
  typedef ReferenceCounting::SmartPtr<UseMapType> UseMap_var;

  UseMap_var map = init_map<MapT>(
    traits.path.c_str(), SEQ_MAP_PATH, EXTEND_TIME, false);

  TestShare test_share;

  Generics::Timer timer;
  timer.start();

  for(unsigned long i = 0; i < traits.iters; ++i)
  {
    FunType fun(*map, traits, &test_share);
    fun();
  }

  timer.stop();

  return timer.elapsed_time();
}

template<typename FunType>
Generics::Time calc_independent_exec_time(const TestTraits& traits)
{
//static const char* FUN = "calc_independent_exec_time()";

  typedef typename FunType::MapT MapT;
  typedef ProfileMap<typename MapT::KeyTypeT> UseMapType;
  typedef ReferenceCounting::SmartPtr<UseMapType> UseMap_var;

  UseMap_var map = init_map<MapT>(
    traits.path.c_str(), IND_MAP_PATH, EXTEND_TIME, false);

  Logging::Logger_var logger =
    new Logging::OStream::Logger(Logging::OStream::Config(std::cerr));
  
  Generics::ActiveObjectCallback_var callback(
    new Logging::ActiveObjectCallbackImpl(logger));

  Generics::TaskRunner_var task_runner(
    new Generics::TaskRunner(callback, traits.thread_count));

  Sync::Semaphore sema(0);
  TestShare test_share;
  volatile _Atomic_word count = traits.iters;

  for(unsigned long i = 0; i < traits.thread_count; ++i)
  {
    task_runner->enqueue_task(
      Generics::Task_var(
        new IndTask<FunType>(sema, count, i, traits, &test_share)));
  }

  Generics::Timer timer;
  timer.start();

  task_runner->activate_object();

  for(unsigned long i = 0; i < traits.thread_count; ++i)
  {
    sema.acquire();
  }
  
  timer.stop();
  task_runner->deactivate_object();
  task_runner->wait_object();

  return timer.elapsed_time();
}

template<typename FunType>
void calc_coef(Generics::Time& /*ind_time*/,
  Generics::Time& dep_time,
  Generics::Time& /*seq_time*/,
  const TestTraits& traits)
{
  dep_time = calc_dependent_exec_time<FunType>(traits);
//ind_time = calc_independent_exec_time<FunType>(traits);
//seq_time = calc_seq_exec_time<FunType>(traits);
//std::cout << "-------------------" << std::endl;
}

int main(int argc, char* argv[]) noexcept
{
  using namespace Generics::AppUtils;
  Args args;
  StringOption root_path(DEFAULT_ROOT_PATH);
  Option<unsigned long> opt_thread_count(10);
  CheckOption opt_help;

  args.add(equal_name("path") || short_name("p"), root_path);
  args.add(equal_name("help") || short_name("h"), opt_help);
  args.add(equal_name("thread") || short_name("t"), opt_thread_count);

  args.parse(argc - 1, argv + 1);

  if (opt_help.enabled())
  {
    std::cout << USAGE << std::endl;
    return 0;
  }

  try
  {
    TestTraits traits;
    traits.path = *root_path;
    traits.size_start = 100*1000;
//  traits.size_start = 1000;
    traits.size_step = 0;
    traits.iters = 100000;
//  traits.iters = 10000;
    traits.thread_count = *opt_thread_count;
    
    {
      system((std::string("rm -r ") + traits.path +
        DEP_MAP_PATH + " 2>/dev/null; mkdir -p " + traits.path +
        DEP_MAP_PATH + "/").c_str());

      system((std::string("rm -r ") + traits.path +
        IND_MAP_PATH + " 2>/dev/null; mkdir -p " + traits.path +
        IND_MAP_PATH + "/").c_str());

      system((std::string("rm -r ") + traits.path +
        SEQ_MAP_PATH + " 2>/dev/null; mkdir -p " + traits.path +
        SEQ_MAP_PATH + "/").c_str());

      Generics::Time ind_time;
      Generics::Time dep_time;
      Generics::Time seq_time;

      /*
      calc_coef<SaveProfileFunctor<StringMap> >(
        ind_time, dep_time, seq_time, traits);

      std::cout << "====> create profiles <====" << std::endl <<
        "serialized: " << seq_time << std::endl <<
        "dependent(" << *opt_thread_count << "): " << dep_time << std::endl <<
        "independent: " << ind_time << std::endl <<
        "independent/dependent: " <<
        (ind_time.tv_sec * 1000000.0 + ind_time.tv_usec) /
        (dep_time.tv_sec * 1000000.0 + dep_time.tv_usec) <<
        std::endl;
      */

      calc_coef<SaveProfileFunctor<StringMap> >(
        ind_time, dep_time, seq_time, traits);

      std::cout << "====> rewrite profiles <====" << std::endl <<
        "serialized: " << seq_time << std::endl <<
        "dependent: " << dep_time << std::endl <<
        "independent: " << ind_time << std::endl <<
        "independent/dependent: " <<
        (ind_time.tv_sec * 1000000.0 + ind_time.tv_usec) /
        (dep_time.tv_sec * 1000000.0 + dep_time.tv_usec) <<
        std::endl;

      /*
      calc_coef<ReadProfileFunctor<StringMap> >(
        ind_time, dep_time, seq_time, traits);

      std::cout << "====> read profiles <====" << std::endl <<
        "serialized: " << seq_time << std::endl <<
        "dependent: " << dep_time << std::endl <<
        "independent: " << ind_time << std::endl <<
        "independent/dependent: " <<
        (ind_time.tv_sec * 1000000.0 + ind_time.tv_usec) /
        (dep_time.tv_sec * 1000000.0 + dep_time.tv_usec) <<
        std::endl;
      */
    }

    /*
    {
      system((std::string("rm -r ") + traits.path +
        DEP_MAP_PATH + " 2>/dev/null; mkdir -p " + traits.path +
        DEP_MAP_PATH + "/").c_str());

      system((std::string("rm -r ") + traits.path +
        IND_MAP_PATH + " 2>/dev/null; mkdir -p " + traits.path +
        IND_MAP_PATH + "/").c_str());

      Generics::Time ind_time;
      Generics::Time dep_time;

      calc_coef<SaveFileLayerFunctor<PlainStorage::FileLayer> >(
        ind_time, dep_time, traits);

      std::cout << ind_time << "/" << dep_time << "(common locks) = " <<
        (ind_time.tv_sec * 1000000.0 + ind_time.tv_usec) /
        (dep_time.tv_sec * 1000000.0 + dep_time.tv_usec) <<
        std::endl;

      std::cout << "=========================" << std::endl;
    
      calc_coef<SaveFileLayerFunctor<PlainStorage::FileLayer> >(
        ind_time, dep_time, traits);

      std::cout << ind_time << "/" << dep_time << "(common locks) = " <<
        (ind_time.tv_sec * 1000000.0 + ind_time.tv_usec) /
        (dep_time.tv_sec * 1000000.0 + dep_time.tv_usec) <<
        std::endl;
    }
    */
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }

  return -1;
}
