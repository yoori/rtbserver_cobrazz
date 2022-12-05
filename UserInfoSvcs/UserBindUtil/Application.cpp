#include <sys/stat.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include <Generics/AppUtils.hpp>
#include <Generics/Time.hpp>
#include <Generics/DirSelector.hpp>

#include <LogCommons/LogCommons.hpp>
#include <ProfilingCommons/PlainStorage3/FileReader.hpp>
#include <ProfilingCommons/PlainStorage3/FileWriter.hpp>
#include <Commons/PathManip.hpp>

namespace
{
  const char USAGE[] =
    "UserBindUtil [OPTIONS] divide-to-chunks <source path file> <result chunk folders prefix>\n"
    "UserBindUtil [OPTIONS] merge <destination path file> <result chunk folders prefix>\n"
    "OPTIONS:\n"
    "  -h, --help : show this message.\n"
    "  --chunks-number : number of chunks to divide\n"
    "  --time-period-number : number of time periods for merge\n";
}

namespace
{
  const char USER_BIND_SUFFIX_TIME_FORMAT[] = "%Y%m%d.%H%M%S";

  struct ResultChunkFile: public ReferenceCounting::AtomicImpl
  {
    std::string file_path;
    std::ofstream file;

  protected:
    virtual ~ResultChunkFile() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<ResultChunkFile> ResultChunkFile_var;

  typedef std::list<std::string> StringList;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  enum HashType
  {
    HT_STRING,
    HT_VALUE
  };
}

void
divide_file_to_chunks(
  HashType hash_type,
  const char* source_file_path,
  const char* dst_path_prefix,
  unsigned long result_chunks_number)
{
  try
  {
    std::map<unsigned long, ResultChunkFile_var> destination_chunks;

    std::string source_file_name;
    AdServer::PathManip::split_path(
      source_file_path, 0, &source_file_name, false);

    std::ifstream source_file(source_file_path, std::ios_base::in);

    if(!source_file.is_open())
    {
      Stream::Error ostr;
      ostr << "Can't open source file '" << source_file_path << "'";
      throw Exception(ostr);
    }

    while(!source_file.eof())
    {
      AdServer::LogProcessing::SpacesString external_id;
      source_file >> external_id;
      std::string res_line;
      std::getline(source_file, res_line);

      size_t distrib_hash;

      if(hash_type == HT_VALUE)
      {
        if(!String::StringManip::str_to_int(
          external_id,
          distrib_hash))
        {
          Stream::Error ostr;
          ostr << " Can't convert hash value to number: '" << external_id << "'";
          throw Exception(ostr);          
        }
      }
      else
      {
        // determine result chunk
        distrib_hash =
          AdServer::Commons::external_id_distribution_hash(external_id);
      }

      unsigned long chunk_id = distrib_hash % result_chunks_number;
      ResultChunkFile_var& dest_file = destination_chunks[chunk_id];
      if(!dest_file.in())
      {
        std::ostringstream dest_dir;
        dest_dir << dst_path_prefix << chunk_id;
        ::mkdir(dest_dir.str().c_str(), 0777);
        dest_file = new ResultChunkFile();
        std::string target_file_path = dest_dir.str() + "/" + source_file_name;
        dest_file->file_path = target_file_path;
        dest_file->file.open(
          target_file_path.c_str(),
          std::ios_base::out | std::ios_base::app);
        if(!dest_file->file.is_open())
        {
          Stream::Error ostr;
          ostr << "Can't open target file '" << target_file_path << "'";
          throw Exception(ostr);
        }
      }
      else
      {
        dest_file->file << "\n";
      }

      dest_file->file << external_id << res_line;
      if(dest_file->file.fail())
      {
        Stream::Error ostr;
        ostr << "Can't write data to target file '" << dest_file->file_path << "'";
        throw Exception(ostr);
      }
    }

    for(std::map<unsigned long, ResultChunkFile_var>::iterator chunk_it =
          destination_chunks.begin();
        chunk_it != destination_chunks.end(); ++chunk_it)
    {
      chunk_it->second->file.close();
      if(chunk_it->second->file.fail())
      {
        Stream::Error ostr;
        ostr << "Can't write data to target file '" <<
          chunk_it->second->file_path << "'";
        throw Exception(ostr);
      }
    }
  }
  catch(const eh::Exception&)
  {
    throw;
  }
}

class DivideFileFetcher
{
public:
  DivideFileFetcher(
    HashType hash_type,
    const char* file_prefix,
    const char* dst_path_prefix,
    unsigned long result_chunks_number)
    : hash_type_(hash_type),
      file_prefix_(file_prefix),
      dst_path_prefix_(dst_path_prefix),
      result_chunks_number_(result_chunks_number),
      divided_(false)
  {}

  bool
  operator ()(const char* full_path, const struct stat&)
  {
    String::SubString file_name(
      Generics::DirSelect::file_name(full_path));
    if(file_name.compare(0, file_prefix_.length(), file_prefix_) == 0)
    {
      divide_file_to_chunks(
        hash_type_,
        full_path,
        dst_path_prefix_.c_str(),
        result_chunks_number_);
      divided_ = true;
    }

    return false;
  }

  bool
  divided() const noexcept
  {
    return divided_;
  }

protected:
  const HashType hash_type_;
  const std::string file_prefix_;
  const std::string dst_path_prefix_;
  unsigned long result_chunks_number_;
  bool divided_;
};

void
divide_to_chunks(
  HashType hash_type,
  const char* source_file_path,
  const char* dst_path_prefix,
  unsigned long result_chunks_number)
{
  // fetch all files by mask <source_file_path>*

  std::string source_file_dir;
  std::string source_file_prefix;
  AdServer::PathManip::split_path(
    source_file_path, &source_file_dir, &source_file_prefix, false);

  DivideFileFetcher file_fetcher(
    hash_type,
    source_file_prefix.c_str(),
    dst_path_prefix,
    result_chunks_number);

  Generics::DirSelect::directory_selector(
    source_file_dir.c_str(),
    file_fetcher,
    "*",
    Generics::DirSelect::DSF_NON_RECURSIVE |
      Generics::DirSelect::DSF_REGULAR_ONLY);

  if(!file_fetcher.divided())
  {
    Stream::Error ostr;
    ostr << "No files with by mask: '" << source_file_path << "'";
    throw Exception(ostr);
  }
}

class MaxTimeSelector
{
public:
  MaxTimeSelector(const char* file_prefix)
    : file_prefix_(std::string(file_prefix) + ".")
  {}

  bool
  operator ()(const char* full_path, const struct stat&)
    noexcept
  {
    String::SubString file_name(
      Generics::DirSelect::file_name(full_path));
    if(file_name.compare(0, file_prefix_.length(), file_prefix_) == 0)
    {
      try
      {
        Generics::Time time(
          file_name.substr(file_prefix_.length()),
          USER_BIND_SUFFIX_TIME_FORMAT);
        max_time_ = std::max(max_time_, time);
      }
      catch(const eh::Exception& ex)
      {}
    }

    return false;
  }

  Generics::Time
  max_time() const
  {
    return max_time_;
  }

private:
  const std::string file_prefix_;
  Generics::Time max_time_;
};

class MergeSelector
{
public:
  MergeSelector(
    const char* result_dir,
    const char* file_prefix)
    : result_dir_(result_dir),
      file_prefix_(std::string(file_prefix) + "."),
      merged_(false)
  {}

  bool
  operator ()(const char* full_path, const struct stat&)
    noexcept
  {
    unsigned long BUF_SIZE = 10*1024*1024;
    String::SubString file_name(
      Generics::DirSelect::file_name(full_path));
    if(file_name.compare(0, file_prefix_.length(), file_prefix_) == 0)
    {
      merged_ = true;

      std::string result_file = result_dir_ + "/" + file_name.str();

      Generics::ArrayAutoPtr<unsigned char> buf(BUF_SIZE);
      AdServer::ProfilingCommons::FileReader file_reader(full_path, 1024*1024);
      bool write_eol = false;
      if(::access(result_file.c_str(), F_OK) != 0)
      {
        if(errno != ENOENT)
        {
          Stream::Error ostr;
          ostr << "No access to result file '" << result_file << "'";
          throw Exception(ostr);
        }
      }
      else
      {
        write_eol = true;
      }

      AdServer::ProfilingCommons::FileWriter file_writer(
        result_file.c_str(), 1024*1024, true);
      while(!file_reader.eof())
      {
        unsigned long read_size = file_reader.read(buf.get(), BUF_SIZE);
        if(read_size)
        {
          if(write_eol)
          {
            file_writer.write("\n", 1);
            write_eol = false;
          }
          file_writer.write(buf.get(), read_size);
        }
      }
      file_writer.close(); // throw exception if flush failed
    }

    return false;
  }

  bool
  merged() const noexcept
  {
    return merged_;
  }

private:
  const std::string result_dir_;
  const std::string file_prefix_;
  bool merged_;
};

void
merge(const char* destination_dir, const StringList& src_files)
{
  for(StringList::const_iterator src_file_it = src_files.begin();
      src_file_it != src_files.end(); ++src_file_it)
  {
    std::string source_file_dir;
    std::string source_file_prefix;
    AdServer::PathManip::split_path(
      src_file_it->c_str(),
      &source_file_dir,
      &source_file_prefix,
      false);

    MergeSelector merge_selector(
      destination_dir, source_file_prefix.c_str());

    Generics::DirSelect::directory_selector(
      source_file_dir.c_str(),
      merge_selector,
      "*",
      Generics::DirSelect::DSF_NON_RECURSIVE |
        Generics::DirSelect::DSF_REGULAR_ONLY);

    if(!merge_selector.merged())
    {
      Stream::Error ostr;
      ostr << "No files with by mask: '" << *src_file_it << "'";
      throw Exception(ostr);
    }
  }
}

int
main(int argc, char* argv[]) 
{
  int result = 0;

  try
  {
    Generics::AppUtils::CheckOption opt_help;
    Generics::AppUtils::CheckOption opt_print_content;
    Generics::AppUtils::Option<unsigned long> opt_chunks_number;
    Generics::AppUtils::StringOption opt_hash_type;
    Generics::AppUtils::Args args(-1);

    args.add(
      Generics::AppUtils::equal_name("help") ||
      Generics::AppUtils::short_name("h"),
      opt_help);
    args.add(
      Generics::AppUtils::equal_name("chunks-number"),
      opt_chunks_number);
    args.add(
      Generics::AppUtils::equal_name("hash-type"),
      opt_hash_type);

    args.parse(argc - 1, argv + 1);

    const Generics::AppUtils::Args::CommandList& commands = args.commands();

    if(commands.empty() || opt_help.enabled())
    {
      std::cout << USAGE << std::endl;
      return 1;
    }

    Generics::AppUtils::Args::CommandList::const_iterator command_it =
      commands.begin();

    std::string command = *command_it++;

    if(command == "divide-to-chunks")
    {
      if(command_it == commands.end())
      {
        std::cout << "source file not defined. See usage: " << std::endl <<
          USAGE << std::endl;
      }

      std::string source_file = *command_it++;

      if(command_it == commands.end())
      {
        std::cout << "destination file not defined. See usage: " << std::endl <<
          USAGE << std::endl;
      }

      std::string dest_file = *command_it++;

      if(!opt_chunks_number.installed())
      {
        std::cout << "chunks-number is required for divide-to-chunks" <<
          std::endl;        
      }

      HashType hash_type = HT_STRING;
      if(opt_hash_type.installed())
      {
        if(*opt_hash_type == "string")
        {
          hash_type = HT_STRING;
        }
        else if(*opt_hash_type == "value")
        {
          hash_type = HT_VALUE;
        }
      }

      divide_to_chunks(
        hash_type,
        source_file.c_str(),
        dest_file.c_str(),
        *opt_chunks_number);
    }
    else if(command == "merge")
    {
      if(command_it == commands.end())
      {
        std::cout << "source file not defined. See usage: " << std::endl <<
          USAGE << std::endl;
      }

      std::string destination_dir = *command_it++;

      if(command_it == commands.end())
      {
        std::cout << "destination file not defined. See usage: " << std::endl <<
          USAGE << std::endl;
      }

      StringList src_files;
      while(command_it != commands.end())
      {
        src_files.push_back(*command_it);
        ++command_it;
      }

      if(src_files.empty())
      {
        std::cout << "source file not defined. See usage: " << std::endl <<
          USAGE << std::endl;
      }

      merge(destination_dir.c_str(), src_files);
    }
    else
    {
      std::cerr << "unknown command" << std::endl;
      result = -1;
    }
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
    result = -1;
  }

  return result;
}
