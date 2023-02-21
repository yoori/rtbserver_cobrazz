#ifndef ADSERVER_COMMONS_MALLOCSTATSPROVIDER_HPP
#define ADSERVER_COMMONS_MALLOCSTATSPROVIDER_HPP

// STD
#include <cstdint>
#include <memory>

// POSIX
#include <malloc.h>
#include <stdio.h>

// BOOST
#include <boost/exception/diagnostic_information.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

// THIS
#include <eh/Exception.hpp>

namespace AdServer
{
namespace Commons
{

class MallocStatsProvider
{
public:
  struct MemoryInfo
  {
    MemoryInfo(
      const std::uint64_t total_memory,
      const std::uint64_t in_use_memory)
      : total_memory(total_memory),
        in_use_memory(in_use_memory)
    {
    }

    std::uint64_t total_memory = 0;
    std::uint64_t in_use_memory = 0;
  };

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  static MemoryInfo get_momory_info()
  {
    char* buffer = nullptr;
    std::size_t buffer_size = 0;

    auto deleter = [] (FILE* f) {
      fclose(f);
    };

    std::unique_ptr<FILE, decltype(deleter)> stream(
      open_memstream(&buffer, &buffer_size),
      deleter);
    if (!stream)
    {
      Stream::Error ostr;
      ostr << __PRETTY_FUNCTION__
           << " : Reason : "
           << "open_memstream is failed";
      throw Exception(ostr);
    }

    if (malloc_info(0, stream.get()) == -1)
    {
      Stream::Error ostr;
      ostr << __PRETTY_FUNCTION__
           << " : Reason : "
           << "malloc_info is failed";
      throw Exception(ostr);
    }

    fflush(stream.get());
    if (buffer_size == 0)
    {
      Stream::Error ostr;
      ostr << __PRETTY_FUNCTION__
           << " : Reason : "
           << "buffer is empty";
      throw Exception(ostr);
    }

    try
    {
      std::stringstream stream;
      stream << std::string_view(buffer, buffer_size);
      boost::property_tree::ptree pt;
      boost::property_tree::read_xml(stream, pt);

      std::uint64_t free_mem = 0;
      std::uint64_t total_mem = 0;

      const auto total_range = pt.get_child("malloc").equal_range("total");
      auto it = total_range.first;
      auto it_end = total_range.second;
      for (; it != it_end; ++it)
      {
        const auto& subtree = it->second;
        const std::string type = subtree.get<std::string>("<xmlattr>.type");
        if (type == "rest" || type == "fast")
        {
          free_mem += subtree.get<std::size_t>("<xmlattr>.size");
        }
        else if (type == "mmap")
        {
          total_mem += subtree.get<std::size_t>("<xmlattr>.size");
        }
      }

      const auto system_range = pt.get_child("malloc").equal_range("system");
      it = system_range.first;
      it_end = system_range.second;
      for (; it != it_end; ++it)
      {
        const auto& subtree = it->second;
        const std::string type = subtree.get<std::string>("<xmlattr>.type");
        if (type == "current")
        {
          total_mem += subtree.get<std::size_t>("<xmlattr>.size");
        }
      }

      return MemoryInfo(total_mem, total_mem - free_mem);
    }
    catch (const boost::exception& exc)
    {
      const std::string error_info = diagnostic_information(exc);
      Stream::Error ostr;
      ostr << __PRETTY_FUNCTION__
           << " : Reason : "
           << error_info;
      throw Exception(ostr);
    }
  }
};

} // namespace Commons
} // namespace AdServer

#endif //ADSERVER_COMMONS_MALLOCSTATSPROVIDER_HPP
