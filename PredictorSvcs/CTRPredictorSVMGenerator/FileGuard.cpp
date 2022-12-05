/* $Id: FileGuard.cpp 185976 2020-07-01 22:11:23Z jurij_kuznecov $
* @file FileGuard.cpp
* @author Artem V. Nikitin (artem_nikitin@ocslab.com)
* File guard implementation
*/

#include "FileGuard.hpp"
#include <Commons/FileManip.hpp>

namespace AdServer
{
  namespace Predictor
  {
    FileGuard::FileGuard(
      const std::string& filepath,
      bool check_exist) :
      filepath_(filepath),
      tmp_filepath_(filepath_ + "~"),
      check_exist_(check_exist)
    {
      AdServer::FileManip::rename(filepath_, tmp_filepath_, !check_exist); 
    }
    
    const std::string& FileGuard::filepath() const
    {
        return filepath_;
    }
      
    const std::string& FileGuard::tmp_filepath() const
    {
      return tmp_filepath_;
    }
      
    
    FileGuard::~FileGuard() noexcept
    {
      if (!(check_exist_ && AdServer::FileManip::file_exists(filepath_)))
      {
        AdServer::FileManip::rename(tmp_filepath_, filepath_, true); 
      }
    }
  }
}
 
