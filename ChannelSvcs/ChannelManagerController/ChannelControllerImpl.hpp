#ifndef AD_SERVER_CHANNEL_CONTROLLER_IMPL_HPP_
#define AD_SERVER_CHANNEL_CONTROLLER_IMPL_HPP_

#include <set>
#include <vector>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Logger/Logger.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>
#include <Sync/SyncPolicy.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>
#include <CORBACommons/ProcessControl.hpp>
#include <CORBACommons/Stats.hpp>
#include <CORBACommons/Stats_s.hpp>

#include <CampaignSvcs/CampaignServer/CampaignServer.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>

#include <ChannelSvcs/ChannelCommons/ChannelServer.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelManagerController_s.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelClusterControl_s.hpp>

#include <xsd/AdServerCommons/AdServerCommons.hpp>
#include <xsd/ChannelSvcs/ChannelManagerControllerConfig.hpp>


namespace AdServer
{
namespace ChannelSvcs
{

  /**
   * Implementation of common part ChannelController
   */
  class ChannelControllerImpl:
    public virtual Generics::CompositeActiveObject,
    public virtual CORBACommons::ReferenceCounting::ServantImpl
     <POA_AdServer::ChannelSvcs::ChannelManagerController>
  {

  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    typedef xsd::AdServer::Configuration::ChannelControllerConfigType
      ChannelControllerConfig;

    ChannelControllerImpl(
      Logging::Logger* logger,
      const ChannelControllerConfig* config) /*throw(Exception)*/;

  public:

    //
    // IDL:AdServer/ChannelSvcs/ChannelManagerController/get_channel_session:1.0
    //
    virtual ::AdServer::ChannelSvcs::ChannelServerSession*
      get_channel_session()
        /*throw(AdServer::ChannelSvcs::ImplementationException,
            ::CORBA::SystemException)*/;

    //
    // IDL:AdServer/ChannelSvcs/ChannelProxy/get_count_chunks:1.0
    //
    virtual ::CORBA::ULong get_count_chunks() /*throw(::CORBA::SystemException)*/;

    //
    // IDL:AdServer/ChannelSvcs/ChannelManagerController/get_load_session:1.0
    //
    ::AdServer::ChannelSvcs::ChannelLoadSession* get_load_session()
      /*throw(AdServer::ChannelSvcs::ImplementationException,
            ::CORBA::SystemException)*/;

    virtual ~ChannelControllerImpl() noexcept;

    ::AdServer::ChannelSvcs::ChannelClusterSession*
      get_control_session()
        /*throw(AdServer::ChannelSvcs::ChannelClusterControl::ImplementationException)*/;

    CORBACommons::StatsValueSeq* get_stats()
      /*throw(CORBACommons::ProcessStatsControl::ImplementationException)*/;

    static void add_refs_to_sum(
      std::size_t& check_sum,
      const xsd::AdServer::Configuration::MultiCorbaObjectRefType& refs)
      noexcept;

  protected:
    typedef Generics::TaskGoal TaskBase;
    typedef ReferenceCounting::SmartPtr<TaskBase> Task_var;

    class ControlTask: public TaskBase
    {
    public:

      ControlTask(ChannelControllerImpl* impl,
        Generics::TaskRunner* task_runner) noexcept;
      virtual ~ControlTask() noexcept;
      virtual void execute() noexcept;

      private:
      ChannelControllerImpl* controller_impl_;
    };

    class CSInfo:
      public ReferenceCounting::DefaultImpl<Sync::Policy::PosixThread>
    {
    public:
      CSInfo() noexcept;
      virtual ~CSInfo() noexcept {};

    public:
      AdServer::ChannelSvcs::ChannelServerControl_var control_ref;
      CORBACommons::IProcessControl_var proccontrol_ref;
      AdServer::ChannelSvcs::ChannelServer_var server_ref;
      AdServer::ChannelSvcs::ChannelUpdateBase_v33_var update_ref;
      CORBACommons::ProcessStatsControl_var process_stat_ref;
      std::vector<unsigned long> chunks;
      std::size_t check_sum[2];
    };

    typedef ReferenceCounting::SmartPtr<CSInfo> CSInfo_var;

    struct CSConnection
    {
      typedef std::vector<CSInfo_var> CSInfoVector;
      typedef std::vector<CSInfoVector> CSGroups;
      unsigned long colo;
      long source_id;
      std::string version;
      std::string pg_connection;
      std::vector<unsigned long> local_groups;
      CORBACommons::CorbaObjectRefList proxy_refs;
      CORBACommons::CorbaObjectRefList campaign_server_refs;
      CSGroups servers;
    };

    void scheduler_control_task_(unsigned long period) noexcept;

    void check_() noexcept;

    void init_corba_refs_(const ChannelControllerConfig* config)
      /*throw(Exception)*/;

    void read_regular_connection_(
      const xsd::AdServer::Configuration::RegularSourceType& regular_config)
      /*throw(Exception)*/;

    void read_proxy_connection_(
      const xsd::AdServer::Configuration::ProxySourceType& config)
      /*throw(Exception)*/;

    void read_list_refs_(
      const xsd::AdServer::Configuration::MultiCorbaObjectRefType& refs,
      CORBACommons::CorbaObjectRefList& list_refs)
      /*throw(Exception)*/;

    void assign_chunks_() /*throw(eh::Exception)*/;

    void calc_proxy_sums_(
      const xsd::AdServer::Configuration::MultiCorbaObjectRefType& refs,
      bool use_local_on_first_load)
      noexcept;

    void parse_groups_(
      const String::SubString& groups_str,
      std::vector<unsigned long>& groups)
      /*throw(Exception)*/;

    static void pack_ref_list_(
      const CORBACommons::CorbaObjectRefList& refs,
      ::AdServer::ChannelSvcs::ChannelServerControl::
        CorbaObjectRefDefSeq& out) 
      /*throw(eh::Exception, CORBACommons::CorbaObjectRef::Exception)*/;

    static void pack_load_description_(
      const CSConnection::CSInfoVector& info,
      AdServer::ChannelSvcs::ChannelLoadDescriptionSeq& descr,
      unsigned long count_chunks)
      /*throw(eh::Exception)*/;

    Logging::Logger* logger () noexcept;

  private:
    Logging::ActiveObjectCallbackImpl_var callback_;
    Generics::Planner_var scheduler_;
    Generics::TaskRunner_var task_runner_;

    /* common params for ChannelController's */
    unsigned long update_period_;
    unsigned long count_chunks_;
    bool control_servers_;
    std::size_t check_sum_;

    CORBACommons::CorbaClientAdapter_var c_adapter_;
    CSConnection connection_;
  };

  typedef ReferenceCounting::SmartPtr<ChannelControllerImpl>
      ChannelControllerImpl_var;

  class ChannelClusterControlImpl :
    public CORBACommons::ProcessControlDefault<
      POA_AdServer::ChannelSvcs::ChannelClusterControl>
  {
  public:
    ChannelClusterControlImpl(ChannelControllerImpl* controller) noexcept;

    virtual
    ~ChannelClusterControlImpl() noexcept;

    virtual ::AdServer::ChannelSvcs::ChannelClusterSession* 
      get_control_session()
        /*throw(AdServer::ChannelSvcs::ChannelClusterControl::ImplementationException)*/;

    virtual CORBACommons::AProcessControl::ALIVE_STATUS is_alive() noexcept;

    virtual char*
    comment() /*throw(CORBACommons::OutOfMemory)*/;

  private:
    ChannelControllerImpl_var delegate_;
  };

  typedef ReferenceCounting::SmartPtr<ChannelClusterControlImpl>
      ChannelClusterControlImpl_var;

  class ChannelStatImpl:
    public virtual CORBACommons::ReferenceCounting::ServantImpl
    <POA_CORBACommons::ProcessStatsControl>
  {
  public:
    ChannelStatImpl(ChannelControllerImpl* controller) noexcept;
    ~ChannelStatImpl() noexcept {}

    virtual CORBACommons::StatsValueSeq* get_stats()
      /*throw(CORBACommons::ProcessStatsControl::ImplementationException)*/;

  private:
    ChannelControllerImpl_var delegate_;
  };
  typedef ReferenceCounting::SmartPtr<ChannelStatImpl>
    ChannelStatImpl_var;

} /* ChannelSvcs */
} /* AdServer */

namespace AdServer
{
  namespace ChannelSvcs
  {
    inline Logging::Logger*
    ChannelControllerImpl::logger ()
      noexcept
    {
      return callback_->logger();
    }

    /* ChannelControllerImpl */
    inline
    ChannelControllerImpl::ControlTask::ControlTask(
      ChannelControllerImpl* impl, Generics::TaskRunner* task_runner)
      noexcept
      : TaskBase(task_runner), controller_impl_(impl)
    {
    }

    inline
    ChannelControllerImpl::ControlTask::~ControlTask() noexcept
    {
    }

    inline void
    ChannelControllerImpl::ControlTask::execute() noexcept
    {
      controller_impl_->check_();
    }

  } /* ChannelSvcs */
} /* AdServer */


#endif /*AD_SERVER_CHANNEL_CONTROLLER_IMPL_HPP_*/
