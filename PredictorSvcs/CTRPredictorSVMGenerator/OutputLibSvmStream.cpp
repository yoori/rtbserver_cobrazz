/* $Id: OutputLibSvmStream.cpp 185976 2020-07-01 22:11:23Z jurij_kuznecov $
* @file OutputLibSvmStream.cpp
* @author Artem V. Nikitin (artem_nikitin@ocslab.com)
* Output libsvm stream implementation
*/

#include "OutputLibSvmStream.hpp"
#include <LogCommons/CsvUtils.hpp>
#include <String/StringManip.hpp>
#include <eh/Errno.hpp>
#include "Utils.hpp"

namespace
{
  const char GLOBAL_FOLDER[] = "Global";
  const mode_t DIR_PERMS = 0777;
}

namespace AdServer
{
  namespace Predictor
  {

    DECLARE_EXCEPTION(MkDirException, eh::DescriptiveException);
    
    // CampaignSvmFiles
    CampaignSvmFiles::CampaignSvmFiles(
      const CampaignFolder::FilePair& log_pair,
      const std::string& path)
    {
      filename_no_ext = remove_ext(log_pair.second.filename);
      std::ostringstream out_path;
      out_path  << path <<  "/" << log_pair.first;
      output_path = out_path.str();

      if (mkdir(output_path.c_str(), DIR_PERMS) == -1 &&
        errno != EEXIST)
      {
        eh::throw_errno_exception<MkDirException>(
          "CampaignSvmFiles::CampaignSvmFiles(): failed to create folder '",
          output_path, "'");
      }

      std::string output_path_prefix(
        output_path + "/" + filename_no_ext + "_" +
        log_pair.second.last_modify.get_gm_time().format(LIBSVM_FILENAME_TIME_FORMAT));
      svm_path = output_path_prefix + LIBSVM_EXTENSION;
      dict_path = output_path_prefix + DICT_EXTENSION;

    }

    
    // GlobalSvmFiles

    GlobalSvmFiles::GlobalSvmFiles(
      const CampaignFolder::FileList& files,
      const std::string& path)
    {
      assert(!files.empty());
      
      output_path = path + "/" + GLOBAL_FOLDER;

      if (mkdir(output_path.c_str(), DIR_PERMS) == -1 &&
        errno != EEXIST)
      {
        eh::throw_errno_exception<MkDirException>(
          "CampaignSvmFiles::CampaignSvmFiles(): failed to create folder '",
          output_path, "'");
      }

      std::list<Generics::Time> file_last_modifies;
      for (CampaignFolder::FileList::const_iterator it = files.begin();
           it != files.end(); ++it)
      {
        const FileObject& file = it->second;
        file_last_modifies.push_back(file.last_modify);
      }

      Generics::Time max_last_modify(
        *std::max_element(
          file_last_modifies.begin(),
          file_last_modifies.end()));

      filename_no_ext = remove_ext(
        files.front().second.filename);
      
      std::string output_path_prefix(
        output_path + "/" + filename_no_ext + "_" +
        max_last_modify.get_gm_time().format(LIBSVM_FILENAME_TIME_FORMAT));

      svm_path = output_path_prefix + LIBSVM_EXTENSION;
      dict_path = output_path_prefix + DICT_EXTENSION;
    }

    // class OutputLibSvmStream

    void OutputLibSvmStream::write_record(
        const std::string& label,
        const AdServer::CampaignSvcs::CTRGenerator::CalculateParams& calc_params)
    {
      AdServer::CampaignSvcs::CTRGenerator::Calculation ctr_calculation;
      ctr_generator_.calculate(ctr_calculation, calc_params);
      
      // output hashes
      std::ostringstream res_line_ostr;
      res_line_ostr << label;
      
      std::map<unsigned long, unsigned long> ordered_hashes;
      
      for(auto hash_it = ctr_calculation.hashes.begin();
          hash_it != ctr_calculation.hashes.end(); ++hash_it)
      {
        unsigned long index = hash_it->first >> index_shifter_;
        ordered_hashes.insert(std::make_pair(index + 1, hash_it->second));
      }
      
      for(auto hash_it = ordered_hashes.begin();
          hash_it != ordered_hashes.end(); ++hash_it)
      {
        res_line_ostr << ' ' << hash_it->first << ":" << hash_it->second;
      }
      
      // fill dict
      ctr_generator_.fill_dictionary(dict_table_, calc_params);
      
      // push line to output
      svm_stream_ << res_line_ostr.str() << std::endl;
    }
    
    void OutputLibSvmStream::write_dictionary_() noexcept
    {
      for(auto it = dict_table_.begin(); it != dict_table_.end(); ++it)
      {
        dict_stream_ << ((it->first >> index_shifter_) + 1) << ",";
        if(!it->second.empty())
        {
          AdServer::LogProcessing::write_not_empty_string_as_csv(
            dict_stream_,
            it->second);
        }
        dict_stream_ << std::endl;
      }
    }
    
    OutputLibSvmStream::~OutputLibSvmStream() noexcept
    {
      write_dictionary_();
      
      // remove old files
      remove_files(
        output_path_.c_str(), dict_mask_.c_str(), logger_);
      remove_files(
        output_path_.c_str(), svm_mask_.c_str(), logger_);        

      dict_stream_.close();
      svm_stream_.close();
    }
  }
}

