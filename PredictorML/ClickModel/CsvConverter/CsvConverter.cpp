// STD
#include <filesystem>
#include <fstream>
#include <iostream>

// THIS
#include <PredictorML/ClickModel/CsvConverter/CsvConverter.hpp>

namespace Aspect
{

inline constexpr char CSV_CONVERTER[] = "CSV_CONVERTER";

} // namespace Aspect

namespace
{

constexpr std::size_t kChVectorSize = 1024;

struct MLParams final
{
  std::string_view is_clicked;          // label
  std::uint32_t hour = 0;               // number
  std::uint32_t minute = 0;             // |^
  std::uint32_t second = 0;             // |^
  std::uint32_t day_of_week = 0;        // number
  std::string_view device;              // category
  std::string_view ip;                  // category
  std::string_view host;                // category
  std::string_view publisher_id;        // category
  std::string_view tag_id;              // category
  std::string_view campaign_id;         // category
  std::string_view ccg_id;              // category
  std::string_view ccid;                // category
  bool ch_vector[kChVectorSize];        // numbers
  std::string_view bid_floor;           // number
  std::string_view size_id;             // category
  std::string_view campaign_freq;       // number
};

inline std::string_view trim_string(
  const std::string_view data) noexcept
{
  std::size_t size = data.size();
  if (size == 0)
  {
    return {};
  }

  const char* begin = data.data();
  if (*begin == ' ')
  {
    while (size > 0 && *begin == ' ')
    {
      begin += 1;
      size -= 1;
    }

    if (size == 0)
    {
      return {};
    }
  }

  if (*(begin + size - 1) == ' ')
  {
    while (size > 0 && *(begin + size - 1) == ' ')
    {
      size -= 1;
    }

    if (size == 0)
    {
      return {};
    }
  }

  if (*begin == '"')
  {
    begin += 1;
    size -= 1;
  }

  if (size == 0)
  {
    return {};
  }

  if (*(begin + size - 1) == '"')
  {
    size -= 1;
  }

  return std::string_view(
    begin,
    size);
}

inline std::uint32_t get_day_of_week(
  std::uint32_t year,
  std::uint32_t month,
  std::uint32_t day) noexcept
{
  static constexpr std::uint32_t month_code[] =
    {6, 2, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  year -= month < 3;
  return 1 + (year + year / 4 - year / 100 +
              year / 400 + month_code[month - 1] + day) % 7;
}

// You must ensure that after string_view followed zero (performance reason)
inline bool parse_time(
  const std::string_view& str_time,
  std::uint32_t& hour,
  std::uint32_t& minute,
  std::uint32_t& second,
  std::uint32_t& day_of_week) noexcept
{
  std::uint32_t year = 0;
  std::uint32_t month = 0;
  std::uint32_t day = 0;

  const auto result = sscanf(
    str_time.data(),
    "%d-%d-%d %d:%d:%d",
    &year,
    &month,
    &day,
    &hour,
    &minute,
    &second);
  if (result != 6)
  {
    return false;
  }

  day_of_week = get_day_of_week(
    year,
    month,
    day);

  return true;
}

inline std::string_view ip_converter(
  const std::string_view& ip) noexcept
{
  auto size = ip.size();
  if (size < 7)
  {
    return {};
  }

  if (ip[size - 2] == '.')
  {
    size -= 2;
  } else if (ip[size - 3] == '.')
  {
    size -= 3;
  } else if (ip[size - 4] == '.')
  {
    size -= 4;
  }

  return std::string_view{ip.data(), size};
}

inline std::string_view extract_host_from_url(
  const std::string_view& url) noexcept
{
  static constexpr std::string_view begin_host = "//";
  static constexpr std::string_view www = "www.";

  if (url.empty())
  {
    return {};
  }

  auto start = url.find(begin_host);
  if (start == std::string_view::npos)
  {
    start = 0;
  }
  else
  {
    start += 2;
  }

  if (url.size() - start >= 4
    && url.substr(start, 4) == www)
  {
    start += 4;
  }

  auto end = url.find('/', start);
  if (end == std::string_view::npos)
  {
    end = url.size();
  }

  return std::string_view{
    url.data() + start,
    end - start};
}

template<class Container>
void split(
  const std::string_view data,
  const char delimiter,
  Container& container) noexcept
{
  static constexpr char quote = '"';

  bool is_quoted = false;
  const auto size = data.size();
  const char *begin = &data[0];
  const char *const end = begin + size;

  const char *current = begin;
  while (current != end)
  {
    if (*current != delimiter || is_quoted)
    {
      if (*current == quote)
      {
        is_quoted = !is_quoted;
      }

      current += 1;
      continue;
    }

    container.emplace_back(
      begin,
      current - begin);

    current += 1;
    begin = current;
  }

  container.emplace_back(
    begin,
    current - begin);
}

template<std::size_t N>
bool fill_ch_vector(
  bool (&array)[N],
  const std::string_view& data,
  std::vector<std::string_view>& helper) noexcept
{
  if (data.empty())
  {
    return true;
  }
  helper.clear();

  split(data, '|', helper);
  std::size_t number = 0;
  for (const auto& data: helper)
  {
    if (data.empty())
    {
      continue;
    }

    auto result = std::from_chars(
      data.data(),
      data.data() + data.size(),
      number);
    if (result.ec == std::errc{})
    {
      array[number % N] = true;
    } else
    {
      return false;
    }
  }

  return true;
}

} // namespace

CsvConverter::CsvConverter(
  Logger* logger,
  const std::string& csv_original_path,
  const std::string& csv_process_path)
  : logger_(ReferenceCounting::add_ref(logger)),
    csv_original_path_(csv_original_path),
    csv_process_path_(csv_process_path)
{
}

void CsvConverter::process()
{
  const std::vector<std::string> requested_header_names = {
    "is_clicked", "any(timestamp)", "device",
    "ip", "url", "publisher_id", "tag_id",
    "etag", "campaign_id", "ccg_id",
    "ccid", "geo_ch", "user_ch",
    "imp_ch", "bid_price", "bid_floor",
    "size_id", "campaign_freq", "predicted_cr",
    "win_price", "viewability"
  };
  const std::size_t number_columns =
    requested_header_names.size();

  std::ofstream out_file(
    csv_process_path_,
    std::ios::out | std::ios::trunc);
  if (!out_file)
  {
    std::ostringstream stream;
    stream << "Can't open file="
           << csv_process_path_;
    logger_->critical(
      stream.str(),
      Aspect::CSV_CONVERTER);
    throw std::runtime_error(stream.str());
  }

  out_file << "is_clicked,"
           << "device,"
           << "ip,"
           << "host,"
           << "publisher_id,"
           << "tag_id,"
           << "campaign_id,"
           << "ccg_id,"
           << "ccid,"
           << "size_id,"
           << "time,"
           << "day_of_week,"
           << "bid_floor,"
           << "campaign_freq,";
  for (std::size_t i = 1; i <= kChVectorSize; i += 1)
  {
    if (i >= 2)
    {
      out_file << ',';
    }
    out_file << "ch_"
             << i;
  }
  out_file << '\n';

  {
    std::ostringstream stream;
    stream << "Number categorial feature: "
           << 9;
    logger_->info(stream.str());
  }

  const std::uint64_t file_size =
    std::filesystem::file_size(csv_original_path_);
  {
    std::ostringstream stream;
    stream << "File size[mb] = "
           <<  file_size / 1048576;
    logger_->info(stream.str());
  }

  std::ifstream file(csv_original_path_);
  if (!file)
  {
    std::ostringstream stream;
    stream << "Can't open file="
           << csv_original_path_;
    logger_->critical(
      stream.str(),
      Aspect::CSV_CONVERTER);
    throw std::runtime_error(stream.str());
  }

  const char delimiter = ',';
  std::vector<std::string_view> data_list;
  data_list.reserve(number_columns);
  std::string line;
  line.reserve(1000);

  std::getline(file, line);
  split(line, delimiter, data_list);
  {
    std::ostringstream stream;
    stream << "Number columns in original csv : "
           << data_list.size();
    logger_->info(stream.str());
  }

  {
    std::vector<std::string> header_names;
    header_names.reserve(number_columns);

    std::ostringstream stream;
    stream << "Original column names: [";
    bool first = true;
    for (const auto& name : data_list)
    {
      std::string_view trim_name =
        trim_string(name);
      if (trim_name.empty())
      {
        std::ostringstream stream;
        stream << FNS
               << "Not correct column name="
               << name;
        throw std::runtime_error(stream.str());
      }
      header_names.emplace_back(trim_name);

      if (first)
      {
        first = false;
      }
      else
      {
        stream << ", ";
      }

      stream << trim_name;
    }
    stream << "]";
    logger_->info(stream.str());

    if (header_names != requested_header_names)
    {
      std::ostringstream stream;
      stream << FNS
             << "Not correct format file";
      throw std::runtime_error(stream.str());
    }
  }

  MLParams params;
  std::uint64_t line_number = 0;
  std::uint64_t process_size = 0;
  const auto ch_vector_number_bytes = sizeof(params.ch_vector);
  std::vector<std::string_view> helper;
  helper.reserve(10000);
  std::uint64_t label_zero_count = 0;
  std::uint64_t label_one_count = 0;
  std::string_view zero("0");
  while (std::getline(file, line))
  {
    line_number += 1;
    process_size += line.size();
    data_list.clear();
    std::memset(
      params.ch_vector,
      0,
      ch_vector_number_bytes);

    if (line_number % 200000 == 0)
    {
      std::ostringstream  stream;
      stream << "process percentage="
             << (process_size * 100) / file_size
             << "%";
      logger_->info(stream.str(), Aspect::CSV_CONVERTER);
    }

    split(line, delimiter, data_list);
    if (data_list.size() != number_columns)
    {
      std::ostringstream stream;
      stream << FNS
             << "Not correct number datas number data="
             << data_list.size()
             << ", number_columns="
             << number_columns
             << '\n'
             << line;
      logger_->error(stream.str(), Aspect::CSV_CONVERTER);
      continue;
    }

    params.is_clicked = trim_string(data_list[0]);
    if (params.is_clicked.empty())
    {
      std::ostringstream stream;
      stream << FNS
             << "Not correct line="
             << line_number
             << ", is_clicked is empty"
             << '\n'
             << line;
      logger_->error(stream.str(), Aspect::CSV_CONVERTER);
      continue;
    }

    if (params.is_clicked == zero)
    {
      label_zero_count += 1;
    }
    else
    {
      label_one_count += 1;
    }

    auto string_time = trim_string(data_list[1]);
    if (string_time.empty())
    {
      std::ostringstream stream;
      stream << FNS
             << "Not correct line="
             << line_number
             << ", string_time is empty";
      logger_->error(stream.str(), Aspect::CSV_CONVERTER);
      continue;
    }

    const char save_ch = string_time[string_time.size()];
    *const_cast<char*>(&string_time[string_time.size()]) = '\0';
    if (!parse_time(
      string_time,
      params.hour,
      params.minute,
      params.second,
      params.day_of_week))
    {
      std::ostringstream  stream;
      stream << FNS
             << "Not correct date/time on line="
             << line_number
             << '\n'
             << line;
      logger_->error(stream.str(), Aspect::CSV_CONVERTER);
      continue;
    }
    *const_cast<char*>(&string_time[string_time.size()]) = save_ch;

    params.device = trim_string(data_list[2]);
    if (params.device.empty())
    {
      std::ostringstream stream;
      stream << FNS
             << "Not correct line="
             << line_number
             << ", device is empty"
             << '\n'
             << line;
      logger_->error(stream.str(), Aspect::CSV_CONVERTER);
      continue;
    }

    params.ip = ip_converter(
      trim_string(data_list[3]));
    if (params.ip.empty())
    {
      std::ostringstream stream;
      stream << FNS
             << "Not correct line="
             << line_number
             << ", ip is empty"
             << '\n'
             << line;
      logger_->error(stream.str(), Aspect::CSV_CONVERTER);
      continue;
    }

    params.host = extract_host_from_url(
      trim_string(data_list[4]));
    if (params.host.empty())
    {
      std::ostringstream stream;
      stream << FNS
             << "Not correct line="
             << line_number
             << ", host is empty"
             << '\n'
             << line;
      logger_->error(stream.str(), Aspect::CSV_CONVERTER);
      continue;
    }

    params.publisher_id = trim_string(data_list[5]);
    if (params.publisher_id.empty())
    {
      std::ostringstream stream;
      stream << FNS
             << "Not correct line="
             << line_number
             << ", publisher_id is empty"
             << '\n'
             << line;
      logger_->error(stream.str(), Aspect::CSV_CONVERTER);
      continue;
    }

    params.tag_id = trim_string(data_list[6]);
    if (params.tag_id.empty())
    {
      std::ostringstream stream;
      stream << FNS
             << "Not correct line="
             << line_number
             << ", tag_id is empty"
             << '\n'
             << line;
      logger_->error(stream.str(), Aspect::CSV_CONVERTER);
      continue;
    }

    params.campaign_id = trim_string(data_list[8]);
    if (params.campaign_id.empty())
    {
      std::ostringstream stream;
      stream << FNS
             << "Not correct line="
             << line_number
             << ", campaign_id is empty"
             << '\n'
             << line;
      logger_->error(stream.str(), Aspect::CSV_CONVERTER);
      continue;
    }

    params.ccg_id = trim_string(data_list[9]);
    if (params.ccg_id.empty())
    {
      std::ostringstream stream;
      stream << FNS
             << "Not correct line="
             << line_number
             << ", ccg_id is empty"
             << '\n'
             << line;
      logger_->error(stream.str(), Aspect::CSV_CONVERTER);
      continue;
    }

    params.ccid = trim_string(data_list[10]);
    if (params.ccid.empty())
    {
      std::ostringstream stream;
      stream << FNS
             << "Not correct line="
             << line_number
             << ", ccid is empty"
             << '\n'
             << line;
      logger_->error(stream.str(), Aspect::CSV_CONVERTER);
      continue;
    }

    if (!fill_ch_vector(
      params.ch_vector,
      trim_string(data_list[11]),
      helper))
    {
      std::ostringstream stream;
      stream << FNS
             << "Not correct line="
             << line_number
             << ", geo_ch is wrong"
             << '\n'
             << line;
      logger_->error(stream.str(), Aspect::CSV_CONVERTER);
      continue;
    }
    helper.clear();

    if (!fill_ch_vector(
      params.ch_vector,
      trim_string(data_list[12]),
      helper))
    {
      std::ostringstream stream;
      stream << FNS
             << "Not correct line="
             << line_number
             << ", user_ch is wrong"
             << '\n'
             << line;
      logger_->error(stream.str(), Aspect::CSV_CONVERTER);
      continue;
    }
    helper.clear();

    if (!fill_ch_vector(
      params.ch_vector,
      trim_string(data_list[13]),
      helper))
    {
      std::ostringstream stream;
      stream << FNS
             << "Not correct line="
             << line_number
             << ", imp_ch is wrong"
             << '\n'
             << line;
      logger_->error(stream.str(), Aspect::CSV_CONVERTER);
      continue;
    }
    helper.clear();

    params.bid_floor = trim_string(data_list[15]);
    if (params.bid_floor.empty())
    {
      std::ostringstream stream;
      stream << FNS
             << "Not correct line="
             << line_number
             << ", bid_floor is empty"
             << '\n'
             << line;
      logger_->error(stream.str(), Aspect::CSV_CONVERTER);
      continue;
    }

    params.size_id = trim_string(data_list[16]);
    if (params.size_id.empty())
    {
      std::ostringstream stream;
      stream << FNS
             << "Not correct line="
             << line_number
             << ", size_id is empty"
             << '\n'
             << line;
      logger_->error(stream.str(), Aspect::CSV_CONVERTER);
      continue;
    }

    params.campaign_freq = trim_string(data_list[17]);
    if (params.campaign_freq.empty())
    {
      std::ostringstream stream;
      stream << FNS
             << "Not correct line="
             << line_number
             << ", campaign_freq is empty"
             << '\n'
             << line;
      logger_->error(stream.str(), Aspect::CSV_CONVERTER);
      continue;
    }

    out_file << params.is_clicked << ','
             << params.device << ','
             << params.ip << ','
             << params.host << ','
             << params.publisher_id << ','
             << params.tag_id << ','
             << params.campaign_id << ','
             << params.ccg_id << ','
             << params.ccid << ','
             << params.size_id << ','
             << params.hour * 3600 + params.minute * 60 + params.second << ','
             << params.day_of_week << ','
             << params.bid_floor << ','
             << params.campaign_freq;
    for (std::size_t i = 1; i <= kChVectorSize; i += 1)
    {
      out_file << ',' << static_cast<std::uint32_t>(params.ch_vector[i - 1]);
    }
    out_file << '\n';
  }

  std::ostringstream stream;
  stream << "label=1 equal: " << label_one_count << '\n'
         << "label=0 equal: " << label_zero_count << '\n';
  logger_->error(stream.str(), Aspect::CSV_CONVERTER);
}