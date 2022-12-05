#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <Commons/PathManip.hpp>
#include <Generics/AppUtils.hpp>
#include <TestCommons/ActiveObjectCallback.hpp>
#include <ProfilingCommons/PlainStorageAdapters.hpp>

#include <ProfilingCommons/PlainStorage3/FileReader.hpp>
#include <ProfilingCommons/PlainStorage3/FileWriter.hpp>
#include <ProfilingCommons/PlainStorage3/FileLevel.hpp>
#include <ProfilingCommons/PlainStorage3/ReadMemLevel.hpp>
#include <ProfilingCommons/PlainStorage3/RWMemLevel.hpp>
#include <ProfilingCommons/PlainStorage3/MergeIterator.hpp>
#include <ProfilingCommons/PlainStorage3/LevelProfileMap.hpp>
#include <ProfilingCommons/PlainStorage3/MergeIterator.hpp>
#include <TestCommons/ActiveObjectCallback.hpp>

namespace
{
  const char USAGE[] =
    "LevelCheckUtil [OPTIONS]\n"
    "OPTIONS:\n"
    "  -h, --help : show this message.\n"
    "LevelCheckUtil print-keys <index file> <body file>\n"
    "LevelCheckUtil [OPTIONS] divide-to-chunks <source path with prefix> <chunks destination path prefix>\n"
    "OPTIONS:\n"
    "  --chunks-number : number of chunks to divide \n"
    "  --rw-level-max-size : rwlevel max size\n"
    "  --max-undumped-size : max undumped size\n"
    "  --max-levels0 : max levels0 count"
    "LevelCheckUtil [OPTIONS] merge <source path with prefix>[ <source path with prefix>...] <chunks destination path with prefix>\n"
    "OPTIONS:\n"
    "  --chunks-number : number of chunks to divide \n"
    "  --rw-level-max-size : rwlevel max size\n"
    "  --max-undumped-size : max undumped size\n"
    "  --max-levels0 : max levels0 count";
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

typedef AdServer::ProfilingCommons::LevelProfileMap<StringKey, StringSerializer> ProfileMap;
typedef AdServer::ProfilingCommons::ReadFileLevel<StringKey, StringSerializer> ReadFileLevel;

ReferenceCounting::SmartPtr<ReadFileLevel>
open_file_level(
  const char* index_file_name,
  const char* body_file_name)
{
  return new ReadFileLevel(
    index_file_name,
    body_file_name,
    10*1024*1024,
    true // disable caching
    );
}

void
print_keys(
  std::ostream& ostr,
  ReadFileLevel::Iterator* it)
{
  StringKey key;
  AdServer::ProfilingCommons::ProfileOperation operation;
  Generics::Time access_time;
  while(it->get_next(key, operation, access_time))
  {
    ostr << key << std::endl;
  }
}

void
divide_to_chunks(
  const std::string& source_base_path,
  const std::string& dst_path_prefix,
  unsigned long chunks_number,
  unsigned long rw_buffer_size,
  unsigned long rwlevel_max_size,
  unsigned long max_undumped_size,
  unsigned long max_levels0)
{
  try
  {
    std::string source_path;
    std::string prefix;
    AdServer::PathManip::split_path(source_base_path.c_str(), &source_path, &prefix, false);
    ReferenceCounting::SmartPtr<ProfileMap> source_map =
      new ProfileMap(
        Generics::ActiveObjectCallback_var(
          new TestCommons::ActiveObjectCallbackStreamImpl(std::cerr, "LevelCheckUtil")),
        source_path.c_str(),
        prefix.c_str(),
        AdServer::ProfilingCommons::LevelMapTraits(
          AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
          rw_buffer_size,
          rwlevel_max_size,
          max_undumped_size,
          max_levels0,
          Generics::Time::ZERO));

    std::map<unsigned long, ReferenceCounting::SmartPtr<ProfileMap> > destination_chunks;
    Generics::Time now = Generics::Time::get_time_of_day();

    ProfileMap::KeyList key_list;
    source_map->copy_keys(key_list);
    for (auto iter = key_list.begin(); iter != key_list.end(); ++iter)
    {
      Generics::ConstSmartMemBuf_var profile = source_map->get_profile(*iter);
      if (profile.in())
      {
        unsigned long hash = Generics::CRC::quick(0, iter->data(), iter->size());
        unsigned long chunk_num = hash % chunks_number;

        if (!destination_chunks[chunk_num].in())
        {
          std::ostringstream ostr;
          ostr << dst_path_prefix << chunk_num;
          std::string destination_map_path = ostr.str();

          mkdir(destination_map_path.c_str(), 0777);

          ReferenceCounting::SmartPtr<ProfileMap> map =
            new ProfileMap(
              Generics::ActiveObjectCallback_var(
                new TestCommons::ActiveObjectCallbackStreamImpl(std::cerr, "LevelCheckUtil")),
              destination_map_path.c_str(),
              prefix.c_str(),
              AdServer::ProfilingCommons::LevelMapTraits(
                AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
                rw_buffer_size,
                rwlevel_max_size,
                max_undumped_size,
                max_levels0,
                Generics::Time::ZERO));

          map->activate_object();

          destination_chunks[chunk_num] = map;
        }

        destination_chunks[chunk_num]->save_profile(*iter, profile, now);
      }
    }

    for (unsigned long chunk_num = 0; chunk_num < chunks_number; ++chunk_num)
    {
      if (destination_chunks[chunk_num].in())
      {
        destination_chunks[chunk_num]->deactivate_object();
        destination_chunks[chunk_num]->wait_object();
      }
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Can't divide to chunks, an exception has been caught: " <<
      ex.what() << std::endl;
  }
}

void merge(
  const std::vector<std::string>& paths_to_merge,
  const std::string destination_path,
  unsigned long rw_buffer_size,
  unsigned long rwlevel_max_size,
  unsigned long max_undumped_size,
  unsigned long max_levels0)
{
  try
  {
    std::string destination_path_base;
    std::string destination_path_prefix;
    AdServer::PathManip::split_path(
      destination_path.c_str(),
      &destination_path_base,
      &destination_path_prefix,
      false);

    mkdir(destination_path_base.c_str(), 0777);

    ReferenceCounting::SmartPtr<ProfileMap> destination_map =
      new ProfileMap(
        Generics::ActiveObjectCallback_var(
          new TestCommons::ActiveObjectCallbackStreamImpl(std::cerr, "LevelCheckUtil")),
        destination_path_base.c_str(),
        destination_path_prefix.c_str(),
        AdServer::ProfilingCommons::LevelMapTraits(
          AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
          rw_buffer_size,
          rwlevel_max_size,
          max_undumped_size,
          max_levels0,
          Generics::Time::ZERO));
    destination_map->activate_object();

    std::string source_path;
    std::string source_prefix;
    Generics::Time now = Generics::Time::get_time_of_day();
    for (auto iter = paths_to_merge.begin(); iter != paths_to_merge.end(); ++iter)
    {
      AdServer::PathManip::split_path(
        iter->c_str(),
        &source_path,
        &source_prefix,
        false);

      ReferenceCounting::SmartPtr<ProfileMap> source_map =
        new ProfileMap(
          Generics::ActiveObjectCallback_var(
            new TestCommons::ActiveObjectCallbackStreamImpl(std::cerr, "LevelCheckUtil")),
          source_path.c_str(),
          source_prefix.c_str(),
          AdServer::ProfilingCommons::LevelMapTraits(
            AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
            rw_buffer_size,
            rwlevel_max_size,
            max_undumped_size,
            max_levels0,
            Generics::Time::ZERO));

      ProfileMap::KeyList key_list;
      source_map->copy_keys(key_list);
      for (auto iter = key_list.begin(); iter != key_list.end(); ++iter)
      {
        Generics::ConstSmartMemBuf_var profile = source_map->get_profile(*iter);
        destination_map->save_profile(*iter, profile, now);
      }
    }

    destination_map->deactivate_object();
    destination_map->wait_object();
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Can't merge, an exception has been caught: " <<
      ex.what() << std::endl;
  }
}

int
main(int argc, char* argv[]) noexcept
{
  try
  {
    using namespace Generics::AppUtils;

    CheckOption opt_help;
    Option<unsigned long> opt_chunks_number(32);
    Option<unsigned long> opt_rw_level_max_size(104857600);
    Option<unsigned long> opt_max_undumped_size(262144000);
    Option<unsigned long> max_levels0(20);
    Option<unsigned long> opt_expire_time(31536000); // year by default

    Args args(-1);
    args.add(equal_name("help") || short_name("h"), opt_help);
    args.add(equal_name("chunks-number"), opt_chunks_number);
    args.add(equal_name("rw-level-max-size"), opt_rw_level_max_size);
    args.add(equal_name("max-undumped-size"), opt_max_undumped_size);
    args.add(equal_name("max-levels0"), max_levels0);
    args.add(equal_name("expire-time"), opt_expire_time);

    args.parse(argc - 1, argv + 1);

    const Generics::AppUtils::Args::CommandList& commands = args.commands();

    if(commands.empty() || opt_help.enabled())
    {
      std::cout << USAGE << std::endl;
      return 1;
    }

    std::string command = *commands.begin();
    if(command == "print-keys" || command == "clear-expired")
    {
      if(commands.size() < 3)
      {
        std::cerr << "invalid number of arguments for " << command << std::endl;
        return 1;
      }

      Generics::AppUtils::Args::CommandList::const_iterator command_it = ++commands.begin();
      std::string index_file = *command_it++;
      std::string body_file = *command_it++;

      ReferenceCounting::SmartPtr<ReadFileLevel>
        src_file_level = open_file_level(
          index_file.c_str(),
          body_file.c_str());

      AdServer::ProfilingCommons::ReadBaseLevel<StringKey>::Iterator_var src_it =
        src_file_level->get_iterator(10000);

      if(command == "print-keys")
      {
        print_keys(std::cout, src_it);
      }
      else if(command == "clear-expired")
      {
        if(commands.size() < 5)
        {
          std::cerr << "invalid number of arguments for " << command << std::endl;
          return 1;
        }

        std::string result_index_file = *command_it++;
        std::string result_body_file = *command_it++;

        AdServer::ProfilingCommons::ReadBaseLevel<StringKey>::Iterator_var it =
          new AdServer::ProfilingCommons::AccessTimeFilterIterator<StringKey>(
            src_it,
            Generics::Time::get_time_of_day() - *opt_expire_time);

        ReferenceCounting::SmartPtr<ReadFileLevel> result_file_level =
          new ReadFileLevel(
            result_index_file.c_str(),
            result_body_file.c_str(),
            it,
            100000,
            false, // disable caching
            0);
      }
    }
    else if(command == "divide-to-chunks")
    {
      if(commands.size() != 3)
      {
        std::cerr << "source path and destination path prefix should be specified." << std::endl;
        return 1;
      }
      Generics::AppUtils::Args::CommandList::const_iterator cit =
        ++commands.begin();
      std::string source_psth = *cit++;
      std::string destination_path = *cit;
      divide_to_chunks(
        source_psth,
        destination_path,
        *opt_chunks_number,
        104857600,
        *opt_rw_level_max_size,
        *opt_max_undumped_size,
        *max_levels0);
    }
    else if(command == "merge")
    {
      if(commands.size() < 3)
      {
        std::cerr << "source paths and destination path should be specified." << std::endl;
        return 1;
      }

      Generics::AppUtils::Args::CommandList::const_iterator cit =
        ++commands.begin();
      Generics::AppUtils::Args::CommandList::const_iterator sources_end =
        --commands.end();

      std::vector<std::string> source_psths;
      for (; cit != sources_end; ++cit)
      {
        source_psths.push_back(*cit);
      }

      std::string destination_path = *sources_end;

      merge(
        source_psths,
        destination_path,
        104857600,
        *opt_rw_level_max_size,
        *opt_max_undumped_size,
        *max_levels0);
    }

    return 0;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }

  return 1;
}
