#include <memory>
#include <vector>
#include <iostream>
#include <Sync/SyncPolicy.hpp>

#include <Commons/zmq.hpp>
#include "TagRequestProfiler.hpp"

//#define DEBUG_OUTPUT

// TODO: multithread mode (TagRequestProfiler correctly works only on one thread)
namespace AdServer
{
namespace RequestInfoSvcs
{
  namespace
  {
    const unsigned long BANKS_COUNT = 50;

    struct Request
    {
      Request(
        const Generics::Time& time_val,
        const Generics::Time& push_time_val,
        const AdServer::Commons::UserId& user_id_val,
        unsigned long colo_id_val,
        const HTTP::BrowserAddress& url_val,
        Commons::StringHolder* user_agent_val)
        noexcept;

      const Generics::Time time;
      const Generics::Time push_time;
      const AdServer::Commons::UserId user_id;
      const unsigned long colo_id;
      const HTTP::BrowserAddress url;
      const Commons::StringHolder_var user_agent;
      uint64_t filter_hash;
    };
  }

  struct RequestPool: public ReferenceCounting::AtomicImpl
  {
    typedef std::list<Request> RequestList;
    typedef Sync::Policy::PosixThread SyncPolicy;

    // UID + url hash
    typedef Generics::NumericHashAdapter<uint64_t>
      RequestHashAdapter;

    struct RequestFilter
    {
      typedef std::set<Generics::Time> TimeSet;
      TimeSet times;
    };

    typedef Generics::GnuHashTable<
      RequestHashAdapter, RequestFilter>
      RequestFilterMap;

    struct Bank
    {
      Bank(
        const Generics::Time& start_time_val,
        const Generics::Time& end_time_val)
        noexcept
        : start_time(start_time_val),
          end_time(end_time_val),
          requests_size(0)
      {}

      void
      filter_requests(
        RequestList& filtered_requests,
        RequestList& requests,
        const Generics::Time& time_start,
        const Generics::Time& time_end)
        const noexcept;

      const Generics::Time start_time;
      const Generics::Time end_time;
      RequestList requests;
      // time markers
      RequestFilterMap request_filters;
      unsigned long requests_size;
    };

    typedef std::list<Bank> BankList;

    RequestPool(
      const Generics::Time& bank_period_val,
      unsigned long max_request_pool_size_val)
      : bank_period(bank_period_val),
        max_request_pool_size(max_request_pool_size_val),
        sum_requests_size(0)
    {}

    Generics::Time
    time_bank_round(const Generics::Time& time) const
      noexcept;

    void
    print(std::ostream& ostr) const noexcept;

    const Generics::Time bank_period;
    const unsigned long max_request_pool_size;

    mutable SyncPolicy::Mutex lock;
    mutable Sync::Conditional cond;
    BankList banks; // sorted by start_time ascending
    unsigned long sum_requests_size;
    Generics::Time last_request_time;

    unsigned long cur_bank_sent_requests;
    unsigned long cur_bank_target_requests;

  protected:
    virtual ~RequestPool() noexcept
    {}
  };

  typedef ReferenceCounting::SmartPtr<RequestPool>
    RequestPool_var;

  namespace
  {
    struct ZmqSocketHolder: public ReferenceCounting::DefaultImpl<>
    {
    public:
      typedef Sync::Policy::PosixThread SyncPolicy;

    public:
      ZmqSocketHolder(zmq::context_t& zmq_context, const char* address)
        /*throw(eh::Exception)*/;

      SyncPolicy::Mutex lock;
      zmq::socket_t sock;

    protected:
      virtual ~ZmqSocketHolder() noexcept
      {}
    };

    typedef ReferenceCounting::SmartPtr<ZmqSocketHolder> ZmqSocketHolder_var;

    typedef std::vector<ZmqSocketHolder_var> ZmqSocketHolderArray;

    class Sender:
      public Generics::ActiveObjectCommonImpl
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      Sender(
        Generics::ActiveObjectCallback* callback,
        unsigned long thread_count,
        RequestPool* request_pool,
        const Generics::Time& window_size,
        unsigned long max_request_pool_size,
        const char* uid_private_key,
        const TagRequestProfiler::AddressList& addresses)
        /*throw(eh::Exception)*/;

    protected:
      virtual ~Sender() noexcept
      {}

    private:
      struct Context: public ReferenceCounting::AtomicImpl
      {
        std::unique_ptr<Generics::SignedUuidGenerator> uid_generator;
        std::unique_ptr<zmq::context_t> zmq_context;
        ZmqSocketHolderArray zmq_socks;

      protected:
        virtual ~Context() noexcept
        {}
      };

      typedef ReferenceCounting::SmartPtr<Context>
        Context_var;

      class Job: public SingleJob
      {
      public:
        Job(
          Generics::ActiveObjectCallback* callback,
          Context* context,
          RequestPool* request_pool,
          unsigned long thread_count,
          const Generics::Time& window_size,
          unsigned long max_request_pool_size)
          /*throw(eh::Exception)*/;

        virtual void
        work() noexcept;

        virtual void
        terminate() noexcept;

      protected:
        virtual ~Job() noexcept
        {}

        void
        get_requests_to_process_(
          Generics::Time& next_request_time,
          RequestPool::RequestList& process_requests,
          const Generics::Time& now);

        void
        send_request_(
          Context& context,
          const Request& request)
          /*throw(Sender::Exception)*/;

        template<typename IntType>
        static
        void
        init_int_message_(
          zmq::message_t& message,
          const IntType& value)
          noexcept;

        static
        unsigned long
        get_url_hash_(const HTTP::HTTPAddress& url) noexcept;

        static
        bool
        zmq_send_(
          zmq::socket_t& sock,
          zmq::message_t& msg,
          int flags,
          const char* part_name)
          /*throw(eh::Exception)*/;

      private:
        const Context_var context_;
        const RequestPool_var request_pool_;
        const unsigned long thread_count_;
        const Generics::Time window_size_;
        const unsigned long max_request_pool_size_;
        const Generics::Time min_sending_period_;

        bool interrupt_;
      };

    private:
      static Context_var
      init_context_(
        const char* uid_private_key,
        const TagRequestProfiler::AddressList& addresses)
        /*throw(Exception)*/;
    };
  }

  namespace
  {
    // ZmqSocketHolder
    ZmqSocketHolder::ZmqSocketHolder(
      zmq::context_t& zmq_context,
      const char* address)
      /*throw(eh::Exception)*/
      : sock(zmq_context, ZMQ_PUSH)
    {
#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(3, 0, 0)
      const int hwm = 1000;
      sock.setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));
      sock.setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
#else
      const uint64_t hwm = 1000;
      sock.setsockopt(ZMQ_HWM, &hwm, sizeof(hwm));
#endif

      const int linger = 10; // 10 ms
      sock.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
      const int reconnect_interval = 100; // 100 ms
      sock.setsockopt(ZMQ_RECONNECT_IVL,
        &reconnect_interval, sizeof(reconnect_interval));
      sock.connect(address);
    }

    // Request
    Request::Request(
      const Generics::Time& time_val,
      const Generics::Time& push_time_val,
      const AdServer::Commons::UserId& user_id_val,
      unsigned long colo_id_val,
      const HTTP::BrowserAddress& url_val,
      Commons::StringHolder* user_agent_val)
      noexcept
      : time(time_val),
        push_time(push_time_val),
        user_id(user_id_val),
        colo_id(colo_id_val),
        url(url_val),
        user_agent(ReferenceCounting::add_ref(user_agent_val))
    {
      Generics::Murmur64Hash hash(filter_hash);
      size_t user_id_hash = user_id.hash();
      hash.add(&user_id_hash, sizeof(user_id_hash));
      hash.add(url.url().data(), url.url().size());
    }

    // Sender::Job
    Sender::Job::Job(
      Generics::ActiveObjectCallback* callback,
      Context* context,
      RequestPool* request_pool,
      unsigned long thread_count,
      const Generics::Time& window_size,
      unsigned long max_request_pool_size)
      /*throw(eh::Exception)*/
      : SingleJob(callback),
        context_(ReferenceCounting::add_ref(context)),
        request_pool_(ReferenceCounting::add_ref(request_pool)),
        thread_count_(thread_count),
        window_size_(window_size),
        max_request_pool_size_(max_request_pool_size),
        min_sending_period_(10),
        interrupt_(false)
    {}

    void
    Sender::Job::get_requests_to_process_(
      Generics::Time& next_request_time,
      RequestPool::RequestList& process_requests,
      const Generics::Time& now)
    {
      const Generics::Time up_now_bound = request_pool_->time_bank_round(now);
      const Generics::Time up_time_bound = request_pool_->time_bank_round(
        now - window_size_);

      RequestPool::SyncPolicy::WriteGuard lock(request_pool_->lock);

#     ifdef DEBUG_OUTPUT
      std::cout << "RP(" << now.get_gm_time().format("%F %T.%q") << "): ";
      request_pool_->print(std::cout);
      std::cout << std::endl;
#     endif

      if(!request_pool_->banks.empty())
      {
        // check expired banks
        RequestPool::BankList::iterator bank_it =
          request_pool_->banks.begin();

        while(bank_it != request_pool_->banks.end() &&
          bank_it->start_time <= up_time_bound)
        {
          if(bank_it->requests_size != 0)
          {
            process_requests.splice(
              process_requests.begin(),
              bank_it->requests,
              bank_it->requests.begin(),
              bank_it->requests.end());
            request_pool_->sum_requests_size -= bank_it->requests_size;
          }
          ++bank_it;
        }

#       ifdef DEBUG_OUTPUT
        if(request_pool_->banks.begin() != bank_it)
        {
          std::cout << "EXPIRED(" << now.get_gm_time().format("%F %T.%q") << ")" << std::endl;
        }
#       endif

        request_pool_->banks.erase(
          request_pool_->banks.begin(),
          bank_it);

        Generics::Time round_last_request_time =
          request_pool_->time_bank_round(request_pool_->last_request_time);

        if(round_last_request_time - window_size_ < up_time_bound)
        {
#         ifdef DEBUG_OUTPUT
          std::cout << "SWITCH BANK(" << now.get_gm_time().format("%F %T.%q") << ")" << std::endl;
#         endif

          // switch banks - recalculate sending period
          unsigned long cur_bank_target_requests = 0;
          unsigned long sum_requests_size = 0;

          for(; bank_it != request_pool_->banks.end();
              ++bank_it)
          {
            if(bank_it->requests_size != 0)
            {
              sum_requests_size += bank_it->requests_size;
              cur_bank_target_requests = std::max(
                cur_bank_target_requests,
                static_cast<unsigned long>(
                  static_cast<unsigned long long>(sum_requests_size) *
                    request_pool_->bank_period.tv_sec /
                    (bank_it->start_time - up_time_bound).tv_sec));

#             ifdef DEBUG_OUTPUT
              std::cout << "CBRT: cbtr = " << cur_bank_target_requests <<
                ", sum_requests_size = " << sum_requests_size <<
                ", bp.i = " << request_pool_->bank_period.tv_sec <<
                ", su.i = " << (bank_it->start_time - up_time_bound).tv_sec <<
                std::endl;
#             endif
            }
          }

          request_pool_->cur_bank_target_requests = cur_bank_target_requests;
          request_pool_->cur_bank_sent_requests = 0;
        }

        // pop request from oldest bank
        unsigned long send_requests = 1;
        if(max_request_pool_size_ - 1 < request_pool_->sum_requests_size)
        {
          send_requests = request_pool_->sum_requests_size + 1 -
            max_request_pool_size_;
        }

        while(!request_pool_->banks.empty() && send_requests != 0)
        {
          RequestPool::BankList::iterator first_bank_it = request_pool_->banks.begin();

          assert(!first_bank_it->requests.empty());

          ++request_pool_->cur_bank_sent_requests;
          --request_pool_->sum_requests_size;
          --send_requests;

          process_requests.splice(
            process_requests.begin(),
            first_bank_it->requests,
            first_bank_it->requests.begin(),
            ++(first_bank_it->requests.begin()));

          if(--(first_bank_it->requests_size) == 0)
          {
            request_pool_->banks.erase(first_bank_it);
          }
        }

        request_pool_->last_request_time = std::max(
          request_pool_->last_request_time, now);

        // eval next sending period by cur_bank_target_requests
        // allow to adapt speed for additional processing time
        if(request_pool_->cur_bank_target_requests >
           request_pool_->cur_bank_sent_requests)
        {
#         ifdef DEBUG_OUTPUT
          std::cout << "NT1" << std::endl;
#         endif
          next_request_time = now + (up_now_bound - now) /
            (request_pool_->cur_bank_target_requests - request_pool_->cur_bank_sent_requests);
        }
        else
        {
#         ifdef DEBUG_OUTPUT
          std::cout << "NT2: cbsr = " << request_pool_->cur_bank_sent_requests <<
            ", cbtr = " << request_pool_->cur_bank_target_requests << std::endl;
#         endif
          next_request_time = up_now_bound;
        }
      }
      else
      {
#       ifdef DEBUG_OUTPUT
        std::cout << "NT3" << std::endl;
#       endif
        next_request_time = up_now_bound;
      }
    }

    void
    Sender::Job::work() noexcept
    {
      // Spread requests in time with using period = min(t1 / 1, t2 / 2, ...)
      // where tX is time of request with index X
      // but recalculation of min when request processed by all pool requests is very hard
      // approximate all requests to fixed number of banks:
      // bank #1 contains requests with time in (0, delta), bank #2 - (delta, 2*delta), ...
      Generics::Time next_request_time;

      while(true)
      {
        {
          bool process_requests = false;

          // wait next sending time
          Sync::ConditionalGuard guard(
            request_pool_->cond,
            request_pool_->lock);

          while(!interrupt_ && !process_requests)
          {
            try
            {
              process_requests = !guard.timed_wait(&next_request_time);
            }
            catch(const eh::Exception& ex)
            {
              callback()->critical(String::SubString(ex.what()));
              process_requests = false;
            }

            process_requests |= (
              request_pool_->sum_requests_size > request_pool_->max_request_pool_size);
          }
        }

        if(interrupt_)
        {
          return;
        }

        Generics::Time now = Generics::Time::get_time_of_day();
        RequestPool::RequestList process_requests;

        get_requests_to_process_(
          next_request_time,
          process_requests,
          now);

        if(!process_requests.empty())
        {
#         ifdef DEBUG_OUTPUT
          std::cerr << Generics::Time::get_time_of_day().get_gm_time().format("%F %T.%q") <<
            ": send " << process_requests.size() <<
            " requests, next_request_time = " << next_request_time.get_gm_time().format("%F %T.%q") << std::endl;
#         endif

          // send requests
          for(RequestPool::RequestList::const_iterator it =
                process_requests.begin();
              it != process_requests.end(); ++it)
          {
            try
            {
              send_request_(*context_, *it);
            }
            catch(const eh::Exception& ex)
            {
              callback()->critical(String::SubString(ex.what()));
            }
          }

          process_requests.clear();
        }
      }
    }

    void
    Sender::Job::terminate() noexcept
    {
      RequestPool::SyncPolicy::WriteGuard guard(request_pool_->lock);
      interrupt_ = true;
      request_pool_->cond.broadcast();
    }

    template<typename IntType>
    void
    Sender::Job::init_int_message_(
      zmq::message_t& message,
      const IntType& value)
      noexcept
    {
      char BUF[std::numeric_limits<IntType>::digits10 + 3];
      size_t res_size = String::StringManip::int_to_str(
        value,
        BUF,
        sizeof(BUF));
      assert(res_size > 0);
      zmq::message_t m(res_size);
      message.rebuild(res_size);
      ::memcpy(message.data(), BUF, res_size);
    }

    void
    Sender::Job::send_request_(
      Context& context,
      const Request& request)
      /*throw(Sender::Exception)*/
    {
      static const char* FUN = "Sender::Job::send_request_()";

      try
      {
        uint32_t domain_hash = get_url_hash_(request.url);
        size_t sock_ind = domain_hash % context.zmq_socks.size();
        ZmqSocketHolder& sock_holder = *context.zmq_socks[sock_ind];

        // init empty messages array (one message can't be reused in send)
        zmq::message_t empty_messages_holder[4];
        zmq::message_t* empty_message = &empty_messages_holder[0];
        zmq::message_t request_id_message;

        {
          const std::string request_id_str = AdServer::Commons::
            RequestId::create_random_based().to_string();
          request_id_message.rebuild(request_id_str.size());
          ::memcpy(request_id_message.data(), request_id_str.data(), request_id_str.size());
        }

        zmq::message_t url_message;

        {
          HTTP::BrowserAddress workaround_url(request.url);
          std::string send_url;
          workaround_url.get_view(
            HTTP::HTTPAddress::VW_FULL & ~HTTP::HTTPAddress::VW_PROTOCOL,
            send_url);
          url_message.rebuild(send_url.size());
          ::memcpy(url_message.data(), send_url.data(), send_url.size());
        }

        zmq::message_t user_agent_message(
          request.user_agent ? request.user_agent->str().size() : 0);
        if(request.user_agent)
        {
          ::memcpy(
            user_agent_message.data(),
            request.user_agent->str().data(),
            request.user_agent->str().size());
        }

        zmq::message_t user_id_message;

        {
          const std::string signed_user_id_str = context.uid_generator->sign(
            request.user_id).str();
          user_id_message.rebuild(signed_user_id_str.size());
          ::memcpy(
            user_id_message.data(),
            signed_user_id_str.data(),
            signed_user_id_str.size());
        }

        zmq::message_t tv_sec_message;
        init_int_message_(tv_sec_message, request.time.tv_sec);

        zmq::message_t tv_usec_message;
        init_int_message_(tv_usec_message, request.time.tv_usec);

        zmq::message_t colo_id_message;
        init_int_message_(colo_id_message, request.colo_id);

        ZmqSocketHolder::SyncPolicy::WriteGuard lock(sock_holder.lock);

        zmq::socket_t& sock = sock_holder.sock;

        // send messages array
        // RequestID
        if(zmq_send_(sock, request_id_message, ZMQ_SNDMORE | ZMQ_NOBLOCK, "request id")) // ! HWM
        {
          // URL without protocol prefix
          zmq_send_(sock, url_message, ZMQ_SNDMORE, "url");
          // Server IP
          zmq_send_(sock, *empty_message++, ZMQ_SNDMORE, "server ip");
          // Referrer
          zmq_send_(sock, *empty_message++, ZMQ_SNDMORE, "referer");
          // User-Agent
          zmq_send_(sock, user_agent_message, ZMQ_SNDMORE, "user-agent");
          // signed UID
          zmq_send_(sock, user_id_message, ZMQ_SNDMORE, "uid");

          // TUID
          zmq_send_(sock, *empty_message++, ZMQ_SNDMORE, "tuid");
          // Keywords
          zmq_send_(sock, *empty_message++, ZMQ_SNDMORE, "keywords");
          // Time secs
          zmq_send_(sock, tv_sec_message, ZMQ_SNDMORE, "secs");
          // Time usecs
          zmq_send_(sock, tv_usec_message, ZMQ_SNDMORE, "usecs");
          // Colo
          zmq_send_(sock, colo_id_message, 0, "colo");
        }
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't send message: " << ex.what();
        throw Sender::Exception(ostr);
      }
    }

    unsigned long
    Sender::Job::get_url_hash_(const HTTP::HTTPAddress& url)
      noexcept
    {
      String::SubString host = url.host();
      return Generics::CRC::quick(0, host.data(), host.length());
    }

    bool
    Sender::Job::zmq_send_(
      zmq::socket_t& sock,
      zmq::message_t& msg,
      int flags,
      const char* part_name)
      /*throw(eh::Exception)*/
    {
      bool send_res;

      try
      {
        send_res = sock.send(msg, flags);
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << "On sending '" << part_name << "' "
          "caught zmq::exception: " << ex.what();
        throw TagRequestProcessor::Exception(ostr);
      }

      if(!send_res)
      {
        if(errno != EAGAIN)
        {
          Stream::Error ostr;
          ostr << "Can't send '" << part_name << "' part, errno: " <<
            strerror(errno);
          throw TagRequestProcessor::Exception(ostr);
        }

        return false;
      }

      return true;
    }

    // Sender
    Sender::Sender(
      Generics::ActiveObjectCallback* callback,
      unsigned long thread_count,
      RequestPool* request_pool,
      const Generics::Time& window_size,
      unsigned long max_request_pool_size,
      const char* uid_private_key,
      const TagRequestProfiler::AddressList& addresses)
      /*throw(eh::Exception)*/
      : Generics::ActiveObjectCommonImpl(
          SingleJob_var(new Job(
            callback,
            init_context_(uid_private_key, addresses),
            request_pool,
            thread_count,
            window_size,
            max_request_pool_size)),
          thread_count)
    {}

    Sender::Context_var
    Sender::init_context_(
      const char* uid_private_key,
      const TagRequestProfiler::AddressList& addresses)
      /*throw(Exception)*/
    {
      static const char* FUN = "Sender::init_context_()";

      try
      {
        Context_var context = new Context();

        context->zmq_context.reset(new zmq::context_t(addresses.size()));

        context->uid_generator.reset(
          new Generics::SignedUuidGenerator(uid_private_key));

        for(TagRequestProfiler::AddressList::const_iterator addr_it =
              addresses.begin();
            addr_it != addresses.end(); ++addr_it)
        {
          context->zmq_socks.push_back(
            new ZmqSocketHolder(*context->zmq_context, addr_it->c_str()));
        }

        return context;
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": can't init zmq sockets pool: " << ex.what();
        throw Exception(ostr);
      }
    }
  }

  // RequestPool
  void
  RequestPool::Bank::filter_requests(
    RequestList& filtered_requests,
    RequestList& requests,
    const Generics::Time& time_start,
    const Generics::Time& time_end)
    const noexcept
  {
    for(RequestList::iterator req_it = requests.begin();
        req_it != requests.end(); )
    {
      RequestFilterMap::const_iterator f_it =
        request_filters.find(req_it->filter_hash);
      if(f_it != request_filters.end())
      {
        RequestFilter::TimeSet::const_iterator time_it =
          f_it->second.times.lower_bound(time_start);
        if(time_it != f_it->second.times.end() && *time_it < time_end)
        {
          filtered_requests.splice(filtered_requests.begin(), requests, req_it++);
        }
        else
        {
          ++req_it;
        }
      }
      else
      {
        ++req_it;
      }
    }
  }

  Generics::Time
  RequestPool::time_bank_round(const Generics::Time& time) const
    noexcept
  {
    Generics::Time res((time.tv_sec / bank_period.tv_sec + 1) *
      bank_period.tv_sec);

    /*
    std::cout << "time_bank_round(" << time.get_gm_time().format("%F %T.%q") <<
      "): " << res.get_gm_time().format("%F %T.%q") <<
      ", t = " << (time / bank_period.tv_sec).get_gm_time().format("%F %T.%q") <<
      ", i = " << (((time / bank_period.tv_sec).tv_sec + 1)) << std::endl;
    */
    return res;
  }

  void
  RequestPool::print(std::ostream& ostr) const noexcept
  {
    ostr <<
      "bank_period = " << bank_period.get_gm_time().format("%s.%q") <<
      ", sum_requests_size = " << sum_requests_size <<
      ", last_request_time = " << last_request_time.get_gm_time().format("%F %T.%q") <<
      ", cbsr = " << cur_bank_sent_requests <<
      ", cbtr = " << cur_bank_target_requests <<
      ":";
    for(BankList::const_iterator bank_it = banks.begin();
        bank_it != banks.end(); ++bank_it)
    {
      ostr << "[s=" << bank_it->start_time.get_gm_time().format("%F %T") <<
        ",e=" << bank_it->end_time.get_gm_time().format("%F %T") <<
        ",sz=" << bank_it->requests_size <<
        ",rsz=" << bank_it->requests.size() << "]";
    }
  }

  // TagRequestProfiler
  TagRequestProfiler::TagRequestProfiler(
    Logging::Logger* logger,
    Generics::ActiveObjectCallback* callback,
    unsigned long thread_count,
    const Generics::Time& window_size,
    unsigned long max_request_pool_size,
    const char* uid_private_key,
    const AddressList& addresses,
    const Generics::Time& repeat_trigger_timeout)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      max_request_pool_size_(max_request_pool_size),
      repeat_trigger_timeout_(repeat_trigger_timeout)
  {
    request_pool_ = new RequestPool(
      Generics::Time(
        window_size.tv_sec / BANKS_COUNT +
        ((window_size.tv_sec % BANKS_COUNT) ? 1 : 0)),
      max_request_pool_size);

    add_child_object(Generics::ActiveObject_var(
      new Sender(
        callback,
        thread_count,
        request_pool_,
        window_size,
        max_request_pool_size,
        uid_private_key,
        addresses)));
  }

  TagRequestProfiler::~TagRequestProfiler()
    noexcept
  {}

  void
  TagRequestProfiler::process_tag_request(
    const TagRequestInfo& tag_request_info)
    /*throw(TagRequestProcessor::Exception)*/
  {
    static const char* FUN = "TagRequestProfiler::process_tag_request()";

    RequestPool::RequestList filtered_requests;

    try
    {
      const bool referer_not_empty = !tag_request_info.referer.empty() &&
        tag_request_info.referer != "-";

      if ((referer_not_empty || !tag_request_info.urls.empty()) &&
         !tag_request_info.user_id.is_null() &&
         tag_request_info.profile_referer)
      {
        Generics::Time now = Generics::Time::get_time_of_day();

        Generics::Time round_now = request_pool_->time_bank_round(now);
        bool create_new_bank = false;

        RequestPool::RequestList req_list;

        if (referer_not_empty)
        {
          req_list.push_back(
            Request(
              tag_request_info.time,
              now,
              tag_request_info.user_id,
              tag_request_info.colo_id,
              HTTP::BrowserAddress(tag_request_info.referer),
              tag_request_info.user_agent));
        }

        for (auto it = tag_request_info.urls.begin();
          it != tag_request_info.urls.end(); ++it)
        {
          req_list.push_back(
            Request(
              tag_request_info.time,
              now,
              tag_request_info.user_id,
              tag_request_info.colo_id,
              HTTP::BrowserAddress(*it),
              tag_request_info.user_agent));
        }

        RequestPool::SyncPolicy::WriteGuard guard(request_pool_->lock);

        RequestPool::BankList::iterator bank_it =
          request_pool_->banks.end();
        if(request_pool_->banks.empty() ||
           request_pool_->banks.rbegin()->end_time < round_now)
        {
          create_new_bank = true;
        }
        else
        {
          bool found_bank = false;

          do
          {
            --bank_it;

            if(round_now > bank_it->start_time)
            {
              if(round_now <= bank_it->end_time)
              {
                found_bank = true;
              }
              break;
            }
          }
          while(bank_it != request_pool_->banks.begin());

          create_new_bank = !found_bank;
        }

        if(create_new_bank)
        {
          RequestPool::Bank new_bank(
            round_now - request_pool_->bank_period,
            round_now);
          bank_it = request_pool_->banks.insert(bank_it, new_bank);
        }

        if(repeat_trigger_timeout_ != Generics::Time::ZERO)
        {
          // check filtering
          const Generics::Time request_filter_start_time =
            tag_request_info.time - repeat_trigger_timeout_;
          const Generics::Time request_filter_end_time =
            tag_request_info.time + repeat_trigger_timeout_;

          // TODO: review possibility to remove full fetch
          for(RequestPool::BankList::const_iterator fbank_it = request_pool_->banks.begin();
            fbank_it != request_pool_->banks.end();
            ++fbank_it)
          {
            fbank_it->filter_requests(
              filtered_requests,
              req_list,
              request_filter_start_time,
              request_filter_end_time);
          }
        }

        if(!req_list.empty())
        {
          const unsigned long req_size = req_list.size();

          // insert filtering marker
          for(RequestPool::RequestList::const_iterator req_it = req_list.begin();
              req_it != req_list.end(); ++req_it)
          {
            bank_it->request_filters[req_it->filter_hash].times.insert(
              tag_request_info.time);
          }

          bank_it->requests.splice(bank_it->requests.end(), req_list);
          bank_it->requests_size += req_size;
          request_pool_->sum_requests_size += req_size;
          if(request_pool_->sum_requests_size >
             request_pool_->max_request_pool_size)
          {
            request_pool_->cond.signal();
          }
        }
      }
    }
    catch (eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::ERROR) << FUN <<
        ": got eh::Exception: " << ex.what();
    }
  }
}
}
