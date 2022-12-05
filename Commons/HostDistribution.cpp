
#include <fstream>
#include <Commons/ErrorHandler.hpp>
#include <xsd/AdServerCommons/HostDistributionFile.hpp>
#include "HostDistribution.hpp"

namespace
{
  const char ADSERVER_NAMESPACE[] = "http://www.adintelligence.net/xsd/AdServer/Configuration";
}

namespace AdServer
{
namespace Commons
{
  HostDistributionFile::HostDistributionFile(
    const char* file,
    const char* schema_file)
    /*throw(FileNotFound, InvalidFile)*/
  {
    static const char* FUN = "HostDistributionFile::HostDistributionFile()";

    if(!file)
    {
      throw InvalidFile("name file is null");
    }

    Config::ErrorHandler error_handler;

    try
    {
      using namespace xsd::AdServer::Configuration;

      xml_schema::properties props;
      props.schema_location(ADSERVER_NAMESPACE, schema_file);

      std::ifstream ifile(file);
      if(!ifile.is_open())
      {
        throw FileNotFound("");
      }

      std::unique_ptr<DistributionIndexFileType> config =
        DistributionIndexFile(
          ifile, error_handler, xml_schema::flags::dont_initialize, props);

      if(error_handler.has_errors())
      {
        std::string error_string;
        throw InvalidFile(error_handler.text(error_string));
      }
      unsigned long limit = config->hosts().index_limit();
      IndexToHostMap::const_iterator fnd;
      for(DistributionHostsType::host_sequence::const_iterator host_it =
          config->hosts().host().begin();
          host_it != config->hosts().host().end(); ++host_it)
      {
        const std::string& host = host_it->name();
        for(DistributionHostType::index_sequence::const_iterator ind_it =
            host_it->index().begin();
            ind_it != host_it->index().end(); ++ind_it)
        {
          unsigned long index = ind_it->value();
          if(index >= limit)
          {
            Stream::Error err;
            err << FUN << ": index = " << index << " more than limit " << limit <<
              "for host = '" << host << "'";
            throw InvalidFile(err);
          }
          fnd = host_map_.find(index);
          if(fnd != host_map_.end())
          {
            Stream::Error err;
            err << FUN << ": index = " << index << " is cross between hosts '" <<
              host << "' and '" << fnd->second << "'";
            throw InvalidFile(err);
          }
          host_map_[index] = host;
        }
      }
      if(host_map_.size() != limit)
      {
        Stream::Error err;
        err << FUN  << ":";
        unsigned long index = 0;
        bool coma = false;
        for(fnd = host_map_.begin(); fnd != host_map_.end(); ++fnd, index++)
        {
          while(index < fnd->first)
          {
            err << (coma ? ',' : ' ') << "host for index = " << index << " doesn't set in config";
            coma = true;
            index++;
          }
        }
        err << ".";
        throw InvalidFile(err);
      }
    }
    catch(const xml_schema::parsing& e)
    {
      Stream::Error ostr;
      ostr << "Can't parse config file '" << file << "': ";
      if(error_handler.has_errors())
      {
        std::string error_string;
        ostr << error_handler.text(error_string);
      }
      throw InvalidFile(ostr);
    }
    catch(const InvalidFile&)
    {
      throw;
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "Can't parse config file '" << file << "': " << e.what();
      throw InvalidFile(ostr);
    }
  }
}
}

