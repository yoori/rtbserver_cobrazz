#include <Generics/Time.hpp>
#include <CORBACommons/ObjectPool.hpp>
#include <Commons/FileManip.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <Commons/DelegateTaskGoal.hpp>
#include <LogCommons/LogCommons.hpp>

#include <CampaignSvcs/CampaignManager/BillingStateContainer.hpp>

#include "BillingProcessor.hpp"

// #define DEBUG_OUTPUT 1

/*
 * BillingProcessor
 *
 * all elements passed to process_impression, process_click
 * divided between servers : servers_ array
 * inside Server aggregated into agg_amounts and pushed to send_amounts for keep required sending order
 */
namespace AdServer
{
namespace RequestInfoSvcs
{
  namespace
  {
    namespace Aspect
    {
      const char* BILLING_PROCESSOR = "BillingProcessor";
    }
  }

  //
  // BillingProcessor -> Sender(Planner for activate servers) -> RequestPool <- BillingProcessor
  // BillingProcessor -> RequestPool
  //

  // BillingProcessor::RequestPool
  struct BillingProcessor::RequestPool:
    public Generics::SimpleActiveObject,
    public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, BillingProcessor::Exception);

  public:
    RequestPool(unsigned long server_count)
      /*throw(BillingProcessor::Exception)*/;

    virtual void
    deactivate_object()
      /*throw(ActiveObject::Exception, eh::Exception)*/;

    void
    add(const Generics::Time& time,
      unsigned long account_id,
      unsigned long advertiser_id,
      unsigned long campaign_id,
      unsigned long ccg_id,
      const RevenueDecimal& ctr,
      const RevenueDecimal& account_amount,
      const RevenueDecimal& amount,
      const RevenueDecimal& imps,
      const RevenueDecimal& clicks,
      bool check_active = true)
      /*throw(Exception)*/;

    void
    add_to_server(
      unsigned long server_index,
      RequestArray& requests)
      /*throw(Exception)*/;

    void
    add_to_all_servers(
      const RequestArray& requests)
      /*throw(Exception)*/;

    void
    add_resend_to_server(
      unsigned long server_index,
      RequestArray& requests)
      /*throw(Exception)*/;

    void
    add_delayed(RequestArray& requests)
      /*throw(Exception)*/;

    unsigned long
    server_count() const noexcept
    {
      return servers_.size();
    }

    void
    load(const String::SubString& storage_root)
      /*throw(Exception)*/;

    void
    dump(const String::SubString& storage_root)
      /*throw(Exception)*/;

    // blocking if here no requests to process
    bool
    get_requests(
      unsigned long& server_index,
      RequestArray& requests,
      unsigned long max_requests)
      noexcept;

    bool
    get_delayed_requests(RequestArray& requests)
      noexcept;

    void
    deactivate_server(unsigned long server_index)
      noexcept;

    void
    activate_server(unsigned long server_index)
      noexcept;

    void
    print(std::ostream& ostr) noexcept;

  protected:
    typedef Sync::Policy::PosixThread SyncPolicy;  
    typedef Sync::Policy::PosixThread DumpSyncPolicy;

    // aggregate amount set
    struct AggregateRequestHashAdapter
    {
    public:
      AggregateRequestHashAdapter(Request* request)
        noexcept;

      size_t
      hash() const noexcept;

      bool
      operator==(const AggregateRequestHashAdapter& right) const noexcept;

      const Request_var&
      request() const noexcept;

    protected:
      Request_var request_;
      size_t hash_;
    };

    typedef Generics::GnuHashSet<AggregateRequestHashAdapter> AggregateAmountSet;
 
    struct SaveAggregateRequestHashAdapter
    {
    public:
      SaveAggregateRequestHashAdapter(Request* request)
        noexcept;

      size_t
      hash() const noexcept;

      bool
      operator==(const SaveAggregateRequestHashAdapter& right) const noexcept;

      const Request_var&
      request() const noexcept;

    protected:
      Request_var request_;
      size_t hash_;
    };

    typedef Generics::GnuHashSet<SaveAggregateRequestHashAdapter> SaveAggregateAmountSet;

    // sending order amount set
    struct FirstRequestLess
    {
      bool
      operator()(const Request* left, const Request* right) const noexcept;
    };

    typedef std::set<Request_var, FirstRequestLess> SendOrderAmountSet;

    //
    struct AmountAggregateHolder: public ReferenceCounting::AtomicImpl
    {
      // agg_amounts, send_amounts should consistent by first_request_time
      RequestPool::SyncPolicy::Mutex lock;
      AggregateAmountSet agg_amounts;
      SendOrderAmountSet send_amounts;

    protected:
      virtual
      ~AmountAggregateHolder() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<AmountAggregateHolder>
      AmountAggregateHolder_var;

    struct ServerStateHolder: public AmountAggregateHolder
    {
      ServerStateHolder(unsigned long server_id_val)
        noexcept;

      const unsigned long server_id;
      bool active;

    protected:
      virtual
      ~ServerStateHolder() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<ServerStateHolder>
      ServerStateHolder_var;

    typedef std::vector<ServerStateHolder_var> ServerArray;

    typedef std::deque<ServerStateHolder_var> ServerDeque;

    struct ServerStateFirstRequestLess
    {
      bool
      operator()(const ServerStateHolder* left,
        const ServerStateHolder* right) const noexcept;
    };

    typedef std::multiset<ServerStateHolder_var, ServerStateFirstRequestLess>
      FirstRequestTimeServerStateSet;

    typedef Sync::Policy::PosixThread NonEmptyServersSyncPolicy;

  protected:
    virtual
    ~RequestPool() noexcept = default;

    bool
    add_to_server_(
      unsigned long server_index,
      RequestArray& requests)
      /*throw(Exception)*/;

    // get ownership on requests, but don't clear container (do it outside lock)
    static void
    add_to_aggregator_i_(
      AmountAggregateHolder* amount_aggregator,
      RequestArray& requests)
      /*throw(Exception)*/;

    static void
    get_aggregator_requests_i_(
      RequestArray& res_requests,
      AmountAggregateHolder* amount_aggregator,
      unsigned long max_requests)
      noexcept;

    static void
    collect_aggregator_(
      SaveAggregateAmountSet& res_agg_amounts,
      AmountAggregateHolder* amount_aggregator)
      /*throw(Exception)*/;

    static ServerArray
    init_servers_(unsigned long server_count) noexcept;

    void
    load_amounts_(const String::SubString& file)
      /*throw(Exception)*/;

    void
    save_amounts_(const String::SubString& file)
      /*throw(Exception)*/;

    void
    remove_storage_(const String::SubString& path)
      /*throw(Exception)*/;

  protected:
    static const char* AMOUNT_FILE_;

    // all servers
    const ServerArray servers_;

    // locking order: server->lock, non_empty_servers_lock_
    mutable NonEmptyServersSyncPolicy::Mutex non_empty_servers_lock_;
    mutable Sync::Conditional non_empty_servers_cond_;

    // non empty servers (ordered by first_request_time)
    FirstRequestTimeServerStateSet non_empty_servers_;

    // delayed amounts - processor try to add it periodicaly
    AmountAggregateHolder_var delayed_amounts_;

    mutable DumpSyncPolicy::Mutex dump_lock_;
  };

  // BillingProcessor::Sender : job that process requests queue
  class BillingProcessor::Sender:
    public Generics::ActiveObjectCommonImpl
  {
  public:
    // Context: shared struct between Job threads
    struct Context: public ReferenceCounting::AtomicImpl
    {
      Logging::Logger_var logger;
      Generics::Planner_var scheduler;
      RequestSender_var request_sender;

      // unsafe pointer, for avoid cyclic references
      BillingProcessor* billing_processor;

    protected:
      virtual
      ~Context() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Context> Context_var;

  public:
    Sender(
      Generics::ActiveObjectCallback* callback,
      unsigned long thread_count,
      Context* context,
      BillingProcessor::RequestPool* request_pool)
      /*throw(eh::Exception)*/;

  protected:
    typedef CORBACommons::ObjectPool<
      AdServer::CampaignSvcs::BillingServer,
      CORBACommons::ObjectPoolRefConfiguration>
      SingleBillingServerPool;

    struct BillingServerDescr
    {
      BillingServerDescr(SingleBillingServerPool* pool_val)
        : pool(pool_val)
      {}

      BillingServerDescr(BillingServerDescr&& init)
        : pool(std::move(init.pool))
      {}

      BillingServerDescr&
      operator=(BillingServerDescr&& init)
      {
        std::swap(pool, init.pool);
        return *this;
      }

      std::unique_ptr<SingleBillingServerPool> pool;
    };

    typedef std::vector<BillingServerDescr> BillingServerDescrArray;

    class Job: public SingleJob
    {
    public:
      Job(Generics::ActiveObjectCallback* callback,
        Context* context,
        RequestPool* request_pool,
        unsigned long thread_count)
        /*throw(eh::Exception)*/;

      virtual void
      work() noexcept;

      virtual void
      terminate() noexcept;

    protected:
      class ActivateServerGoal:
        public Generics::Goal,
        public ReferenceCounting::AtomicImpl
      {
      public:
        ActivateServerGoal(
          RequestPool* request_pool,
          unsigned long server_index)
          noexcept;

        virtual
        void deliver() noexcept;

      protected:
        virtual ~ActivateServerGoal() noexcept = default;

      protected:
        const RequestPool_var request_pool_;
        const unsigned long server_index_;
      };

    protected:
      virtual ~Job() noexcept
      {}

      void
      deactivate_server_(unsigned long server_index)
        noexcept;

    private:
      const Context_var context_;
      const RequestPool_var request_pool_;
      const unsigned long thread_count_;
      const Generics::Time deactivate_period_;
    };

  protected:
    virtual
    ~Sender() noexcept = default;
  };

  struct BillingProcessor::BillingServerRequestSender::BillingServerArrayHolder
  {
    typedef AdServer::Commons::CorbaObject<AdServer::CampaignSvcs::BillingServer>
      BillingServerObject;

    typedef std::vector<BillingServerObject> BillingServerArray;

    BillingServerArray billing_servers;
  };

  const char*
  BillingProcessor::RequestPool::AMOUNT_FILE_ = "Amounts";

  // BillingProcessor::Request impl
  BillingProcessor::Request::Request(
    const Generics::Time& time_val,
    unsigned long account_id_val,
    unsigned long advertiser_id_val,
    unsigned long campaign_id_val,
    unsigned long ccg_id_val,
    const RevenueDecimal& ctr_val,
    Mode mode_val,
    const RevenueDecimal& account_amount_val,
    const RevenueDecimal& amount_val,
    const RevenueDecimal& imps_val,
    const RevenueDecimal& clicks_val)
    noexcept
    : rounded_time(time_val.tv_sec / Generics::Time::ONE_HOUR.tv_sec * Generics::Time::ONE_HOUR.tv_sec),
      account_id(account_id_val),
      advertiser_id(advertiser_id_val),
      campaign_id(campaign_id_val),
      ccg_id(ccg_id_val),
      ctr(ctr_val),
      mode(mode_val),
      first_request_time(time_val),
      account_amount(account_amount_val),
      amount(amount_val),
      imps(imps_val),
      clicks(clicks_val)
  {}

  BillingProcessor::Request::Request(
    const Generics::Time& rounded_time_val,
    unsigned long account_id_val,
    unsigned long advertiser_id_val,
    unsigned long campaign_id_val,
    unsigned long ccg_id_val,
    const RevenueDecimal& ctr_val,
    Mode mode_val,
    const Generics::Time& first_request_time_val,
    const RevenueDecimal& account_amount_val,
    const RevenueDecimal& amount_val,
    const RevenueDecimal& imps_val,
    const RevenueDecimal& clicks_val)
    noexcept
    : rounded_time(rounded_time_val),
      account_id(account_id_val),
      advertiser_id(advertiser_id_val),
      campaign_id(campaign_id_val),
      ccg_id(ccg_id_val),
      ctr(ctr_val),
      mode(mode_val),
      first_request_time(first_request_time_val),
      account_amount(account_amount_val),
      amount(amount_val),
      imps(imps_val),
      clicks(clicks_val)
  {
    assert(rounded_time.tv_sec % Generics::Time::ONE_HOUR.tv_sec == 0);
  }

  bool
  BillingProcessor::Request::add(const Request& right)
    /*throw(RevenueDecimal::Overflow)*/
  {
    account_amount += right.account_amount;
    amount += right.amount;
    imps += right.imps;
    clicks += right.clicks;
    if(first_request_time > right.first_request_time)
    {
      first_request_time = right.first_request_time;
      return true;
    }
    return false;
  }

  void
  BillingProcessor::Request::print(
    std::ostream& out,
    const char* prefix)
    const noexcept
  {
    out << prefix << "rounded_time: " << rounded_time.gm_ft() << std::endl <<
      prefix << "account_id: " << account_id << std::endl <<
      prefix << "advertiser_id: " << advertiser_id << std::endl <<
      prefix << "campaign_id: " << campaign_id << std::endl <<
      prefix << "ccg_id: " << ccg_id << std::endl <<
      prefix << "ctr: " << ctr << std::endl <<
      prefix << "mode: ";
    if(mode == RM_NORMAL)
    {
      out << "normal";
    }
    else if(mode == RM_RESEND)
    {
      out << "resend";
    }
    else if(mode == RM_FORCED)
    {
      out << "forced";
    }

    out << std::endl <<
      prefix << "first_request_time: " << first_request_time.gm_ft() << std::endl <<
      prefix << "account_amount: " << account_amount << std::endl <<
      prefix << "amount: " << amount << std::endl <<
      prefix << "imps: " << imps << std::endl <<
      prefix << "clicks: " << clicks;
  }

  // BillingProcessor::RequestPool::AggregateRequestHashAdapter impl
  BillingProcessor::RequestPool::
  AggregateRequestHashAdapter::AggregateRequestHashAdapter(
    Request* request)
    noexcept
    : request_(ReferenceCounting::add_ref(request))
  {
    // init hash
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, request_->rounded_time.tv_sec);
    hash_add(hasher, request_->account_id);
    hash_add(hasher, request_->advertiser_id);
    hash_add(hasher, request_->campaign_id);
    hash_add(hasher, request_->ccg_id);
    hash_add(hasher, request_->ctr);
    hash_add(hasher, static_cast<int>(request_->mode));
  }

  size_t
  BillingProcessor::RequestPool::
  AggregateRequestHashAdapter::hash() const noexcept
  {
    return hash_;
  }

  bool
  BillingProcessor::RequestPool::
  AggregateRequestHashAdapter::operator==(
    const AggregateRequestHashAdapter& right)
    const noexcept
  {
    return request_->rounded_time == right.request_->rounded_time &&
      request_->account_id == right.request_->account_id &&
      request_->advertiser_id == right.request_->advertiser_id &&
      request_->campaign_id == right.request_->campaign_id &&
      request_->ccg_id == right.request_->ccg_id &&
      request_->ctr == right.request_->ctr &&
      request_->mode == right.request_->mode;
  }

  const BillingProcessor::Request_var&
  BillingProcessor::RequestPool::
  AggregateRequestHashAdapter::request() const noexcept
  {
    return request_;
  }

  // BillingProcessor::RequestPool::SaveAggregateRequestHashAdapter impl
  BillingProcessor::RequestPool::
  SaveAggregateRequestHashAdapter::SaveAggregateRequestHashAdapter(
    Request* request)
    noexcept
    : request_(ReferenceCounting::add_ref(request))
  {
    // init hash
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, request_->rounded_time.tv_sec);
    hash_add(hasher, request_->account_id);
    hash_add(hasher, request_->advertiser_id);
    hash_add(hasher, request_->campaign_id);
    hash_add(hasher, request_->ccg_id);
    hash_add(hasher, request_->ctr);
  }

  size_t
  BillingProcessor::RequestPool::
  SaveAggregateRequestHashAdapter::hash() const noexcept
  {
    return hash_;
  }

  bool
  BillingProcessor::RequestPool::
  SaveAggregateRequestHashAdapter::operator==(
    const SaveAggregateRequestHashAdapter& right)
    const noexcept
  {
    return request_->rounded_time == right.request_->rounded_time &&
      request_->account_id == right.request_->account_id &&
      request_->advertiser_id == right.request_->advertiser_id &&
      request_->campaign_id == right.request_->campaign_id &&
      request_->ccg_id == right.request_->ccg_id &&
      request_->ctr == right.request_->ctr;
  }

  const BillingProcessor::Request_var&
  BillingProcessor::RequestPool::
  SaveAggregateRequestHashAdapter::request() const noexcept
  {
    return request_;
  }

  // BillingProcessor::RequestPool::FirstRequestLess impl
  bool
  BillingProcessor::RequestPool::FirstRequestLess::operator()(
    const BillingProcessor::Request* left,
    const BillingProcessor::Request* right) const
    noexcept
  {
    if(left->first_request_time < right->first_request_time)
    {
      return true;
    }
    else if(left->first_request_time > right->first_request_time)
    {
      return false;
    }

    if(left->account_id < right->account_id)
    {
      return true;
    }
    else if(left->account_id > right->account_id)
    {
      return false;
    }

    if(left->advertiser_id < right->advertiser_id)
    {
      return true;
    }
    else if(left->advertiser_id > right->advertiser_id)
    {
      return false;
    }

    if(left->advertiser_id < right->advertiser_id)
    {
      return true;
    }
    else if(left->advertiser_id > right->advertiser_id)
    {
      return false;
    }

    if(left->campaign_id < right->campaign_id)
    {
      return true;
    }
    else if(left->campaign_id > right->campaign_id)
    {
      return false;
    }

    if(left->ccg_id < right->ccg_id)
    {
      return true;
    }
    else if(left->ccg_id > right->ccg_id)
    {
      return false;
    }

    if(left->ctr < right->ctr)
    {
      return true;
    }
    else if(left->ctr > right->ctr)
    {
      return false;
    }

    return left->mode < right->mode;
  }

  // BillingProcessor::RequestPool::ServerStateHolder impl
  BillingProcessor::RequestPool::ServerStateHolder::ServerStateHolder(
    unsigned long server_id_val)
    noexcept
    : server_id(server_id_val),
      active(true)
  {}

  // BillingProcessor::RequestPool::ServerStateHolderLess impl
  bool
  BillingProcessor::RequestPool::ServerStateFirstRequestLess::operator()(
    const ServerStateHolder* left,
    const ServerStateHolder* right)
    const noexcept
  {
    bool left_empty = left->send_amounts.empty();
    bool right_empty = right->send_amounts.empty();
    if(left_empty && right_empty)
    {
      return left->server_id < right->server_id;
    }
    else if(left_empty)
    {
      return false;
    }
    else if(right_empty)
    {
      return true;
    }

    return (*left->send_amounts.begin())->first_request_time <
      (*right->send_amounts.begin())->first_request_time;
  }

  // BillingProcessor::RequestPool impl
  BillingProcessor::RequestPool::RequestPool(unsigned long server_count)
    /*throw(BillingProcessor::Exception)*/
    : servers_(init_servers_(server_count)),
      delayed_amounts_(new AmountAggregateHolder())
  {}

  void
  BillingProcessor::RequestPool::deactivate_object()
    /*throw(ActiveObject::Exception, eh::Exception)*/
  {
    Generics::SimpleActiveObject::deactivate_object();

    SyncPolicy::WriteGuard guard(non_empty_servers_lock_);
    non_empty_servers_cond_.broadcast();
  }

  void
  BillingProcessor::RequestPool::add(
    const Generics::Time& time,
    unsigned long account_id,
    unsigned long advertiser_id,
    unsigned long campaign_id,
    unsigned long ccg_id,
    const RevenueDecimal& ctr,
    const RevenueDecimal& account_amount,
    const RevenueDecimal& amount,
    const RevenueDecimal& imps,
    const RevenueDecimal& clicks,
    bool check_active)
    /*throw(Exception)*/
  {
    assert(ctr <= RevenueDecimal(false, 1, 0));

    Request_var request(new Request(
      // key fields
      time,
      account_id,
      advertiser_id,
      campaign_id,
      ccg_id,
      ctr,
      Request::RM_NORMAL,
      // aggregate fields
      account_amount,
      amount,
      imps,
      clicks
      ));

    RequestArray requests;
    requests.push_back(request);

    add_to_all_servers(requests);

    if(check_active && !active())
    {
      throw Exception("RequestPool::add() called after deactivating");
    }
  }

  void
  BillingProcessor::RequestPool::add_to_all_servers(
    const RequestArray& requests)
    /*throw(Exception)*/
  {
#   ifdef DEBUG_OUTPUT
    {
      std::ostringstream ostr;
      ostr << Generics::Time::get_time_of_day().get_gm_time().format("%F %T.%q") <<
        ": add_to_all_servers(requests = " << requests.size() << ")" <<
        std::endl;
      std::cout << ostr.str();
      std::cout.flush();
    }
#   endif

    assert(!servers_.empty());

    RequestArray first_server_requests;
    RequestArray non_first_server_requests;

    for(auto req_it = requests.begin(); req_it != requests.end(); ++req_it)
    {
      if(servers_.size() > 1)
      {
        // divide amount between servers
        RevenueDecimal first_account_amount;
        RevenueDecimal first_amount;
        RevenueDecimal first_imps;
        RevenueDecimal first_clicks;

        RevenueDecimal non_first_account_amount;
        RevenueDecimal non_first_amount;
        RevenueDecimal non_first_imps;
        RevenueDecimal non_first_clicks;

        {
          RevenueDecimal account_amount_div_reminder;
          non_first_account_amount = RevenueDecimal::div(
            (*req_it)->account_amount,
            RevenueDecimal(false, servers_.size(), 0),
            account_amount_div_reminder);
          first_account_amount =
            non_first_account_amount + account_amount_div_reminder;

          RevenueDecimal amount_div_reminder;
          non_first_amount = RevenueDecimal::div(
            (*req_it)->amount,
            RevenueDecimal(false, servers_.size(), 0),
            amount_div_reminder);
          first_amount = non_first_amount + amount_div_reminder;

          RevenueDecimal imps_div_reminder;
          non_first_imps = RevenueDecimal::div(
            (*req_it)->imps,
            RevenueDecimal(false, servers_.size(), 0),
            imps_div_reminder);
          first_imps = non_first_imps + imps_div_reminder;

          RevenueDecimal clicks_div_reminder;
          non_first_clicks = RevenueDecimal::div(
            (*req_it)->clicks,
            RevenueDecimal(false, servers_.size(), 0),
            clicks_div_reminder);
          first_clicks = non_first_clicks + clicks_div_reminder;
        }

        Request_var first_server_request = new Request(
          // key fields
          (*req_it)->rounded_time,
          (*req_it)->account_id,
          (*req_it)->advertiser_id,
          (*req_it)->campaign_id,
          (*req_it)->ccg_id,
          (*req_it)->ctr,
          (*req_it)->mode,
          // aggregate fields
          (*req_it)->first_request_time,
          first_account_amount,
          first_amount,
          first_imps,
          first_clicks
          );

        first_server_requests.push_back(first_server_request);

        Request_var non_first_server_request = new Request(
          // key fields
          (*req_it)->rounded_time,
          (*req_it)->account_id,
          (*req_it)->advertiser_id,
          (*req_it)->campaign_id,
          (*req_it)->ccg_id, 
          (*req_it)->ctr,
          (*req_it)->mode,
          // aggregate fields
          (*req_it)->first_request_time,
          non_first_account_amount,
          non_first_amount,
          non_first_imps,
          non_first_clicks
          );

        non_first_server_requests.push_back(non_first_server_request);
      }
      else
      {
        first_server_requests.push_back(*req_it);
      }
    }
    
    // check key existance by aggregate map
    // if key exists:
    //   if existing first_request_time value great then current - reinsert key into send_amounts
    //   otherwise - add value into key
    // otherwise - insert into agg_amounts, send_amounts
    //
    add_to_server(0, first_server_requests);

    if(servers_.size() > 1)
    {
      for(unsigned long server_i = 2; server_i < servers_.size(); ++server_i)
      {
        // copy requests, because add_to_server get ownership and
        // can modify passed requests
        RequestArray copy_requests;
        for(auto req_it = non_first_server_requests.begin();
          req_it != non_first_server_requests.end(); ++req_it)
        {
          copy_requests.push_back(new Request(**req_it));
        }
        add_to_server(server_i, copy_requests);
      }

      // add without coping (only after copy)
      add_to_server(1, non_first_server_requests);
    }
  }

  void
  BillingProcessor::RequestPool::add_to_server(
    unsigned long server_index,
    RequestArray& requests)
    /*throw(Exception)*/
  {
#   ifdef DEBUG_OUTPUT
    {
      std::ostringstream ostr;
      ostr << Generics::Time::get_time_of_day().get_gm_time().format("%F %T.%q") <<
        ": add_to_server(server_index = "  << server_index <<
        ", requests = " << requests.size() << ")" <<
        std::endl;
      std::cout << ostr.str();
      std::cout.flush();
    }
#   endif

    if(!add_to_server_(server_index, requests))
    {
      add_resend_to_server(server_index + 1, requests);
    }
  }

  void
  BillingProcessor::RequestPool::add_resend_to_server(
    unsigned long server_index,
    RequestArray& requests)
    /*throw(Exception)*/
  {
#   ifdef DEBUG_OUTPUT
    {
      std::ostringstream ostr;
      ostr << Generics::Time::get_time_of_day().get_gm_time().format("%F %T.%q") <<
        ": add_resend_to_server(server_index = "  << server_index <<
        ", requests = " << requests.size() << ")" <<
        std::endl;
      std::cout << ostr.str();
      std::cout.flush();
    }
#   endif

    RequestArray resend_requests_first_server;
    RequestArray resend_requests_next_server;

    resend_requests_first_server.reserve(requests.size());
    resend_requests_next_server.reserve(requests.size());

    for(RequestArray::iterator it = requests.begin();
      it != requests.end(); ++it)
    {
      if((*it)->mode == Request::RM_RESEND)
      {
        resend_requests_next_server.push_back(Request_var());
        resend_requests_next_server.back().swap(*it);
      }
      else // RM_NORMAL
      {
        resend_requests_first_server.push_back(Request_var());
        resend_requests_first_server.back().swap(*it);
      }
    }

    if(!resend_requests_first_server.empty())
    {
      mark_resend_(resend_requests_first_server);

      unsigned long server_i = server_index <= 1 ? 1 : 0;
      while(server_i < server_count())
      {
#       ifdef DEBUG_OUTPUT
        {
          std::ostringstream ostr;
          ostr << Generics::Time::get_time_of_day().get_gm_time().format("%F %T.%q") <<
            ": move resend requests to server #" << server_i <<
            " = " << resend_requests_first_server.size()  << std::endl;
          std::cout << ostr.str();
          std::cout.flush();
        }
#       endif

        if(add_to_server_(server_i, resend_requests_first_server))
        {
          resend_requests_first_server.clear();
          break;
        }
        ++server_i;
      }
    }

    if(server_index < server_count() &&
      !resend_requests_next_server.empty())
    {
      unsigned long server_i = server_index + 1;
      while(server_i < server_count())
      {
#       ifdef DEBUG_OUTPUT
        {
          std::ostringstream ostr;
          ostr << Generics::Time::get_time_of_day().get_gm_time().format("%F %T.%q") <<
            ": move resend requests to server #" << server_i <<
            " = " << resend_requests_next_server.size()  << std::endl;
          std::cout << ostr.str();
          std::cout.flush();
        }
#       endif

        if(add_to_server_(server_i, resend_requests_next_server))
        {
          // resend_requests_next_server owned only if returned true
          resend_requests_next_server.clear();
          break;
        }
        ++server_i;
      }
    }

    if(!resend_requests_first_server.empty() || !resend_requests_next_server.empty())
    {
      // delay requests
      std::copy(resend_requests_next_server.begin(),
        resend_requests_next_server.end(),
        std::back_inserter(resend_requests_first_server));

#     ifdef DEBUG_OUTPUT
      {
        std::ostringstream ostr;
        ostr << Generics::Time::get_time_of_day().get_gm_time().format("%F %T.%q") <<
          ": delayed requests = " << resend_requests_first_server.size()  << std::endl;
        std::cout << ostr.str();
        std::cout.flush();
      }
#     endif

      add_delayed(resend_requests_first_server);
    }
  }

  void
  BillingProcessor::RequestPool::add_delayed(
    RequestArray& requests)
    /*throw(Exception)*/
  {
#   ifdef DEBUG_OUTPUT
    {
      std::ostringstream ostr;
      ostr << Generics::Time::get_time_of_day().get_gm_time().format("%F %T.%q") <<
        ": add_delayed(requests = " << requests.size() << ")" <<
        std::endl;
      std::cout << ostr.str();
      std::cout.flush();
    }
#   endif

    {
      RequestPool::SyncPolicy::WriteGuard guard(delayed_amounts_->lock);
      add_to_aggregator_i_(delayed_amounts_, requests);
    }

    requests.clear();
  }

  bool
  BillingProcessor::RequestPool::add_to_server_(
    unsigned long server_index,
    RequestArray& requests)
    /*throw(Exception)*/
  {
    bool added = false;

    ServerStateHolder_var server = servers_[server_index];

    {
      RequestPool::SyncPolicy::WriteGuard guard(server->lock);

      if(server->active)
      {
        bool server_empty = server->send_amounts.empty();

        // remove from non_empty_servers_ before modifications (
        //   see ServerStateFirstRequestLess)
        if(!server_empty)
        {
          RequestPool::SyncPolicy::WriteGuard guard(non_empty_servers_lock_);
          non_empty_servers_.erase(server);
        }

        add_to_aggregator_i_(server, requests);

        {
#         ifdef DEBUG_OUTPUT
          {
            std::ostringstream ostr;
            ostr << Generics::Time::get_time_of_day().get_gm_time().format("%F %T.%q") <<
              ": signal #" << server_index << std::endl;
            std::cout << ostr.str();
            std::cout.flush();
          }
#         endif

          RequestPool::SyncPolicy::WriteGuard guard(non_empty_servers_lock_);
          non_empty_servers_.insert(server);
          // signal inside non_empty_servers_lock_, because receiver do job outside lock
          non_empty_servers_cond_.signal();
        }

        added = true;
      }
    }

    if(added)
    {
      requests.clear();
    }

    return added;
  }

  void
  BillingProcessor::RequestPool::add_to_aggregator_i_(
    AmountAggregateHolder* amount_aggregator,
    RequestArray& requests)
    /*throw(Exception)*/
  {
    static const char* FUN = "BillingProcessor::RequestPool::add_to_aggregator_i_()";

    try
    {
      for(RequestArray::const_iterator req_it = requests.begin();
        req_it != requests.end(); ++req_it)
      {
        assert(req_it->in());

        AggregateAmountSet::iterator agg_it = amount_aggregator->agg_amounts.find(
          AggregateRequestHashAdapter(*req_it));

        if(agg_it != amount_aggregator->agg_amounts.end())
        {
          const Request_var& add_req = agg_it->request();
          assert(add_req.in());

          if(add_req->first_request_time <= (*req_it)->first_request_time)
          {
            agg_it->request()->add(**req_it);
          }
          else
          {
            // first_request_time changed - reinsert into send_amounts
            amount_aggregator->send_amounts.erase(add_req);
            add_req->add(**req_it);
            amount_aggregator->send_amounts.insert(add_req);
          }
        }
        else
        {
          amount_aggregator->agg_amounts.insert(AggregateRequestHashAdapter(*req_it));
          amount_aggregator->send_amounts.insert(*req_it);
        }
      }
    }
    catch(const RevenueDecimal::Overflow&)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught RevenueDecimal::Overflow";
      throw Exception(ostr);
    }
  }

  void
  BillingProcessor::RequestPool::get_aggregator_requests_i_(
    RequestArray& res_requests,
    AmountAggregateHolder* amount_aggregator,
    unsigned long max_requests)
    noexcept
  {
    SendOrderAmountSet::iterator end_it;
    if(max_requests == 0 ||
      amount_aggregator->send_amounts.size() <= max_requests)
    {
      std::copy(amount_aggregator->send_amounts.begin(),
        amount_aggregator->send_amounts.end(),
        std::back_inserter(res_requests));

      amount_aggregator->send_amounts.clear();
      amount_aggregator->agg_amounts.clear();
    }
    else
    {
      unsigned long req_i = 0;
      while(req_i < max_requests &&
        !amount_aggregator->send_amounts.empty())
      {
        res_requests.push_back(*amount_aggregator->send_amounts.begin());
        amount_aggregator->agg_amounts.erase(
          AggregateRequestHashAdapter(*amount_aggregator->send_amounts.begin()));
        amount_aggregator->send_amounts.erase(amount_aggregator->send_amounts.begin());

        ++req_i;
      }
    }
  }

  void
  BillingProcessor::RequestPool::collect_aggregator_(
    SaveAggregateAmountSet& res_agg_amounts,
    AmountAggregateHolder* amount_aggregator)
    /*throw(Exception)*/
  {
    static const char* FUN = "BillingProcessor::RequestPool::collect_aggregator_()";

    try
    {
      RequestPool::SyncPolicy::ReadGuard guard(amount_aggregator->lock);

      for(AggregateAmountSet::const_iterator agg_it = amount_aggregator->agg_amounts.begin();
        agg_it != amount_aggregator->agg_amounts.end(); ++agg_it)
      {
        SaveAggregateAmountSet::iterator res_agg_it =
          res_agg_amounts.find(SaveAggregateRequestHashAdapter(agg_it->request()));

        if(res_agg_it != res_agg_amounts.end())
        {
          res_agg_it->request()->add(*agg_it->request());
        }
        else
        {
          res_agg_amounts.insert(
            SaveAggregateRequestHashAdapter(
              Request_var(new Request(*agg_it->request()))));
        }
      }
    }
    catch(const RevenueDecimal::Overflow&)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught RevenueDecimal::Overflow";
      throw Exception(ostr);
    }
  }

  bool
  BillingProcessor::RequestPool::get_requests(
    unsigned long& server_index,
    RequestArray& res_requests,
    unsigned long max_requests)
    noexcept
  {
    // get top elements(minimal first_request_time) from send_requests
    res_requests.reserve(max_requests > 0 ? max_requests : 1024);

    while(true)
    {
      ServerStateHolder_var check_server;

      {
        // collect non empty servers
        RequestPool::NonEmptyServersSyncPolicy::WriteGuard guard(
          non_empty_servers_lock_);

        if(!active())
        {
          return false;
        }
        
        while(non_empty_servers_.empty())
        {
          non_empty_servers_cond_.wait(non_empty_servers_lock_);

          if(!active())
          {
            return false;
          }
        }

        // remove server from non_empty_servers_ before modifications(
        //   see ServerStateFirstRequestLess)
        // here no garantees that this server will not be returned by add_to_server_
        // need check non_empty_servers_ again after check_server->lock locking
        //
        ServerStateHolder_var server_state_holder = *non_empty_servers_.begin();
        non_empty_servers_.erase(non_empty_servers_.begin());

        check_server.swap(server_state_holder);

#       ifdef DEBUG_OUTPUT
        {
          std::ostringstream ostr;
          ostr << Generics::Time::get_time_of_day().get_gm_time().format("%F %T.%q") <<
            ": from wait for #" << (check_server ? check_server->server_id : 1000)  <<
            ", non_empty_servers = " << non_empty_servers_.size() << std::endl;
          std::cout << ostr.str();
          std::cout.flush();
        }
#       endif
      }

      if(check_server)
      {
        {
          RequestPool::SyncPolicy::WriteGuard check_server_lock(check_server->lock);

          if(!check_server->send_amounts.empty()) // not cleaned by other thread
          {
            // try remove from non_empty_servers_ again, because it can be returned by add_to_server_,
            // that lock in order server->lock, non_empty_servers_
            {
              RequestPool::NonEmptyServersSyncPolicy::WriteGuard guard(
                non_empty_servers_lock_);

              non_empty_servers_.erase(check_server);
            }

            get_aggregator_requests_i_(
              res_requests,
              check_server,
              max_requests);

            if(!check_server->send_amounts.empty())
            {
              RequestPool::NonEmptyServersSyncPolicy::WriteGuard guard(
                non_empty_servers_lock_);
              non_empty_servers_.insert(check_server);

              non_empty_servers_cond_.signal(); // signal always because this isn't broadcast
            }
          }
        } // check_server->lock

        if(!res_requests.empty())
        {
          server_index = check_server->server_id;
          return true;
        }

        check_server = ServerStateHolder_var();
      } // if(check_server)
    }

    return false; // unreachable
  }

  bool
  BillingProcessor::RequestPool::get_delayed_requests(
    RequestArray& requests)
    noexcept
  {
    RequestPool::SyncPolicy::WriteGuard guard(delayed_amounts_->lock);

    get_aggregator_requests_i_(
      requests,
      delayed_amounts_,
      0 // max_requests : unlimited
      );

    return !requests.empty();
  }

  void
  BillingProcessor::RequestPool::deactivate_server(
    unsigned long server_index)
    noexcept
  {
    ServerStateHolder& server = *(servers_[server_index]);

    // server unreachable - deactivate it (stop tring to send requests)
    RequestPool::SyncPolicy::WriteGuard guard(server.lock);
    server.active = false;
  }

  void
  BillingProcessor::RequestPool::activate_server(
    unsigned long server_index)
    noexcept
  {
    ServerStateHolder_var server = servers_[server_index];

    // server unreachable - deactivate it (stop tring to send requests)
    RequestPool::SyncPolicy::WriteGuard guard(server->lock);
    server->active = true;
    if(!server->send_amounts.empty())
    {
      RequestPool::SyncPolicy::WriteGuard guard(non_empty_servers_lock_);
      non_empty_servers_.insert(server);
      non_empty_servers_cond_.signal();
    }
  }

  void
  BillingProcessor::RequestPool::print(std::ostream& ostr) noexcept
  {
    std::list<std::unique_ptr<RequestPool::SyncPolicy::WriteGuard> > locks;

    locks.emplace_back(new RequestPool::SyncPolicy::WriteGuard(delayed_amounts_->lock));

    for(ServerArray::const_iterator server_it = servers_.begin();
      server_it != servers_.end(); ++server_it)
    {
      locks.emplace_back(new RequestPool::SyncPolicy::WriteGuard((*server_it)->lock));
    }

    ostr << "Delayed amounts:" << std::endl;

    for(AggregateAmountSet::const_iterator agg_it = delayed_amounts_->agg_amounts.begin();
      agg_it != delayed_amounts_->agg_amounts.end(); ++agg_it)
    {
      agg_it->request()->print(ostr, "  ");
      ostr << std::endl;
    }

    unsigned long server_i = 0;
    for(ServerArray::const_iterator server_it = servers_.begin();
        server_it != servers_.end(); ++server_it, ++server_i)
    {
      ostr << "Server #" << server_i << ":" << std::endl;
      for(AggregateAmountSet::const_iterator agg_it = (*server_it)->agg_amounts.begin();
        agg_it != (*server_it)->agg_amounts.end(); ++agg_it)
      {
        agg_it->request()->print(ostr, "  ");
        ostr << std::endl;
      }
    }
  }

  void
  BillingProcessor::RequestPool::load(
    const String::SubString& storage_root)
    /*throw(Exception)*/
  {
    // load storage
    if(FileManip::dir_exists(storage_root))
    {
      load_amounts_(storage_root.str() + "/" + AMOUNT_FILE_);
    }
  }

  void
  BillingProcessor::RequestPool::dump(
    const String::SubString& storage_root)
    /*throw(Exception)*/
  {
    static const char* FUN = "BillingProcessor::RequestPool::dump()";

    // dump storage
    const std::string tmp_root = storage_root.str() + "~";
    const std::string old_storage_tmp_root = storage_root.str() + ".bak";

    DumpSyncPolicy::WriteGuard flush_lock(dump_lock_);

    try
    {
      remove_storage_(tmp_root);
      remove_storage_(old_storage_tmp_root);

      if(mkdir(tmp_root.c_str(), 0777) == -1)
      {
        // folder must not exist
        eh::throw_errno_exception<Exception>(
          FUN,
          ": failed to create folder '",
          tmp_root.c_str(),
          "'");
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception on dump prepairing: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      save_amounts_(tmp_root + "/" + AMOUNT_FILE_);

      FileManip::rename(storage_root, old_storage_tmp_root, true);
      FileManip::rename(tmp_root, storage_root, false);
      remove_storage_(old_storage_tmp_root);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception on dumping: " << ex.what();
      throw Exception(ostr);
    }
  }

  BillingProcessor::RequestPool::ServerArray
  BillingProcessor::RequestPool::init_servers_(unsigned long server_count)
    noexcept
  {
    ServerArray servers;
    servers.resize(server_count, nullptr);
    unsigned long i = 0;
    for(ServerArray::iterator it = servers.begin(); it != servers.end(); ++it, ++i)
    {
      *it = new ServerStateHolder(i);
    }
    return servers;
  }

  void
  BillingProcessor::RequestPool::load_amounts_(
    const String::SubString& file_path)
    /*throw(Exception)*/
  {
    static const char* FUN = "BillingProcessor::RequestPool::load_amounts_()";

    std::ifstream file(file_path.str().c_str(), std::ios_base::in);

    if(!file.is_open())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't open file '" << file_path << "'";
      throw Exception(ostr);
    }

    unsigned long line_i = 0;

    {
      // empty file allowed
      char sym;
      file.get(sym);
      if(!file.eof())
      {
        file.putback(sym);
      }
    }

    while(!file.eof())
    {
      std::string line;

      try
      {
        AdServer::LogProcessing::read_until_eol(file, line);

        if(file.fail())
        {
          throw Exception("read failed or empty line");
        }

        String::StringManip::SplitTab tokenizer(line);
        String::SubString date_str;
        if(!tokenizer.get_token(date_str))
        {
          throw Exception("no date");
        }

        String::SubString account_id_str;
        if(!tokenizer.get_token(account_id_str))
        {
          throw Exception("no account id");
        }

        String::SubString advertiser_id_str;
        if(!tokenizer.get_token(advertiser_id_str))
        {
          throw Exception("no advertiser id");
        }

        String::SubString campaign_id_str;
        if(!tokenizer.get_token(campaign_id_str))
        {
          throw Exception("no campaign id");
        }

        String::SubString ccg_id_str;
        if(!tokenizer.get_token(ccg_id_str))
        {
          throw Exception("no ccg id");
        }

        String::SubString account_amount_str;
        if(!tokenizer.get_token(account_amount_str))
        {
          throw Exception("no account amount");
        }

        String::SubString amount_str;
        if(!tokenizer.get_token(amount_str))
        {
          throw Exception("no amount");
        }

        String::SubString ctr_str;
        tokenizer.get_token(ctr_str);

        String::SubString imps_str;
        tokenizer.get_token(imps_str);

        String::SubString clicks_str;
        tokenizer.get_token(clicks_str);

        /*
        if(amount_str.end() != &line[0] + line.size())
        {
          throw Exception("unexpected content after amount");
        }
        */

        {
          char eol;
          file.get(eol);
          if(!file.eof() && (file.fail() || eol != '\n'))
          {
            Stream::Error ostr;
            ostr << "line isn't closed when expected";
            throw Exception(ostr);
          }
        }

        // use tokenized fields
        unsigned long account_id;
        if(!String::StringManip::str_to_int(account_id_str, account_id))
        {
          Stream::Error ostr;
          ostr << "invalid account id value: '" << account_id_str << "'";
          throw Exception(ostr);
        }

        unsigned long advertiser_id;
        if(!String::StringManip::str_to_int(advertiser_id_str, advertiser_id))
        {
          Stream::Error ostr;
          ostr << "invalid advertiser id value: '" << advertiser_id_str << "'";
          throw Exception(ostr);
        }

        unsigned long campaign_id;
        if(!String::StringManip::str_to_int(campaign_id_str, campaign_id))
        {
          Stream::Error ostr;
          ostr << "invalid campaign id value: '" << campaign_id_str << "'";
          throw Exception(ostr);
        }

        unsigned long ccg_id;
        if(!String::StringManip::str_to_int(ccg_id_str, ccg_id))
        {
          Stream::Error ostr;
          ostr << "invalid ccg id value: '" << ccg_id_str << "'";
          throw Exception(ostr);
        }

        Generics::Time date(date_str, "%Y-%m-%d %H"); // %F
        RevenueDecimal account_amount(account_amount_str);
        RevenueDecimal amount(amount_str);
        RevenueDecimal ctr = !ctr_str.empty() ?
          RevenueDecimal(ctr_str) : RevenueDecimal::ZERO;
        RevenueDecimal imps = !imps_str.empty() ?
          RevenueDecimal(imps_str) : RevenueDecimal::ZERO;
        RevenueDecimal clicks = !clicks_str.empty() ?
          RevenueDecimal(clicks_str) : RevenueDecimal::ZERO;

        add(date,
          account_id,
          advertiser_id,
          campaign_id,
          ccg_id,
          ctr,
          account_amount,
          amount,
          imps,
          clicks,
          false // don't check active
          );
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": can't read '" << file_path <<
          "', invalid line #" << line_i << " '" << line <<
          "': " << ex.what();
        throw Exception(ostr);
      }

      ++line_i;
    }

    file.close();
  }

  void
  BillingProcessor::RequestPool::save_amounts_(
    const String::SubString& file_path)
    /*throw(Exception)*/
  {
    static const char* FUN = "BillingProcessor::RequestPool::save_amounts_()";

    // aggregate servers and dump
    SaveAggregateAmountSet res_agg_amounts;

    collect_aggregator_(res_agg_amounts, delayed_amounts_);

    for(ServerArray::const_iterator server_it = servers_.begin();
      server_it != servers_.end(); ++server_it)
    {
      collect_aggregator_(res_agg_amounts, *server_it);
    }

    // save aggregated stats
    std::ofstream file(
      file_path.str().c_str(),
      std::ios_base::out | std::ios_base::trunc);

    if(!file.is_open())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't open file '" << file_path << "'";
      throw Exception(ostr);
    }

    for(SaveAggregateAmountSet::const_iterator agg_it = res_agg_amounts.begin();
      agg_it != res_agg_amounts.end(); ++agg_it)
    {
      if(agg_it != res_agg_amounts.begin())
      {
        file << std::endl;
      }

      file << agg_it->request()->rounded_time.get_gm_time().format("%F %H") << '\t' <<
        agg_it->request()->account_id << '\t' <<
        agg_it->request()->advertiser_id << '\t' <<
        agg_it->request()->campaign_id << '\t' <<
        agg_it->request()->ccg_id << '\t' <<
        agg_it->request()->account_amount << '\t' <<
        agg_it->request()->amount << '\t' <<
        agg_it->request()->ctr << '\t' <<
        agg_it->request()->imps << '\t' <<
        agg_it->request()->clicks;
    }
  }

  void
  BillingProcessor::RequestPool::remove_storage_(const String::SubString& path)
    /*throw(Exception)*/
  {
    static const char* FUN = "BillingProcessor::RequestPool::remove_storage_()";

    const std::string path_str = path.str();

    ::unlink((path_str + "/" + AMOUNT_FILE_).c_str());

    if(::rmdir(path_str.c_str()) == -1 && errno != ENOENT)
    {
      eh::throw_errno_exception<Exception>(
        FUN,
        ": failed to remove '",
        path_str.c_str(),
        "'");
    }
  }

  // BillingProcessor::Sender impl
  BillingProcessor::Sender::Sender(
    Generics::ActiveObjectCallback* callback,
    unsigned long thread_count,
    Context* context,
    BillingProcessor::RequestPool* request_pool)
    /*throw(eh::Exception)*/
    : Generics::ActiveObjectCommonImpl(
        SingleJob_var(new Job(
          callback,
          context,
          request_pool,
          thread_count)),
        thread_count)
  {}

  // BillingProcessor::Sender::Job::ActivateServerGoal
  BillingProcessor::Sender::Job::ActivateServerGoal::ActivateServerGoal(
    RequestPool* request_pool,
    unsigned long server_index)
    noexcept
    : request_pool_(ReferenceCounting::add_ref(request_pool)),
      server_index_(server_index)
  {}

  void
  BillingProcessor::Sender::Job::ActivateServerGoal::deliver() noexcept
  {
    request_pool_->activate_server(server_index_);
  }

  // BillingProcessor::Sender::Job
  BillingProcessor::Sender::Job::Job(
    Generics::ActiveObjectCallback* callback,
    Context* context,
    RequestPool* request_pool,
    unsigned long thread_count)
    /*throw(eh::Exception)*/
    : SingleJob(callback),
      context_(ReferenceCounting::add_ref(context)),
      request_pool_(ReferenceCounting::add_ref(request_pool)),
      thread_count_(thread_count),
      deactivate_period_(Generics::Time(10))
  {}

  void
  BillingProcessor::Sender::Job::work() noexcept
  {
    while(true)
    {
      unsigned long server_index;
      RequestArray process_requests;

      if(!request_pool_->get_requests(
           server_index,
           process_requests,
           100))
      {
        // interrupted through pool
#       ifdef DEBUG_OUTPUT
        {
          std::ostringstream ostr;
          ostr << Generics::Time::get_time_of_day().get_gm_time().format("%F %T.%q") <<
            ": worker interrupted" << std::endl;
          std::cout << ostr.str();
          std::cout.flush();
        }
#       endif
        break;
      }

#     ifdef DEBUG_OUTPUT
      {
        std::ostringstream ostr;
        ostr << Generics::Time::get_time_of_day().get_gm_time().format("%F %T.%q") <<
          ": send " << process_requests.size()  << std::endl;
        std::cout << ostr.str();
        std::cout.flush();
      }
#     endif

      if(!process_requests.empty())
      {
        try
        {
          context_->request_sender->send_requests(
            process_requests,
            server_index);
        }
        catch(const RequestSender::ServerUnreachable& ex)
        {
          callback()->critical(String::SubString(ex.what()), "ADS-IMPL-3031");

          deactivate_server_(server_index);
        }
        catch(const eh::Exception& ex)
        {
          callback()->critical(String::SubString(ex.what()), "ADS-IMPL-3031");
        }

        // push left normal requests to looped resend mode or
        // to next server if it already have resend marker
        // independent on fail reason (reminded or server is not available)
        if(!process_requests.empty())
        {
          request_pool_->add_resend_to_server(
            server_index + 1,
            process_requests);
        }
      }
    }
  }

  void
  BillingProcessor::Sender::Job::terminate() noexcept
  {
    // terminate based on pool terminate
  }

  void
  BillingProcessor::Sender::Job::deactivate_server_(
    unsigned long server_index)
    noexcept
  {
    request_pool_->deactivate_server(server_index);

    Generics::Goal_var msg = new ActivateServerGoal(
      request_pool_,
      server_index);

    context_->scheduler->schedule(
      msg,
      Generics::Time::get_time_of_day() + deactivate_period_);
  }

  // BillingServerRequestSender impl
  BillingProcessor::BillingServerRequestSender::BillingServerRequestSender(
    const CORBACommons::CorbaObjectRefList& billing_server_refs)
    noexcept
  {
    CORBACommons::CorbaClientAdapter_var corba_client_adapter(
      new CORBACommons::CorbaClientAdapter());

    billing_servers_holder_.reset(new BillingServerArrayHolder());

    for(auto it = billing_server_refs.begin(); it != billing_server_refs.end(); ++it)
    {
      billing_servers_holder_->billing_servers.push_back(
        AdServer::Commons::CorbaObject<AdServer::CampaignSvcs::BillingServer>(
          corba_client_adapter,
          *it));
    }
  }

  BillingProcessor::BillingServerRequestSender::~BillingServerRequestSender()
    noexcept
  {}

  void
  BillingProcessor::BillingServerRequestSender::send_requests(
    RequestArray& requests,
    unsigned long service_index)
    /*throw(ServerUnreachable, Exception)*/
  {
    static const char* FUN = "BillingProcessor::BillingServerRequestSender::send_requests()";

    try
    {
      AdServer::CampaignSvcs::BillingServer::ConfirmBidSeq request_seq;
      request_seq.length(requests.size());

      CORBA::ULong req_i = 0;
      for(RequestArray::const_iterator req_it = requests.begin();
          req_it != requests.end(); ++req_it, ++req_i)
      {
        AdServer::CampaignSvcs::BillingServer::ConfirmBidInfo& confirm_bid_info = request_seq[req_i];
        confirm_bid_info.time = CorbaAlgs::pack_time((*req_it)->rounded_time);
        confirm_bid_info.account_id = (*req_it)->account_id;
        confirm_bid_info.advertiser_id = (*req_it)->advertiser_id;
        confirm_bid_info.campaign_id = (*req_it)->campaign_id;
        confirm_bid_info.ccg_id = (*req_it)->ccg_id;
        confirm_bid_info.ctr = CorbaAlgs::pack_decimal((*req_it)->ctr);
        confirm_bid_info.account_spent_budget = CorbaAlgs::pack_decimal((*req_it)->account_amount);
        confirm_bid_info.spent_budget = CorbaAlgs::pack_decimal((*req_it)->amount);
        confirm_bid_info.reserved_budget = CorbaAlgs::pack_decimal(RevenueDecimal::ZERO);
        confirm_bid_info.imps = CorbaAlgs::pack_decimal((*req_it)->imps);
        confirm_bid_info.clicks = CorbaAlgs::pack_decimal((*req_it)->clicks);
        confirm_bid_info.forced = false;
      }
      
      AdServer::CampaignSvcs::BillingServer::ConfirmBidRefSeq_var remainder_request_seq;

      billing_servers_holder_->billing_servers[service_index]->add_amount(
        remainder_request_seq.out(),
        request_seq);

      RequestArray remind_requests;

      for(CORBA::ULong rr_i = 0; rr_i < remainder_request_seq->length(); ++rr_i)
      {
        const AdServer::CampaignSvcs::BillingServer::ConfirmBidRefInfo& remainder_request =
          (*remainder_request_seq)[rr_i];

        Request_var result = new Request(*requests[remainder_request.index]);
        result->account_amount = CorbaAlgs::unpack_decimal<RevenueDecimal>(
          remainder_request.confirm_bid.account_spent_budget);
        result->amount = CorbaAlgs::unpack_decimal<RevenueDecimal>(
          remainder_request.confirm_bid.spent_budget);
        result->imps = CorbaAlgs::unpack_decimal<RevenueDecimal>(
          remainder_request.confirm_bid.imps);
        result->clicks = CorbaAlgs::unpack_decimal<RevenueDecimal>(
          remainder_request.confirm_bid.clicks);
        remind_requests.push_back(result);
      }

      requests.swap(remind_requests);
    }
    catch(const AdServer::CampaignSvcs::BillingServer::NotReady&)
    {
      Stream::Error ostr;
      ostr << FUN << ": BillingServer::NotReady caught";
      throw RequestSender::ServerUnreachable(ostr.str());
    }
    catch(const AdServer::CampaignSvcs::BillingServer::ImplementationException& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": BillingServer::ImplementationException caught: " <<
        e.description;
      throw RequestSender::ServerUnreachable(ostr.str());
    }
    catch(const CORBA::SystemException& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": CORBA::SystemException caught: " << e;
      throw RequestSender::ServerUnreachable(ostr.str());
    }
    catch(const BillingServerArrayHolder::BillingServerObject::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": BillingServerObject::Exception caught: " << ex.what();
      throw RequestSender::ServerUnreachable(ostr.str());
    }
  }

  unsigned long
  BillingProcessor::BillingServerRequestSender::server_count()
    const noexcept
  {
    return billing_servers_holder_->billing_servers.size();
  }

  BillingProcessor::GrpcBillingServerRequestSender::GrpcBillingServerRequestSender(
    GrpcBillingManagerPool* grpc_billing_manager_pool) noexcept
    : grpc_billing_manager_pool_(ReferenceCounting::add_ref(grpc_billing_manager_pool))
  {
  }

  void BillingProcessor::GrpcBillingServerRequestSender::send_requests(
    RequestArray& requests,
    unsigned long service_index)
  {
    using ConfirmBidInfo = GrpcBillingManagerPool::ConfirmBidInfo;
    try
    {
      std::vector<ConfirmBidInfo> request_seq;
      request_seq.reserve(requests.size());
      for (const auto& request : requests)
      {
        request_seq.emplace_back(
          request->rounded_time,
          request->account_id,
          request->advertiser_id,
          request->campaign_id,
          request->ccg_id,
          request->ctr,
          request->account_amount,
          request->amount,
          RevenueDecimal::ZERO,
          request->imps,
          request->clicks,
          false);
      }

      auto response = grpc_billing_manager_pool_->add_amount(
        service_index,
        request_seq);
      if (!response)
      {
        Stream::Error stream;
        stream << FNS
               << "GrpcBillingManagerPool internal error";
        throw  Exception(stream);
      }
      if (response->has_error())
      {
        const auto& error = response->error();
        const auto& description = response->error().description();
        Stream::Error stream;
        stream << FNS;

        switch (error.type())
        {
        case AdServer::CampaignSvcs::Billing::Proto::Error_Type_NotReady:
          stream << "Not ready";
          break;
        case AdServer::CampaignSvcs::Billing::Proto::Error_Type_Implementation:
          stream << "Implementation";
          break;
        default:
          stream << "Unknown error type";
        }

        stream << ", description="
               << description;
        throw Exception(stream);
      }

      RequestArray remind_requests;
      const auto& info = response->info();
      const auto& remainder_request_seq = info.remainder_request_seq();
      remind_requests.reserve(remainder_request_seq.size());
      for (const auto& remainder_request : remainder_request_seq)
      {
        const auto& request_info = remainder_request.request_info();
        Request_var result = new Request(*requests[remainder_request.index()]);
        result->account_amount = GrpcAlgs::unpack_decimal<RevenueDecimal>(
          request_info.account_spent_budget());
        result->amount = GrpcAlgs::unpack_decimal<RevenueDecimal>(
          request_info.spent_budget());
        result->imps = GrpcAlgs::unpack_decimal<RevenueDecimal>(
          request_info.imps());
        result->clicks = GrpcAlgs::unpack_decimal<RevenueDecimal>(
          request_info.clicks());
        remind_requests.emplace_back(result);
      }

      requests.swap(remind_requests);
    }
    catch (eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << "Exception caught: "
             << exc.what();
      throw RequestSender::ServerUnreachable(stream.str());
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
             << "Exception caught: Unknown error";
      throw RequestSender::ServerUnreachable(stream.str());
    }
  }

  unsigned long BillingProcessor::GrpcBillingServerRequestSender::server_count() const noexcept
  {
    return grpc_billing_manager_pool_->size();
  }

  // BillingProcessor impl
  BillingProcessor::BillingProcessor(
    Logging::Logger* logger,
    Generics::ActiveObjectCallback* callback,
    const String::SubString& storage_root,
    RequestSender* request_sender,
    unsigned long thread_count,
    const Generics::Time& dump_period,
    const Generics::Time& send_delayed_period)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      storage_root_(storage_root.str()),
      dump_period_(dump_period),
      send_delayed_period_(send_delayed_period),
      scheduler_(new Generics::Planner(callback)),
      task_runner_(new Generics::TaskRunner(callback, 2)), // dump, send delayed tasks
      request_sender_(ReferenceCounting::add_ref(request_sender)),
      request_pool_(new RequestPool(request_sender->server_count()))
  {
    add_child_object(scheduler_);
    add_child_object(task_runner_);
    add_child_object(request_pool_);

    // init context
    Sender::Context_var context(new Sender::Context());
    context->logger = logger_;
    context->scheduler = scheduler_;
    context->request_sender = ReferenceCounting::add_ref(request_sender);
    context->billing_processor = this;

    ReferenceCounting::SmartPtr<BillingProcessor::Sender> sender = new Sender(
      callback,
      thread_count,
      context,
      request_pool_
      );

    add_child_object(sender);

    // TODO: do in background
    if(FileManip::dir_exists(storage_root_))
    {
      // load storage
      request_pool_->load(storage_root_);
    }

    Commons::make_repeating_task(
      std::bind(&BillingProcessor::dump_, this),
      task_runner_,
      scheduler_)->deliver();

    Commons::make_repeating_task(
      std::bind(&BillingProcessor::send_delayed_, this),
      task_runner_,
      scheduler_)->deliver();
  }

  BillingProcessor::~BillingProcessor() noexcept
  {}

  void
  BillingProcessor::process_impression(
    const RequestInfo& ri,
    const ImpressionInfo&,
    const ProcessingState& processing_state)
    /*throw(RequestActionProcessor::Exception)*/
  {
    // each impression on frontend side confirmed with adv_revenue
    // adv_revenue + delta_adv_revenue = confirmed adv_revenue (orig)
    if(processing_state.state == RequestInfo::RS_FRAUD ||
      processing_state.state == RequestInfo::RS_DUPLICATE)
    {
      request_pool_->add(
        ri.time,
        ri.adv_account_id,
        ri.advertiser_id,
        ri.campaign_id,
        ri.ccg_id,
        ri.ctr,
        RevenueDecimal(ri.adv_revenue.impression +
          ri.delta_adv_revenue.impression + ri.adv_payable_comm_amount.impression).negate(),
        RevenueDecimal(ri.adv_revenue.impression + ri.delta_adv_revenue.impression).negate(),
        RevenueDecimal(true, 1, 0), // imps: -1
        RevenueDecimal::ZERO // clicks
        );
    }
    // corection on normal impression (delta_adv_revenue)
    else if(processing_state.state == RequestInfo::RS_NORMAL &&
      ri.delta_adv_revenue.impression != RevenueDecimal::ZERO)
    {
      request_pool_->add(
        ri.time,
        ri.adv_account_id,
        ri.advertiser_id,
        ri.campaign_id,
        ri.ccg_id,
        ri.ctr,
        RevenueDecimal(ri.delta_adv_revenue.impression).negate(), // account amount
        RevenueDecimal(ri.delta_adv_revenue.impression).negate(), // campaign & ccg amount
        RevenueDecimal::ZERO, // imps
        RevenueDecimal::ZERO // clicks
        );
    }
  }

  void
  BillingProcessor::process_click(
    const RequestInfo& ri,
    const ProcessingState& processing_state)
    /*throw(RequestActionProcessor::Exception)*/
  {
    if(processing_state.state == RequestInfo::RS_FRAUD ||
      processing_state.state == RequestInfo::RS_DUPLICATE)
    {
      request_pool_->add(
        ri.time,
        ri.adv_account_id,
        ri.advertiser_id,
        ri.campaign_id,
        ri.ccg_id,
        ri.ctr,
        RevenueDecimal(ri.adv_revenue.click +
          ri.delta_adv_revenue.click + ri.adv_payable_comm_amount.click).negate(),
        RevenueDecimal(ri.adv_revenue.click + ri.delta_adv_revenue.click).negate(),
        RevenueDecimal::ZERO, // imps
        RevenueDecimal(true, 1, 0) // clicks: -1
        );
    }
    // corection on normal impression (delta_adv_revenue)
    else if(processing_state.state == RequestInfo::RS_NORMAL &&
      ri.delta_adv_revenue.click != RevenueDecimal::ZERO)
    {
      request_pool_->add(
        ri.time,
        ri.adv_account_id,
        ri.advertiser_id,
        ri.campaign_id,
        ri.ccg_id,
        ri.ctr,
        RevenueDecimal(ri.delta_adv_revenue.click).negate(), // account amount
        RevenueDecimal(ri.delta_adv_revenue.click).negate(), // campaign & ccg amount
        RevenueDecimal::ZERO, // imps
        RevenueDecimal::ZERO // clicks
        );
    }
  }

  void
  BillingProcessor::wait_object() noexcept
  {
    Generics::CompositeActiveObject::wait_object();

    // all tasks stopped, send_delayed_
    // this is required, because send_delayed_ can pull out requests for processing
    dump_();
  }

  Generics::Time
  BillingProcessor::dump_()
    noexcept
  {
    static const char* FUN = "BillingProcessor::dump_()";

    try
    {
      request_pool_->dump(storage_root_);
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::BILLING_PROCESSOR, "ADS-IMPL-3032") <<
        FUN << ": caught eh::Exception: " << ex.what();
    }

    return Generics::Time::get_time_of_day() + dump_period_; // reschedule
  }

  Generics::Time
  BillingProcessor::send_delayed_()
    noexcept
  {
    static const char* FUN = "BillingProcessor::send_delayed_()";

    RequestArray requests;
    request_pool_->get_delayed_requests(requests);

    try
    {
      // send delayed
      for(unsigned long server_index = 0;
        server_index < request_pool_->server_count() && !requests.empty();
        ++server_index)
      {
        try
        {
          request_sender_->send_requests(
            requests,
            server_index);
        }
        catch(const RequestSender::ServerUnreachable& ex)
        {
          logger_->sstream(Logging::Logger::EMERGENCY,
            Aspect::BILLING_PROCESSOR, "ADS-IMPL-3031") <<
            FUN << ": caught RequestSender::ServerUnreachable: " << ex.what();
        }
        catch(const eh::Exception& ex)
        {
          logger_->sstream(Logging::Logger::EMERGENCY,
            Aspect::BILLING_PROCESSOR, "ADS-IMPL-3031") <<
            FUN << ": caught eh::Exception: " << ex.what();
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::BILLING_PROCESSOR, "ADS-IMPL-3031") <<
        FUN << ": caught eh::Exception: " << ex.what();
    }

    if(!requests.empty())
    {
      // pass unprocessed delayed back to pool
      request_pool_->add_delayed(requests);
    }

    return Generics::Time::get_time_of_day() + send_delayed_period_; // reschedule
  }

  void
  BillingProcessor::mark_resend_(RequestArray& requests)
    noexcept
  {
    for(RequestArray::iterator req_it = requests.begin();
      req_it != requests.end(); ++req_it)
    {
      (*req_it)->mode = Request::RM_RESEND;
    }
  }
}
}
