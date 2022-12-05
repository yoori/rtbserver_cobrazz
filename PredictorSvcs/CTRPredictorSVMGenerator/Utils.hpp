/* $Id: Utils.hpp 185976 2020-07-01 22:11:23Z jurij_kuznecov $
* @file Utils.hpp
* @author Artem V. Nikitin (artem_nikitin@ocslab.com)
* Utils
*/

#include <Logger/Logger.hpp>
#include <string>
#include <list>

#pragma once

namespace AdServer
{
  namespace Predictor
  {

    static const char ASPECT[] = "CTRPredictorSVMGenerator";
    static const char CSV_EXTENSION[] = ".csv";
    static const char DICT_EXTENSION[] = ".dict";
    static const char LIBSVM_EXTENSION[] = ".libsvm";
    static const char LIBSVM_FILENAME_TIME_FORMAT[] = "%Y%m%d%H%M%S";
    static const char CSV_MASK[] = "PRImpression_*.csv";

    typedef std::list<std::string> FileList;

    /**
     * @brief remove extension from file name.
     *
     * @param file name
     */
    std::string remove_ext(
      const std::string& filename);

    /**
     * @brief remove files by mask.
     *
     * @param path
     * @param mask
     * @param logger
     */
    void
    remove_files (
      const char* path,
      const char* mask,
      Logging::Logger* logger) noexcept;

    /**
     * @brief remove obsolete files by mask.
     *
     * @param path
     * @param mask
     * @param interval to keep fresh files
     * @param logger
     */
    void
    remove_obsolete_files(
      const char* path,
      const char* mask,
      Generics::Time keep_interval,
      Logging::Logger* logger) noexcept;
  }
}
