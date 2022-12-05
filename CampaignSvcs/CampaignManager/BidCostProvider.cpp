#include <cassert>
#include <fstream>
#include <Generics/Singleton.hpp>
#include <Generics/Rand.hpp>
#include <Generics/DirSelector.hpp>
#include <String/RegEx.hpp>

#include "BidCostProvider.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  namespace
  {
    const std::string COST_FILE_NAME("bid_cost.csv");
    const String::SubString BIDCOST_CONFIG_FOLDER_NAME_REGEXP("\\d{8}\\.\\d{6}");

    bool
    str_to_revenue_decimal(const String::SubString& str, RevenueDecimal& value)
    {
      try
      {
        value = RevenueDecimal(str);
        return true;
      }
      catch(const eh::Exception& ex)
      {
        return false;
      }
    }
  };

  class RemoveConfigTask: public Generics::Task,
    public ReferenceCounting::AtomicImpl
  {
  public:
    typedef std::list<std::string> FileList;

  public:
    RemoveConfigTask(
      const FileList& config_files,
      const FileList& config_directories)
      /*throw(eh::Exception)*/;

    virtual void
    execute() noexcept;

  protected:
    virtual
    ~RemoveConfigTask() noexcept = default;

  private:
    const FileList config_files_;
    const FileList config_directories_;
  };

  // RemoveConfigTask
  RemoveConfigTask::RemoveConfigTask(
    const FileList& config_files,
    const FileList& config_directories)
    /*throw(eh::Exception)*/
    : config_files_(config_files),
      config_directories_(config_directories)
  {}

  void
  RemoveConfigTask::execute() noexcept
  {
    for(FileList::const_iterator fit = config_files_.begin();
      fit != config_files_.end();
      ++fit)
    {
      ::unlink(fit->c_str());
    }

    for(FileList::const_iterator fit = config_directories_.begin();
      fit != config_directories_.end(); ++fit)
    {
      ::rmdir(fit->c_str());
    }
  }

  // BidCostConfigSelector
  struct BidCostConfigSelector
  {
    BidCostConfigSelector()
      : reg_exp_(BIDCOST_CONFIG_FOLDER_NAME_REGEXP)
    {}

    bool
    operator ()(const char* full_path, const struct stat& file_stat)
      noexcept
    {
      if(S_ISDIR(file_stat.st_mode))
      {
        String::RegEx::Result sub_strs;

        String::SubString file_name(
          Generics::DirSelect::file_name(full_path));

        if(reg_exp_.search(sub_strs, file_name) &&
          (assert(!sub_strs.empty()), sub_strs[0].length() == file_name.size()))
        {
          if(result_folder < file_name)
          {
            file_name.assign_to(result_folder);
          }
        }
      }

      return false; // check only top dirs
    }

    std::string result_folder;

  protected:
    String::RegEx reg_exp_;
  };

  // BidCostProvider
  BidCostProvider::BidCostProvider(
    const String::SubString& directory,
    const Generics::Time& config_timestamp,
    Generics::TaskRunner* task_runner)
    : task_runner_(ReferenceCounting::add_ref(task_runner)),
      config_timestamp_(config_timestamp),
      remove_config_files_at_destroy_(false)
  {
    load_(directory, COST_FILE_NAME);
  }

  BidCostProvider::~BidCostProvider() noexcept
  {
    try
    {
      clear_config_files_();
    }
    catch(const eh::Exception& ex)
    {
      std::cerr << "BidCostProvider::~BidCostProvider(): " << ex.what() << std::endl;
    }
  }

  void
  BidCostProvider::remove_config_files_at_destroy(bool val)
    const noexcept
  {
    RemoveFilesSyncPolicy::WriteGuard lock(remove_config_files_lock_);
    remove_config_files_at_destroy_ = val;
  }

  Generics::Time
  BidCostProvider::check_config_appearance(
    std::string& config_root,
    const String::SubString& check_root)
  {
    const char FOLDER_NAME_FORMAT[] = "%Y%m%d.%H%M%S";
    BidCostConfigSelector bid_cost_config_selector;

    std::string check_root_s = check_root.str();

    Generics::DirSelect::directory_selector(
      check_root_s.c_str(),
      bid_cost_config_selector,
      "*",
      Generics::DirSelect::DSF_NON_RECURSIVE | Generics::DirSelect::DSF_ALL_FILES);

    if(!bid_cost_config_selector.result_folder.empty())
    {
      config_root = check_root_s + "/" + bid_cost_config_selector.result_folder;
      try
      {
        return Generics::Time(bid_cost_config_selector.result_folder, FOLDER_NAME_FORMAT);
      }
      catch(const eh::Exception& ex)
      {}
    }

    config_root.clear();
    return Generics::Time::ZERO;
  }

  AdServer::Commons::Optional<RevenueDecimal>
  BidCostProvider::get_bid_cost(
    const RequestParams& request_params,
    const RevenueDecimal& allowable_lose_win_percentage,
    const RevenueDecimal& orig_bid_cost)
    const
  {
    /*
    std::cerr << "BidCostProvider::get_bid_cost(): tag_id = " << request_params.tag_id <<
      ", domain = " << request_params.domain << std::endl;
    */

    assert(cost_mapping_);
    if(request_params.tag_id == 0)
    {
      return AdServer::Commons::Optional<RevenueDecimal>();
    }
    
    CostMapping::Key key(request_params.tag_id, request_params.domain);
    auto cost_map_it = cost_mapping_->cost_map.find(key);
    if(cost_map_it == cost_mapping_->cost_map.end())
    {
      //std::cerr << "BidCostProvider::get_bid_cost(): cost element not found" << std::endl;
      return AdServer::Commons::Optional<RevenueDecimal>();
    }

    const auto& cells = cost_map_it->second->cells;
    auto cell_it = std::lower_bound(cells.begin(), cells.end(), CostMapping::Cell(
      allowable_lose_win_percentage, RevenueDecimal::ZERO, RevenueDecimal::ZERO));
    if(cell_it == cells.end())
    {
      /*
      std::cerr << "BidCostProvider::get_bid_cost(): cell not found #1 for "
        "allowable_lose_win_percentage = " << allowable_lose_win_percentage <<
        ", cells = [";
      for(auto it = cells.begin(); it != cells.end(); ++it)
      {
        std::cerr << " " << it->win_rate_reduction;
      }
      std::cerr << " ]" << std::endl;
      */
      return AdServer::Commons::Optional<RevenueDecimal>();
    }

    if(cell_it->win_rate_reduction < allowable_lose_win_percentage)
    {
      ++cell_it;
    }

    if(cell_it == cells.end())
    {
      //std::cerr << "BidCostProvider::get_bid_cost(): cell not found #2" << std::endl;
      return AdServer::Commons::Optional<RevenueDecimal>();
    }

    if(cell_it->max_sys_impression < orig_bid_cost)
    {
      //std::cerr << "BidCostProvider::get_bid_cost(): max_sys_impression < orig_bid_cost" << std::endl;
      return AdServer::Commons::Optional<RevenueDecimal>();
    }

    //std::cerr << "BidCostProvider::get_bid_cost(): return " << cell_it->sys_impression << std::endl;
    return AdServer::Commons::Optional<RevenueDecimal>(cell_it->sys_impression);
  }

  void
  BidCostProvider::load_cost_map_(
    BidCostProvider::CostMapping::CostMap& res,
    const String::SubString& cost_file_path)
  {
    std::ifstream cost_file(cost_file_path.str().c_str());
    if(!cost_file.is_open())
    {
      Stream::Error ostr;
      ostr << "Can't open '" << cost_file_path << "'";
      throw InvalidConfig(ostr);
    }

    while(!cost_file.eof())
    {
      std::string line;
      std::getline(cost_file, line);

      if(!line.empty())
      {
        // tag_id,domain,win_rate,cost,max_cost,result_win_rate
        String::StringManip::Splitter<
          String::AsciiStringManip::SepComma> tokenizer(line);

        String::SubString tag_id_str;
        String::SubString domain;
        String::SubString win_rate_str;
        String::SubString cost_str;
        String::SubString max_cost_str;

        uint32_t tag_id;
        RevenueDecimal win_rate;
        RevenueDecimal cost;
        RevenueDecimal max_cost;

        if(!tokenizer.get_token(tag_id_str) ||
          !tokenizer.get_token(domain) ||
          !tokenizer.get_token(win_rate_str) ||
          !tokenizer.get_token(cost_str) ||
          !tokenizer.get_token(max_cost_str) ||
          !String::StringManip::str_to_int(tag_id_str, tag_id) ||
          !str_to_revenue_decimal(win_rate_str, win_rate) ||
          !str_to_revenue_decimal(cost_str, cost) ||
          !str_to_revenue_decimal(max_cost_str, max_cost))
        {
          Stream::Error ostr;
          ostr << "Invalid line in '" << cost_file_path << "': line = '" << line << "'";
          throw InvalidConfig(ostr);
        }

        CostMapping::Key key(tag_id, domain);
        CostMapping::Cell cell(win_rate, cost, max_cost);
        CostMapping::Value_var& value = res[key];
        if(!value.in())
        {
          value = CostMapping::Value_var(new CostMapping::Value());
        }
        auto ins_it = std::upper_bound(value->cells.begin(), value->cells.end(), cell);
        value->cells.insert(ins_it, cell);
      }
    }
  }

  void
  BidCostProvider::clear_config_files_()
  {
    bool remove_config_files_at_destroy;

    {
      RemoveFilesSyncPolicy::ReadGuard lock(remove_config_files_lock_);
      remove_config_files_at_destroy = remove_config_files_at_destroy_;
    }

    if(remove_config_files_at_destroy && task_runner_)
    {
      task_runner_->enqueue_task(Generics::Task_var(
        new RemoveConfigTask(
          config_files_, config_directories_)));
    }
  }

  void
  BidCostProvider::load_(
    const String::SubString& directory,
    const String::SubString& file)
  {
    // parse config files
    config_files_.clear();

    std::string directory_str;
    directory.assign_to(directory_str);

    const std::string cost_file = directory_str + "/" + COST_FILE_NAME;

    CostMapping_var cost_mapping(new CostMapping());

    load_cost_map_(cost_mapping->cost_map, cost_file);

    config_files_.push_back(cost_file);

    config_directories_.push_back(directory_str);

    cost_mapping_ = cost_mapping;
  }

}
}
