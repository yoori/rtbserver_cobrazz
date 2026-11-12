#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

#include <Logger/Logger.hpp>
#include <CampaignSvcs/CampaignManager/CampaignManagerLogger.hpp>

namespace
{
  using Logger = AdServer::CampaignSvcs::CampaignManagerLogger;
  using Core = AdServer::CampaignSvcs::CampaignManagerCore;

  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  class Gate
  {
  public:
    void block();
    void wait_entered();
    void release();

  private:
    std::mutex mutex_;
    std::condition_variable condition_;
    bool entered_ = false;
    bool released_ = false;
  };

  void Gate::block()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    entered_ = true;
    condition_.notify_all();
    condition_.wait(lock, [this]() { return released_; });
  }

  void Gate::wait_entered()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    require(condition_.wait_for(lock, std::chrono::seconds(10), [this]() { return entered_; }),
      "worker did not reach the gate");
  }

  void Gate::release()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    released_ = true;
    condition_.notify_all();
  }

  struct Session
  {
    Session();
    ~Session();

    Gate first;
    Gate second;
    Gate shutdown;
    std::atomic<unsigned long> destroyed{0};
    std::filesystem::path root;
    Logging::Logger_var diagnostic_logger;
    AdServer::CampaignSvcs::CampaignManagerLogger_var logger;
  };

  Session::Session()
    : diagnostic_logger(new Logging::Null::Logger)
  {
    char path[] = "/tmp/cm-variant-queue-XXXXXX";
    require(mkdtemp(path) != nullptr, "cannot create test directory");
    root = path;
    Logger::Params params;
    AdServer::LogProcessing::LogFlushTraits* traits[] = {
      &params.request_basic_channels, &params.opt_out_stat, &params.creative_stat,
      &params.action_request, &params.request, &params.impression, &params.click,
      &params.advertiser_action, &params.passback_impression, &params.user_properties,
      &params.tag_request, &params.tag_position_stat, &params.ccg_stat, &params.cc_stat,
      &params.search_term_stat, &params.search_engine_stat, &params.tag_auction_stat,
      &params.passback_stat, &params.user_agent_stat, &params.prof_research,
      &params.web_stat, &params.research_web_stat};
    unsigned long index = 0;
    for (auto* trait : traits)
    {
      trait->period = Generics::Time::ZERO;
      trait->out_dir = (root / std::to_string(index++)).string();
      std::filesystem::create_directories(trait->out_dir);
    }

    params.request_basic_channels.distrib_count = 1;
    params.request_basic_channels.inventory_sampling = 100;
    params.profiling_log_sampling = 0;
    params.threads = 1;
    logger = new Logger(params, diagnostic_logger);
  }

  Session::~Session()
  {
    first.release();
    second.release();
    shutdown.release();
    if (logger)
    {
      logger->deactivate_object();
      logger->wait_object();
    }

    std::error_code error;
    std::filesystem::remove_all(root, error);
  }

  std::shared_ptr<Logger::RequestInfo> make_request(Session& session, Gate* gate = nullptr)
  {
    return std::shared_ptr<Logger::RequestInfo>(new Logger::RequestInfo(),
      [&session, gate](Logger::RequestInfo* request)
      {
        if (gate)
        {
          gate->block();
        }

        delete request;
        session.destroyed.fetch_add(1);
      });
  }

  void check_closed_queue(Session& session)
  {
    auto request = make_request(session);
    std::weak_ptr<Logger::RequestInfo> weak = request;
    bool rejected = false;
    try
    {
      session.logger->process_request(std::move(request), 0);
    }
    catch (const eh::Exception& error)
    {
      require(std::string(error.what()).find("queue is closed") != std::string::npos,
        "unexpected enqueue error");
      rejected = true;
    }

    require(rejected, "closed queue accepted a task");
    require(weak.expired(), "rejected task retained its payload");
  }

  void test_task_lifetime_and_shutdown()
  {
    Session session;
    check_closed_queue(session);
    session.logger->activate_object();
    session.logger->process_request(make_request(session, &session.first), 0);
    session.first.wait_entered();

    // Hold the worker so these two tasks are fetched in the same batch.
    session.logger->process_request(make_request(session, &session.second), 0);
    auto last = make_request(session);
    std::weak_ptr<Logger::RequestInfo> weak_last = last;
    session.logger->process_request(std::move(last), 0);
    session.first.release();
    session.second.wait_entered();
    const auto blocked_stats = session.logger->get_stats();
    require(blocked_stats.processing_requests == 2, "task payload survived batch processing");
    require(!weak_last.expired(), "following task ran before the first task was destroyed");
    session.second.release();
    session.logger->wait_processing();
    require(weak_last.expired(), "wait_processing returned before payload destruction");
    require(session.destroyed.load() == 4, "request task was lost or destroyed twice");

    auto request = std::make_shared<Core::GetAdRequest>();
    request->common_info = std::make_shared<Core::CommonAdRequest>(request->arena());
    request->context_info = std::make_shared<Core::ContextAdRequest>(request->arena());
    std::weak_ptr<Core::GetAdRequest> weak_request = request;
    session.logger->process_request(nullptr, request, Core::AdSlotContext(), 0, nullptr, false);
    session.logger->process_ad_request(nullptr, std::move(request), {}, nullptr, false);
    session.logger->wait_processing();
    require(weak_request.expired(), "snapshot task retained the request arena");

    session.logger->process_request(make_request(session, &session.shutdown), 0);
    session.shutdown.wait_entered();
    for (unsigned long i = 0; i < 256; ++i)
    {
      session.logger->process_request(make_request(session), 0);
    }

    session.logger->deactivate_object();
    session.shutdown.release();
    session.logger->wait_object();
    check_closed_queue(session);
    session.logger->wait_processing();
    const auto stats = session.logger->get_stats();
    require(stats.queue_total == 262, "unexpected accepted task count");
    require(stats.processing_requests_total == stats.queue_total, "shutdown lost tasks");
    require(stats.request_in_progress == 0, "shutdown left tasks in progress");
    require(session.destroyed.load() == 262, "shutdown retained task payloads");
  }
}

int main()
{
  try
  {
    test_task_lifetime_and_shutdown();
    std::cout << "CampaignManagerLoggerQueueTest: PASS\n";
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
