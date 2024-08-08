// STD
#include <cassert>
#include <filesystem>
#include <fstream>

// UNIXCOMMONS
#include <String/StringManip.hpp>

// THIS
#include "CtrCollector.hpp"

namespace PredictorSvcs::BidCostPredictor
{

namespace
{

inline constexpr char empty_string_place_holder[] = "-";

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
           << '\t'
           << (std::get<1>(k).empty() ? empty_string_place_holder : std::get<1>(k))
           << '\t'
           << std::get<2>(k)
           << '\t'
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
  CreativeCategoryId creative_category_id = 0;
  Ctr ctr = Ctr::ZERO;

  std::string buffer;
  buffer.reserve(1024);

  String::SubString tag_id_str;
  String::SubString url;
  String::SubString creative_category_id_str;
  String::SubString ctr_str;
  while (!file.eof() && file.peek() != std::char_traits<char>::eof())
  {
    buffer.clear();
    tag_id_str.clear();
    url.clear();
    creative_category_id_str.clear();
    ctr_str.clear();

    std::getline(file, buffer, '\n');
    if (buffer.empty())
    {
      continue;
    }

    Splitter splitter(buffer);
    if(!splitter.get_token(tag_id_str) ||
       !splitter.get_token(url) ||
       !splitter.get_token(creative_category_id_str) ||
       !splitter.get_token(ctr_str))
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

      if (!String::StringManip::str_to_int(creative_category_id_str, creative_category_id))
      {
        Stream::Error stream;
        stream << "Can't convert string to creative_category_id"
               << line;
        throw Exception(stream);
      }

      ctr = Types::Ctr(ctr_str);
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << "Invalid line config: line = "
             << line;
      throw Exception(stream);
    }

    add(tag_id, url.str(), creative_category_id, ctr);

    line += 1;
  }

  if (!file)
  {
    clear();

    Stream::Error stream;
    stream << FNS
           << "Error reading file="
           << path;
    throw Exception(stream);
  }
}

void CtrCollector::clear() noexcept
{
  map_.clear();
  url_hash_helper_.clear();
  urls_.clear();
}

} // namespace PredictorSvcs::BidCostPredictor