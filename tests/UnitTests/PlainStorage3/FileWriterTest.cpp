#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <Generics/AppUtils.hpp>
#include <TestCommons/ActiveObjectCallback.hpp>

#include <ProfilingCommons/PlainStorage3/FileReader.hpp>
#include <ProfilingCommons/PlainStorage3/FileWriter.hpp>
#include <ProfilingCommons/PlainStorage3/FileLevel.hpp>
#include <ProfilingCommons/PlainStorage3/ReadMemLevel.hpp>
#include <ProfilingCommons/PlainStorage3/RWMemLevel.hpp>
#include <ProfilingCommons/PlainStorage3/MergeIterator.hpp>
#include <ProfilingCommons/PlainStorage3/LevelProfileMap.hpp>
#include <ProfilingCommons/PlainStorage3/MergeIterator.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>

using namespace AdServer::ProfilingCommons;

namespace
{
  const char USAGE[] =
    "FileWriterTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -p, --path : path to folder with temporary files.\n"
    "  -h, --help : show this message.\n";

  const char DEFAULT_ROOT_PATH[] = ".";
}

int
test1(const char* file_name, unsigned long buf_size, unsigned long file_size)
{
  {
    ::srand(time(0));
    int fd = ::open64(file_name, O_RDWR | O_CREAT, S_IWRITE | S_IREAD);
    if(fd < 0)
    {
      std::cerr << "can't open file" << std::endl;
      return 1;
    }
    FileWriter file_writer(fd, buf_size);
    unsigned long pos = 0;
    while(pos < file_size)
    {
      bool big_block = (::rand() % 10) == 0;
      int sz;
      if(big_block)
      {
        sz = buf_size / 2 + ::rand() % (2 * buf_size) + 1;
      }
      else
      {
        sz = ::rand() % (buf_size / 2) + 1;
      }
      Generics::MemBuf mb(sz);
      for(int i = 0; i < sz; ++i)
      {
        *(static_cast<unsigned char*>(mb.data()) + i) = (pos + i) % 256;
      }
      file_writer.write(mb.data(), mb.size());
      pos += mb.size();
    }
    ::close(fd);
  }

  {
    int fd = ::open64(file_name, O_RDONLY, S_IREAD);
    if(fd < 0)
    {
      std::cerr << "can't open file" << std::endl;
      return 1;
    }
    FileReader file_reader(fd, false, buf_size);
    unsigned long pos = 0;
    while(true)
    {
      bool skip = (::rand() % 10) == 0;
      bool big_block = (::rand() % 10) == 0;
      int sz;
      if(big_block)
      {
        sz = buf_size / 2 + ::rand() % (2 * buf_size) + 1;
      }
      else
      {
        sz = ::rand() % (buf_size / 2) + 1;
      }

      if(skip)
      {
        pos += file_reader.skip(sz);
      }
      else
      {
        Generics::MemBuf mb(sz);
        unsigned read_size = file_reader.read(mb.data(), mb.size());
        for(unsigned long i = 0; i < read_size; ++i)
        {
          if(*(static_cast<unsigned char*>(mb.data()) + i) != (pos + i) % 256)
          {
            std::cerr << "assert on pos + i = " << (pos + i) << std::endl;
            assert(0);
          }
          //assert(*(static_cast<unsigned char*>(mb.data()) + i) == (pos + i) % 256);
        }
        if(read_size < mb.size())
        {
          break;
        }
        pos += read_size;
      }
    }
  }

  return 0;
}

struct StringKey: public std::string
{
  StringKey()
  {}

  StringKey(const char* val)
    : std::string(val)
  {}

  StringKey(const std::string& val)
    : std::string(val)
  {}

  unsigned long
  area_size() const
  {
    return sizeof(void*) + size() + 1;
  }
};

struct StringSerializer
{
  static void
  read(StringKey& key, void* buf, unsigned long buf_size)
  {
    key.assign(static_cast<const char*>(buf), buf_size);
  }

  static void
  write(void* buf, unsigned long buf_size, const StringKey& key)
  {
    ::memcpy(buf, key.data(), buf_size);
  }

  static unsigned long
  size(const StringKey& key)
  {
    return key.size();
  }
};

ReferenceCounting::SmartPtr<ReadFileLevel<StringKey, StringSerializer> >
open_file_level(
  RWMemLevel<StringKey, StringSerializer>* src_rw_mem_level,
  const char* dir,
  const char* test_name,
  const char* file_prefix,
  bool remove_existing_files)
{
  std::string index_file_name = std::string(dir) +
    "/" + test_name + "/" + file_prefix + ".index";
  std::string data_file_name = std::string(dir) +
    "/" + test_name + "/" + file_prefix + ".data";

  if(remove_existing_files)
  {
    ::system((std::string("rm -f ") + index_file_name).c_str());
    ::system((std::string("rm -f ") + data_file_name).c_str());
  }

  ::system((std::string("mkdir -p ") + dir + "/" + test_name).c_str());

  ReferenceCounting::SmartPtr<ReadFileLevel<StringKey, StringSerializer> >
    file_level;

  if(src_rw_mem_level)
  {
    ReferenceCounting::SmartPtr<ReadMemLevel<StringKey> > src_read_mem_level =
      src_rw_mem_level->convert_to_read_mem_level();
    file_level = new ReadFileLevel<StringKey, StringSerializer>(
      index_file_name.c_str(),
      data_file_name.c_str(),
      src_read_mem_level->get_iterator(1000),
      10*1024*1024,
      false,
      0);
  }
  else
  {
    file_level = new ReadFileLevel<StringKey, StringSerializer>(
      index_file_name.c_str(),
      data_file_name.c_str(),
      10*1024*1024,
      false);
  }

  return file_level;
}

int
simple_levels_test(const char* dir)
{
  static const char* TEST_NAME = "SimpleLevelsTest";

  ::system((std::string("rm -f ") + dir + "/level0.index").c_str());
  ::system((std::string("rm -f ") + dir + "/level0.data").c_str());

  {
    ReferenceCounting::SmartPtr<RWMemLevel<StringKey, StringSerializer> > src_rw_mem_level(
      new RWMemLevel<StringKey, StringSerializer>());
    Generics::SmartMemBuf_var mb(new Generics::SmartMemBuf(1));
    *static_cast<unsigned char*>(mb->membuf().data()) = 'A';
    src_rw_mem_level->save_profile(
      "key0",
      Generics::transfer_membuf(mb),
      PO_INSERT,
      0,
      Generics::Time::ZERO);

    open_file_level(src_rw_mem_level, dir, TEST_NAME, "level0", true);
  }

  {
    ReferenceCounting::SmartPtr<ReadFileLevel<StringKey, StringSerializer> >
      file_level = open_file_level(0, dir, TEST_NAME, "level0", false);

    GetProfileResult res = file_level->get_profile("key0");
    if(res.operation == PO_ERASE)
    {
      std::cerr << "read result erased" << std::endl;
      return 1;
    }
    else if(!res.mem_buf)
    {
      std::cerr << "read result not found" << std::endl;
      return 1;
    }
    else
    {
      std::cout << "read result(" << res.mem_buf->membuf().size() << "): '" <<
        std::string(static_cast<const char*>(
          res.mem_buf->membuf().data()), res.mem_buf->membuf().size()) <<
        "'" << std::endl;
    }
  }

  return 0;
}

void
print_all_records(
  std::ostream& ostr,
  ReadFileLevel<StringKey, StringSerializer>::Iterator* it)
{
  StringKey key;
  ProfileOperation operation;
  Generics::Time access_time;
  int i = 0;
  while(it->get_next(key, operation, access_time))
  {
    ostr << "[" << i++ << "]: key = '" << key << "', operation = " <<
      operation << std::endl;
  }
}

int
merge_test(const char* dir)
{
  static const char* TEST_NAME = "MergeIteratorTest";

  {
    ReferenceCounting::SmartPtr<RWMemLevel<StringKey, StringSerializer> > src_rw_mem_level(
      new RWMemLevel<StringKey, StringSerializer>());

    {
      Generics::SmartMemBuf_var mb(new Generics::SmartMemBuf(1));
      *static_cast<unsigned char*>(mb->membuf().data()) = 'A';
      src_rw_mem_level->save_profile(
        "key0",
        Generics::transfer_membuf(mb),
        PO_INSERT,
        0,
        Generics::Time::ZERO);
    }

    {
      Generics::SmartMemBuf_var mb(new Generics::SmartMemBuf(1));
      *static_cast<unsigned char*>(mb->membuf().data()) = 'A';
      src_rw_mem_level->save_profile(
        "key1",
        Generics::transfer_membuf(mb),
        PO_INSERT,
        0,
        Generics::Time::ZERO);
    }

    open_file_level(src_rw_mem_level, dir, TEST_NAME, "level1", true);
  }

  {
    ReferenceCounting::SmartPtr<RWMemLevel<StringKey, StringSerializer> > src_rw_mem_level(
      new RWMemLevel<StringKey, StringSerializer>());

    {
      Generics::SmartMemBuf_var mb(new Generics::SmartMemBuf(1));
      *static_cast<unsigned char*>(mb->membuf().data()) = 'A';
      src_rw_mem_level->save_profile(
        "key0",
        Generics::transfer_membuf(mb),
        PO_ERASE,
        0,
        Generics::Time::ZERO);
    }

    {
      Generics::SmartMemBuf_var mb(new Generics::SmartMemBuf(1));
      *static_cast<unsigned char*>(mb->membuf().data()) = 'A';
      src_rw_mem_level->save_profile(
        "key1",
        Generics::transfer_membuf(mb),
        PO_REWRITE,
        0,
        Generics::Time::ZERO);
    }

    open_file_level(src_rw_mem_level, dir, TEST_NAME, "level0", true);
  }

  {
    ReferenceCounting::SmartPtr<ReadFileLevel<StringKey, StringSerializer> >
      file_level1 = open_file_level(0, dir, TEST_NAME, "level1", false);

    std::cout << "L1 records:" << std::endl;
    print_all_records(std::cout, file_level1->get_iterator(10*1024));

    {
      // check iterator
      ReadFileLevel<StringKey, StringSerializer>::Iterator_var it =
        file_level1->get_iterator(10*1024);
      StringKey key;
      ProfileOperation operation;
      Generics::Time access_time;
      if(!it->get_next(key, operation, access_time))
      {
        std::cerr << "L1: unexpected end at first record" << std::endl;
        return 1;
      }

      if(key != "key0" || operation != PO_INSERT)
      {
        std::cerr << "L1: incorrect key or operation on first record, "
          "key = '" << key << "', "
          "operation = " << operation << std::endl;
        return 1;
      }

      if(!it->get_next(key, operation, access_time))
      {
        std::cerr << "L1: unexpected end at second record" << std::endl;
        return 1;
      }

      if(key != "key1" || operation != PO_INSERT)
      {
        std::cerr << "L1: incorrect key or operation on second record" << std::endl;
        return 1;
      }

      if(it->get_next(key, operation, access_time))
      {
        std::cerr << "L1: no end when expected" << std::endl;
        return 1;
      }
    }

    ReferenceCounting::SmartPtr<ReadFileLevel<StringKey, StringSerializer> >
      file_level0 = open_file_level(0, dir, TEST_NAME, "level0", false);

    std::cout << "L0 records:" << std::endl;
    print_all_records(std::cout, file_level0->get_iterator(10*1024));

    {
      // check iterator
      ReadFileLevel<StringKey, StringSerializer>::Iterator_var it =
        file_level0->get_iterator(10*1024);
      StringKey key;
      ProfileOperation operation;
      Generics::Time access_time;
      if(!it->get_next(key, operation, access_time))
      {
        std::cerr << "L0: unexpected end at first record" << std::endl;
        return 1;
      }

      if(key != "key0" || operation != PO_ERASE)
      {
        std::cerr << "L0: incorrect key or operation on first record, "
          "key = '" << key << "', "
          "operation = " << operation << std::endl;
        return 1;
      }

      if(!it->get_next(key, operation, access_time))
      {
        std::cerr << "L0: unexpected end at second record" << std::endl;
        return 1;
      }

      if(key != "key1" || operation != PO_REWRITE)
      {
        std::cerr << "L0: incorrect key or operation on second record, "
          "key = '" << key << "', "
          "operation = " << operation << std::endl;
        return 1;
      }

      if(it->get_next(key, operation, access_time))
      {
        std::cerr << "L0: no end when expected" << std::endl;
        return 1;
      }
    }

    {
      // check merge iterator
      ReadFileLevel<StringKey, StringSerializer>::Iterator_var it0 =
        file_level0->get_iterator(10*1024);
      ReadFileLevel<StringKey, StringSerializer>::Iterator_var it1 =
        file_level1->get_iterator(10*1024);
      std::list<ReadBaseLevel<StringKey>::Iterator_var> its;
      its.push_back(it0);
      its.push_back(it1);

      std::cout << "=== LM records ===" << std::endl;
      print_all_records(
        std::cout,
        ReadBaseLevel<StringKey>::Iterator_var(new MergeIterator<StringKey>(its)));
      std::cout << "==================" << std::endl;
    }

    {
      // check merge iterator
      ReadFileLevel<StringKey, StringSerializer>::Iterator_var it0 =
        file_level0->get_iterator(10*1024);
      ReadFileLevel<StringKey, StringSerializer>::Iterator_var it1 =
        file_level1->get_iterator(10*1024);
      std::list<ReadBaseLevel<StringKey>::Iterator_var> its;
      its.push_back(it0);
      its.push_back(it1);

      std::cout << "=== LMP records ===" << std::endl;
      print_all_records(
        std::cout,
        ReadBaseLevel<StringKey>::Iterator_var(
          new OperationPackIterator<StringKey>(
            ReadBaseLevel<StringKey>::Iterator_var(new MergeIterator<StringKey>(its)))));
      std::cout << "==================" << std::endl;
    }

    {
      // check merge iterator
      ReadFileLevel<StringKey, StringSerializer>::Iterator_var it0 =
        file_level0->get_iterator(10*1024);
      ReadFileLevel<StringKey, StringSerializer>::Iterator_var it1 =
        file_level1->get_iterator(10*1024);
      std::list<ReadBaseLevel<StringKey>::Iterator_var> its;
      its.push_back(it0);
      its.push_back(it1);

      ReadBaseLevel<StringKey>::Iterator_var it(
        new OperationPackIterator<StringKey>(
          ReadBaseLevel<StringKey>::Iterator_var(
            new MergeIterator<StringKey>(its))));
      StringKey key;
      ProfileOperation operation;
      Generics::Time access_time;
      if(!it->get_next(key, operation, access_time))
      {
        std::cerr << "LM: unexpected end at first record" << std::endl;
        return 1;
      }

      if(key != "key1" || operation != PO_INSERT)
      {
        std::cerr << "LM: incorrect key or operation on first record, "
          "key = '" << key << "', "
          "operation = " << operation << std::endl;
        return 1;
      }

      if(it->get_next(key, operation, access_time))
      {
        std::cerr << "LM: no end when expected" << std::endl;
        return 1;
      }
    }
  }

  return 0;
}

int check_key(
  ProfileMap<StringKey>* profile_map,
  unsigned long i,
  unsigned long expected_size)
{
  std::ostringstream ostr;
  ostr << "key" << i;
  Generics::ConstSmartMemBuf_var mem_buf =
    profile_map->get_profile(StringKey(ostr.str()));
  if(mem_buf.in())
  {
    if(expected_size == 0)
    {
      std::cerr << "error: key '" << ostr.str() <<
        "' found, record size = " << mem_buf->membuf().size() << std::endl;
      return 1;
    }
    else if(mem_buf->membuf().size() != expected_size)
    {
      std::cerr << "error: key '" << ostr.str() <<
        "' found, but have incorrect size = " << mem_buf->membuf().size() <<
        " instead " << expected_size << std::endl;
      return 1;
    }
    else
    {
      std::cout << "key '" << ostr.str() <<
        "' found, record size = " << mem_buf->membuf().size() << std::endl;
    }
  }
  else
  {
    if(expected_size > 0)
    {
      std::cerr << "error: key '" << ostr.str() << "' not found" << std::endl;
      return 1;
    }
    else
    {
      std::cout << "key '" << ostr.str() << "' not found" << std::endl;
    }
  }

  return 0;
}

int
test_level_profile_map(
  const char* dir,
  unsigned long rw_buffer_size,
  unsigned long rwlevel_max_size,
  unsigned long max_undumped_size,
  unsigned long max_levels0,
  unsigned long RECORDS_NUM,
  unsigned long RECORD_SIZE)
{
  static const char* TEST_NAME = "TestLevelProfileMap";

  std::ostringstream map_dir_ostr;
  map_dir_ostr << dir << "/" << TEST_NAME <<
    "-" << rw_buffer_size <<
    "-" << rwlevel_max_size <<
    "-" << max_undumped_size << "/";
  std::string map_dir = map_dir_ostr.str();

  ::system((std::string("rm -rf ") + map_dir).c_str());
  ::system((std::string("mkdir -p ") + map_dir).c_str());

  Generics::ActiveObjectCallback_var active_object_callback(
    new TestCommons::ActiveObjectCallbackStreamImpl(std::cerr, TEST_NAME));

  int res = 0;
  unsigned long area_size = 0;
  Generics::Timer timer;
  timer.start();

  {
    ReferenceCounting::SmartPtr<LevelProfileMap<StringKey, StringSerializer> >
      level_profile_map(new LevelProfileMap<StringKey, StringSerializer>(
        active_object_callback,
        map_dir.c_str(),
        "TestLevelProfileMap",
        AdServer::ProfilingCommons::LevelMapTraits(
          AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
          rw_buffer_size,
          rwlevel_max_size,
          max_undumped_size,
          max_levels0,
          Generics::Time::ZERO)));
    level_profile_map->activate_object();

    Generics::Time now = Generics::Time::get_time_of_day();
    for(unsigned long i = 0; i < RECORDS_NUM; ++i)
    {
      std::ostringstream ostr;
      ostr << "key" << i;
      Generics::SmartMemBuf_var mb(new Generics::SmartMemBuf(RECORD_SIZE));
      ::memset(mb->membuf().data(), i % 256, RECORD_SIZE);
      level_profile_map->save_profile(
        StringKey(ostr.str()),
        Generics::transfer_membuf(mb),
        now);
    }

    for(unsigned long i = 0; i < RECORDS_NUM / 100; ++i)
    {
      std::ostringstream ostr;
      ostr << "key" << (i*100 + 2);
      Generics::SmartMemBuf_var mb(new Generics::SmartMemBuf(RECORD_SIZE*3));
      ::memset(mb->membuf().data(), i % 256, RECORD_SIZE*2);
      level_profile_map->save_profile(
        StringKey(ostr.str()),
        Generics::transfer_membuf(mb),
        now);
    }

    for(unsigned long i = 0; i < RECORDS_NUM / 10; ++i)
    {
      std::ostringstream ostr;
      ostr << "key" << (i*10);
      level_profile_map->remove_profile(ostr.str());
    }

    for(unsigned long i = 0; i < RECORDS_NUM / 100; ++i)
    {
      std::ostringstream ostr;
      ostr << "key" << (i*100);
      Generics::SmartMemBuf_var mb(new Generics::SmartMemBuf(RECORD_SIZE*2));
      ::memset(mb->membuf().data(), i % 256, RECORD_SIZE*2);
      level_profile_map->save_profile(
        StringKey(ostr.str()),
        Generics::transfer_membuf(mb),
        now);
    }

    level_profile_map->deactivate_object();
    level_profile_map->wait_object();

    area_size = level_profile_map->area_size();
  }

  timer.stop();
  std::cout << "saving(rw_buffer_size = " << rw_buffer_size <<
    ", rwlevel_max_size = " << rwlevel_max_size <<
    ", max_undumped_size = " << max_undumped_size <<
    ": time = " << timer.elapsed_time() <<
    ", time per record = " << (timer.elapsed_time() / RECORDS_NUM) << std::endl;

  {
    ReferenceCounting::SmartPtr<LevelProfileMap<StringKey, StringSerializer> >
      level_profile_map(new LevelProfileMap<StringKey, StringSerializer>(
        active_object_callback,
        map_dir.c_str(),
        "TestLevelProfileMap",
        AdServer::ProfilingCommons::LevelMapTraits(
          AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
          10000,
          10000,
          12000,
          20,
          Generics::Time::ZERO)));
    level_profile_map->activate_object();

    unsigned long new_area_size = level_profile_map->area_size();
    if(new_area_size != area_size)
    {
      std::cerr << "error: opened area_size = " << new_area_size <<
        " not equal to area_size before save = " << area_size << std::endl;
      res += 1;
    }

    res += check_key(level_profile_map, 0, 2*RECORD_SIZE);
    res += check_key(level_profile_map, 1, RECORD_SIZE);
    res += check_key(level_profile_map, 2, 3*RECORD_SIZE);
    res += check_key(level_profile_map, 10, 0);
    res += check_key(level_profile_map, 11, RECORD_SIZE);
    res += check_key(level_profile_map, 100, 2*RECORD_SIZE);
    res += check_key(level_profile_map, 102, 3*RECORD_SIZE);
    res += check_key(level_profile_map, RECORDS_NUM, 0);

    level_profile_map->deactivate_object();
    level_profile_map->wait_object();
  }

  return res;
}

int
test_level_profile_latency(
  const char* dir,
  unsigned long rw_buffer_size,
  unsigned long rwlevel_max_size,
  unsigned long max_undumped_size,
  unsigned long RECORDS_NUM,
  unsigned long RECORD_SIZE)
{
  static const char* TEST_NAME = "TestLevelProfileMap2";
  //const unsigned long ITERATIONS_NUMBER = 5;

  typedef std::map<StringKey, unsigned long> EtalonMap;

  std::ostringstream map_dir_ostr;
  map_dir_ostr << dir << "/" << TEST_NAME <<
    "-" << rw_buffer_size <<
    "-" << rwlevel_max_size <<
    "-" << max_undumped_size << "/";
  std::string map_dir = map_dir_ostr.str();

  ::system((std::string("rm -rf ") + map_dir).c_str());
  ::system((std::string("mkdir -p ") + map_dir).c_str());

  Generics::ActiveObjectCallback_var active_object_callback(
    new TestCommons::ActiveObjectCallbackStreamImpl(std::cerr, TEST_NAME));

  Generics::Timer timer;
  timer.start();

  EtalonMap etalons;
  Generics::Time max_time;

  {
    ReferenceCounting::SmartPtr<LevelProfileMap<StringKey, StringSerializer> >
      level_profile_map(new LevelProfileMap<StringKey, StringSerializer>(
        active_object_callback,
        map_dir.c_str(),
        "TestLevelProfileMap",
        AdServer::ProfilingCommons::LevelMapTraits(
          AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
          rw_buffer_size,
          rwlevel_max_size,
          max_undumped_size,
          20,
          Generics::Time::ZERO)));
    level_profile_map->activate_object();

    // check latency
    for(unsigned long i = 0; i < RECORDS_NUM; ++i)
    {
      std::ostringstream ostr;
      ostr << "key" << i;
      unsigned char b = Generics::safe_rand() % 255;
      Generics::SmartMemBuf_var mb(new Generics::SmartMemBuf(RECORD_SIZE));
      ::memset(mb->membuf().data(), b, RECORD_SIZE);
      Generics::Time start_time = Generics::Time::get_time_of_day();
      level_profile_map->save_profile(
        StringKey(ostr.str()),
        Generics::transfer_membuf(mb),
        start_time);
      Generics::Time fin_time = Generics::Time::get_time_of_day();
      max_time = std::max(max_time, fin_time - start_time);
      etalons[StringKey(ostr.str())] = b;
    }

    /*
    for(unsigned long j = 0; j < ITERATIONS_NUMBER; ++j)
    {
      for(EtalonMap::iterator it = etalons.begin();
          it != etalons.end(); ++it)
      {
        unsigned char b = Generics::safe_rand() % 256;

        if(false)
        {
          level_profile_map->remove_profile(it->first);
          it->second = 0xFF;
        }
        else
        {
          Generics::SmartMemBuf_var mb(new Generics::SmartMemBuf(RECORD_SIZE));
          ::memset(mb->membuf().data(), b, RECORD_SIZE);
          level_profile_map->save_profile(
            it->first,
            Generics::transfer_membuf(mb),
            now);
          it->second = b;
        }
      }
    }
    */

    level_profile_map->dump();
    level_profile_map->deactivate_object();
    level_profile_map->wait_object();
  }

  timer.stop();
  std::cout << "saving(rw_buffer_size = " << rw_buffer_size <<
    ", rwlevel_max_size = " << rwlevel_max_size <<
    ", max_undumped_size = " << max_undumped_size <<
    ": time = " << timer.elapsed_time() <<
    ", time per record = " << (timer.elapsed_time() / RECORDS_NUM) <<
    ", max save time = " << max_time << std::endl;


  {
    ReferenceCounting::SmartPtr<LevelProfileMap<StringKey, StringSerializer> >
      level_profile_map(new LevelProfileMap<StringKey, StringSerializer>(
        active_object_callback,
        map_dir.c_str(),
        "TestLevelProfileMap",
        AdServer::ProfilingCommons::LevelMapTraits(
          AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
          rw_buffer_size,
          rwlevel_max_size,
          max_undumped_size,
          20,
          Generics::Time::ZERO)));

    for(EtalonMap::iterator it = etalons.begin();
        it != etalons.end(); ++it)
    {
      Generics::ConstSmartMemBuf_var mem_buf =
        level_profile_map->get_profile(it->first);

      if(it->second == 0xFF)
      {
        if(mem_buf.in())
        {
          std::cerr << "error: found remove key '" << it->first << "'" << std::endl;
        }
      }
      else
      {
        if(mem_buf->membuf().size() != RECORD_SIZE)
        {
          std::cerr << "error: incorrect size of key '" << it->first << "'" << std::endl;
        }
        else
        {
          bool invalid = false;
          for(unsigned long data_i = 0;
              data_i < mem_buf->membuf().size();
              ++data_i)
          {
            if(*(mem_buf->membuf().get<unsigned char>() + data_i) != it->second)
            {
              invalid = true;
              break;
            }
          }

          if(invalid)
          {
            std::cerr << "error: incorrect data of key '" << it->first << "'" << std::endl;
          }
        }
      }
    }

    level_profile_map->deactivate_object();
    level_profile_map->wait_object();
  }

  return 0;
}

StringKey make_key(unsigned long i)
{
  std::ostringstream ostr;
  ostr << "key " << i;
  return StringKey(ostr.str());
}

int
test_merge_operation(const char* dir) noexcept
{
  static const char* TEST_NAME = "TestMergeOperation";

  std::ostringstream map_dir_ostr;
  map_dir_ostr << dir << "/" << TEST_NAME << "/";
  const std::string map_dir = map_dir_ostr.str();

  ::system((std::string("rm -rf ") + map_dir).c_str());
  ::system((std::string("mkdir -p ") + map_dir).c_str());
  const unsigned long RECORD_COUNT = 20;

  Generics::ActiveObjectCallback_var active_object_callback(
    new TestCommons::ActiveObjectCallbackStreamImpl(std::cerr, TEST_NAME));

  int res = 0;

  {
    ReferenceCounting::SmartPtr<LevelProfileMap<StringKey, StringSerializer> >
      level_profile_map(new LevelProfileMap<StringKey, StringSerializer>(
        active_object_callback,
        map_dir.c_str(),
        TEST_NAME,
        AdServer::ProfilingCommons::LevelMapTraits(
          AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
          10000,
          10000,
          12000,
          20,
          Generics::Time::ZERO)));
    level_profile_map->activate_object();

    Generics::Time now = Generics::Time::get_time_of_day();
    for(unsigned long i = 0; i < RECORD_COUNT; ++i)
    {
      Generics::SmartMemBuf_var mb(new Generics::SmartMemBuf(1000));
      ::memset(mb->membuf().data(), 0, 1000);
      std::ostringstream ostr;
      ostr << "insert " << i;
      const std::string content = ostr.str();
      ::memcpy(mb->membuf().data(), content.c_str(), content.length());

      level_profile_map->save_profile(
        make_key(i),
        Generics::transfer_membuf(mb),
        now);
    }

    std::cout << "INSERTED" << std::endl;

    for (unsigned long n = 0; n < RECORD_COUNT; ++n)
    {
      for(unsigned long i = n; i < RECORD_COUNT; ++i)
      {
        Generics::SmartMemBuf_var mb(new Generics::SmartMemBuf(1000));
        ::memset(mb->membuf().data(), 0, 1000);
        std::ostringstream ostr;
        ostr << "update " << n << " for " << i;
        const std::string content = ostr.str();
        ::memcpy(mb->membuf().data(), content.c_str(), content.length());

        level_profile_map->save_profile(
          make_key(i),
          Generics::transfer_membuf(mb),
          now);
      }
    }

    std::cout << "UPDATED" << std::endl;

    for(unsigned long i = 0; i < RECORD_COUNT; i += 10)
    {
      level_profile_map->remove_profile(make_key(i));
    }

    std::cout << "REMOVED" << std::endl;

    for(unsigned long i = RECORD_COUNT; i < 10 * RECORD_COUNT; ++i)
    {
      Generics::SmartMemBuf_var mb(new Generics::SmartMemBuf(1000));
      ::memset(mb->membuf().data(), 0, 1000);
      std::ostringstream ostr;
      ostr << "insert " << i;
      const std::string content = ostr.str();
      ::memcpy(mb->membuf().data(), content.c_str(), content.length());

      level_profile_map->save_profile(
        make_key(i),
        Generics::transfer_membuf(mb),
        now);
    }

    for(unsigned long i = 0; i < RECORD_COUNT; ++i)
    {
      const StringKey key = make_key(i);
      Generics::ConstSmartMemBuf_var mb =
        level_profile_map->get_profile(key);

      if (i % 10 == 0)
      {
        if (mb)
        {
          std::cerr << key << " not removed" << std::endl;
        }
      }
      else if (mb)
      {
        const std::string content =
          reinterpret_cast<const char*>(mb->membuf().data());
        std::ostringstream ostr;
        ostr << "update " << i << " for " << i;

        if (content != ostr.str())
        {
          std::cerr << "expected content = '" << ostr.str() << "', actual content = '"
            << content << "'" << std::endl;
        }
      }
      else
      {
        std::cerr << key << " not found" << std::endl;
      }
    }

    std::cout << "CHECKED" << std::endl;

    AdServer::ProfilingCommons::ProfileMap<StringKey>::KeyList keys;
    level_profile_map->copy_keys(keys);
    std::set<std::string> keysSet(keys.begin(), keys.end());

    if (keysSet.find("key 1") == keysSet.end())
    {
      std::cerr << "key 1 isn't found" << std::endl;
    }

    /*if (keysSet.find("key 10") != keysSet.end())
    {
      std::cerr << "key 10 found" << std::endl;
    }

    if (keysSet.find("key 90") != keysSet.end())
    {
      std::cerr << "key 90 found" << std::endl;
    }*/

    std::cout << "FINISHED" << std::endl;

    level_profile_map->deactivate_object();
    level_profile_map->wait_object();
  }

  return res;
}

int
test_write_huge_profile(const char* dir)  noexcept
{
  static const char* TEST_NAME = "TestWriteHugeProfile";
  std::ostringstream map_dir_ostr;
  map_dir_ostr << dir << "/" << TEST_NAME << "/";
  const std::string map_dir = map_dir_ostr.str();

  ::system((std::string("rm -rf ") + map_dir).c_str());
  ::system((std::string("mkdir -p ") + map_dir).c_str());

  Generics::ActiveObjectCallback_var active_object_callback(
    new TestCommons::ActiveObjectCallbackStreamImpl(std::cerr, TEST_NAME));

  ReferenceCounting::SmartPtr<LevelProfileMap<StringKey, StringSerializer> >
    level_profile_map(new LevelProfileMap<StringKey, StringSerializer>(
      active_object_callback,
      map_dir.c_str(),
      TEST_NAME,
      AdServer::ProfilingCommons::LevelMapTraits(
        AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
        10000,
        10000,
        12000,
        20,
        Generics::Time::ZERO)));
  level_profile_map->activate_object();

  try
  {
    Generics::SmartMemBuf_var mb(new Generics::SmartMemBuf(5000000000ul));
    level_profile_map->save_profile(
      StringKey("KEY"),
      Generics::transfer_membuf(mb),
      Generics::Time::get_time_of_day());
  }
  catch (eh::Exception& ex)
  {
    level_profile_map->deactivate_object();
    level_profile_map->wait_object();
    std::cout << ex.what() << std::endl;
    return 0;
  }

  level_profile_map->deactivate_object();
  level_profile_map->wait_object();
  std::cerr << TEST_NAME << " : Exception wasn't thrown" << std::endl;
  return 1;
}

int
test_corrupted_file(const char* dir)  noexcept
{
  static const char* TEST_NAME = "TestCorruptedFile";
  std::ostringstream map_dir_ostr;
  map_dir_ostr << dir << "/" << TEST_NAME << "/";
  const std::string map_dir = map_dir_ostr.str();

  ::system((std::string("rm -rf ") + map_dir).c_str());
  ::system((std::string("mkdir -p ") + map_dir).c_str());

  {
    std::ofstream ofs(map_dir + TEST_NAME + ".00000000.00000000.00429295.data", std::ios::binary);
  }

  {
    std::ofstream ofs(map_dir + TEST_NAME + ".00000000.00000000.00429295.index", std::ios::binary);
    const char buf[256] = { 0 };
    ofs.write(buf, sizeof(buf));
    uint32_t key_size = 100;
    ofs.write(reinterpret_cast<const char*>(&key_size), sizeof(key_size));
  }

  Generics::ActiveObjectCallback_var active_object_callback(
    new TestCommons::ActiveObjectCallbackStreamImpl(std::cerr, TEST_NAME));

  ReferenceCounting::SmartPtr<LevelProfileMap<StringKey, StringSerializer> >
    level_profile_map;

  try
  {
    level_profile_map = new LevelProfileMap<StringKey, StringSerializer>(
      active_object_callback,
      map_dir.c_str(),
      TEST_NAME,
      AdServer::ProfilingCommons::LevelMapTraits(
        AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
        10000,
        10000,
        12000,
        20,
        Generics::Time::ZERO));
    level_profile_map->activate_object();
  }
  catch (eh::Exception& ex)
  {
    std::cout << ex.what() << std::endl;
    return 0;
  }

  if (level_profile_map)
  {
    level_profile_map->deactivate_object();
    level_profile_map->wait_object();
  }

  std::cerr << TEST_NAME << " : Exception wasn't thrown" << std::endl;
  return 1;
}

int
main(int argc, char* argv[]) noexcept
{
  try
  {
    using namespace Generics::AppUtils;

    Generics::AppUtils::Option<std::string> root_path(DEFAULT_ROOT_PATH);
    CheckOption opt_help;

    Args args;
    args.add(equal_name("path") || short_name("p"), root_path);
    args.add(equal_name("help") || short_name("h"), opt_help);

    args.parse(argc - 1, argv + 1);

    if (opt_help.enabled())
    {
      std::cout << USAGE << std::endl;
      return false;
    }

    int res = 0;

    res += test1((*root_path + "/test1").c_str(), 1024, 100*1024*1024);
    res += simple_levels_test(root_path->c_str());
    res += merge_test(root_path->c_str());
    res += test_level_profile_map(
      root_path->c_str(), 4 * 4 * 1024, 10 * 1024, 100 * 1024, 20, 100000, 100);
    res += test_level_profile_map(
      root_path->c_str(), 4 * 4 * 1024, 100 * 1024, 2 * 100 * 1024, 20, 10000, 100);
    res += test_level_profile_map(
      root_path->c_str(), 4 * 4 * 1024, 1024 * 1024, 2 * 1024 * 1024, 20, 10000, 100);
    res += test_level_profile_map(
      root_path->c_str(), 4 * 4 * 1024, 20 * 1024 * 1024, 2 * 20 * 1024 * 1024, 20, 10000, 100);
    res += test_merge_operation(root_path->c_str());

    res += test_level_profile_latency(
      root_path->c_str(),
      16 * 1024 * 1024,
      104857600, // 100 Mb
      262144000, // 250 Mb
      1000000, // record number
      1000 // record size
      );
    res += test_write_huge_profile(root_path->c_str());
    res += test_corrupted_file(root_path->c_str());

    return res;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }
  return -1;
}
