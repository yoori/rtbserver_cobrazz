#include <Stream/MemoryStream.hpp>
#include <String/StringManip.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/ZmqSocketHolder.hpp>
#include <Commons/ZmqConfig.hpp>

#include <Frontends/ProfilingServer/DMPProfilingInfo.hpp>
#include <Frontends/UIDGeneratorAdapter/UIDGeneratorProtocol.pb.h>

#include "KeywordsProcessor.hpp"
#include "AcceptorDescriptorHandler.hpp"
#include "UIDGeneratorAdapterImpl.hpp"

namespace
{
  const char ASPECT[] = "UIDGeneratorAdapter";
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
  const char UID_GENERATOR_ADAPTER_STATS_OBJ_KEY[] = "UIDGeneratorAdapterStats";

  const char KEYWORD_HIT_STAT_OUT_DIR[] = "KeywordHitStat";
  const bool CHECK_CONCURRENCY_ = true;
  const size_t MSG_MAXIOVLEN = 64;
}

namespace AdServer
{
namespace Frontends
{
  namespace
  {
    LogProcessing::LogFlushTraits
    read_flush_policy(
      const xsd::AdServer::Configuration::LogFlushPolicyType&
      log_flush_policy,
      const char* default_path)
    {
      AdServer::LogProcessing::LogFlushTraits res;
      res.period = Generics::Time(log_flush_policy.period());
      res.out_dir = log_flush_policy.path().present() ?
        log_flush_policy.path()->c_str() : default_path;
      return res;
    }
  }
  
  class DMPProfilingSender:
    public ReferenceCounting::AtomicImpl,
    public Generics::SimpleActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  public:
    DMPProfilingSender(
      unsigned long thread_count,
      const xsd::AdServer::Configuration::ZmqSocketType& connect_socket_config,
      const char* source)
      /*throw(Exception)*/;

    void
    send(const Generics::Time& time,
      const String::SubString& external_user_id,
      const String::SubString& bind_user_ids,
      const String::SubString& url,
      const String::SubString& keywords);

  protected:
    virtual
    ~DMPProfilingSender() noexcept
    {}

  protected:
    const std::string source_;
    std::unique_ptr<zmq::context_t> zmq_context_;
    AdServer::Commons::ZmqSocketHolder_var zmq_socket_;
  };

  typedef ReferenceCounting::SmartPtr<DMPProfilingSender>
    DMPProfilingSender_var;

  // VKProtobufDescriptorHandler
  class VKProtobufDescriptorHandler: public DescriptorHandler
  {
  public:
    VKProtobufDescriptorHandler(
      int fd,
      DMPProfilingSender* dmp_sender,
      KeywordsProcessor* keywords_processor,
      UIDGeneratorAdapterLogger* stat_logger,
      UIDGeneratorAdapterImpl::Stats* stats)
      noexcept;

    virtual int
    fd() const noexcept;

    // return false if need stop loop
    virtual unsigned long
    read() /*throw(Exception)*/;

    // return false if need stop loop
    virtual unsigned long
    write() /*throw(Exception)*/;

    virtual void
    stopped() noexcept;

  protected:
    typedef std::vector<unsigned char> Buf;
    typedef std::list<Buf> BufList;

  protected:
    virtual
    ~VKProtobufDescriptorHandler() noexcept;

    unsigned long
    error_() /*throw(Exception)*/;

    void
    add_write_buf_(Buf& buf) noexcept;

    void
    reconfigure_write_bufs_(unsigned long sent_size)
      noexcept;

    void
    process_input_message_(const void* buf, unsigned long size)
      noexcept;

  protected:
    static const unsigned long READ_BLOCK_SIZE_ = 128 * 1024;
    static const unsigned long SHRINK_READ_SIZE_ = 64 * 1024;

    int fd_;
    DMPProfilingSender_var dmp_sender_;
    KeywordsProcessor_var keywords_processor_;
    UIDGeneratorAdapterLogger_var stat_logger_;
    UIDGeneratorAdapterImpl::Stats_var stats_;
    const std::string source_;

    unsigned long read_processed_buf_pos_;
    unsigned long read_buf_pos_;
    Buf read_buf_;

    // all next fields consistent and ready for sendmsg
    BufList write_bufs_;
    unsigned long first_write_buf_pos_; // optimization : allow to avoid buf restruct
    std::vector<iovec> write_iovec_buf_;
    struct msghdr write_msg_;

    Algs::AtomicInt concurrency_count_;
  };

  void
  print_hex(
    std::ostream& ostr,
    const void* buf,
    unsigned long size,
    const char* prefix)
    noexcept
  {
    ostr << prefix << std::hex << std::setfill('0') << std::setw(2);

    for(unsigned long i = 0; i < size; ++i)
    {
      ostr << "0x" << (int)*((const unsigned char*)buf + i) << " ";

      if(i && (i + 1) % 16 == 0)
      {
        ostr << std::endl << prefix;
      }
    }

    ostr << std::dec << std::setw(0);
  }

  // VKProtobufAcceptorDescriptorHandler
  class VKProtobufAcceptorDescriptorHandler: public AcceptorDescriptorHandler
  {
  public:
    VKProtobufAcceptorDescriptorHandler(
      unsigned long port,
      DescriptorHandlerPoller::Proxy* poller_proxy,
      DMPProfilingSender* dmp_sender,
      KeywordsProcessor* keywords_processor,
      UIDGeneratorAdapterLogger* stat_logger,
      UIDGeneratorAdapterImpl::Stats* stats)
      /*throw(Exception)*/;

  protected:
    virtual DescriptorHandler_var
    create_descriptor_handler(int fd)
      noexcept;

  protected:
    DMPProfilingSender_var dmp_sender_;
    KeywordsProcessor_var keywords_processor_;
    UIDGeneratorAdapterLogger_var stat_logger_;
    UIDGeneratorAdapterImpl::Stats_var stats_;
  };

  // DMPProfilingSender
  DMPProfilingSender::DMPProfilingSender(
    unsigned long thread_count,
    const xsd::AdServer::Configuration::ZmqSocketType& connect_socket_config,
    const char* source)
    /*throw(Exception)*/
    : source_(source)
  {
    zmq_context_.reset(new zmq::context_t(thread_count));
    zmq_socket_ = new AdServer::Commons::ZmqSocketHolder(
      *zmq_context_,
      Config::ZmqConfigReader::get_socket_type(connect_socket_config.type()));

    const int send_timeout = 100; // 100 ms                                                              |
    zmq_socket_->sock_i().setsockopt(ZMQ_SNDTIMEO, &send_timeout, sizeof(send_timeout));

    for(auto it = connect_socket_config.Address().begin();
        it != connect_socket_config.Address().end(); ++it)
    {
      const std::string addr = Config::ZmqConfigReader::get_address(*it);
      zmq_socket_->sock_i().connect(addr.c_str());
    }
  }

  void
  DMPProfilingSender::send(
    const Generics::Time& time,
    const String::SubString& external_user_id,
    const String::SubString& bind_user_ids,
    const String::SubString& url,
    const String::SubString& keywords)
  {
    AdServer::Profiling::DMPProfilingInfoWriter writer;
    writer.version() = 1;
    writer.time() = time.tv_sec;
    writer.source() = source_;
    writer.external_user_id() = external_user_id.str();
    writer.bind_user_ids() = bind_user_ids.str();
    writer.url() = url.str();
    writer.keywords() = keywords.str();
    writer.longitude() = 0;
    writer.latitude() = 0;
    unsigned long sz = writer.size();
    zmq::message_t msg(sz);
    writer.save(msg.data(), sz);

    bool send_fin = false;

    while(active() && !send_fin)
    {
      AdServer::Commons::ZmqSocketHolder::SendGuard send_guard(*zmq_socket_);
      send_fin = send_guard.send(msg, 0, "DMPProfiling");
    }
  }

  // VKProtobufDescriptorHandler impl
  VKProtobufDescriptorHandler::VKProtobufDescriptorHandler(
    int fd,
    DMPProfilingSender* dmp_sender,
    KeywordsProcessor* keywords_processor,
    UIDGeneratorAdapterLogger* stat_logger,
    UIDGeneratorAdapterImpl::Stats* stats)
    noexcept
    : fd_(fd),
      dmp_sender_(ReferenceCounting::add_ref(dmp_sender)),
      keywords_processor_(ReferenceCounting::add_ref(keywords_processor)),
      stat_logger_(ReferenceCounting::add_ref(stat_logger)),
      stats_(ReferenceCounting::add_ref(stats)),
      read_processed_buf_pos_(0),
      read_buf_pos_(0),
      first_write_buf_pos_(0),
      concurrency_count_(0)
  {
    read_buf_.reserve(SHRINK_READ_SIZE_ + READ_BLOCK_SIZE_);

    write_iovec_buf_.reserve(10);
    ::memset(&write_msg_, 0, sizeof(write_msg_));

    DescriptorHandlerPoller::set_non_blocking(fd_);
  }

  VKProtobufDescriptorHandler::~VKProtobufDescriptorHandler()
    noexcept
  {
    //std::cerr << "~VKProtobufDescriptorHandler::~VKProtobufDescriptorHandler()" << std::endl;
    ::close(fd_);
  }

  int
  VKProtobufDescriptorHandler::fd() const noexcept
  {
    return fd_;
  }

  unsigned long
  VKProtobufDescriptorHandler::read() /*throw(Exception)*/
  {
    ssize_t read_res;

    while(true)
    {
      // prepare buf for read READ_BLOCK_SIZE_
      if(read_buf_.size() < read_buf_pos_ + READ_BLOCK_SIZE_)
      {
        read_buf_.resize(read_buf_pos_ + READ_BLOCK_SIZE_);
      }

      // do read
      read_res = ::read(fd_, read_buf_.data() + read_buf_pos_, READ_BLOCK_SIZE_);

      if(read_res < 0)
      {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
        {
          return !write_bufs_.empty() ? START_WRITE_HANDLE : STOP_WRITE_HANDLE;
        }

        //std::cerr << "error on read" << std::endl;
        return error_();
      }

      if(read_res == 0)
      {
        // connection closed
        //std::cerr << "connection closed" << std::endl;
        return static_cast<unsigned long>(STOP_READ_HANDLE) | (
          !write_bufs_.empty() ? START_WRITE_HANDLE : STOP_WRITE_HANDLE);
      }

      // some bytes got
      /*
      std::cerr << "Got " << read_res << " bytes:" << std::endl;
      print_hex(std::cerr, read_buf_.data() + read_buf_pos_, read_res, "  ");
      */

      read_buf_pos_ += read_res;

      // check that message fully received
      while(read_buf_pos_ - read_processed_buf_pos_ >= 4)
      {
        // little endian to big
        /*
        unsigned long msg_size = ::htonl(
          *reinterpret_cast<uint32_t*>(
            read_buf_.data() + read_processed_buf_pos_));
        */
        unsigned long msg_size = *reinterpret_cast<uint32_t*>(
            read_buf_.data() + read_processed_buf_pos_);

        if(4 + msg_size > read_buf_pos_ - read_processed_buf_pos_)
        {
          break;
        }

        // process fully available message
        process_input_message_(
          read_buf_.data() + read_processed_buf_pos_ + 4,
          msg_size);
        read_processed_buf_pos_ += 4 + msg_size;

        assert(read_processed_buf_pos_ <= read_buf_pos_);
      }

      assert(read_processed_buf_pos_ <= read_buf_pos_);

      // shrink buf
      if(read_processed_buf_pos_ > SHRINK_READ_SIZE_)
      {
        auto tgt_it = read_buf_.begin();
        for(auto it = read_buf_.begin() + read_processed_buf_pos_;
          it != read_buf_.begin() + read_buf_pos_;
          ++it, ++tgt_it)
        {
          *tgt_it = *it;
        }

        read_buf_pos_ -= read_processed_buf_pos_;
        read_processed_buf_pos_ = 0;
      }
    }

    assert(false); // never reach
    return 0;
  }

  unsigned long
  VKProtobufDescriptorHandler::write() /*throw(Exception)*/
  {
    /*
    std::cerr << "VKProtobufDescriptorHandler::write(): write_bufs_.size() = " << write_bufs_.size() <<
      ", write_msg_.msg_iovlen = " << write_msg_.msg_iovlen << std::endl;
    */

    ssize_t sent_size;

    while(!write_bufs_.empty())
    {
      sent_size = ::sendmsg(fd_, &write_msg_, 0);

      if(sent_size <= 0)
      {
        //std::cerr << "sent_size = " << sent_size << ", errno = " << errno << std::endl;

        if(errno == EAGAIN || errno == EWOULDBLOCK || sent_size == 0)
        {
          return CONTINUE_HANDLE;
        }

        // error on write
        //std::cerr << "error on write, errno = " << errno << std::endl;
        return error_();
      }

      //std::cerr << "sent_size = " << sent_size << std::endl;

      assert(sent_size > 0);

      reconfigure_write_bufs_(sent_size);
    }

    return write_bufs_.empty() ? STOP_WRITE_HANDLE : START_WRITE_HANDLE;
  }

  unsigned long
  VKProtobufDescriptorHandler::error_() /*throw(Exception)*/
  {
    return static_cast<unsigned long>(STOP_READ_HANDLE) | STOP_WRITE_HANDLE;
  }

  void
  VKProtobufDescriptorHandler::stopped() noexcept
  {}

  void
  VKProtobufDescriptorHandler::add_write_buf_(Buf& buf) noexcept
  {
    if(CHECK_CONCURRENCY_)
    {
      int p = concurrency_count_.exchange_and_add(1);
      assert(p == 0);
    }

    write_bufs_.push_back(Buf());
    Buf& new_buf = write_bufs_.back();
    new_buf.swap(buf);

    iovec el;
    el.iov_base = new_buf.data();
    el.iov_len = new_buf.size();
    write_iovec_buf_.push_back(el);

    write_msg_.msg_iov = write_iovec_buf_.data();
    write_msg_.msg_iovlen = std::min(write_iovec_buf_.size(), MSG_MAXIOVLEN);

    if(CHECK_CONCURRENCY_)
    {
      int p = concurrency_count_.exchange_and_add(-1);
      assert(p == 1);
    }
  }

  void
  VKProtobufDescriptorHandler::reconfigure_write_bufs_(
    unsigned long sent_size)
    noexcept
  {
    if(CHECK_CONCURRENCY_)
    {
      int p = concurrency_count_.exchange_and_add(1);
      assert(p == 0);
    }

    if(sent_size > 0)
    {
      assert(!write_bufs_.empty());

      unsigned long prev_cur_size = 0;
      unsigned long cur_size = 0;
      auto sent_it = write_bufs_.begin();
      unsigned long sent_i = 0;

      cur_size += sent_it->size() - first_write_buf_pos_;

      for(; sent_it != write_bufs_.end(); ++sent_it, ++sent_i)
      {
        if(cur_size > sent_size)
        {
          break;
        }

        prev_cur_size = cur_size;
        cur_size += sent_it->size();
      }

      // sent_it at pos before that we should remove elements and
      // sent_it need to shrink
      write_bufs_.erase(write_bufs_.begin(), sent_it);
      assert(sent_i <= write_iovec_buf_.size());
      write_iovec_buf_.erase(write_iovec_buf_.begin(), write_iovec_buf_.begin() + sent_i);

      // check last element
      if(cur_size > sent_size && prev_cur_size < sent_size)
      {
        assert(!write_bufs_.empty());
        assert(write_bufs_.size() == write_iovec_buf_.size());

        first_write_buf_pos_ = sent_size - prev_cur_size;
        Buf& buf = *write_bufs_.begin();
        iovec& io_vec = *write_iovec_buf_.begin();
        io_vec.iov_base = &buf[first_write_buf_pos_];
        io_vec.iov_len = buf.size() - first_write_buf_pos_;
      }
      else
      {
        first_write_buf_pos_ = 0;
      }

      write_msg_.msg_iov = write_iovec_buf_.data();
      write_msg_.msg_iovlen = std::min(write_iovec_buf_.size(), MSG_MAXIOVLEN);
    }

    if(CHECK_CONCURRENCY_)
    {
      int p = concurrency_count_.exchange_and_add(-1);
      assert(p == 1);
    }
  }

  void
  VKProtobufDescriptorHandler::process_input_message_(
    const void* buf,
    unsigned long size)
    noexcept
  {
    //std::cerr << "VKProtobufDescriptorHandler::process_input_message_()" << std::endl;

    stats_->processed_messages_count += 1;

    KeywordsProcessor::KeywordArray keywords;
    std::string beeline_uid;
    std::string bind_user_ids;

    {
      ru::madnet::enrichment::protocol::DmpRequest dmp_request;

      {
        Stream::MemoryStream::InputMemoryStream<char> istr(
          static_cast<const char*>(buf),
          size);
        dmp_request.ParseFromIstream(&istr);
      }

      if(!dmp_request.has_options() || dmp_request.options().need_response())
      {
        // fill response
        ru::madnet::enrichment::protocol::DmpResponse dmp_response;
        dmp_response.set_id(dmp_request.id());
        dmp_response.set_code(0);

        std::ostringstream ostr;
        dmp_response.SerializeToOstream(&ostr);

        std::string strbuf = ostr.str();
        uint32_t sz = strbuf.size();

        Buf write_buf(sz + 4);
        //*reinterpret_cast<uint32_t*>(write_buf.data()) = htonl(ostr.str().size());
        *reinterpret_cast<uint32_t*>(write_buf.data()) = sz;
        ::memcpy(write_buf.data() + 4, strbuf.data(), strbuf.size());

        add_write_buf_(write_buf);
      }

      // process
      // find UID Beeline
      for(int i = 0; i < dmp_request.ids_size(); ++i)
      {
        if(dmp_request.ids(i).source() == "beeline")
        {
          beeline_uid = dmp_request.ids(i).value();
        }
        else
        {
          bind_user_ids = dmp_request.ids(i).value();
        }
      }

      if(!beeline_uid.empty() || !bind_user_ids.empty())
      {
        keywords_processor_->process(keywords, dmp_request);
      }
    }

    if(!keywords.empty())
    {
      /*
      {
        Stream::Error ostr;
        ostr << "MESSAGE: beeline_uid = " << beeline_uid << std::endl <<
          ", bind_user_ids = " << bind_user_ids << std::endl <<
          ", keywords =";
        for(auto keyword_it = keywords.begin(); keyword_it != keywords.end(); ++keyword_it)
        {
          ostr << ' ' << (*keyword_it)->str();
        }
        ostr << std::endl
          //<< ", orig_keywords = " << orig_keywords
          ;
        std::cerr << ostr.str() << std::endl;
      }
      */

      const Generics::Time now = Generics::Time::get_time_of_day();

      {
        std::string keywords_str;
        for(auto keyword_it = keywords.begin(); keyword_it != keywords.end(); ++keyword_it)
        {
          if(!keywords_str.empty())
          {
            keywords_str += ' ';
          }
          keywords_str += (*keyword_it)->str();
        }

        dmp_sender_->send(
          now,
          beeline_uid,
          bind_user_ids,
          String::SubString(), // url
          keywords_str);
      }

      {
        UIDGeneratorAdapterLogger::RequestInfo request_info;
        request_info.time = now;
        request_info.keywords.swap(keywords);

        stat_logger_->process_request(request_info);
      }
    }
  }

  // VKProtobufAcceptorDescriptorHandler
  VKProtobufAcceptorDescriptorHandler::VKProtobufAcceptorDescriptorHandler(
    unsigned long port,
    DescriptorHandlerPoller::Proxy* poller_proxy,
    DMPProfilingSender* dmp_sender,
    KeywordsProcessor* keywords_processor,
    UIDGeneratorAdapterLogger* stat_logger,
    UIDGeneratorAdapterImpl::Stats* stats)
    /*throw(Exception)*/
    : AcceptorDescriptorHandler(port, poller_proxy),
      dmp_sender_(ReferenceCounting::add_ref(dmp_sender)),
      keywords_processor_(ReferenceCounting::add_ref(keywords_processor)),
      stat_logger_(ReferenceCounting::add_ref(stat_logger)),
      stats_(ReferenceCounting::add_ref(stats))
  {}

  DescriptorHandler_var
  VKProtobufAcceptorDescriptorHandler::create_descriptor_handler(int fd)
    noexcept
  {
    return new VKProtobufDescriptorHandler(
      fd,
      dmp_sender_,
      keywords_processor_,
      stat_logger_,
      stats_);
  }

  // UIDGeneratorAdapterImpl impl
  UIDGeneratorAdapterImpl::UIDGeneratorAdapterImpl(
    Logging::Logger* logger,
    const UIDGeneratorAdapterImpl::Config& config)
    /*throw(eh::Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      config_(config),
      stats_(new Stats())
  {
    // create poller
    Generics::ActiveObjectCallback_var callback =
      new Logging::ActiveObjectCallbackImpl(
        logger_,
        "AdServer::Frontends::UIDGeneratorAdapterImpl",
        ASPECT,
        "ADS-IMPL-4016");

    scheduler_ = new Generics::Planner(callback);

    add_child_object(scheduler_);

    task_runner_ = new Generics::TaskRunner(callback, 1);

    add_child_object(task_runner_);

    const xsd::AdServer::Configuration::OutLogsType& lp_config =
      config.LogProcessing().OutLogs();

    stat_logger_ = new UIDGeneratorAdapterLogger(
      logger_,
      lp_config.KeywordHitStat().present() ?
        read_flush_policy(
          lp_config.KeywordHitStat().get(),
          (lp_config.log_root() + KEYWORD_HIT_STAT_OUT_DIR).c_str()) :
        LogProcessing::LogFlushTraits());
    
    DescriptorHandlerPoller_var poller = new DescriptorHandlerPoller(
      callback,
      config.Processing().threads(),
      Generics::Time::ONE_SECOND / 4);

    DMPProfilingSender_var dmp_profiling_sender = new DMPProfilingSender(
      config.Processing().send_threads(),
      config.Processing().ZmqSend(),
      config.Processing().source().c_str());

    DescriptorHandler_var acceptor = new VKProtobufAcceptorDescriptorHandler(
      config.Processing().port(),
      poller->proxy(),
      dmp_profiling_sender,
      KeywordsProcessor_var(new KeywordsProcessor()),
      stat_logger_,
      stats_);

    poller->add(acceptor);

    add_child_object(dmp_profiling_sender);
    add_child_object(poller);

    flush_logs_();

    print_stats_();
  }

  void
  UIDGeneratorAdapterImpl::print_stats_() noexcept
  {
    static const char* FUN = "UIDGeneratorAdapterImpl::print_stats_()";

    std::cerr << Generics::Time::get_time_of_day().gm_ft() << ": processed " <<
      stats_->processed_messages_count <<
      " messages" << std::endl;

    try
    {
      Task_var msg = new PrintStatsTask(*task_runner_, this);
      Generics::Time next_time = Generics::Time::get_time_of_day() + Generics::Time::ONE_MINUTE;
      scheduler_->schedule(msg, next_time);
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(
        Logging::Logger::EMERGENCY,
        ASPECT,
        "ADS-IMPL-3018") << FUN <<
        ": Can't schedule next logs flush task. Caught eh::Exception: " <<
        ex.what();
    }
  }

  void
  UIDGeneratorAdapterImpl::flush_logs_() noexcept
  {
    static const char* FUN = "UIDGeneratorAdapterImpl::flush_logs_()";

    const Generics::Time LOGS_DUMP_ERROR_RESCHEDULE_PERIOD(2);

    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->sstream(
        Logging::Logger::TRACE,
        ASPECT) << FUN << ": Flush logs.";
    }

    Generics::Time next_flush;
    
    try
    {
      next_flush = stat_logger_->flush_if_required();
    }
    catch(const eh::Exception& ex)
    {
      next_flush = Generics::Time::get_time_of_day() +
        LOGS_DUMP_ERROR_RESCHEDULE_PERIOD;

      logger_->sstream(
        Logging::Logger::EMERGENCY,
        ASPECT,
        "ADS-IMPL-3017") << FUN <<
        ": Can't flush logs. Caught eh::Exception: " << ex.what();
    }

    if(next_flush != Generics::Time::ZERO)
    {
      try
      {
        Task_var msg = new FlushLogsTask(*task_runner_, this);
        scheduler_->schedule(msg, next_flush);
      }
      catch(const eh::Exception& ex)
      {
        logger_->sstream(
          Logging::Logger::EMERGENCY,
          ASPECT,
          "ADS-IMPL-3018") << FUN <<
          ": Can't schedule next logs flush task. Caught eh::Exception: " <<
          ex.what();
      }
    }

    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->sstream(
        Logging::Logger::TRACE,
        ASPECT) << FUN << ": Next flush at " << next_flush.gm_ft() << ".";
    }
  }
}
}
