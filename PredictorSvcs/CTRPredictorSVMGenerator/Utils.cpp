/* $Id: Utils.cpp 185976 2020-07-01 22:11:23Z jurij_kuznecov $
* @file Utils.cpp
* @author Artem V. Nikitin (artem_nikitin@ocslab.com)
* Utils
*/

#include <unistd.h>
#include "Utils.hpp"
#include <Generics/DirSelector.hpp>


namespace AdServer
{
  namespace Predictor
  {

    std::string remove_ext(
      const std::string& filename)
    {
      return filename.substr(0, filename.rfind('.'));
    }

    void
    remove_files (
      const char* path,
      const char* mask,
      Logging::Logger* logger) noexcept
    {

      static const char* FUN = "remove_files";
      
      try
      {
        
        FileList file_list;
        Generics::DirSelect::directory_selector(
          path,
          Generics::DirSelect::list_creator(std::back_inserter(file_list)),
          mask,
          Generics::DirSelect::DSF_DONT_RESOLVE_LINKS |
          Generics::DirSelect::DSF_EXCEPTION_ON_OPEN);

        for (FileList::const_iterator it = file_list.begin();
             it != file_list.end(); ++it)
        {
          struct stat fs;
          
          if(::stat(it->c_str(), &fs) < 0)
          {
            Stream::Error ostr;
            ostr << FUN << ": Failed stat: '" << *it << "'";
            logger->log(ostr.str(),
              Logging::Logger::CRITICAL,
              ASPECT);
          }
          else
          {
            Generics::Time mod_time(fs.st_mtime);
            Generics::Time now = Generics::Time::get_time_of_day();
            
            if(logger->log_level() >= Logging::Logger::TRACE)
            {
              Stream::Error ostr;
              ostr << FUN << ": Remove file: '" << *it << "'";
              logger->log(ostr.str(),
                Logging::Logger::TRACE,
                ASPECT);
            }
            unlink(it->c_str());
          }
        }
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Unecpected exception: " << ex.what();
        logger->log(ostr.str(),
          Logging::Logger::ERROR,
          ASPECT);
      }
    }

    void
    remove_obsolete_files(
      const char* path,
      const char* mask,
      Generics::Time keep_interval,
      Logging::Logger* logger) noexcept
    {
      static const char* FUN = "remove_obsolete_files";
      
      try
      {
        FileList file_list;
        Generics::DirSelect::directory_selector(
          path,
          Generics::DirSelect::list_creator(std::back_inserter(file_list)),
          mask,
          Generics::DirSelect::DSF_DONT_RESOLVE_LINKS |
          Generics::DirSelect::DSF_EXCEPTION_ON_OPEN);
        
        for (FileList::const_iterator it = file_list.begin();
             it != file_list.end(); ++it)
        {
          struct stat fs;
          
          if(::stat(it->c_str(), &fs) < 0)
          {
            Stream::Error ostr;
            ostr << FUN << ": Failed stat: '" << *it << "'";
            logger->log(ostr.str(),
              Logging::Logger::CRITICAL,
              ASPECT);
          }
          else
          {
            Generics::Time mod_time(fs.st_mtime);
            Generics::Time now = Generics::Time::get_time_of_day();
            
            if (now - mod_time >= keep_interval)
            {
              Stream::Error ostr;
              ostr << FUN << ": Remove file '" << *it << "'" <<
                ", " << "last modification: '" <<
                mod_time.get_gm_time().format("%Y-%m-%d %H:%M:%S") <<
                "' more than " <<
                keep_interval.tv_sec / Generics::Time::ONE_DAY.tv_sec <<
                " days";
              
              logger->log(ostr.str(),
                Logging::Logger::INFO,
                ASPECT);
              
              unlink(it->c_str());
            }
          }
        }
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Unecpected exception: " << ex.what();
        logger->log(ostr.str(),
          Logging::Logger::ERROR,
          ASPECT);

      }
    }

  }
}
    
