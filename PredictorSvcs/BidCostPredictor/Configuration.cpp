// STD
#include <fstream>

// THIS
#include "Configuration.h"

namespace PredictorSvcs
{
namespace BidCostPredictor
{

Configuration::Configuration(
  const std::string& path_json_config)
{
  try
  {
    std::ifstream file(path_json_config);
    if (!file)
    {
      Stream::Error stream;
      stream << __PRETTY_FUNCTION__
             << "Can't open file="
             << path_json_config;
      throw Exception(stream);
    }

    boost::property_tree::read_json(
      file,
      ptree_);
  }
  catch(const boost::exception& exc)
  {
    Stream::Error stream;
    stream << __PRETTY_FUNCTION__
           << boost::diagnostic_information(exc);
    throw Exception(stream);
  }
  catch(const std::exception& exc)
  {
    Stream::Error stream;
    stream << __PRETTY_FUNCTION__
           << exc.what();
    throw Exception(stream);
  }
}

Configuration::Configuration(
  const boost::property_tree::ptree& ptree)
  : ptree_(ptree)
{
}

bool Configuration::exists(const std::string& path) const
{
  if (ptree_.empty())
    return false;

  try
  {
    std::vector<std::string> subpaths;
    subpaths.reserve(5);
    boost::split(
      subpaths,
      path,
      [] (const char data) {
        return data == '.';
      });

    auto* pointer = &ptree_;
    for (const auto& subpath : subpaths) {
      auto it2 = pointer->find(subpath);
      if (it2 == pointer->not_found())
        return false;
      pointer = &it2->second;
    }
    return true;
  }
  catch(const boost::exception& exc)
  {
    Stream::Error stream;
    stream << __PRETTY_FUNCTION__
           << boost::diagnostic_information(exc);
    throw Exception(stream);
  }
  catch(const std::exception& exc)
  {
    Stream::Error stream;
    stream << __PRETTY_FUNCTION__
           << exc.what();
    throw Exception(stream);
  }
}

std::string Configuration::get(
  const std::string& path) const
{
  return get<std::string>(path);
}

std::list<Configuration> Configuration::get_list_of(
  const std::string& path) const
{
  if (!exists(path))
  {
    Stream::Error stream;
    stream << __PRETTY_FUNCTION__
           << " : Not existing path="
           << path;
    throw Exception(stream);
  }

  std::list<Configuration> configurations;
  const auto position = path.find_last_of('.');
  if (position == std::string::npos)
  {
    configurations.emplace_back(ptree_);
    return configurations;
  }

  const auto key = path.substr(position + 1);
  const auto subpath = path.substr(0, position);
  const auto& sub_ptree = ptree_.get_child(subpath);

  auto it_range = sub_ptree.equal_range(key);
  for (auto it = it_range.first; it != it_range.second; ++it)
  {
    configurations.emplace_back(it->second);
  }

  return configurations;
}

Configuration Configuration::get_config(
  const std::string& path) const
{
  const auto configs = get_list_of(path);
  if (configs.size() != 1)
  {
    Stream::Error stream;
    stream << __PRETTY_FUNCTION__
           << " : Reason : "
           << " multiple values of json";
    throw Exception(stream);
  }
  return configs.front();
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs
