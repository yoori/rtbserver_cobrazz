/* $Id: OutputLibSvmStream.hpp 185976 2020-07-01 22:11:23Z jurij_kuznecov $
* @file OutputLibSvmStream.hpp
* @author Artem V. Nikitin (artem_nikitin@ocslab.com)
* Output libsvm stream
*/

#pragma once

#include "CampaignFolder.hpp"
#include "FeatureContainer.hpp"
#include "FileGuard.hpp"
#include <Utils/CTRGenerator/CalculateParamsFilter.hpp>
#include "Utils.hpp"

namespace AdServer
{
  namespace Predictor
  {
    /**
     * Campaign libsvm files descriptor
     */
    struct CampaignSvmFiles
    {
      /**
       * @brief Constructor.
       *
       * @param campaign_id, FileObject pair
       * @param libsvm root path
       */
      CampaignSvmFiles(
        const CampaignFolder::FilePair& log_pair,
        const std::string& path);

      std::string output_path;
      std::string filename_no_ext;
      std::string svm_path;
      std::string dict_path;
    };

    /**
     * Global libsvm files descriptor
     */
    struct GlobalSvmFiles
    {
      /**
       * @brief Constructor.
       *
       * @param campaign_id, FileObject pair list
       * @param libsvm root path
       */
      GlobalSvmFiles(
        const CampaignFolder::FileList& log_files,
        const std::string& path);

      std::string output_path;
      std::string filename_no_ext;
      std::string svm_path;
      std::string dict_path;
    };


    /**
     * Libsvm & dictionary output streams
     */
    class OutputLibSvmStream :
       public ReferenceCounting::AtomicCopyImpl
    {
    public:

      /**
       * @brief Constructor.
       *
       * @param libsvm descriptor
       * @param index shifter
       * @param logger
       */
      template <class SvmFiles>
      OutputLibSvmStream(
        const SvmFiles& files,
        unsigned long index_shifter,
        Logging::Logger* logger) :
        dict_guard_(files.dict_path),
        svm_guard_(files.svm_path),
        dict_stream_(dict_guard_.tmp_filepath(), std::ios_base::out),
        svm_stream_(svm_guard_.tmp_filepath(), std::ios_base::out),
        index_shifter_(index_shifter),
        ctr_generator_(FeatureContainer::instance().features(), true),
        svm_mask_(files.filename_no_ext + "*" + LIBSVM_EXTENSION),
        dict_mask_(files.filename_no_ext + "*" + DICT_EXTENSION),
        output_path_(files.output_path),
        logger_(ReferenceCounting::add_ref(logger))
      { }

      /**
       * @brief Write libsvm record.
       *
       * @param label
       * @param params calculator
       */
      void write_record(
        const std::string& label,
        const AdServer::CampaignSvcs::CTRGenerator::CalculateParams& calc_params);

      /**
       * @brief Destructor.
       */
      virtual ~OutputLibSvmStream() noexcept;

    private:

      /**
       * @brief Write dictionary.
       */
      void write_dictionary_() noexcept;

    private:
      FileGuard dict_guard_;
      FileGuard svm_guard_;
      std::ofstream dict_stream_;
      std::ofstream svm_stream_;
      AdServer::CampaignSvcs::CTRGenerator::FeatureDictionary dict_table_;
      unsigned long index_shifter_;
      AdServer::CampaignSvcs::CTRGenerator ctr_generator_;
      std::string svm_mask_;
      std::string dict_mask_;
      std::string output_path_;
      Logging::Logger_var logger_;
    };

    typedef ReferenceCounting::SmartPtr<OutputLibSvmStream> OutputLibSvmStream_var;
    typedef std::list<OutputLibSvmStream_var> OutputLibSvmStreamList;
  }
}

