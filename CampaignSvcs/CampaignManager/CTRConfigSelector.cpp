#include <cassert>

#include <Generics/DirSelector.hpp>
#include <String/RegEx.hpp>

#include "CTRConfigSelector.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  namespace
  {
    const String::SubString CONFIG_FOLDER_NAME_REGEXP(
      "\\d{8}\\.\\d{6}(\\.(CTR|VTR))?");
    constexpr std::size_t CONFIG_TIMESTAMP_LENGTH = 15;

    struct ConfigSelector
    {
      ConfigSelector()
        : reg_exp(CONFIG_FOLDER_NAME_REGEXP)
      {}

      bool operator()(const char* full_path, const struct stat& file_stat) noexcept
      {
        if (S_ISDIR(file_stat.st_mode))
        {
          String::RegEx::Result sub_strs;
          String::SubString file_name(Generics::DirSelect::file_name(full_path));
          if (
              reg_exp.search(sub_strs, file_name) &&
              (assert(!sub_strs.empty()), sub_strs[0].length() == file_name.size()) &&
              result_folder < file_name)
          {
            file_name.assign_to(result_folder);
          }
        }
        return false;
      }

      std::string result_folder;
      String::RegEx reg_exp;
    };
  }

  Generics::Time
  select_latest_config(std::string& config_root, const String::SubString& check_root)
  {
    const char FOLDER_NAME_FORMAT[] = "%Y%m%d.%H%M%S";
    ConfigSelector selector;
    const std::string check_root_string = check_root.str();
    Generics::DirSelect::directory_selector(
      check_root_string.c_str(),
      selector,
      "*",
      Generics::DirSelect::DSF_NON_RECURSIVE | Generics::DirSelect::DSF_ALL_FILES);

    if (!selector.result_folder.empty())
    {
      config_root = check_root_string + "/" + selector.result_folder;
      try
      {
        return Generics::Time(
          selector.result_folder.substr(0, CONFIG_TIMESTAMP_LENGTH),
          FOLDER_NAME_FORMAT);
      }
      catch (const eh::Exception&)
      {}
    }

    config_root.clear();
    return Generics::Time::ZERO;
  }
}
