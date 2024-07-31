#ifndef BIDCOSTPREDICTOR_CONFIGURATION_H
#define BIDCOSTPREDICTOR_CONFIGURATION_H

// STD
#include <list>
#include <string>
#include <vector>

// BOOST
#include <boost/algorithm/string/split.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

// UNIXCOMMONS
#include <eh/Exception.hpp>

// THIS
#include <LogCommons/LogCommons.hpp>

namespace PredictorSvcs::BidCostPredictor
{

class Configuration final
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit Configuration(
    const std::string& path_json_config);

  explicit Configuration(
    const boost::property_tree::ptree& ptree);

  ~Configuration() = default;

  bool exists(const std::string& path) const;

  template<class T>
  T get(const std::string& path) const;

  template<class T>
  T get(
    const std::string& path,
    const T& default_value) const;

  std::string get(const std::string& path) const;

  std::list<Configuration> get_list_of(
    const std::string& path) const;

  Configuration get_config(
    const std::string& path) const;

  friend std::ostream& operator<<(
    std::ostream&,
    const Configuration&);

private:
  boost::property_tree::ptree ptree_;
};

template<class T>
T Configuration::get(const std::string& path) const
{
  if (!exists(path))
  {
    Stream::Error stream;
    stream << FNS
           << "Not existing path=" << path;
    throw Exception(stream);
  }

  const auto value = ptree_.get_optional<T>(path);
  if (value)
  {
    return *value;
  }
  else
  {
    Stream::Error stream;
    stream << FNS
           << "Not correct value of path=" << path;
    throw Exception(stream);
  }
}

template<class T>
T Configuration::get(
  const std::string& path,
  const T& default_value) const
{
  const auto value = ptree_.get_optional<T>(path);
  if (value)
  {
    return *value;
  }
  else
  {
    return default_value;
  }
}

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_CONFIGURATION_H