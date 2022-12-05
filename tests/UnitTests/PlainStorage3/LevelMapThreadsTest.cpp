#include <Sync/SyncPolicy.hpp>
#include <Generics/AppUtils.hpp>
#include <TestCommons/MTTester.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.cpp>
#include <Commons/AtomicInt.hpp>

using namespace AdServer::ProfilingCommons;

namespace
{
  const char THREAD_STORM_CHECK_PATH[] =
    "/LevelMapTestCont/ThreadStorm/";

  const char USAGE[] =
    "LevelMapThreadsTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -h, --help : show this message.\n";
}

struct Interrupter:
  public Generics::SimpleActiveObject,
  public ReferenceCounting::AtomicImpl
{};

struct StringKey: public Generics::StringHashAdapter
{
  StringKey()
  {}

  StringKey(const char* val)
    : Generics::StringHashAdapter(val)
  {}

  StringKey(const std::string& val)
    : Generics::StringHashAdapter(val)
  {}

  unsigned long
  size() const
  {
    return text().size();
  }

  unsigned long
  area_size() const
  {
    return sizeof(void*) + text().size() + 1;
  }
};

struct StringSerializer
{
  static void
  read(StringKey& key, void* buf, unsigned long buf_size)
  {
    key = StringKey(
      std::string(static_cast<const char*>(buf), buf_size));
  }

  static void
  write(void* buf, unsigned long buf_size, const StringKey& key)
  {
    ::memcpy(buf, key.text().data(), buf_size);
  }

  static unsigned long
  size(const StringKey& key)
  {
    return key.size();
  }
};

typedef AdServer::ProfilingCommons::LevelProfileMap<
  StringKey,
  StringSerializer>
  StringMap;

struct SynGen
{
  SynGen(): id(0) {}

  Algs::AtomicInt id;
};

template<typename MapType, typename LMapType>
class FillerTask: public Generics::Task,
  public ReferenceCounting::AtomicImpl
{
public:
  FillerTask(
    Generics::ActiveObject* interrupter,
    SynGen* syn_gen_val,
    MapType& map_val,
    AdServer::ProfilingCommons::PosixFileController::Stat* op_stat_val,
    unsigned long max_profile_size_val,
    const char* id_val,
    bool overwrite_val,
    unsigned long profiles_count_val)
    : interrupter_(ReferenceCounting::add_ref(interrupter)),
      syn_gen(syn_gen_val),
      map(map_val),
      op_stat(ReferenceCounting::add_ref(op_stat_val)),
      max_profile_size(max_profile_size_val),
      id(id_val),
      overwrite(overwrite_val),
      profiles_count(profiles_count_val)
  {}

  virtual void
  execute() noexcept
  {
    const unsigned long SIZE_STEP = 10*1024;

    Generics::Time time = Generics::Time::get_time_of_day();

    unsigned long int_id = 0;

    while((int_id < profiles_count || overwrite) && interrupter_->active())
    {
      int_id = syn_gen->id.exchange_and_add(1);
      std::ostringstream id_str;
      id_str << id;
      id_str << (int_id % profiles_count);

      Generics::ConstSmartMemBuf_var read_buf(map.get_profile(id_str.str().c_str()));
      if(!read_buf.in() || overwrite)
      {
        if(!overwrite)
        {
          std::cout << "fill '" << id_str.str() << "'" << std::endl;
        }

        try
        {
          Generics::SmartMemBuf_var new_buf(new Generics::SmartMemBuf(SIZE_STEP));
          map.save_profile(id_str.str().c_str(), Generics::transfer_membuf(new_buf), time);
        }
        catch(const typename LMapType::NotActive&)
        {
          sleep(1);
        }
      }
    }
  };

  virtual ~FillerTask() noexcept {}

  Generics::ActiveObject_var interrupter_;
  SynGen* syn_gen;
  MapType& map;
  AdServer::ProfilingCommons::PosixFileController::Stat_var op_stat;
  unsigned long max_profile_size;
  std::string id;
  bool overwrite;
  unsigned long profiles_count;
};

template<typename MapType>
class ReaderTask: public Generics::Task,
  public ReferenceCounting::AtomicImpl
{
public:
  ReaderTask(
    Generics::ActiveObject* interrupter,
    MapType& map_val,
    long profiles_count_val = -1)
    : interrupter_(ReferenceCounting::add_ref(interrupter)),
      map(map_val),
      id("test"),
      profiles_count(profiles_count_val)
  {}

  virtual void
  execute() noexcept
  {
    while(interrupter_->active())
    {
      std::ostringstream id_str;
      id_str << id;
      id_str << (Generics::safe_rand() % profiles_count);

      Generics::ConstSmartMemBuf_var read_buf(map.get_profile(id_str.str().c_str()));
      if(!read_buf.in())
      {
        std::cout << "read_buf '" << id_str.str() << "' is null" << std::endl;
      }
    }
  };

  virtual ~ReaderTask() noexcept {}

  Generics::ActiveObject_var interrupter_;
  MapType& map;
  std::string id;
  long profiles_count;
};

template<typename MapType>
struct ReadWriteProfileFunctor
{
  ReadWriteProfileFunctor(
    SynGen* syn_gen_val,
    MapType& map_val,
    AdServer::ProfilingCommons::PosixFileController::Stat* op_stat_val,
    unsigned long max_profile_size_val,
    long profiles_count_val = -1)
    : syn_gen(syn_gen_val),
      map(map_val),
      op_stat(ReferenceCounting::add_ref(op_stat_val)),
      max_profile_size(max_profile_size_val),
      id("test"),
      profiles_count(profiles_count_val)
  {}

  void operator()()
  {
    const unsigned long SIZE_STEP = 100*1024;

    Generics::Time time(0);

    std::ostringstream id_str;
    id_str << id;

    if(profiles_count != -1)
    {
      id_str << (Generics::safe_rand() % profiles_count);
    }

    Generics::ConstSmartMemBuf_var read_buf(map.get_profile(id_str.str().c_str()));

    unsigned long old_size = read_buf.in() ? read_buf->membuf().size() : 0;
    assert(old_size % SIZE_STEP == 0);

    unsigned long new_size = 0;
    if(old_size + SIZE_STEP < max_profile_size)
    {
      new_size = old_size + SIZE_STEP;
    }

    if(::rand() % 2 == 0 || old_size == 0)
    {
      Generics::SmartMemBuf_var new_buf(new Generics::SmartMemBuf(new_size));
      map.save_profile(id_str.str().c_str(), Generics::transfer_membuf(new_buf), time);
    }

    //std::cerr << "saved '" << id_str.str() << "'" << std::endl;
  }

  SynGen* syn_gen;
  MapType& map;
  AdServer::ProfilingCommons::PosixFileController::Stat_var op_stat;
  unsigned long max_profile_size;
  std::string id;
  long profiles_count;
};

/*
template<typename MapType>
struct TimedWriteReadProfileFunctor
{
  TimedWriteReadProfileFunctor(
    MapType& map_val,
    unsigned long min_profile_size_val,
    unsigned long max_profile_size_val,
    unsigned long profile_size_step_val,
    long profiles_count_val = -1)
    : map(map_val),
      min_profile_size(min_profile_size_val),
      max_profile_size(max_profile_size_val),
      profile_size_step(profile_size_step_val),
      id("test"),
      profiles_count(profiles_count_val)
  {}

  void operator()()
  {
    Generics::Time time(::rand() % 100);

    std::ostringstream id_str;
    id_str << id;

    if(profiles_count != -1)
    {
      id_str << (::rand() % profiles_count);
    }

    Generics::SmartMemBuf_var read_buf(map.get_profile(id_str.str().c_str()));
    unsigned long old_size = 0;

    if(read_buf.in())
    {
      old_size = read_buf->membuf().size();
      if(old_size < min_profile_size || old_size > max_profile_size)
      {
        assert(0);
      }
    }

    unsigned long new_size = min_profile_size;
    if(old_size + profile_size_step <= max_profile_size)
    {
      new_size = old_size + profile_size_step;
    }

    Generics::SmartMemBuf_var new_buf(new Generics::SmartMemBuf(new_size));
    map.save_profile(id_str.str().c_str(), Generics::transfer_membuf(new_buf), time);
  }

  MapType& map;
  unsigned long min_profile_size;
  unsigned long max_profile_size;
  unsigned long profile_size_step;
  std::string id;
  long profiles_count;
};
*/

void remove_and_create(const char* folder_name)
{
  system((std::string("rm -r ") + folder_name + " 2>/dev/null ; "
    "mkdir -p " + folder_name).c_str());
}

template<typename MapType>
ReferenceCounting::SmartPtr<ProfileMap<typename MapType::KeyTypeT> >
init_map(
  Generics::ActiveObject_var& active_object,
  Generics::ActiveObjectCallback* active_object_callback,
  const char* root,
  const char* prefix,
  PosixFileController::Stat* stat,
  bool ssd_mode)
{
  AdServer::ProfilingCommons::FileController_var file_controller(
    new AdServer::ProfilingCommons::PosixFileController(
      stat,
      10ull*1024*1024*1024 // 10 GB
      ));

  if(ssd_mode)
  {
    file_controller = new AdServer::ProfilingCommons::SSDFileController(
      file_controller);
  }

  return AdServer::ProfilingCommons::ProfileMapFactory::open_transaction_level_map<
    typename MapType::KeyTypeT,
    typename MapType::KeySerializerT>(
      active_object,
      active_object_callback,
      root,
      prefix,
      AdServer::ProfilingCommons::LevelMapTraits(
        AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
        10*1024*1024,
        1*1024*1024,
        2000*1024*1024,
        20, // max_levels0
        Generics::Time::ONE_DAY,
        file_controller));
}

class DumpStat:
  public Generics::Goal,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  DumpStat(
    Generics::Planner* planner,
    StatImpl* stat)
    : planner_(planner),
      stat_(ReferenceCounting::add_ref(stat))
  {}

  virtual void
  deliver() /*throw(eh::Exception)*/
  {
    StatImpl_var stat = stat_->reset();
    const auto read_counters = stat->read_counters();

    if(read_counters.count > 0)
    {
      std::cout << "Stat(" <<
        Generics::Time::get_time_of_day().gm_ft() <<
        "): max_time = " << read_counters.max_time <<
        ", avg_time = " << read_counters.avg_time() <<
        ", count = " << read_counters.count << std::endl;
    }

    planner_->schedule(
      this,
      Generics::Time::get_time_of_day() + Generics::Time::ONE_SECOND);
  }

protected:
  Generics::Planner* planner_;
  StatImpl_var stat_;
};

template<typename MapType>
int threads_profile_storm_fill(
  const char* root,
  long profiles_count)
{
  static const char* FUN = "threads_profile_storm_fill()";

  typedef ProfileMap<typename MapType::KeyTypeT> UseMapType;
  typedef ReferenceCounting::SmartPtr<UseMapType> UseMap_var;

  try
  {
    Generics::ActiveObjectCallback_var active_object_callback =
      new Logging::ActiveObjectCallbackImpl(
        Logging::Logger_var(
          new Logging::OStream::Logger(Logging::OStream::Config(std::cout))),
        "LevelMapThreadsTest",
        "LevelMapThreadsTest",
        "ADS-IMPL-?");

    Generics::ActiveObject_var active_object;

    UseMap_var test = init_map<MapType>(
      active_object,
      active_object_callback,
      root,
      "Prefix",
      nullptr, // stat
      false);

    active_object->activate_object();

    UseMapType& map = *test;

    const unsigned long SIZE_STEP = 10*1024;

    Generics::Time now = Generics::Time::get_time_of_day();

    for(unsigned long i = 0; i < static_cast<unsigned long>(profiles_count); ++i)
    {
      std::ostringstream id_str;
      id_str << "test" << i;

      Generics::ConstSmartMemBuf_var read_buf(map.get_profile(id_str.str().c_str()));
      if(!read_buf.in())
      {
        std::cout << "fill '" << id_str.str() << "'" << std::endl;
        Generics::SmartMemBuf_var new_buf(new Generics::SmartMemBuf(SIZE_STEP));
        map.save_profile(id_str.str().c_str(), Generics::transfer_membuf(new_buf), now);
      }
    }

    active_object->deactivate_object();
    active_object->wait_object();
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << FUN << ": eh::Exception: " << ex.what() << std::endl;
    return 1;
  }

  //remove_and_create(root);

  return 0;
}

template<typename MapType, template <typename> class FunType>
int threads_profile_storm_test(
  const char* root,
  long profiles_count,
  unsigned long /*time*/,
  StatImpl* stat,
  bool ssd_mode,
  unsigned long fill_threads,
  unsigned long write_threads,
  unsigned long read_threads)
{
  static const char* FUN = "threads_profile_storm_test()";

  typedef ProfileMap<typename MapType::KeyTypeT> UseMapType;
  typedef ReferenceCounting::SmartPtr<UseMapType> UseMap_var;

  try
  {
    Generics::ActiveObjectCallback_var active_object_callback =
      new Logging::ActiveObjectCallbackImpl(
        Logging::Logger_var(
          new Logging::OStream::Logger(Logging::OStream::Config(std::cout))),
        "LevelMapThreadsTest",
        "LevelMapThreadsTest",
        "ADS-IMPL-?");

    Generics::Planner_var planner = new Generics::Planner(active_object_callback);
    planner->activate_object();
    planner->schedule(
      Generics::Goal_var(new DumpStat(planner, stat)),
      Generics::Time::get_time_of_day() + Generics::Time::ONE_SECOND);

    Generics::ActiveObject_var active_object;

    UseMap_var test = init_map<MapType>(
      active_object,
      active_object_callback,
      root,
      "Prefix",
      stat,
      ssd_mode);

    active_object->activate_object();

    Generics::ActiveObject_var interrupter(new Interrupter());
    interrupter->activate_object();

    //FunType<UseMapType> fun(*test, stat, 2000, profiles_count);
    SynGen syn_gen;

    Generics::TaskRunner_var task_runner(
      new Generics::TaskRunner(
        active_object_callback,
        fill_threads + write_threads + read_threads));
    for(unsigned long i = 0; i < fill_threads; ++i)
    {
      task_runner->enqueue_task(
        Generics::Task_var(new FillerTask<UseMapType, MapType>(
          interrupter,
          &syn_gen,
          *test,
          stat,
          2000,
          "test",
          false,
          static_cast<unsigned long>(profiles_count))));
    }

    for(unsigned long i = 0; i < write_threads; ++i)
    {
      task_runner->enqueue_task(
        Generics::Task_var(new FillerTask<UseMapType, MapType>(
          interrupter, &syn_gen, *test, stat, 2000, "testX", true, profiles_count)));
    }

    for(unsigned long i = 0; i < read_threads; ++i)
    {
      task_runner->enqueue_task(
        Generics::Task_var(new ReaderTask<UseMapType>(interrupter, *test, profiles_count)));
    }

    /*
    TestCommons::MTTester<FunType<UseMapType>&> mt_tester(fun, 300);
    mt_tester.run(profiles_count, time, -1);
    */

    task_runner->activate_object();

    std::cout << "testing started";
    sleep(600);

    active_object->deactivate_object();
    interrupter->deactivate_object();

    task_runner->deactivate_object();
    task_runner->wait_object();

    active_object->wait_object();
    interrupter->wait_object();

    planner->deactivate_object();
    planner->wait_object();
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << FUN << ": eh::Exception: " << ex.what() << std::endl;
    return 1;
  }

  //remove_and_create(root);

  return 0;
}

int
main(int argc, char* argv[]) noexcept
{
  using namespace Generics::AppUtils;
  Args args(-1);
  CheckOption opt_help;
  Option<unsigned long> opt_time(100);
  Option<unsigned long> opt_profiles_count(100);
  Option<std::string> opt_root_dir("./");
  CheckOption opt_ssd;
  Option<unsigned long> opt_fill_threads(0);
  Option<unsigned long> opt_write_threads(30);
  Option<unsigned long> opt_read_threads(50);

  args.add(equal_name("help") || short_name("h"), opt_help);
  args.add(equal_name("time") || short_name("t"), opt_time);
  args.add(equal_name("dir") || short_name("d"), opt_root_dir);
  args.add(equal_name("count") || short_name("c"), opt_profiles_count);
  args.add(equal_name("ssd"), opt_ssd);
  args.add(equal_name("ft"), opt_fill_threads);
  args.add(equal_name("wt"), opt_write_threads);
  args.add(equal_name("rt"), opt_read_threads);

  args.parse(argc - 1, argv + 1);

  const Generics::AppUtils::Args::CommandList& commands = args.commands();

  if(commands.empty() || opt_help.enabled() ||
     *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return 0;
  }

  int result = 0;

  try
  {
    // test latency & average time in fed up state
    system((std::string("mkdir -p ") + *opt_root_dir + THREAD_STORM_CHECK_PATH).c_str());

    if(*commands.begin() == "fill")
    {
      result += threads_profile_storm_fill<StringMap>(
        (*opt_root_dir + THREAD_STORM_CHECK_PATH).c_str(),
        *opt_profiles_count);
    }
    else
    {
      std::cout << "==== few profiles testing ====" << std::endl;

      /*
      remove_and_create(
        (*opt_root_dir + THREAD_STORM_CHECK_PATH).c_str());
      */

      StatImpl_var stat = new StatImpl();

      result += threads_profile_storm_test<StringMap, ReadWriteProfileFunctor>(
        (*opt_root_dir + THREAD_STORM_CHECK_PATH).c_str(),
        *opt_profiles_count,
        *opt_time,
        stat,
        opt_ssd.enabled(),
        *opt_fill_threads,
        *opt_write_threads,
        *opt_read_threads);

      while(true)
      {
        // repeat with reopen
        result += threads_profile_storm_test<StringMap, ReadWriteProfileFunctor>(
          (*opt_root_dir + THREAD_STORM_CHECK_PATH).c_str(),
          *opt_profiles_count,
          *opt_time,
          stat,
          opt_ssd.enabled(),
          *opt_fill_threads,
          *opt_write_threads,
          *opt_read_threads);
      }

      const auto read_counters = stat->read_counters();

      std::cout << "avg time = " << read_counters.avg_time() <<
        ", max_time = " << read_counters.max_time << std::endl;
    }

    return result;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "unknown exception" << std::endl;
  }

  return -1;
}
