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

  const auto& sub_ptree = ptree_.get_child(path);
  auto it_range = sub_ptree.equal_range("");
  for (auto it = it_range.first; it != it_range.second; ++it)
  {
    configurations.emplace_back(it->second);
  }

  return configurations;
}

Configuration Configuration::get_config(
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

  const auto& sub_ptree = ptree_.get_child(path);
  return Configuration(sub_ptree);
}

std::ostream& operator<<(
  std::ostream& ostr,
  const Configuration& configuration)
{
  boost::property_tree::json_parser::write_json(
    ostr,
    configuration.ptree_);
  return ostr;
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs
