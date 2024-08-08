// STD
#include <filesystem>
#include <fstream>

// THIS
#include "BidCostCollector.hpp"

namespace PredictorSvcs::BidCostPredictor
{

namespace
{

inline constexpr std::string_view empty_string_place_holder = "-";

std::ostream &operator<<(std::ostream& os, const BidCostCollector::Data& data)
{
  os << data.tag_id
     << '\n'
     << (data.url.empty() ? empty_string_place_holder : data.url)
     << '\n'
     << data.win_rate
     << '\n'
     << data.cost
     << '\n'
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
  Url url;
  WinRate win_rate = WinRate::ZERO;
  Cost cost = WinRate::ZERO;
  Cost max_cost = WinRate::ZERO;
  while (!file.eof() && file.peek() != std::char_traits<char>::eof())
  {
    url.clear();
    file >> tag_id
         >> url
         >> win_rate
         >> cost
         >> max_cost;
    if (url == empty_string_place_holder)
    {
      url.clear();
    }

    add(tag_id, url, win_rate, cost, max_cost);

    if (!file)
    {
      clear();

      Stream::Error stream;
      stream << FNS
             << "Error reading file="
             << path
             << ", line="
             << line;
      throw Exception(stream);
    }

    line += 1;
  }
}

void BidCostCollector::clear() noexcept
{
  container_.clear();
  url_hash_helper_.clear();
  urls_.clear();
}

} // namespace PredictorSvcs::BidCostPredictor