/* $Id: CTRPredictorSVMGeneratorImpl.cpp 185978 2020-07-04 00:12:49Z jurij_kuznecov $
* @file CTRPredictorSVMGeneratorImpl.cpp
* @author Artem V. Nikitin (artem_nikitin@ocslab.com)
* Predictor CTR lobsvm generator implemenation
*/

#include <list>
#include <fstream>
#include <Generics/DirSelector.hpp>
#include "CTRPredictorSVMGeneratorImpl.hpp"
#include "CampaignFolder.hpp"
#include "OutputLibSvmStream.hpp"
#include "Utils.hpp"
#include "InputCsvStream.hpp"


namespace
{
  const Generics::Time TIMEOUT(5l);  // 5 seconds
}

namespace AdServer::Predictor
{
  // CTRPredictorSVMGeneratorImpl
  CTRPredictorSVMGeneratorImpl::CTRPredictorSVMGeneratorImpl(
      Logging::Logger* logger,
      Generics::ActiveObjectCallback* callback,
      const char* input_path,
      const char* output_path,
      unsigned long days_to_keep,
      unsigned long feature_dimension)
    /*throw(eh::Exception)*/ :
    Commons::DelegateActiveObject(callback, 1),
    logger_(ReferenceCounting::add_ref(logger)),
    input_path_(input_path),
    output_path_(output_path),
    keep_interval_(Generics::Time::ONE_DAY * days_to_keep),
    feature_dimension_(feature_dimension)
  {}

  void
  CTRPredictorSVMGeneratorImpl::process_files_() noexcept
  {
    static const char* FUN = "CTRPredictorSVMGeneratorImpl::process_files_";

    try
    {
      unsigned long index_shifter = sizeof(uint32_t) * 8 - feature_dimension_;
      CampaignFoldersContainer container(
        input_path_.c_str(), keep_interval_, logger_);

      while (active())
      {
        CampaignFolder::FileList file_list;
        container.get_files(file_list);

        if (file_list.empty())
        {
          break;
        }

        if (check_files_changed(file_list, output_path_))
        {
          {
            Stream::Error ostr;
            const FileObject& file = file_list.front().second;
            ostr << FUN << ": Process '" << file_list.size() << "' files for '" <<
              file.timestamp.get_gm_time().format("%Y-%m-%d %H") << "'";
            logger_->log(
              ostr.str(),
              Logging::Logger::INFO,
              ASPECT);
          }

          GlobalSvmFiles global_svm(file_list, output_path_);

          OutputLibSvm_var global_svm_stream = new OutputLibSvmSet(
            GlobalSvmFiles(file_list, output_path_),
            index_shifter,
            logger_);

          for (CampaignFolder::FileList::const_iterator it = file_list.begin();
            it != file_list.end(); ++it)
          {
            if(logger_->log_level() >= Logging::Logger::TRACE)
            {
              Stream::Error ostr;
              ostr << FUN << ": Process file: '" << it->second.path << "'";
              logger_->log(ostr.str(),
                Logging::Logger::TRACE,
                ASPECT);
            }

            OutputLibSvmList output_svms;
            output_svms.emplace_back(global_svm_stream);
            if (check_file_is_changed(*it, output_path_))
            {
              output_svms.emplace_back(new OutputLibSvmSet(
                CampaignSvmFiles(*it, output_path_),
                index_shifter,
                logger_));
            }

            OutputLibSvm_var output_svm = new OutputLibSvmListWrapper(
              output_svms);

            if (!InputCsvStream(it->second, logger_).process(*output_svm))
            {
              Stream::Error ostr;
              ostr << FUN << ": Can't process: '" << it->second.path <<
                "': possible locked";
              logger_->log(ostr.str(),
                Logging::Logger::WARNING,
                ASPECT); 
            }
          }
        }
      }
    }
    catch (const eh::Exception &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": got Exception: " << ex.what();
      logger_->log(ostr.str(),
        Logging::Logger::ERROR,
        ASPECT);
    }
  }

  void
  CTRPredictorSVMGeneratorImpl::work_() noexcept
  {
    while(active())
    {
      // Process files
      process_files_();

      {
        SyncPolicy::WriteGuard lock(timeout_cond_lock_);
        timeout_condition_.timed_wait(timeout_cond_lock_, &TIMEOUT, true);
      }
    }
  }

  void
  CTRPredictorSVMGeneratorImpl::terminate_() noexcept
  {
    SyncPolicy::WriteGuard lock(timeout_cond_lock_);
    timeout_condition_.broadcast();
  }

  CTRPredictorSVMGeneratorImpl::~CTRPredictorSVMGeneratorImpl() noexcept
  {}
}
