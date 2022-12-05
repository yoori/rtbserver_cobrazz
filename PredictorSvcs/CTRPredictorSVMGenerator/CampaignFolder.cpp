/* $Id: CampaignFolder.cpp 185978 2020-07-04 00:12:49Z jurij_kuznecov $
* @file CampaignFolder.cpp
* @author Artem V. Nikitin (artem_nikitin@ocslab.com)
* Campaign folder
*/

#include <functional>
#include "CampaignFolder.hpp"
#include "Utils.hpp"
#include <sstream>

namespace {
  typedef std::list<std::string> FileList;
  static const char CSV_FILENAME_TIME_FORMAT[] = "%Y%m%d%H";
}

namespace AdServer
{
  namespace Predictor
  {

    // Utils

    void
    get_log_time(
      const std::string& filename,
      Generics::Time& time) /*throw(InvalidFile)*/
    {
      std::string filename_no_ext = remove_ext(filename);
      // Get timestamp from name
      std::size_t underscore_found = filename_no_ext.find_last_of('_');
      if (underscore_found != std::string::npos)
      {
        std::string filename_time_str = filename_no_ext.substr(
          underscore_found + 1);
        time.set(
          filename_time_str,
          CSV_FILENAME_TIME_FORMAT);
        return;
      }

      Stream::Error ostr;
      ostr << "Unexpected log filename'" << filename << "': invalid timestamp";
      throw InvalidFile(ostr);
    }

    struct LogTimeCompare
    {
      bool operator()(
        const FileObject& lhs,
        const FileObject& rhs) const
      {
        return (lhs.timestamp > rhs.timestamp);
      }
    };


    bool check_file_obsolete(
      Generics::Time fresh_interval,
      const FileObject& file)
    {
      Generics::Time now = Generics::Time::get_time_of_day();
      return (now - file.timestamp) > fresh_interval;
    }

    bool check_file_is_changed(
      const CampaignFolder::FilePair& log_pair,
      const std::string& output_path)
    {
      std::ostringstream output_campaign_path;
      output_campaign_path << output_path << "/" << log_pair.first;

      std::string in_filename_no_ext = remove_ext(log_pair.second.filename);
      std::string filemask(in_filename_no_ext + "*" + LIBSVM_EXTENSION);

      FileObjectList file_list;

      Generics::DirSelect::directory_selector(
        output_campaign_path.str().c_str(),
        Generics::DirSelect::list_creator(std::back_inserter(file_list)),
        filemask.c_str(),
        Generics::DirSelect::DSF_DONT_RESOLVE_LINKS);

      if (!file_list.empty())
      {

        const FileObject& out_file = file_list.front();
        std::string out_filename_no_ext = remove_ext(out_file.filename);
        // Get timestamp from name
        std::size_t underscore_found = out_filename_no_ext.find_last_of('_');
        if (underscore_found != std::string::npos)
        {
          std::string filename_time_str = out_filename_no_ext.substr(
            underscore_found + 1);

          Generics::Time filename_time(
            filename_time_str,
            LIBSVM_FILENAME_TIME_FORMAT);

          return abs( (filename_time - log_pair.second.last_modify).tv_sec ) > 1;
        }
      }
      return true;
    }

    bool check_files_changed(
      const CampaignFolder::FileList& file_list,
      const std::string& output_path)
    {
      using namespace std::placeholders;
      return
        std::find_if(file_list.begin(), file_list.end(),
          std::bind(
            check_file_is_changed, _1, output_path)) != file_list.end();
        
    }

    
    // FileObject

    FileObject::FileObject(const char* filepath) :
      path(filepath),
      filename(Generics::DirSelect::file_name(filepath))
    {
      
      get_log_time(filename, timestamp);
      
      struct stat fs;
      
      if(::stat(filepath, &fs) < 0)
      {
        Stream::Error ostr;
        ostr << "Failed stat: '" << filepath << "'";
        throw InvalidFile(ostr);
      }
      
      last_modify.set(fs.st_mtime);
    }
    


    // class CampaignFolder

    CampaignFolder::CampaignFolder(
      const std::string& full_path,
      unsigned long campaign_id,
      Generics::Time keep_interval) :
      full_path_(full_path),
      campaign_id_(campaign_id)
    {
      Generics::DirSelect::directory_selector(
        full_path_.c_str(),
        Generics::DirSelect::list_creator(std::back_inserter(file_list_)),
        CSV_MASK,
        Generics::DirSelect::DSF_DONT_RESOLVE_LINKS |
        Generics::DirSelect::DSF_EXCEPTION_ON_OPEN);

      file_list_.sort(LogTimeCompare());
      
      using namespace std::placeholders;
      file_list_.remove_if(
        std::bind(check_file_obsolete, keep_interval, _1));
    }

    const FileObjectList&
    CampaignFolder::get_files() const
    {
      return file_list_;
    }
    
    void
    CampaignFolder::move_file_to_list(
      Generics::Time timestamp,
      CampaignFolder::FileList& file_pairs)
    {
      if (!file_list_.empty() && file_list_.front().timestamp == timestamp)
      {
        file_pairs.push_back(std::make_pair(campaign_id_, file_list_.front()));
        file_list_.pop_front();
      }
    }
    

    // CampaignFoldersContainer
    
    CampaignFoldersContainer::CampaignFoldersContainer(
      const char* path,
      Generics::Time keep_interval,
      Logging::Logger* logger) :
      path_(path),
      keep_interval_(keep_interval),
      logger_(ReferenceCounting::add_ref(logger))
    {

      static const char* FUN = "CampaignFoldersContainer::CampaignFoldersContainer";
      
      FileList folder_list;

      Generics::DirSelect::directory_selector(
        path,
        Generics::DirSelect::list_creator(std::back_inserter(folder_list)),
        "[1-9]*",
        Generics::DirSelect::DSF_DONT_RESOLVE_LINKS |
        Generics::DirSelect::DSF_ALL_FILES);
      
      std::list<CampaignFolder> campaign_folder_list;
      for (auto it = folder_list.begin(); it != folder_list.end(); ++it)
      {
        struct stat path_stat;
        stat(it->c_str(), &path_stat);
        if (!S_ISREG(path_stat.st_mode))
        {
          unsigned long campaign_id;
          if(!String::StringManip::str_to_int(
               String::SubString(
                 Generics::DirSelect::file_name(it->c_str())),
               campaign_id))
          {
            Stream::Error ostr;
            ostr << FUN << ": Unexpected path '" << *it << "': doesn't contain campaign_id";
            throw InvalidCampaignPath(ostr);
          }
          campaign_folder_list_.emplace_back(
              *it, campaign_id, keep_interval);
        }
      }
      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        Stream::Error ostr;
        ostr << FUN << ": Found '" << campaign_folder_list_.size() <<
          "' files in the '" << path_ << "'";
        logger_->log(
          ostr.str(),
          Logging::Logger::TRACE,
          ASPECT);
      }
    }
      
    void CampaignFoldersContainer::get_files(
      CampaignFolder::FileList& file_pairs)
    {
      // Find minimal timestamp
      std::list<Generics::Time> file_timestamps;
      for (FolderTable::const_iterator it = campaign_folder_list_.begin();
           it != campaign_folder_list_.end(); ++it)
      {
        const FileObjectList& files = it->get_files();
        if (!files.empty())
        {
          file_timestamps.push_back(files.front().timestamp);
        }
      }

      if (!file_timestamps.empty())
      {
        Generics::Time max_time(
          *std::max_element(
            file_timestamps.begin(),
            file_timestamps.end()));
        
        // Get files
        for (FolderTable::iterator it = campaign_folder_list_.begin();
             it != campaign_folder_list_.end(); ++it)
        {
          it->move_file_to_list(max_time, file_pairs);
        }
      }
    }

    CampaignFoldersContainer::~CampaignFoldersContainer() noexcept
    {
      remove_obsolete_files(
        path_.c_str(), CSV_MASK,
        keep_interval_, logger_);
    }
  }
}

    


