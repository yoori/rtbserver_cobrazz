#pragma once

#include <atomic>
#include <set>
#include <memory>
#include <mutex>
#include <string>
#include <shared_mutex>
#include <utility>
#include <vector>
#include <map>

#include <eh/Exception.hpp>

#include <Logger/Logger.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>
#include <Language/SegmentorCommons/SegmentorInterface.hpp>

#include <Commons/ProcessControlVarsImpl.hpp>
#include <Commons/UserInfoManip.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUtils.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUpdateBase.hpp>

#include <ChannelSvcs/ChannelServer/ChannelServerTypes.hpp>

#include <xsd/AdServerCommons/AdServerCommons.hpp>
#include <xsd/ChannelSvcs/ChannelServerConfig.hpp>

#include "ChannelContainer.hpp"
#include "UpdateContainer.hpp"

namespace AdServer::ChannelSvcs
{
  class DictionaryMatcher;
  class ChannelServerVariantBase;
  struct ChannelServerCoreState;
  class UpdateData;

  using ChannelServerVariantBasePtr = std::shared_ptr<ChannelServerVariantBase>;
  using UpdateDataPtr = std::shared_ptr<UpdateData>;

  typedef std::map<
    std::string,
    const Language::Segmentor::SegmentorInterface*> SegmMap;

  /**
   * Implementation of common part ChannelServer
   */
  class ChannelServerCore:
    public virtual Generics::CompositeActiveObject
  {

  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(NotConfigured, Exception);

    struct MatchQuery
    {
      std::string request_id;
      std::string first_url;
      std::string first_url_words;
      std::string urls;
      std::string urls_words;
      std::string pwords;
      std::string swords;
      AdServer::Commons::UserId uid;
      char statuses[2] = {'\0', '\0'};
      bool non_strict_word_match = false;
      bool non_strict_url_match = false;
      bool return_negative = false;
      bool simplify_page = false;
      bool fill_content = false;
    };

    struct ChannelAtom
    {
      unsigned long id = 0;
      unsigned long trigger_channel_id = 0;
    };

    struct ContentChannelAtom
    {
      unsigned long id = 0;
      unsigned long weight = 0;
    };

    struct TriggerMatchResult
    {
      std::vector<ChannelAtom> page_channels;
      std::vector<ChannelAtom> search_channels;
      std::vector<ChannelAtom> url_channels;
      std::vector<ChannelAtom> url_keyword_channels;
      std::vector<unsigned long> uid_channels;
    };

    struct MatchResult
    {
      TriggerMatchResult matched_channels;
      std::vector<ContentChannelAtom> content_channels;
      bool no_adv = false;
      bool no_track = false;
      bool full_loaded = false;
      Generics::Time match_time;
    };

    struct CCGKeyword
    {
      unsigned long ccg_keyword_id = 0;
      unsigned long ccg_id = 0;
      unsigned long channel_id = 0;
      CampaignSvcs::RevenueDecimal max_cpc;
      CampaignSvcs::CTRDecimal ctr;
      std::string click_url;
      std::string original_keyword;
    };

    struct TraitsResult
    {
      std::vector<CCGKeyword> ccg_keywords;
      std::vector<unsigned long> neg_ccg;
    };

    struct TriggerVersion
    {
      unsigned long id = 0;
      Generics::Time stamp;
      unsigned long size = 0;
    };

    struct CheckQuery
    {
      Generics::Time master_stamp;
      std::vector<unsigned int> new_ids;
      bool use_only_list = false;
    };

    struct CheckData
    {
      Generics::Time first_stamp;
      Generics::Time master_stamp;
      std::vector<TriggerVersion> versions;
      bool special_track = false;
      bool special_adv = false;
      long source_id = 0;
      Generics::Time max_time;
    };

    struct TriggerInfo
    {
      unsigned long channel_trigger_id = 0;
      std::string trigger;
    };

    struct ChannelById
    {
      unsigned long channel_id = 0;
      std::vector<TriggerInfo> words;
      Generics::Time stamp;
    };

    struct UpdateDataResult
    {
      long source_id = 0;
      std::vector<ChannelById> channels;
    };

    struct CCGQuery
    {
      Generics::Time master_stamp;
      unsigned long start = 0;
      unsigned long limit = 0;
      std::vector<unsigned long> channel_ids;
      bool use_only_list = false;
    };

    struct PosCCGResult
    {
      unsigned long start_id = 0;
      std::vector<CCGKeyword> keywords;
      std::vector<TriggerVersion> deleted;
      long source_id = 0;
    };

    struct DBSourceInfo
    {
      using ObjectRef = std::string;

      std::string pg_connection;
      unsigned long colo = 0;
      std::string version;
      unsigned long count_chunks = 0;
      std::vector<ObjectRef> campaign_refs;
      unsigned long check_sum = 0;
    };

    struct ProxySourceInfo
    {
      using ObjectRef = std::string;

      ChannelSvcs::GroupLoadDescriptionSeq local_descriptor;
      std::vector<ObjectRef> proxy_refs;
      std::vector<ObjectRef> campaign_refs;
      unsigned long count_chunks = 0;
      unsigned long colo = 0;
      std::string version;
      unsigned long check_sum = 0;
    };

    void check(const CheckQuery& query, CheckData& data);

    void set_sources(const DBSourceInfo& db_info, const std::vector<unsigned long>& sources);

    void set_proxy_sources(
      const ProxySourceInfo& proxy_info,
      const std::vector<unsigned long>& sources);

    unsigned long check_configuration() noexcept;

    unsigned long get_count_chunks() noexcept;

    void update_triggers(const std::vector<unsigned long>& ids, UpdateDataResult& result);

    void update_all_ccg(const CCGQuery& query, PosCCGResult& result);

    typedef xsd::AdServer::Configuration::ChannelServerConfigType
      ChannelServerConfig;

    ChannelServerCore(
      Logging::Logger* logger,
      const ChannelServerConfig* server_config) /*throw(Exception)*/;

    virtual void
    deactivate_object() /*throw(Exception, eh::Exception)*/;

    virtual ~ChannelServerCore() noexcept;

    bool ready() noexcept;

    std::string comment() /*throw(eh::Exception)*/;

    void get_stats(ChannelServerStats& stats) noexcept;

    void match(const ChannelServerCore::MatchQuery& query, ChannelServerCore::MatchResult& result);

    void get_ccg_traits(
      const std::vector<unsigned long>& ids,
      ChannelServerCore::TraitsResult& result);

    void change_db_state(bool new_state) /*throw(eh::Exception)*/;

    bool get_db_state() /*throw(Commons::DbStateChanger::NotSupported)*/;

    static const char* TRAFFICKING_PROBLEM;

  protected:

    typedef Generics::TaskGoal TaskBase;

    class UpdateTask: public TaskBase
    {
    public:
      UpdateTask(
        ChannelServerCore* server_impl,
        UpdateDataPtr data,
        Generics::TaskRunner* task_runner) noexcept;

      virtual void execute() noexcept;

    private:
      virtual ~UpdateTask() noexcept;

      ChannelServerCore* server_impl_;
      UpdateDataPtr update_data_;
    };

    void schedule_update_task_(unsigned long period, UpdateDataPtr update_data) noexcept;

    void update_task(UpdateDataPtr update_data) noexcept;

    UpdateContainer* get_update_container() noexcept;

  private:

    Logging::Logger* logger () noexcept;

    static void add_channels_(
      unsigned int channel_id,
      const TriggerMatchItem::value_type& in,
      std::vector<ChannelServerCore::ChannelAtom>& out);

    void fill_result_(
      const TriggerMatchRes& result,
      ChannelServerCore::MatchResult& res,
      bool fill_content);

    void init_logger_(const ChannelServerConfig* config) noexcept;

    void log_update_() noexcept;

    void init_update_(UpdateData* update_data, ChannelServerVariantBase* variant_server)
      /*throw(
        ChannelServerException::TemporyUnavailable,
        ChannelServerException::NotReady,
        ChannelServerException::Exception,
        eh::Exception)*/;

    bool change_source_of_data_() noexcept;

    ChannelServerVariantBasePtr get_source_of_data_() noexcept;

    void trace_ccg_info_changing_(const UpdateData* update_data)
      noexcept;

    static void log_parsed_input_(
      const MatchUrls& url_words,
      const MatchWords match_words[CT_MAX],
      const StringVector& exact_words,
      std::ostream& ostr)
      /*throw(eh::Exception)*/;

    static void log_result_(
      const TriggerMatchRes& res,
      const Generics::Time& time,
      std::ostream& ostr)
      /*throw(eh::Exception)*/;

  protected:
    Logging::ActiveObjectCallbackImpl_var callback_;
    Logging::Logger_var statistic_logger_;
    std::unique_ptr<DictionaryMatcher> dict_matcher_;
    std::unique_ptr<UpdateContainer> update_cont_;

    typedef std::shared_mutex Mutex_;
    typedef std::shared_lock<Mutex_> ReadGuard_;
    typedef std::unique_lock<Mutex_> WriteGuard_;

  private:

    mutable Mutex_ lock_set_sources_;
    std::unique_ptr<ChannelServerCoreState> state_holder_;
    std::string version_;
    unsigned long colo_;
    volatile sig_atomic_t ready_;
    volatile sig_atomic_t source_id_;
    ProgressCounterPtr progress_; /* progress of current operation */
    volatile sig_atomic_t load_keywords_; /* count of loaded keywords */
    volatile sig_atomic_t state_; /* state of update */
    Generics::Planner_var scheduler_;
    Generics::TaskRunner_var task_runner_;
    SegmMap segmentors_;
    Language::Segmentor::SegmentorInterface_var china_segmentor_;
    Language::Segmentor::SegmentorInterface_var normal_segmentor_;
    const Language::Segmentor::SegmentorInterface* segmentor_;

    /* common params for ChannelServer's */
    const unsigned SERVICE_INDEX_; // The number to query campaign server
    unsigned long update_period_; // update period of data
    unsigned long reschedule_period_; // reschedule period
    ChannelServerVariantBasePtr variant_server_; // backend of data server
    ChannelServerVariantBasePtr new_variant_server_; // new backend of data server
    unsigned long count_chunks_; // count chunks in cluster
    unsigned long count_container_chunks_; // count chunks in container
    unsigned long merge_limit_; // max size of merge data in bytes
    unsigned long ccg_limit_; // maximum count keyword in one call
    std::unique_ptr<ChannelContainer> container_;
    //container of triggers and matched channels
    std::set<unsigned short> ports_; // allowed ports in urls

    /* actual channels */
    std::atomic<unsigned long> queries_counter_; // counter of queries to server
    Generics::Time configuration_date_;

    static const char* ASPECT;
    static const char* MATCHASPECT;
  };

  using ChannelServerCorePtr = std::shared_ptr<ChannelServerCore>;

} /* AdServer::ChannelSvcs */

namespace AdServer::ChannelSvcs
{
  inline
  Logging::Logger*
  ChannelServerCore::logger ()
    noexcept
  {
    return callback_->logger();
  }

  /* ChannelServerCore */
  inline
  bool ChannelServerCore::ready() noexcept
  {
    return ready_;
  }

  inline
  ChannelServerVariantBasePtr ChannelServerCore::get_source_of_data_()
  noexcept
  {
    ReadGuard_ lock(lock_set_sources_);
    return variant_server_;
  }

  inline
  UpdateContainer* ChannelServerCore::get_update_container()
    noexcept
  {
    return update_cont_.get();
  }

  inline
  ChannelServerCore::UpdateTask::UpdateTask(
    ChannelServerCore* server_impl,
    UpdateDataPtr data,
    Generics::TaskRunner* task_runner) noexcept
    : TaskBase(task_runner), server_impl_(server_impl),
      update_data_(std::move(data))
  {}

  inline
  ChannelServerCore::UpdateTask::~UpdateTask() noexcept
  {
  }

} /* AdServer::ChannelSvcs */
