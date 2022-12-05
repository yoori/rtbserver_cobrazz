#include <fstream>
#include <sys/stat.h>

#include "../../TestHelpers.hpp"

#include <UserInfoSvcs/UserInfoManager/UserOperationProcessor.hpp>
#include <UserInfoSvcs/UserOperationGenerator/UidChannelSnapshot.hpp>
#include <UserInfoSvcs/UserOperationGenerator/NullUserOperationProcessor.hpp>

using namespace AdServer;
using namespace AdServer::UserInfoSvcs;

namespace
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  const std::string root_path = "./.UidChannelSnapshotTest/";
  const std::string curr_path = root_path + "curr/";
  const std::string temp_path = root_path + "temp/";

  void
  setup() noexcept
  {
    ::system(("rm -r " + root_path + " 2>/dev/null").c_str());
    ::system(("mkdir -p " + curr_path + " 2>/dev/null").c_str());
    ::system(("mkdir -p " + temp_path + " 2>/dev/null").c_str());
  }

  void
  teardown() noexcept
  {}

  void
  create_uid_file(
    const char* file_name,
    std::size_t count = 200,
    bool padding = false)
    noexcept
  {
    std::ofstream ofs(file_name);

    for (std::size_t i = 0; i < count; ++i)
    {
      ofs << Generics::Uuid::create_random_based().to_string(padding) <<
        std::endl;
    }
  }

  void
  update_timestamp(
    const char* file_name,
    const Generics::Time& ts)
    /*throw(eh::Exception)*/
  {
    const struct timeval times[2] = {ts, ts};

    if (utimes(file_name, times))
    {
      eh::throw_errno_exception<Exception>(
        "can't do utimes for '", file_name, "'");
    }
  }

  class MockUserOperationProcessor :
    public NullUserOperationProcessor
  {
  public:
    typedef std::set<AudienceChannel> Channels;
    typedef std::map<Generics::Uuid, Channels> UserChannels;

  public:
    UserChannels added_channels;
    UserChannels removed_channels;

    Generics::ActiveObject_var interrupter;
    std::size_t interrupt_count;
    std::size_t operation_count;

  public:
    MockUserOperationProcessor()
      noexcept
      : interrupter(nullptr), interrupt_count(0), operation_count(0)
    {}

    virtual void
    remove_audience_channels(
      const UserId& uuid,
      const AudienceChannelSet& audience_channels)
      /*throw(ChunkNotFound, Exception)*/
    {
      removed_channels[uuid].insert(
        audience_channels.begin(), audience_channels.end());

      ++operation_count;

      if (interrupter && operation_count == interrupt_count)
      {
        interrupter->deactivate_object();
      }
    }

    virtual void
    add_audience_channels(
      const UserId& uuid,
      const AudienceChannelSet& audience_channels)
      /*throw(NotReady, ChunkNotFound, Exception)*/
    {
      added_channels[uuid].insert(
        audience_channels.begin(), audience_channels.end());
      ++operation_count;

      if (interrupter && operation_count == interrupt_count)
      {
        interrupter->deactivate_object();
      }
    }

  protected:
    virtual
    ~MockUserOperationProcessor() noexcept
    {}
  };

  typedef ReferenceCounting::SmartPtr<MockUserOperationProcessor>
    MockUserOperationProcessor_var;

  class Interrupter :
    public ReferenceCounting::AtomicImpl,
    public Generics::SimpleActiveObject
  {
  private:
    virtual
    ~Interrupter() noexcept
    {}
  };
}

TEST(UidChannelFileConstructor)
{
  const std::string file_name = curr_path + "channel1234567";
  UidChannelFile_var file = new UidChannelFile(file_name.c_str(), Generics::Time());
  ASSERT_EQUALS (file->channel_id(), 1234567UL);
}

TEST_EX(UidChannelFileUpdateTimestamp, setup, teardown)
{
  const std::string file_name = curr_path + "channel1234567";
  create_uid_file(file_name.c_str());

  struct stat64 attrib_before;
  stat64(file_name.c_str(), &attrib_before);

  UidChannelFile_var file = new UidChannelFile(
    file_name.c_str(), Generics::Time(attrib_before.st_mtim.tv_sec));

  sleep(2);

  const Generics::Time now(Generics::Time::get_time_of_day().tv_sec);
  file->update_timestamp(now);

  struct stat64 attrib_after;
  stat64(file_name.c_str(), &attrib_after);
  ASSERT_EQUALS (file->timestamp(), now);
  ASSERT_EQUALS (file->timestamp(), Generics::Time(attrib_after.st_mtim.tv_sec));
}

void
UidChannelFileLoad_(bool padding)
{
  const std::string file_name = curr_path + "channel1234567";
  create_uid_file(file_name.c_str(), 200, padding);

  UidChannelFile_var file = new UidChannelFile(file_name.c_str(), Generics::Time());
  file->load();
  ASSERT_EQUALS (file->count(), 200U);
  file->clear();
  ASSERT_EQUALS (file->count(), 0U);

  std::ofstream ofs(file_name.c_str(), std::ios::app);
  ofs << "INVALID UUID" << std::endl;
  ofs.close();

  try
  {
    file->clear();
    FAIL ("INVALID UUID should produce parse exception");
  }
  catch (eh::Exception&)
  {}
}

TEST_EX(UidChannelFileLoad, setup, teardown)
{
  UidChannelFileLoad_(false);
  UidChannelFileLoad_(true);
}

TEST_EX(UidChannelFileCheck, setup, teardown)
{
  const std::string file_name = curr_path + "channel1234567";
  create_uid_file(file_name.c_str(), 50);

  UidChannelFile_var file = new UidChannelFile(file_name.c_str(), Generics::Time());
  file->load();
  ASSERT_EQUALS (file->count(), 50U);

  try
  {
    file->check();
    FAIL ("50 uuids should produce check exception");
  }
  catch (eh::Exception&)
  {}
}

TEST_EX(UidChannelFileUpdate, setup, teardown)
{
  std::vector<Generics::Uuid> uuids;
  const std::string file_name1 = curr_path + "channel1";
  const std::string new_file_name1 = temp_path + "channel1";

  {
    std::ofstream ofs(file_name1);

    for (std::size_t i = 0; i < 200; ++i)
    {
      uuids.emplace_back(Generics::Uuid::create_random_based());
      ofs << uuids.back().to_string() << std::endl;
    }
  }

  std::random_shuffle(uuids.begin(), uuids.end());

  {
    std::ofstream ofs(new_file_name1);

    for (std::size_t i = 0; i < uuids.size(); ++i)
    {
      ofs << uuids[i].to_string() << std::endl;
    }

    for (std::size_t i = 0; i < 100; ++i)
    {
      ofs << Generics::Uuid::create_random_based().to_string() << std::endl;
    }
  }

  UidChannelFile_var curr_file =
    new UidChannelFile(file_name1.c_str(), Generics::Time());
  curr_file->load();

  UidChannelFile_var new_file =
    new UidChannelFile(new_file_name1.c_str(), Generics::Time());
  new_file->load();

  MockUserOperationProcessor_var processor = new MockUserOperationProcessor();
  curr_file->update(*new_file, *processor, nullptr);
  ASSERT_EQUALS (processor->added_channels.size(), 300U);
  ASSERT_EQUALS (processor->removed_channels.size(), 0U);

  {
    std::ofstream ofs(new_file_name1);

    for (std::size_t i = 0; i < uuids.size(); i += 2)
    {
      ofs << uuids[i].to_string() << std::endl;
    }

    for (std::size_t i = 0; i < 50; ++i)
    {
      ofs << Generics::Uuid::create_random_based().to_string() << std::endl;
    }
  }

  new_file = new UidChannelFile(new_file_name1.c_str(), Generics::Time());
  new_file->load();

  processor = new MockUserOperationProcessor();
  curr_file->update(*new_file, *processor, nullptr);
  ASSERT_EQUALS (processor->added_channels.size(), 150U);
  ASSERT_EQUALS (processor->removed_channels.size(), 100U);
}

TEST_EX(UidChannelFilePerformance, setup, teardown)
{
  {
    const std::size_t COUNT = 1000000U;
    std::ostringstream oss;
    oss << curr_path << "channel" << COUNT;
    const std::string file_name = oss.str();
    create_uid_file(file_name.c_str(), COUNT);

    Generics::CPUTimer timer;
    timer.start();

    UidChannelFile_var file = new UidChannelFile(file_name.c_str(), Generics::Time());
    file->load();
    ASSERT_EQUALS (file->count(), COUNT);

    timer.stop();
    std::cout << COUNT << ": elapsed time: " << timer.elapsed_time() << std::endl;
  }

  {
    const std::size_t COUNT = 5000000U;
    std::ostringstream oss;
    oss << curr_path << "channel" << COUNT;
    const std::string file_name = oss.str();
    create_uid_file(file_name.c_str(), COUNT);

    Generics::CPUTimer timer;
    timer.start();

    UidChannelFile_var file = new UidChannelFile(file_name.c_str(), Generics::Time());
    file->load();
    ASSERT_EQUALS (file->count(), COUNT);

    timer.stop();
    std::cout << COUNT << ": elapsed time: " << timer.elapsed_time() << std::endl;
  }

  {
    const std::size_t COUNT = 10000000U;
    std::ostringstream oss;
    oss << curr_path << "channel" << COUNT;
    const std::string file_name = oss.str();
    create_uid_file(file_name.c_str(), COUNT);

    Generics::CPUTimer timer;
    timer.start();

    UidChannelFile_var file = new UidChannelFile(file_name.c_str(), Generics::Time());
    file->load();
    ASSERT_EQUALS (file->count(), COUNT);

    timer.stop();
    std::cout << COUNT << ": elapsed time: " << timer.elapsed_time() << std::endl;
  }
}

TEST_EX(UidChannelSnapshotLoad, setup, teardown)
{
  const std::string file_name1 = curr_path + "channel1";
  create_uid_file(file_name1.c_str());
  const std::string file_name2 = curr_path + "channel2";
  create_uid_file(file_name2.c_str());

  UidChannelSnapshot_var snapshot = new UidChannelSnapshot(curr_path.c_str());
  snapshot->load();

  ASSERT_EQUALS (snapshot->size(), 2U);
}

TEST_EX(UidChannelSnapshotSyncSelf, setup, teardown)
{
  const std::string file_name1 = curr_path + "channel1";
  create_uid_file(file_name1.c_str());
  const std::string file_name2 = curr_path + "channel2";
  create_uid_file(file_name2.c_str());

  UidChannelSnapshot_var snapshot = new UidChannelSnapshot(curr_path.c_str());
  snapshot->load();

  const std::vector<UidChannelSnapshot::Operation> operations =
    snapshot->sync(*snapshot, 30);
  ASSERT_TRUE (operations.empty());
}

TEST_EX(UidChannelSnapshotSync, setup, teardown)
{
  std::vector<Generics::Uuid> uuids;
  Generics::Time timestamp = Generics::Time::get_time_of_day();
  timestamp -= Generics::Time(60);
  Generics::Time timestamp2 = Generics::Time::get_time_of_day();
  timestamp2 -= Generics::Time(10);

  {
    const std::string file_name1 = curr_path + "channel1";
    std::ofstream ofs(file_name1);

    for (std::size_t i = 0; i < 200; ++i)
    {
      uuids.emplace_back(Generics::Uuid::create_random_based());
      ofs << uuids.back().to_string() << std::endl;
    }

    ofs.close();
    update_timestamp(file_name1.c_str(), timestamp);
  }

  {
    const std::string file_name2 = curr_path + "channel2";
    create_uid_file(file_name2.c_str());
    update_timestamp(file_name2.c_str(), timestamp);
  }

  {
    const std::string file_name3 = curr_path + "channel3";
    create_uid_file(file_name3.c_str());
    update_timestamp(file_name3.c_str(), timestamp);
  }

  {
    const std::string file_name4 = curr_path + "channel4";
    create_uid_file(file_name4.c_str());
    update_timestamp(file_name4.c_str(), timestamp2);
  }

  UidChannelSnapshot_var snapshot = new UidChannelSnapshot(curr_path.c_str());
  snapshot->load();
  sleep(2);

  {
    {
      const std::string new_file_name1 = temp_path + "channel1";
      std::ofstream ofs(new_file_name1);

      for (std::size_t i = 0; i < uuids.size(); i += 2)
      {
        ofs << uuids[i].to_string() << std::endl;
        ofs << Generics::Uuid::create_random_based().to_string() << std::endl;
      }
    }

    {
      const std::string new_file_name3 = temp_path + "channel3";
      create_uid_file(new_file_name3.c_str());
      update_timestamp(new_file_name3.c_str(), timestamp);
    }

    {
      const std::string new_file_name4 = temp_path + "channel4";
      create_uid_file(new_file_name4.c_str());
      update_timestamp(new_file_name4.c_str(), timestamp2);
    }

    {
      const std::string new_file_name5 = temp_path + "channel5";
      create_uid_file(new_file_name5.c_str());
    }

    UidChannelSnapshot_var new_snapshot = new UidChannelSnapshot(temp_path.c_str());
    new_snapshot->load();

    const std::vector<UidChannelSnapshot::Operation> operations =
      snapshot->sync(*new_snapshot, 30);

    ASSERT_EQUALS (operations.size(), 4U);
    ASSERT_EQUALS (operations[0].type, UidChannelSnapshot::OT_UPDATE);
    ASSERT_EQUALS (operations[0].current_file->channel_id(), 1U);
    ASSERT_EQUALS (operations[0].new_file->channel_id(), 1U);
    ASSERT_EQUALS (operations[1].type, UidChannelSnapshot::OT_DELETE);
    ASSERT_EQUALS (operations[1].current_file->channel_id(), 2U);
    ASSERT_EQUALS (operations[2].type, UidChannelSnapshot::OT_REFRESH);
    ASSERT_EQUALS (operations[2].current_file->channel_id(), 3U);
    ASSERT_EQUALS (operations[3].type, UidChannelSnapshot::OT_ADD);
    ASSERT_EQUALS (operations[3].new_file->channel_id(), 5U);

    MockUserOperationProcessor_var processor = new MockUserOperationProcessor();

    for (auto it = operations.begin(); it != operations.end(); ++it)
    {
      snapshot->execute(*it, *processor);
    }

    ASSERT_EQUALS (new_snapshot->size(), 4U);
    ASSERT_EQUALS (processor->added_channels.size(), 600U);
    ASSERT_EQUALS (processor->removed_channels.size(), 300U);

    {
      std::ifstream ifs(curr_path + "channel3");
      std::string str;
      std::getline(ifs, str);
      const auto it = processor->added_channels.find(
        Generics::Uuid(str, false));
      ASSERT_TRUE (it != processor->added_channels.end());
      ASSERT_TRUE (!it->second.empty());
      ASSERT_TRUE (it->second.begin()->channel_id == 3U);
      ASSERT_TRUE (it->second.begin()->time + Generics::Time(30) > Generics::Time::get_time_of_day());
    }
  }
}

TEST_EX(InterruptAdd, setup, teardown)
{
  UidChannelSnapshot_var snapshot = new UidChannelSnapshot(curr_path.c_str());
  snapshot->load();

  const std::string file_name = temp_path + "channel1";
  create_uid_file(file_name.c_str());

  UidChannelSnapshot_var new_snapshot = new UidChannelSnapshot(temp_path.c_str());
    new_snapshot->load();

  const std::vector<UidChannelSnapshot::Operation> operations =
    snapshot->sync(*new_snapshot, 30);

  MockUserOperationProcessor_var processor = new MockUserOperationProcessor();
  processor->interrupter = new Interrupter();
  processor->interrupt_count = 10;

  for (auto it = operations.begin(); it != operations.end(); ++it)
  {
    snapshot->execute(*it, *processor, processor->interrupter);
  }

  std::ifstream check(curr_path + "channel1");
  ASSERT_FALSE (check);
}

TEST_EX(InterruptRemove, setup, teardown)
{
  UidChannelSnapshot_var snapshot = new UidChannelSnapshot(curr_path.c_str());
  snapshot->load();

  const std::string file_name = curr_path + "channel1";
  create_uid_file(file_name.c_str());

  UidChannelSnapshot_var new_snapshot = new UidChannelSnapshot(temp_path.c_str());
    new_snapshot->load();

  const std::vector<UidChannelSnapshot::Operation> operations =
    snapshot->sync(*new_snapshot, 30);

  MockUserOperationProcessor_var processor = new MockUserOperationProcessor();
  processor->interrupter = new Interrupter();
  processor->interrupt_count = 10;

  for (auto it = operations.begin(); it != operations.end(); ++it)
  {
    snapshot->execute(*it, *processor, processor->interrupter);
  }

  std::ifstream check(curr_path + "channel1");
  ASSERT_TRUE (check);
}

RUN_TESTS
