/* $Id: OutputLibSvmStream.hpp 185976 2020-07-01 22:11:23Z jurij_kuznecov $
* @file OutputLibSvmStream.hpp
* @author Artem V. Nikitin (artem_nikitin@ocslab.com)
* Output libsvm stream
*/

#pragma once

#include <Utils/CTRGenerator/CalculateParamsFilter.hpp>

#include "Utils.hpp"
#include "CampaignFolder.hpp"
#include "FeatureContainer.hpp"
#include "FileGuard.hpp"

namespace AdServer::Predictor
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

  struct OutputLibSvm: public virtual ReferenceCounting::Interface
  {
    virtual void write_record(
      const std::string& label,
      const AdServer::CampaignSvcs::CTRGenerator::CalculateParams& calc_params) = 0;
  };

  /**
   * Libsvm & dictionary output streams
   */
  class OutputLibSvmStream:
    public OutputLibSvm,
    public ReferenceCounting::AtomicCopyImpl
  {
  public:
    OutputLibSvmStream(
      std::ostream& dict_stream,
      std::ostream& svm_stream,
      unsigned long index_shifter,
      Logging::Logger* logger)
      : dict_stream_(dict_stream),
        svm_stream_(svm_stream),
        index_shifter_(index_shifter),
        ctr_generator_(FeatureContainer::instance().features(), true),
        logger_(ReferenceCounting::add_ref(logger))
    {}

    void write_record(
      const std::string& label,
      const AdServer::CampaignSvcs::CTRGenerator::CalculateParams& calc_params)
      override;

    virtual ~OutputLibSvmStream() noexcept;

  private:
    void write_dictionary_() noexcept;

  protected:
    std::ostream& dict_stream_;
    std::ostream& svm_stream_;
    AdServer::CampaignSvcs::CTRGenerator::FeatureDictionary dict_table_;
    unsigned long index_shifter_;
    AdServer::CampaignSvcs::CTRGenerator ctr_generator_;
    Logging::Logger_var logger_;
  };

  typedef ReferenceCounting::SmartPtr<OutputLibSvm> OutputLibSvm_var;
  typedef std::list<OutputLibSvm_var> OutputLibSvmList;

  /**
   * Libsvm set & dictionary output streams
   */
  class OutputLibSvmSet:
    public OutputLibSvm,
    public ReferenceCounting::AtomicImpl
  {
  public:
    template <class SvmFiles>
    OutputLibSvmSet(
      const SvmFiles& files,
      unsigned long index_shifter,
      Logging::Logger* logger)
      : dict_guard_(files.dict_path),
        svm_guard_(files.svm_path),
        file_dict_stream_(dict_guard_.tmp_filepath(), std::ios_base::out),
        file_svm_stream_(svm_guard_.tmp_filepath(), std::ios_base::out),
        svm_mask_(files.filename_no_ext + "*" + LIBSVM_EXTENSION),
        dict_mask_(files.filename_no_ext + "*" + DICT_EXTENSION),
        output_path_(files.output_path),
        logger_(ReferenceCounting::add_ref(logger))
    {
      output_libsvm_stream_ = new OutputLibSvmStream(
        file_dict_stream_,
        file_svm_stream_,
        index_shifter,
        logger);
    }

    virtual ~OutputLibSvmSet() noexcept;

    void write_record(
      const std::string& label,
      const AdServer::CampaignSvcs::CTRGenerator::CalculateParams& calc_params)
      override;

  private:
    FileGuard dict_guard_;
    FileGuard svm_guard_;
    std::ofstream file_dict_stream_;
    std::ofstream file_svm_stream_;
    std::string svm_mask_;
    std::string dict_mask_;
    std::string output_path_;
    Logging::Logger_var logger_;
    OutputLibSvm_var output_libsvm_stream_;
  };

  class OutputLibSvmListWrapper:
    public OutputLibSvm,
    public ReferenceCounting::AtomicImpl
  {
  public:
    OutputLibSvmListWrapper(
      const OutputLibSvmList& svm_stream_list);

    void write_record(
      const std::string& label,
      const AdServer::CampaignSvcs::CTRGenerator::CalculateParams& calc_params)
      override;

  private:
    OutputLibSvmList svm_list_;
  };
}

