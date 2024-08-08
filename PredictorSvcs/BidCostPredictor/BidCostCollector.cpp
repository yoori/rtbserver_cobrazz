// STD
#include <filesystem>
#include <fstream>

// UNIXCOMMONS
#include <String/StringManip.hpp>

// THIS
#include "BidCostCollector.hpp"

namespace PredictorSvcs::BidCostPredictor
{

namespace
{

inline constexpr char empty_string_place_holder[] = "-";

std::ostream& operator<<(std::ostream& os, const BidCostCollector::Data& data)
{
  os << data.tag_id
     << '\t'
     << (data.url.empty() ? empty_string_place_holder : data.url)
     << '\t'
     << data.win_rate
     << '\t'
     << data.cost
     << '\t'
     << data.max_cost;
  return os;
}

} // namespace

BidCostCollector::BidCostCollector()
{
  url_hash_helper_.reserve(1000000);
}

const BidCostCollector::Data& BidCostCollector::add(
  const TagId tag_id,
  const Url& url,
  const WinRate& win_rate,
  const Cost& cost,
  const Cost& max_cost)
{
  auto it = url_hash_helper_.find(url);
  if (it == std::end(url_hash_helper_))
  {
    urls_.emplace_back(url);
    it = url_hash_helper_.emplace(urls_.back()).first;
  }
  return container_.emplace_back(tag_id, *it, win_rate, cost, max_cost);
}

std::size_t BidCostCollector::size() const noexcept
{
  return container_.size();
}

BidCostCollector::ConstIterator BidCostCollector::begin() const noexcept
{
  return container_.cbegin();
}

BidCostCollector::ConstIterator BidCostCollector::end() const noexcept
{
  return container_.cend();
}

void BidCostCollector::save(const std::string& file_path) const
{
  std::filesystem::path path(file_path);
  if (std::filesystem::exists(path))
  {
    Stream::Error stream;
    stream << FNS
           << "File with path="
           << file_path
           << " already exist";
    throw Exception(stream);
  }

  if (!path.has_filename())
  {
    Stream::Error stream;
    stream << FNS
           << "Not exist filename for path="
           << file_path;
    throw Exception(stream);
  }
  const std::string filename = path.filename();

  if (!path.has_parent_path())
  {
    Stream::Error stream;
    stream << FNS
           << "Not exist parent directory of path="
           << file_path;
    throw Exception(stream);
  }
  const std::string directory = path.parent_path();

  const std::string temp_file = directory +  "/~" +  filename;
  std::ofstream file(temp_file);
  if (!file)
  {
    Stream::Error stream;
    stream << FNS
           << "Can't open file="
           << temp_file;
    throw Exception(stream);
  }

  try
  {
    bool need_new_line = false;
    for (const auto& data: container_)
    {
      if (need_new_line)
      {
        file << '\n';
      }
      file << data;
      need_new_line = true;
    }

    if (!file)
    {
      std::filesystem::remove(temp_file);
      Stream::Error stream;
      stream << FNS
             << "Can't write data to file="
             << temp_file;
      throw Exception(stream);
    }

    if (std::rename(temp_file.c_str(), file_path.c_str()))
    {
      Stream::Error stream;
      stream << FNS
             << "Can't rename file="
             << temp_file
             << " to file"
             << file_path;
      throw Exception(stream);
    }
  }
  catch (...)
  {
    std::filesystem::remove(temp_file);
    throw;
  }
}

void BidCostCollector::load(const std::string& path)
{
  using Splitter = String::StringManip::Splitter<
    String::AsciiStringManip::SepTab>;

  std::ifstream file(path);
  if (!file)
  {
    Stream::Error stream;
    stream << FNS
           << "Can't open file="
           << path;
    throw Exception(stream);
  }

  clear();

  std::size_t line = 1;
  TagId tag_id = 0;
  WinRate win_rate = WinRate::ZERO;
  Cost cost = WinRate::ZERO;
  Cost max_cost = WinRate::ZERO;

  std::string buffer;
  buffer.reserve(1024);

  String::SubString tag_id_str;
  String::SubString win_rate_str;
  String::SubString cost_str;
  String::SubString max_cost_str;
  String::SubString url;
  while (!file.eof() && file.peek() != std::char_traits<char>::eof())
  {
    buffer.clear();
    tag_id_str.clear();
    win_rate_str.clear();
    cost_str.clear();
    max_cost_str.clear();
    url.clear();

    std::getline(file, buffer, '\n');
    if (buffer.empty())
    {
      continue;
    }

    Splitter splitter(buffer);
    if(!splitter.get_token(tag_id_str) ||
       !splitter.get_token(url) ||
       !splitter.get_token(win_rate_str) ||
       !splitter.get_token(cost_str) ||
       !splitter.get_token(max_cost_str))
    {
      Stream::Error stream;
      stream << "Invalid line config: line = "
             << line;
      throw Exception(stream);
    }

    if (url == empty_string_place_holder)
    {
      url.clear();
    }

    try
    {
      if (!String::StringManip::str_to_int(tag_id_str, tag_id))
      {
        Stream::Error stream;
        stream << "Can't convert string to tag_id"
               << line;
        throw Exception(stream);
      }

      win_rate = Types::WinRate(win_rate_str);
      cost = Types::Cost(cost_str);
      max_cost = Types::Cost(max_cost_str);
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << "Invalid line config: line = "
             << line;
      throw Exception(stream);
    }

    add(tag_id, url.str(), win_rate, cost, max_cost);

    line += 1;
  }

  if (!file)
  {
    clear();

    Stream::Error stream;
    stream << FNS
           << "Error reading file";
    throw Exception(stream);
  }
}

void BidCostCollector::clear() noexcept
{
  container_.clear();
  url_hash_helper_.clear();
  urls_.clear();
}

} // namespace PredictorSvcs::BidCostPredictor