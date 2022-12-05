
#ifndef AD_SERVER_CHANNEL_SERVER_VARIANT_HPP_
#define AD_SERVER_CHANNEL_SERVER_VARIANT_HPP_

#include <vector>
#include <string>
#include <deque>
#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <Generics/SimpleDecimal.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <CORBACommons/ObjectPool.hpp>

#include <Commons/Postgres/ConnectionPool.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <CampaignSvcs/CampaignServer/CampaignServer.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>

#include <ChannelSvcs/ChannelCommons/ChannelUpdateBase.hpp>
#include <ChannelSvcs/ChannelCommons/TriggerParser.hpp>
#include <ChannelSvcs/ChannelServer/ChannelServerTypes.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelLoadSessionImpl.hpp>

#include "ChannelContainer.hpp"

namespace AdServer
{
namespace ChannelSvcs
{
  typedef std::map<
    std::string, const Language::Segmentor::SegmentorInterface*> SegmMap;

  class UpdateData: public ReferenceCounting::DefaultImpl<>
  {
  public:
    enum UpdateState
    {
      US_ZERO = 0,//state before first update
      US_START = 1,//initial state
      US_CHECKED = 2,//actual channels gotten and
                 //update checked (loading of trigger lists)
      US_CCG_LOADED = 3, //ccg loaded
      US_LOADED = 4,// trigger lists are loaded
      US_FINISH = 5
    };

    typedef std::set<unsigned int> CheckContainerType;
    UpdateData(UpdateContainer* cont) noexcept;
    virtual ~UpdateData() noexcept{};

    bool empty() const noexcept;

    size_t size() const noexcept;

    void end_iteration() noexcept;

    Stream::Error& trace_ccg_info_changing(Stream::Error& trace_out) const
      noexcept;

    struct UnmergedData
    {//holder of unmerged data
      UnmergedData() noexcept : index_(0), data_(nullptr) {};

      bool empty() const noexcept
      { 
        return data_.operator->() == nullptr;
      }

      void set_unmered_data(
        ::AdServer::ChannelSvcs::ChannelCurrent::UpdateData_var result,
        CORBA::ULong ind)
        noexcept;

      ::AdServer::ChannelSvcs::ChannelCurrent::UpdateData*
      get_unmered_data(CORBA::ULong& index)
        noexcept;

    private: 
      CORBA::ULong index_;
      ::AdServer::ChannelSvcs::ChannelCurrent::UpdateData_var data_;
    };

  public:
    UpdateState state;
    CheckContainerType check_data;//id need to check
    CheckContainerType uid_check_data;//id of uid channels
    ChannelIdToMatchInfo_var info_ptr;
    unsigned long start_ccg_id;
    bool ccg_loaded;
    bool need_merge;
    ProgressCounter* progress;
    unsigned long merge_size;
    UpdateContainer* container_ptr;
    CCGMap_var old_ccg_map;
    CCGMap_var new_ccg_map;
    Generics::Time old_master;
    Generics::Time new_master;
    CampaignSvcs::RevenueDecimal cost_limit;
    UnmergedData unmerged_data;
  };

  typedef ReferenceCounting::SmartPtr<UpdateData>
    UpdateData_var;

  class ChannelServerVariantBase: public ReferenceCounting::DefaultImpl<>
  {
  public:

    DECLARE_EXCEPTION(NotSupported, eh::DescriptiveException);

    struct TriggerCheckInfo
    {
      bool operator<(const unsigned long comp) const noexcept;
      operator unsigned long () const noexcept;
      unsigned long id;
      std::vector<unsigned long> channels;
    };

    typedef std::vector<TriggerCheckInfo> TriggersCheckInfoContainer;

    typedef std::set<unsigned long> CombinedChannel;

    typedef
      CORBACommons::ObjectPoolRefConfiguration ServerPoolConfig;

    typedef
      CORBACommons::ObjectPool<
        AdServer::CampaignSvcs::CampaignServer,
        ServerPoolConfig>
      CampaignServerPool;

    typedef std::unique_ptr<CampaignServerPool> CampaignServerPoolPtr;

    struct SimpleChannel
    {
      unsigned long channel_id;
      char status;
      std::string lang;
      std::string country;
    };

    typedef std::multimap<unsigned long, SimpleChannel> IdSimpleMap;
    typedef std::multimap<std::string, SimpleChannel> StrSimpleMap;

    ChannelServerVariantBase(
      const std::vector<unsigned int>& sources,
      const std::set<unsigned short>& ports,//allowed ports in urls
      unsigned long count_chunks,
      unsigned long colo,
      const char* version,
      const ServerPoolConfig& campaign_pool_config,
      unsigned service_index,
      Logging::Logger* logger,
      unsigned long check_sum)
      /*throw(ChannelServerException::Exception, ChannelContainer::Exception)*/;

    Generics::Time get_first_load_stamp() noexcept;

    const std::vector<unsigned int>& get_sources(unsigned int& count) noexcept;

    unsigned long get_check_sum() noexcept;

    int get_source_id() noexcept;

  protected:
    virtual ~ChannelServerVariantBase() noexcept{};

    void set_sources_id_(int source_id) noexcept;

    void set_first_load_stamp_(const Generics::Time& stamp) noexcept;

  public:

    /* update triggers*/
    virtual void update(
      unsigned long merge_limit,
      UpdateData* data)
      /*throw(ChannelServerException::Exception,
            ChannelServerException::TemporyUnavailable)*/ = 0;

    /* update ccg keywords*/
    virtual void update_ccg(UpdateData* data, unsigned long limit)
      /*throw(ChannelServerException::Exception,
            ChannelServerException::TemporyUnavailable)*/ = 0;

    /* check stamps for trigger ids and return updated triggers */
    virtual void check_updating(UpdateData* data)//update data
      /*throw(
        ChannelServerException::Exception,
        ChannelServerException::TemporyUnavailable)*/ = 0;

    /* get actual  channels*/
    virtual void update_actual_channels(UpdateData* update_data)
      /*throw(
        ChannelServerException::Exception,
        ChannelServerException::NotReady)*/;

    virtual void change_db_state(bool activation)
      /*throw(ChannelServerException::Exception)*/;

    virtual bool get_db_state() /*throw(NotSupported)*/;

  protected:

    typedef Sync::PosixRWLock Mutex_;
    typedef Sync::PosixRGuard ReadGuard_;
    typedef Sync::PosixWGuard WriteGuard_;

    bool is_my_id_(unsigned long id) noexcept;

    void add_special_(
      unsigned long id,
      ChannelIdToMatchInfo& match_info)
      /*throw(eh::Exception)*/;

    void process_portion_(
      const AdServer::CampaignSvcs::CSSimpleChannelSeq& channels,
      ChannelIdToMatchInfo& info)
      /*throw(eh::Exception)*/;

    const std::set<unsigned short>& ports_;//allowed ports in urls
    std::vector<unsigned int> sources_;
    unsigned int count_chunks_;
    int source_id_;
    unsigned long colo_;
    std::string version_;
    Generics::Time first_load_stamp_;
    CampaignServerPoolPtr campaign_pool_;
    Logging::Logger_var logger_;
    Mutex_ mutex_;

    const unsigned long check_sum_;
    unsigned service_index_;
    static const char* ASPECT;
  };

  typedef ReferenceCounting::SmartPtr<ChannelServerVariantBase>
    ChannelServerVariantBase_var;

  struct DBInfo
  {
    std::string pg_connection;
  };

  class ChannelServerDB:
    public ChannelServerVariantBase,
    public Generics::CompositeActiveObject
  {
  public:

    ChannelServerDB(
      const DBInfo& db_info,
      const std::set<unsigned short>& ports,
      const std::vector<unsigned int>& sources,
      unsigned long count_chunks,
      unsigned long colo,
      const char* version,
      const ServerPoolConfig& campaign_pool_config,
      unsigned service_index,
      const SegmMap& segmentors,
      Logging::Logger* logger,
      unsigned long check_sum)
      /*throw(ChannelServerException::Exception)*/;

    /* update ccg keywords*/
    virtual void update_ccg(UpdateData* data, unsigned long limit)
      /*throw(ChannelServerException::Exception,
        ChannelServerException::TemporyUnavailable)*/;

    /* update triggers*/
    virtual void update(
      unsigned long merge_limit,
      UpdateData* data)
      /*throw(ChannelServerException::Exception,
        ChannelServerException::TemporyUnavailable)*/;

    /* check stamps for trigger ids and return updated triggers */
    virtual void check_updating(UpdateData* data)//update data
      /*throw(
        ChannelServerException::Exception,
        ChannelServerException::TemporyUnavailable)*/;

    virtual void change_db_state(bool activation)
      /*throw(ChannelServerException::Exception)*/;

    virtual bool get_db_state() noexcept;

  protected:
    virtual ~ChannelServerDB() noexcept;

  private:
    void load_uids_(
      Commons::Postgres::Connection* conn,
      unsigned long& merge_limit,
      UpdateData* data)
      /*throw(
        Commons::Postgres::Exception,
        Commons::Postgres::SqlException,
        TriggerParser::Exception)*/;

    bool load_triggers_(
      Commons::Postgres::Connection* conn,
      unsigned long& merge_limit,
      UpdateData* data)//update data
      /*throw(
        Commons::Postgres::Exception,
        Commons::Postgres::SqlException,
        TriggerParser::Exception,
        eh::Exception)*/;

    static void check_id_(
      unsigned int id,
      const Generics::Time& new_db_stamp,
      const Generics::Time& new_master,
      const ExcludeContainerType& updated,
      UpdateData::CheckContainerType& check_ids,
      UpdateData::CheckContainerType& uid_check_ids,
      ChannelIdToMatchInfo& info)
      /*throw(ChannelServerException::Exception)*/;

    void save_channel_size_(unsigned long channel_size) noexcept;

    size_t get_update_size_(unsigned long merge_limit) const
      noexcept;

  private:
    AdServer::Commons::Postgres::Environment_var pg_env_;
    AdServer::Commons::Postgres::ConnectionPool_var pg_pool_;
    Generics::ActiveObjectCallback_var callback_;
    const SegmMap& segmentors_;
    DBInfo db_info_;
    unsigned long update_size_;
    std::list<unsigned long> update_history_;
  };

  class ChannelServerProxy: public ChannelServerVariantBase
  {
  public:
    typedef
      CORBACommons::ObjectPool<
        AdServer::ChannelSvcs::ChannelUpdate_v33,
        ServerPoolConfig>
      ChannelProxyPool;

    typedef std::unique_ptr<ChannelProxyPool> ChannelProxyPoolPtr;

    typedef std::unique_ptr<ChannelLoadSessionImpl> LoadSessionPtr;

    enum
    {
      PRIORITY_SERVERS,
      PRIORITY_PROXY
    };

    ChannelServerProxy(
      const ChannelSvcs::GroupLoadDescriptionSeq& servers,
      const ServerPoolConfig& proxy_pool_config,
      const std::set<unsigned short>& ports,
      const std::vector<unsigned int>& sources,
      unsigned long count_chunks,
      unsigned long colo,
      const char* version,
      const ServerPoolConfig& campaign_pool_config,
      unsigned service_index,
      Logging::Logger* logger,
      unsigned long check_sum,
      unsigned long priority = PRIORITY_SERVERS)
      /*throw(ChannelServerException::Exception)*/;

  protected:
    bool check_source_(
      UpdateData* data,
      int new_source_id,
      const Generics::Time& longest_update,
      const Generics::Time& first_load_stamp,
      const Generics::Time& new_master)
      noexcept;

    template<typename Function, typename...Args>
    void do_query_(const char* fn_name, Function func, Args&... args)
      /*throw(ChannelServerException::Exception)*/;

    virtual ~ChannelServerProxy() noexcept;
  public:

    /* update ccg keywords*/
    virtual void update_ccg(UpdateData* data, unsigned long limit)
      /*throw(ChannelServerException::Exception)*/;

    virtual void update(
      unsigned long merge_limit,
      UpdateData* data)
      /*throw(ChannelServerException::Exception,
        ChannelServerException::TemporyUnavailable)*/;

    virtual void check_updating(UpdateData* data)//update data
      /*throw(ChannelServerException::Exception,
        ChannelServerException::TemporyUnavailable)*/;

  private:
    unsigned long priority_;
    unsigned long tries_;
    ChannelProxyPoolPtr proxy_pool_;
    LoadSessionPtr load_session_;
  };
}
}

namespace AdServer
{
  namespace ChannelSvcs
  {
    inline
    bool UpdateData::empty() const noexcept
    {
      return check_data.empty() && uid_check_data.empty();
    }

    inline
    size_t UpdateData::size() const noexcept
    {
      return check_data.size() + uid_check_data.size();
    }

    inline
    bool ChannelServerVariantBase::TriggerCheckInfo::operator< (
      const unsigned long comp) const noexcept
    {
      return (id < comp);
    }

    inline
    ChannelServerVariantBase::TriggerCheckInfo::operator unsigned long() const
      noexcept
    {
      return id;
    }

    inline
    bool ChannelServerVariantBase::is_my_id_(unsigned long id) noexcept
    {
      return (id &&
              std::find(sources_.begin(), sources_.end(), id%count_chunks_)
              != sources_.end());
    }

    inline
    unsigned long ChannelServerVariantBase::get_check_sum() noexcept
    {
      return check_sum_;
    }

    inline
    int ChannelServerVariantBase::get_source_id() noexcept
    {
      ReadGuard_ lock(mutex_);
      return source_id_;
    }

    inline
    void ChannelServerVariantBase::set_sources_id_(int source_id) noexcept
    {
      WriteGuard_ lock(mutex_);
      source_id_ = source_id;
    }

    inline
    Generics::Time ChannelServerVariantBase::get_first_load_stamp() noexcept
    {
      WriteGuard_ lock(mutex_);
      return first_load_stamp_;
    }

    inline
    void ChannelServerVariantBase::set_first_load_stamp_(const Generics::Time& stamp)
      noexcept
    {
      WriteGuard_ lock(mutex_);
      first_load_stamp_  = stamp;
    }


    template<typename Function, typename...Args>
    void ChannelServerProxy::do_query_(
      const char* fn_name, Function func, Args&... args)
      /*throw(ChannelServerException::Exception)*/
    {
      bool next_try, exception;
      unsigned long iter = 0;
      bool log = false;
      std::ostringstream ostr;
      bool use_servers = (priority_ == PRIORITY_SERVERS && tries_)
        ? true : false;
      Generics::Timer timer;
      do
      {
        exception = false;
        next_try = false;
        try
        {
          timer.start();
          if(use_servers)
          {
            iter++;
            (&*load_session_->*func)(args...);
          }
          else
          {
            ChannelProxyPool::ObjectHandlerType proxy =
              proxy_pool_->get_object();
            try
            {
              (&*proxy->*func)(args...);
            }
            catch(const AdServer::ChannelSvcs::NotConfigured&)
            {
              throw;
            }
            catch(...)
            {
              proxy.release_bad(String::SubString(fn_name));
              throw;
            }
          }
          timer.stop();
          logger_->sstream(Logging::Logger::DEBUG, ASPECT)
            << fn_name << ": execute query time: " << timer.elapsed_time();
        }
        catch(const AdServer::ChannelSvcs::ImplementationException& e)
        {
          ostr << (log ? "; " : fn_name) << ": ImplementationException: "
            << e.description;
          log = true;
          exception = true;
        }
        catch(const AdServer::ChannelSvcs::NotConfigured& e)
        {
          ostr << (log ? "; " : fn_name) << ": NotConfigured"
            << e.description;
          log = true;
          exception = true;
        }
        catch(const CORBA::SystemException& e_123)
        {
          Stream::Error err;
          err << fn_name << ": CORBA::SystemException: " << e_123;
          logger_->log(
              err.str(),
              Logging::Logger::CRITICAL,
              ASPECT,
              "ADS-ICON-0");
          exception = true;
        }
        catch(const eh::Exception& ex)
        {
          ostr << fn_name << ": eh::Exception: " << ex.what();
          log = true;
          exception = true;
        }
        if(exception)
        {
          if(use_servers)
          {
            if(iter < tries_)
            {
              next_try = true;
            }
            else if(proxy_pool_.get() && priority_ == PRIORITY_SERVERS)
            {
              use_servers = false;
              next_try = true;
            }
          }
          else
          {
            if(priority_ == PRIORITY_PROXY && tries_)
            {
              use_servers = true;
              next_try = true;
            }
          }
        }
      } while(next_try);
      if(exception)
      {
        throw ChannelServerException::Exception(
          log ? ostr.str().c_str() : "");
      }
      else if(log)
      {
        logger_->log(
          String::SubString(ostr.str()),
          Logging::Logger::ERROR, ASPECT, "ADS-IMPL-15");
      }
    }
  }
}

#endif

