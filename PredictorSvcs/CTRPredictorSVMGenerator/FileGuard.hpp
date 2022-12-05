/* $Id: FileGuard.hpp 185976 2020-07-01 22:11:23Z jurij_kuznecov $
* @file FileGuard.hpp
* @author Artem V. Nikitin (artem_nikitin@ocslab.com)
* File guard
*/

#pragma once

#include <string>

namespace AdServer
{
  namespace Predictor
  {
    /**
     * File guard, rename to file~ before start processing
     * and rename back after finish processing
     */
    class FileGuard
    {
    public:

      /**
       * @brief Constructor.
       *
       * @param file path
       * @param check or not file exists when finish
       */
      FileGuard(
        const std::string& filepath,
        bool check_exist = false);

      /**
       * @brief Get original file path.
       *
       * @return file path
       */
      const std::string& filepath() const;

      /**
       * @brief Get temporary file path (with ~).
       *
       * @return file path
       */
      const std::string& tmp_filepath() const;
      
      /**
       * @brief Destructor.
       */
      virtual ~FileGuard() noexcept;
      
    private:
      std::string filepath_;
      std::string tmp_filepath_;
      bool check_exist_;
    };
  }
}
