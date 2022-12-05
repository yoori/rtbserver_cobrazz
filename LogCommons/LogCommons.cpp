
#include <LogCommons/LogCommons.ipp>

#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>

#include <Generics/DirSelector.hpp>
#include <Generics/Function.hpp>
#include <String/StringManip.hpp>


namespace AdServer {
namespace LogProcessing {

const Generics::Time ONE_DAY = Generics::Time::ONE_DAY;
const Generics::Time ONE_MONTH = ONE_DAY * 31;

const std::size_t HitsFilter::FILE_VERSION_LEN_ = 12;
const std::size_t HitsFilter::TABLE_SIZE_LEN_ = 8;

const char HitsFilter::CURRENT_FILE_VERSION_[FILE_VERSION_LEN_] = "3.2.0";

const char HitsFilter::EOF_MARKER_[4] = { '.', 'E', 'O', 'F' };

namespace {

struct Aux_Randomizer
{
  Aux_Randomizer() { srandom(time(0)); }
} aux_randomizer;

const mode_t DIR_PERMS = 0777;

const char *const FILE_STORE_CUR_DIR = "Current";

void
make_log_file_name_V_2_6(
  std::ostream& os,
  const LogFileNameInfo &info,
  long int rand_value)
{
  static const char DATE_FMT[] = "%Y%m%d.%H%M%S.%q";
  const std::string& date = info.timestamp.get_gm_time().format(DATE_FMT);
  os << '.' << date
      << '.' << std::setfill('0') << std::setw(8) << rand_value
      << '.' << info.distrib_count << '.' << info.distrib_index;

  if (info.processed_lines_count)
  {
    os << '.' << info.processed_lines_count;
  }
}

} // namespace

const char Aux_::EscapeChar::ESC_CHAR = '^';

// Table to escape for ",:=^" and spaces characters
const char
  Aux_::ConvertSpacesSeparators::WRITE_RESERVED_CHAR_TABLE[128][2] =
{
  {'0', '0'}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {'0', '9'}, {'0', 'A'}, {'0', 'B'}, {'0', 'C'}, {'0', 'D'},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {'2', '0'}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {'2', 'C'}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {'3', 'A'}, {0, 0}, {0, 0}, {'3', 'D'}, {0, 0},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {0, 0}, {'5', 'E'},
};

// Table to escape for "^" and spaces characters
const char
  Aux_::ConvertSpaces::WRITE_RESERVED_CHAR_TABLE[128][2] =
{
  {'0', '0'}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {'0', '9'}, {'0', 'A'}, {'0', 'B'}, {'0', 'C'}, {'0', 'D'},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {'2', '0'}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
  {0, 0}, {0, 0}, {0, 0}, {'5', 'E'},
};

const String::AsciiStringManip::CharCategory Aux_::VALID_USER_STATUSES("BFINOPU");

// FIXME: check
void
parse_string_list(
  const String::SubString& str,
  StringList& list,
  char sep,
  const char* empty
)
{
  list.clear();
  if (str != empty)
  {
    std::size_t start = 0;
    for (std::size_t i = 0; i < str.size(); ++i)
    {
      if (str[i] == sep)
      {
        std::string tmp;
        str.substr(start, i - start).assign_to(tmp);
        list.push_back(std::move(tmp));
        start = i + 1;
      }
    }
    if (start < str.size())
    {
      std::string tmp;
      str.substr(start, str.size() - start).assign_to(tmp);
      list.push_back(std::move(tmp));
    }
  }
}

template <size_t PRECISION_>
FixedPrecisionDouble<PRECISION_>::Format::Format()
{
  Stream::Buffer<sizeof(format)> ostr(format);
  ostr << '%' << '.' << PRECISION_ << 'f';
}

template <size_t PRECISION_>
const typename FixedPrecisionDouble<PRECISION_>::Format
  FixedPrecisionDouble<PRECISION_>::PRINT_;

template <size_t PRECISION_>
const double FixedPrecisionDouble<PRECISION_>::N_ =
  std::pow(10., static_cast<double>(PRECISION_));

template class FixedPrecisionDouble<5>;

inline long int get_four_digit_random_value()
{
  return static_cast<long int>(9999. * (random() / (RAND_MAX + 1.))) + 1;
}

inline long int get_eight_digit_random_value()
{
  return static_cast<long int>(99999999. * (random() / (RAND_MAX + 1.))) + 1;
}

class CsvIndexGenerator: public SafeSequenceGenerator
{
public:
  CsvIndexGenerator(): SafeSequenceGenerator(1, 999999) {}
};

typedef Generics::Singleton<CsvIndexGenerator> CsvIndexGeneratorSingleton;

inline
void
generate_log_file_name(const LogFileNameInfo &info, std::string &file_name)
{
  std::ostringstream oss(info.base_name, std::ios_base::ate);
  switch (info.format)
  {
    case LogFileNameInfo::LFNF_CSV:
      {
        static const char DATE_FMT[] = "%Y%m%d%H%M%S-%q";
        const std::string& date = info.timestamp.get_gm_time().format(DATE_FMT);
        oss << '_' << date << "-";
        oss.width(6);
        oss.fill('0');
        oss << CsvIndexGeneratorSingleton::instance().get() << ".csv";
      }
      break;
    case LogFileNameInfo::LFNF_BASIC:
      {
        static const char DATE_FMT[] = "%Y%m%d";
        const std::string& date = info.timestamp.get_gm_time().format(DATE_FMT);
        const long int rand_value = get_eight_digit_random_value();
        oss << ".log_" << info.distrib_index << ".1_"
            << date << '.' << info.timestamp.tv_sec << '.'
            << std::setfill('0') << std::setw(8) << rand_value;
      }
      break;
    case LogFileNameInfo::LFNF_EXTENDED:
      {
        static const char DATE_FMT[] = "%Y%m%d.%H%M%S.%q";
        const std::string& date = info.timestamp.get_gm_time().format(DATE_FMT);
        const long int rand_value = get_eight_digit_random_value();
        oss << ".log_" << info.distrib_index << ".0_"
            << date << '.' << std::setfill('0')
            << std::setw(8) << rand_value;
      }
      break;
    case LogFileNameInfo::LFNF_V_2_6:
      {
        make_log_file_name_V_2_6(oss, info, get_eight_digit_random_value());
      }
      break;
    default:
      throw InvalidArgValue("generate_log_file_name: Unsupported log file "
                            "name format requested");
  }
  oss.str().swap(file_name);
}

std::string
restore_log_file_name(
  const LogFileNameInfo& info,
  const std::string& out_dir_name)
  /*throw(InvalidArgValue, eh::Exception)*/
{
  std::ostringstream oss;

  if (!out_dir_name.empty())
  {
    oss << out_dir_name << "/";
  }

  oss << info.base_name;

  if (info.format == LogFileNameInfo::LFNF_V_2_6)
  {
    make_log_file_name_V_2_6(oss, info, info.random);
  }
  else
  {
    throw InvalidArgValue(
      "restore_log_file_name: Unsupported log file name format requested");
  }

  return oss.str();
}

std::string
make_log_file_name(
  const LogFileNameInfo &info,
  const std::string &out_dir_name
)
{
  while (1)
  {
    std::string file_name;
    generate_log_file_name(info, file_name);
    file_name.insert(0, "/");
    file_name.insert(0, out_dir_name);
    int fd = open(file_name.c_str(), O_RDONLY, 0);
    if (fd == -1 && errno == ENOENT)
    {
      return file_name;
    }
    if (fd != -1)
    {
      close(fd);
    }
  }
  return std::string(); // suppress the compiler warning
}

StringPair
make_log_file_name_pair(
  const LogFileNameInfo &info,
  const std::string &out_dir_name
)
{
  while (1)
  {
    std::string file_name;
    generate_log_file_name(info, file_name);
    std::string tmp_file_name(file_name);
    file_name.insert(0, "/");
    file_name.insert(0, out_dir_name);
    tmp_file_name.insert(0, "/.$tmp_");
    tmp_file_name.insert(0, out_dir_name);
    tmp_file_name += ".tmp";
    int fd1 = open(file_name.c_str(), O_RDONLY, 0);
    int prev_errno = errno;
    int fd2 = open(tmp_file_name.c_str(), O_RDONLY, 0);
    if (fd1 == -1 && prev_errno == ENOENT && fd2 == -1 && errno == ENOENT)
    {
      return StringPair(file_name, tmp_file_name);
    }
    if (fd1 != -1)
    {
      close(fd1);
    }
    if (fd2 != -1)
    {
      close(fd2);
    }
  }
  return StringPair(); // suppress the compiler warning
}

// Log file names examples:
//
// FORMAT   | Filename
// ---------------------------------------------------------------
// CSV      | CreativeStat_20120831102438-736496.csv
// BASIC    | CreativeStat.log_0.1_20120831.1346408678.06392898
// EXTENDED | CreativeStat.log_0.0_20120831.102438.736496.29437336
// V_2_6    | CreativeStat.20120831.102438.736496.43242548.1.0

void
parse_log_file_name(const std::string& log_file_name, LogFileNameInfo& info)
  /*throw(InvalidLogFileNameFormat)*/
{
  typedef String::SubString SubString;
  using String::StringManip::str_to_int;

  SubString fname = log_file_name;
  SubString::SizeType f_d_pos = fname.find_first_of('.');
  if (!f_d_pos || f_d_pos == SubString::NPOS || (fname.size() - f_d_pos) < 4)
  {
    info = LogFileNameInfo();
    throw InvalidLogFileNameFormat(log_file_name);
  }
#if __i386__ || __x86_64__ // Little-endian
  static const uint32_t u32_csv = 0x7673632e; // ".csv" (little-endian)
  static const uint32_t u32_log = 0x676f6c2e; // ".log" (little-endian)
  const uint32_t u32_cond =
    *reinterpret_cast<const uint32_t*>(&fname[f_d_pos]);
  if (u32_cond == u32_csv) // CSV
#else // Byte order is unknown
  SubString str_cond = fname.substr(f_d_pos + 1, 3);
  if (str_cond == "csv") // CSV
#endif
  {
    SubString::SizeType u_pos = f_d_pos - 29;
    if (fname[u_pos] != '_')
    {
      info = LogFileNameInfo();
      throw InvalidLogFileNameFormat(log_file_name);
    }
    SubString::SizeType m_pos = f_d_pos - 14;
    if (fname[m_pos] != '-')
    {
      info = LogFileNameInfo();
      throw InvalidLogFileNameFormat(log_file_name);
    }
    SubString::SizeType r_pos = f_d_pos - 7;
    if (fname[r_pos] != '-')
    {
      info = LogFileNameInfo();
      throw InvalidLogFileNameFormat(log_file_name);
    }
    info.base_name = fname.substr(0, u_pos).str();
    SubString tstamp = fname.substr(u_pos + 1, r_pos - u_pos - 1);
    try
    {
      info.timestamp.set(tstamp, "%Y%m%d%H%M%S-%q", true);
    }
    catch (...)
    {
      info = LogFileNameInfo();
      throw InvalidLogFileNameFormat(log_file_name);
    }
    SubString rnd = fname.substr(r_pos + 1, f_d_pos - r_pos - 1);
    if (!str_to_int(rnd, info.random))
    {
      info = LogFileNameInfo();
      throw InvalidLogFileNameFormat(log_file_name);
    }



    info.distrib_count = 0;
    info.distrib_index = 0;
    info.format = LogFileNameInfo::LFNF_CSV;
  }
#if __i386__ || __x86_64__ // Little-endian
  else if (u32_cond == u32_log) // BASIC or EXTENDED
#else // Byte order is unknown
  else if (str_cond == "log") // BASIC or EXTENDED
#endif
  {
    SubString::SizeType u_pos = f_d_pos + 4;
    if (fname[u_pos] != '_' || !std::isdigit(fname[u_pos + 1]))
    {
      info = LogFileNameInfo();
      throw InvalidLogFileNameFormat(log_file_name);
    }
    SubString tmp_substr = fname.substr(u_pos + 1, fname.size() - u_pos);
    SubString::SizeType d_pos = tmp_substr.find_first_of('.');
    if (d_pos == SubString::NPOS ||
      !std::isdigit(tmp_substr[d_pos + 1]) ||
      tmp_substr[u_pos = d_pos + 2] != '_')
    {
      info = LogFileNameInfo();
      throw InvalidLogFileNameFormat(log_file_name);
    }
    SubString didx_str = tmp_substr.substr(0, d_pos);
    if (!str_to_int(didx_str, info.distrib_index))
    {
      info = LogFileNameInfo();
      throw InvalidLogFileNameFormat(log_file_name);
    }
    if (tmp_substr[d_pos + 1] == '1') // BASIC
    {
      tmp_substr = tmp_substr.substr(u_pos + 1, tmp_substr.size() - u_pos);
      d_pos = 8;
      if (tmp_substr[d_pos] != '.')
      {
        info = LogFileNameInfo();
        throw InvalidLogFileNameFormat(log_file_name);
      }
      SubString tstamp = tmp_substr.substr(0, d_pos);
      try
      {
        info.timestamp.set(tstamp, "%Y%m%d", true);
      }
      catch (...)
      {
        info = LogFileNameInfo();
        throw InvalidLogFileNameFormat(log_file_name);
      }
      tmp_substr = tmp_substr.substr(d_pos + 1, tmp_substr.size() - d_pos);
      d_pos = tmp_substr.find_first_of('.');
      if (d_pos == SubString::NPOS)
      {
        info = LogFileNameInfo();
        throw InvalidLogFileNameFormat(log_file_name);
      }
      tstamp = tmp_substr.substr(0, d_pos);
      std::time_t unixtime = 0;
      if (!str_to_int(tstamp, unixtime))
      {
        info = LogFileNameInfo();
        throw InvalidLogFileNameFormat(log_file_name);
      }
      info.timestamp.set(unixtime);
      tmp_substr = tmp_substr.substr(d_pos + 1, tmp_substr.size() - d_pos);
      if (tmp_substr.size() != 8 ||
        !str_to_int(tmp_substr, info.random))
      {
        info = LogFileNameInfo();
        throw InvalidLogFileNameFormat(log_file_name);
      }
      info.format = LogFileNameInfo::LFNF_BASIC;
    }
    else if (tmp_substr[d_pos + 1] == '0') // EXTENDED
    {
      u_pos = d_pos + 2;
      tmp_substr = tmp_substr.substr(u_pos + 1, tmp_substr.size() - u_pos);
      SubString tstamp = tmp_substr.substr(0, 22);
      try
      {
        info.timestamp.set(tstamp, "%Y%m%d.%H%M%S.%q", true);
      }
      catch (...)
      {
        info = LogFileNameInfo();
        throw InvalidLogFileNameFormat(log_file_name);
      }
      tmp_substr = tmp_substr.substr(23, tmp_substr.size() - 22);
      if (tmp_substr.size() != 8 ||
        !str_to_int(tmp_substr, info.random))
      {
        info = LogFileNameInfo();
        throw InvalidLogFileNameFormat(log_file_name);
      }
      info.format = LogFileNameInfo::LFNF_EXTENDED;
    }
    else
    {
      info = LogFileNameInfo();
      throw InvalidLogFileNameFormat(log_file_name);
    }
    info.base_name = fname.substr(0, f_d_pos).str();
    info.distrib_count = 0;
  }
  else // V_2_6
  {
    SubString tmp_substr =
      fname.substr(f_d_pos + 1, fname.size() - f_d_pos);
    SubString tstamp = tmp_substr.substr(0, 22);
    try
    {
      info.timestamp.set(tstamp, "%Y%m%d.%H%M%S.%q", true);
    }
    catch (...)
    {
      info = LogFileNameInfo();
      throw InvalidLogFileNameFormat(log_file_name);
    }
    SubString rand_str = tmp_substr.substr(23, 8);
    if (rand_str.size() != 8 ||
      !str_to_int(rand_str, info.random))
    {
      info = LogFileNameInfo();
      throw InvalidLogFileNameFormat(log_file_name);
    }
    tmp_substr = tmp_substr.substr(32, tmp_substr.size() - 31);
    SubString::SizeType d_pos = tmp_substr.find_first_of('.');
    if (d_pos == SubString::NPOS)
    {
      info = LogFileNameInfo();
      throw InvalidLogFileNameFormat(log_file_name);
    }
    SubString dcnt_str = tmp_substr.substr(0, d_pos);
    if (!str_to_int(dcnt_str, info.distrib_count))
    {
      info = LogFileNameInfo();
      throw InvalidLogFileNameFormat(log_file_name);
    }
    tmp_substr = tmp_substr.substr(d_pos + 1, tmp_substr.size() - d_pos);
    d_pos = tmp_substr.find_first_of('.');
    if (d_pos == SubString::NPOS)
    {
      if (!str_to_int(tmp_substr, info.distrib_index))
      {
        info = LogFileNameInfo();
        throw InvalidLogFileNameFormat(log_file_name);
      }
      info.processed_lines_count = 0;
    }
    else
    {
      SubString inx_str = tmp_substr.substr(0, d_pos);
      if (!str_to_int(inx_str, info.distrib_index))
      {
        info = LogFileNameInfo();
        throw InvalidLogFileNameFormat(log_file_name);
      }
      tmp_substr = tmp_substr.substr(d_pos + 1, tmp_substr.size() - d_pos);
      if (!str_to_int(tmp_substr, info.processed_lines_count))
      {
        info = LogFileNameInfo();
        throw InvalidLogFileNameFormat(log_file_name);
      }
    }
    info.format = LogFileNameInfo::LFNF_V_2_6;
    info.base_name = fname.substr(0, f_d_pos).str();
  }
}

/**
 * Functor to fill the LogSortingMap container
 */
class MapCreator
{
public:
  /**
   * @param container container to fill in
   */
  MapCreator(
    LogSortingMap &container,
    const std::string &log_type,
    std::size_t container_inc_size_limit = -1,
    Logging::Logger *logger = 0
  )
    noexcept
  :
    container_(container),
    logger_(logger),
    log_type_(log_type),
    container_inc_size_limit_(container_inc_size_limit),
    container_original_size_(container.size())
  {
  }

  /**
   * Adds another full file name to the LogSortingMap container
   * @param full_path full path to the file
   * @param file information
   */
  void operator()(const char *full_path, const struct stat&)
    /*throw(eh::Exception)*/
  {
    static const char *FUN = "search_for_files(): ";
    const char *name = Generics::DirSelect::file_name(full_path);

    if (logger_ && logger_->log_level() >= TraceLevel::LOW)
    {
      logger_->sstream(TraceLevel::HIGH, log_type_.c_str()) <<
        FUN << "Found a regular file: " << name;
    }

    LogFileNameInfo name_info;

    try
    {
      parse_log_file_name(name, name_info);
    }
    catch (const InvalidLogFileNameFormat&)
    {
      if (logger_ && logger_->log_level() >= TraceLevel::LOW)
      {
        logger_->sstream(TraceLevel::MIDDLE, log_type_.c_str()) <<
          FUN << "File '" << name << "' has invalid file name format. "
          "Skipping...";
      }
      return;
    }
    catch (const eh::Exception &ex)
    {
      if (logger_ && logger_->log_level() >= TraceLevel::LOW)
      {
        logger_->sstream(TraceLevel::MIDDLE, log_type_.c_str()) <<
          FUN << ex.what();
      }
      return;
    }

    if (name_info.format == LogFileNameInfo::LFNF_CSV)
    {
      if (logger_ && logger_->log_level() >= TraceLevel::LOW)
      {
        logger_->sstream(TraceLevel::MIDDLE, log_type_.c_str()) <<
          FUN << "File '" << name << "' has CSV file name format. "
          "Skipping...";
      }
      return;
    }

    if (name_info.base_name != log_type_)
    {
      if (logger_ && logger_->log_level() >= TraceLevel::LOW)
      {
        logger_->sstream(TraceLevel::MIDDLE, log_type_.c_str()) <<
          FUN << "File " << name << " has base name '" <<
          name_info.base_name << "' (must be '" <<
          log_type_ << "'). Skipping...";
      }
      return;
    }

    if (container_inc_size_limit_ == static_cast<std::size_t>(-1) ||
      container_.size() < container_original_size_ + container_inc_size_limit_)
    {
      container_.insert(
        LogSortingMap::value_type(name_info.timestamp, full_path)
      );
    }
    else if (!container_.empty() &&
      name_info.timestamp < container_.rbegin()->first)
    {
      container_.erase(--container_.end());
      container_.insert(
        LogSortingMap::value_type(name_info.timestamp, full_path)
      );
    }
  }

private:
  LogSortingMap &container_;
  Logging::Logger *logger_;
  const std::string &log_type_;
  const std::size_t container_inc_size_limit_;
  const std::size_t container_original_size_;
};

void
search_for_files(
  const std::string &dir_name,
  const std::string &log_type,
  LogSortingMap &log_sorting_map,
  std::size_t log_map_inc_size_limit,
  Logging::Logger *logger
)
{
  MapCreator creator(log_sorting_map, log_type,
    log_map_inc_size_limit, logger);

  Generics::DirSelect::directory_selector(
    dir_name.c_str(),
    creator,
    "[A-Z]*",
    Generics::DirSelect::DSF_DONT_RESOLVE_LINKS |
    Generics::DirSelect::DSF_EXCEPTION_ON_OPEN
  );
}

FileStore::FileStore(
  const std::string &root_dir,
  const std::string &sub_dir,
  bool use_session_name
)
  /*throw(Exception, eh::Exception)*/
:
  use_session_name_(use_session_name)
{
  std::string root_dir_str, sub_dir_str;
  root_dir_str = root_dir.empty() ? "." : root_dir;
  sub_dir_str = sub_dir.empty() ? "." : sub_dir;
  make_dir_(root_dir_str);
  dir_name_ = root_dir_str;
  (dir_name_ += '/') += sub_dir_str;
  size_t pos = root_dir_str.length();
  while (1)
  {
    if ((pos = dir_name_.find('/', ++pos)) == std::string::npos)
    {
      make_dir_(dir_name_);
      break;
    }
    make_dir_(dir_name_.substr(0, pos));
  }
  if (use_session_name_)
  {
    std::string cur_dir_name = dir_name_;
    (cur_dir_name += "/") += FILE_STORE_CUR_DIR;
    make_dir_(cur_dir_name);
  }
}

void
FileStore::store(
  const std::string& file_path,
  std::size_t processed_lines_count)
  /*throw(Exception, eh::Exception)*/
{
  std::string new_file_name;

  try
  {
    AdServer::LogProcessing::LogFileNameInfo name_info;
    parse_log_file_name(extract_file_name_(file_path).c_str(), name_info);
    name_info.processed_lines_count = processed_lines_count;
    new_file_name = restore_log_file_name(name_info, "");
  }
  catch (const eh::Exception&)
  {}

  store_(
    file_path,
    use_session_name_ ? new_session_name_() : "",
    new_file_name);
}

void
FileStore::make_dir_(const std::string &dir_name)
  /*throw(Exception, eh::Exception)*/
{
  if (mkdir(dir_name.c_str(), DIR_PERMS) == -1 &&
    errno != EEXIST)// Possibly already created by another thread
  {
    eh::throw_errno_exception<Exception>(
      "FileStore::make_dir_(): failed to create folder '", dir_name, "'");
  }
}

void
FileStore::make_dir_layout_(
  const std::string &store_dir,
  const std::string &store_sub_dir
)
  /*throw(Exception, eh::Exception)*/
{
  make_dir_(store_dir);
  make_dir_(store_dir + "/" + store_sub_dir);
}

std::string
FileStore::extract_file_name_(const std::string& file_path) const
  /*throw(Exception, eh::Exception)*/
{
  std::string::size_type sep_pos = file_path.find_last_of('/');
  std::string file_name;

  if (sep_pos == std::string::npos)
  {
    file_name = file_path;
  }
  else if (sep_pos == file_path.size() - 1)
  {
    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Invalid file path '" << file_path << "'";
    throw Exception(es);
  }
  else
  {
    file_name = file_path.substr(sep_pos + 1);
  }

  return file_name;
}

void
FileStore::store_(
  const std::string &file_path,
  const std::string &session_name,
  const std::string &new_file_name
)
  /*throw(Exception, eh::Exception)*/
{
  std::ostringstream dest_path_oss(dir_name_, std::ios_base::ate);

  if (!session_name.empty())
  {
    dest_path_oss << "/" << FILE_STORE_CUR_DIR << "/" << session_name;
    const std::string &dest_dir_name = dest_path_oss.str();

    if (mkdir(dest_dir_name.c_str(), DIR_PERMS) == -1)
    {
      if (errno == EMLINK)
      {
        std::string cur_dir_name = dir_name_;
        (cur_dir_name += "/") += FILE_STORE_CUR_DIR;
        std::string rot_dir_name = dir_name_;
        (rot_dir_name += "/") += new_session_name_();
        rename_(cur_dir_name.c_str(), rot_dir_name.c_str());
        make_dir_layout_(dir_name_, cur_dir_name);
        return store_(file_path, session_name);
      }
      if (errno != EEXIST)
      {
        eh::throw_errno_exception<Exception>(__PRETTY_FUNCTION__,
          ": failed to create folder '", dest_dir_name, "'");
      }
    }
  }

  if (!new_file_name.empty())
  {
    dest_path_oss << "/" << new_file_name;
  }
  else
  {
    const std::string file_name = extract_file_name_(file_path);
    dest_path_oss << "/" << file_name;
  }

  rename_(file_path.c_str(), dest_path_oss.str().c_str());
}

void
FileStore::store_(
  const std::string &file_path,
  const Generics::Time &timestamp
)
  /*throw(Exception, eh::Exception)*/
{
  const std::string file_name = extract_file_name_(file_path);
  LogFileNameInfo name_info;
  try
  {
    parse_log_file_name(file_name, name_info);
  }
  catch (const InvalidLogFileNameFormat &ex)
  {
    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Invalid file name '" << file_name << "'";
    throw Exception(es);
  }
  name_info.format = LogFileNameInfo::LFNF_CURRENT;
  name_info.timestamp = timestamp;
  const std::string& dest_file_path = make_log_file_name(name_info, dir_name_);
  rename_(file_path.c_str(), dest_file_path.c_str());
}

std::string
FileStore::new_session_name_()
{
  const char TIME_FMT[] = "%Y-%m-%d-%H-%M-%S";
  const std::string& formatted_time_of_day =
    Generics::Time::get_time_of_day().get_gm_time().format(TIME_FMT);
  long int rand_value = get_four_digit_random_value();
  std::ostringstream session_name_oss(
    formatted_time_of_day, std::ios_base::ate);
  session_name_oss << '.' <<
    std::setfill('0') << std::setw(4) << rand_value;
  return session_name_oss.str();
}

void
FileStore::rename_(const char* old_name, const char* new_name)
  /*throw(Exception)*/
{
  if (std::rename(old_name, new_name))
  {
    eh::throw_errno_exception<Exception>(
      "FileStore::rename_(): failed to rename file '",
        old_name, "' to '", new_name, "'");
  }
}

HitsFilter::HitsFilter(
  unsigned char min_count,
  const char* storage_file_prefix,
  unsigned long table_size
)
  /*throw(Exception, eh::Exception)*/
:
  MIN_COUNT_(min_count),
  TABLE_SIZE_(table_size)
{
  const std::string prefix_str = storage_file_prefix;
  std::string::size_type pos;

  if ((pos = prefix_str.rfind('/')) == std::string::npos)
  {
    Stream::Error es;
    es << FNS << "Persistent file storage prefix '"
       << prefix_str << "' is invalid";
    throw Exception(es);
  }

  storage_dir_ = prefix_str.substr(0, pos);
  file_prefix_ = prefix_str.substr(pos + 1);

  load_();
}

HitsFilter::~HitsFilter() noexcept
{
  try
  {
    save_();
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << FNS << "eh::Exception: " << ex.what() << std::endl;
  }
}

unsigned long
HitsFilter::check(
  const DayTimestamp& date,
  unsigned long hash,
  unsigned long add_count
)
  /*throw(Exception, eh::Exception)*/
{
  CounterMap_::iterator it = counters_.find(date);
  if (it == counters_.end())
  {
    it = counters_.insert(std::make_pair(date,
      ByteVector_(TABLE_SIZE_))).first;
  }
  unsigned long index = hash % it->second.size();
  unsigned char old_counter = it->second[index];
  it->second[index] = std::min(old_counter + add_count, 255UL);
  if (old_counter + add_count < MIN_COUNT_)
  {
    return 0;
  }
  if (old_counter >= MIN_COUNT_)
  {
    return add_count;
  }
  return old_counter + add_count;
}

void
HitsFilter::clear_expired(const DayTimestamp& exp_date)
  /*throw(Exception, eh::Exception)*/
{
  Guard_ guard(lock_);

  std::string file_name;
  for (CounterMap_::iterator it = counters_.begin(); it != counters_.end(); )
  {
    if (it->first < exp_date)
    {
      make_file_name_(it->first, file_name);
      unlink(file_name.c_str());
      counters_.erase(it++);
    }
    else
    {
      ++it;
    }
  }
}

void
HitsFilter::load_() /*throw(Exception, eh::Exception)*/
{
  typedef std::list<std::string> FileList;

  FileList file_list;

  Generics::DirSelect::directory_selector(
    storage_dir_.c_str(),
    Generics::DirSelect::list_creator(std::back_inserter(file_list)),
    (file_prefix_ + '*').c_str(),
    Generics::DirSelect::DSF_DONT_RESOLVE_LINKS |
    Generics::DirSelect::DSF_EXCEPTION_ON_OPEN
  );

  for (FileList::const_iterator it = file_list.begin();
    it != file_list.end(); ++it)
  {
    std::ifstream ifs(it->c_str(), std::ios::binary);

    ifs.seekg(0, std::ios::end);

    const unsigned long file_size = ifs.tellg();

    if (!file_size)
    {
      Stream::Error es;
      es << FNS << "File '" << *it << "' is empty!";
      throw Exception(es);
    }

    ifs.seekg(0, std::ios::beg);

    ByteVector_ bytes;

// TEMPORARY OLD FILE COMPATIBILITY WORKAROUND
    static const unsigned long OLD_FILE_SIZE = 10485760;

    if (file_size == OLD_FILE_SIZE)
    {
      ByteVector_ tmp_bytes(file_size);

      if (!ifs.read(reinterpret_cast<char*>(&tmp_bytes[0]), file_size))
      {
        Stream::Error es;
        es << FNS << "Failed to read data from file '" << *it << "'";
        throw Exception(es);
      }

      tmp_bytes.swap(bytes);
    }
// END OF WORKAROUND CODE
    else
    {
      char file_ver[FILE_VERSION_LEN_] = { 0 };

      if (!ifs.read(file_ver, sizeof(file_ver)))
      {
        Stream::Error es;
        es << FNS << "Failed to read file version from file '" << *it << "'";
        throw Exception(es);
      }
      if (std::memcmp(file_ver, CURRENT_FILE_VERSION_, sizeof(file_ver)))
      {
        Stream::Error es;
        es << FNS << "Invalid file version in file '" << *it << "'";
        throw Exception(es);
      }

      unsigned long table_size = 0;

      if (!ifs.read(reinterpret_cast<char*>(&table_size), TABLE_SIZE_LEN_))
      {
        Stream::Error es;
        es << FNS << "Failed to read table size from file '" << *it << "'";
        throw Exception(es);
      }
      if (!table_size)
      {
        Stream::Error es;
        es << FNS << "File '" << *it << "' has table_size = 0";
        throw Exception(es);
      }

      ByteVector_ tmp_bytes(table_size);

      if (!ifs.read(reinterpret_cast<char*>(&tmp_bytes[0]), table_size))
      {
        Stream::Error es;
        es << FNS << "Failed to read table data from file '" << *it << "'";
        throw Exception(es);
      }

      char eof_marker[sizeof(EOF_MARKER_)] = { 0 };

      if (!ifs.read(eof_marker, sizeof(eof_marker)))
      {
        Stream::Error es;
        es << FNS << "Failed to read EOF marker from file '" << *it << "'";
        throw Exception(es);
      }
      if (std::memcmp(eof_marker, EOF_MARKER_, sizeof(eof_marker)))
      {
        Stream::Error es;
        es << FNS << "Invalid EOF marker in file '" << *it << "'";
        throw Exception(es);
      }

      tmp_bytes.swap(bytes);
    }

    std::string::size_type pos;

    if ((pos = it->rfind('.')) == std::string::npos)
    {
      Stream::Error es;
      es << FNS << "File '" << *it << "' has invalid name format";
      throw Exception(es);
    }

    counters_[Generics::Time(it->substr(pos + 1), "%Y-%m-%d")].swap(bytes);
  }
}

void
HitsFilter::save_() /*throw(Exception, eh::Exception)*/
{
  std::string file_name;

  for (CounterMap_::const_iterator it = counters_.begin();
    it != counters_.end(); ++it)
  {
    make_file_name_(it->first, file_name);

    std::ofstream ofs(file_name.c_str(), std::ios::trunc | std::ios::binary);

    ofs.write(CURRENT_FILE_VERSION_, FILE_VERSION_LEN_);
    const unsigned long table_size = it->second.size();
    ofs.write(reinterpret_cast<const char*>(&table_size), TABLE_SIZE_LEN_);
    ofs.flush();
    ofs.write(reinterpret_cast<const char*>(&it->second[0]), table_size);
    ofs.flush();
    ofs.write(EOF_MARKER_, sizeof(EOF_MARKER_));
    ofs.flush();

    ofs.close();

    if (!ofs)
    {
      std::cerr << FNS << "Failed to write data to file '"
                << file_name << "'" << std::endl;
    }
  }
}

} // namespace LogProcessing
} // namespace AdServer

