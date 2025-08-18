/* $Id: InputCsvStream.cpp 185976 2020-07-01 22:11:23Z jurij_kuznecov $
* @file InputCsvStream.cpp
* @author Artem V. Nikitin (artem_nikitin@ocslab.com)
* Input csv stream implementation
*/

#include <sys/file.h>
#include <ext/stdio_filebuf.h>

#include <Commons/CsvReader.hpp>
#include <LogCommons/CsvUtils.hpp>

#include "Utils.hpp"
#include "InputCsvStream.hpp"

namespace
{
  class FileLock
  {
  public:
    FileLock(const char* path)
      : fd_(-1),
        locked_(false)
    {
      fd_ = ::open64(path, O_RDONLY, 0);
    }

    bool try_lock()
    {
      if (fd_ == -1)
      {
        return false;
      }
      int rc = ::flock(fd_, LOCK_EX | LOCK_NB);
      if (rc != 0)
      {
        return false;
      }
      locked_ = true;
      return true;
    }

    int fd()
    {
      return fd_;
    }
    
    ~FileLock() noexcept
    {
      if (fd_ != -1)
      {
        if (locked_)
        {
          ::flock(fd_, LOCK_UN | LOCK_NB);
        }
        ::close(fd_);
      }
    }
      
  private:
    int fd_;
    bool locked_;
  };
}

namespace AdServer::Predictor
{
  InputCsvStream::InputCsvStream(
    const FileObject& file,
    Logging::Logger* logger)
    : file_(file),
      logger_(ReferenceCounting::add_ref(logger))
  {}

  unsigned long InputCsvStream::parse_header_(
    std::istream& stream,
    FeatureColumns& feature_columns)
  {  
    std::string feature_columns_str;
    std::getline(stream, feature_columns_str);

    FeatureNames feature_column_names;
    ::Commons::CsvReader::parse_line(
      feature_column_names,
      String::SubString(feature_columns_str));

    return FeatureContainer::instance().resolve_features(
      feature_column_names,
      feature_columns);
  }

  bool InputCsvStream::process(OutputLibSvm& lib_svm)
  {
    //static const char* FUN = "InputCsvStream::process";

    FileLock file_lock(file_.path.c_str());

    if (file_lock.try_lock())
    {
      __gnu_cxx::stdio_filebuf<char> filebuf(file_lock.fd(), std::ios::in); 
      std::istream stream(&filebuf);

      process(lib_svm, stream, logger_, file_.path.c_str());

      return true;
    }

    return false;
  }

  void
  InputCsvStream::process(
    OutputLibSvm& lib_svm,
    std::istream& stream,
    Logging::Logger* logger,
    const char* file_path)
  {
    static const char* FUN = "InputCsvStream::process_()";

    FeatureColumns feature_columns;

    unsigned long label_index = parse_header_(stream, feature_columns);

    // fetch input (values)
    AdServer::CampaignSvcs::CalculateParamsFiller calc_params_filler;

    unsigned long line_i = 0;
    std::vector<std::string> feature_column_values;
    feature_column_values.reserve(feature_columns.size());
    while(!stream.eof())
    {
      // parse line
      std::string line;
      std::getline(stream, line);
      if(line.empty())
      {
        continue;
      }

      feature_column_values.resize(0);

      bool skip_line = false;
      AdServer::CampaignSvcs::CTRGenerator::CalculateParams calc_params;
      std::string label;

      try
      {
        ::Commons::CsvReader::parse_line(feature_column_values, line);
        label = feature_column_values[label_index];

        unsigned long column_i = 0;
        for(auto feature_it = feature_columns.begin();
            feature_it != feature_columns.end();
            ++feature_it, ++column_i)
        {
          if(feature_it->second >= feature_column_values.size())
          {
            Stream::Error ostr;
            ostr << FUN << ":";
            if (file_path)
            {
              ostr << " file '" << file_path << "'";
            }
            ostr << " line #" <<
              line_i << ", column #" << feature_it->second <<
              " out of parsed columns (" << feature_column_values.size() << ")";
            throw ParseError(ostr);
          }

          try
          {
            calc_params_filler.set_value(
              calc_params,
              feature_it->first,
              feature_column_values[feature_it->second]);
          }
          catch(const eh::Exception& ex)
          {
            if (logger)
            {
              Stream::Error ostr;
              ostr << FUN << ":";
              if (file_path)
              {
                ostr << " file '" << file_path << "'";
              }
              ostr << " line #" << line_i << ", column #" << feature_it->second <<
                ", feature_id = " << feature_it->first << ": " << ex.what();
              logger->log(ostr.str(),
                Logging::Logger::ERROR,
                ASPECT);
            }

            skip_line = true;
            break;
          }
        }
      }
      catch(const eh::Exception& ex)
      {
        if (logger)
        {
          Stream::Error ostr;
          ostr << FUN << ":";
          if (file_path)
          {
            ostr << " file '" << file_path << "'";
          }
          ostr << " line #" << line_i << ": " << ex.what();

          logger->log(ostr.str(), Logging::Logger::ERROR, ASPECT);
        }

        skip_line = true;
      }

      ++line_i;

      if(skip_line)
      {
        continue;
      }

      lib_svm.write_record(label, calc_params);
    }
  }

  InputCsvStream::~InputCsvStream() noexcept
  {}
}
