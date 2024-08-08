// STD
#include <cassert>
#include <filesystem>
#include <fstream>

// THIS
#include "CtrCollector.hpp"

namespace PredictorSvcs::BidCostPredictor
{

namespace
{

inline constexpr std::string_view empty_string_place_holder = "-";

} // namespace

CtrCollector::CtrCollector(const std::size_t capacity)
{
  map_.reserve(capacity);
}

void CtrCollector::add(
  const TagId tag_id,
  const Url& url,
  const CreativeCategoryId& creative_category_id,
  const Ctr& ctr)
{
  auto it = url_hash_helper_.find(url);
  if (it == std::end(url_hash_helper_))
  {
    urls_.emplace_back(url);
    it = url_hash_helper_.emplace(urls_.back()).first;
  }
  map_.try_emplace(
    {tag_id, std::string_view(*it), creative_category_id},
    ctr);
}

void CtrCollector::save(const std::string& file_path) const
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
    for (const auto& [k, v] : map_)
    {
      if (need_new_line)
      {
        file << '\n';
      }

      file << std::get<0>(k)
           << '\n'
           << (std::get<1>(k).empty() ? empty_string_place_holder : std::get<1>(k))
           << '\n'
           << std::get<2>(k)
           << '\n'
           << v;
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

void CtrCollector::load(const std::string& path)
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
  CreativeCategoryId creative_category_id = 0;
  Ctr ctr = Ctr::ZERO;
  while (!file.eof() && file.peek() != std::char_traits<char>::eof())
  {
    url.clear();

    file >> tag_id
         >> url
         >> creative_category_id
         >> ctr;
    if (url == empty_string_place_holder)
    {
      url.clear();
    }

    add(tag_id, url, creative_category_id, ctr);

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

void CtrCollector::clear() noexcept
{
  map_.clear();
  url_hash_helper_.clear();
  urls_.clear();
}

} // namespace PredictorSvcs::BidCostPredictor