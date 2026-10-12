#include <atomic>
#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <Logger/StreamLogger.hpp>
#include <Commons/Grpc/DbAccess.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/ScopeGuard.hpp>
#include <CampaignSvcs/CampaignServer/StatLogGeneralizerSource.hpp>
#include <LogProcessing/LogGeneralizer/LogGeneralizerGrpc.hpp>

namespace
{
  using namespace AdServer::LogProcessing;
  using namespace std::chrono_literals;

  void require(bool value, const char* message)
  {
    if (!value)
    {
      throw std::runtime_error(message);
    }
  }

  template<class Exception, class Action>
  void expect(Action action)
  {
    try
    {
      action();
    }
    catch (const Exception&)
    {
      return;
    }

    throw std::runtime_error("Expected exception");
  }

  StatInfo sample()
  {
    StatInfo result;
    CampaignStatInfo stat;
    stat.sdate = Generics::Time(1700000000, 123456);
    stat.adv_sdate = stat.sdate + Generics::Time(3600);
    stat.adv_account_id = 4000000000U;
    stat.campaign_id = 12;
    stat.ccg_id = 13;
    stat.adv_account_amount = StatDecimal("-1234567890.12345678");
    stat.adv_amount = StatDecimal("9999999999.99999999");
    stat.adv_comm_amount = StatDecimal("-0.00000001");
    stat.adv_payable_comm_amount = StatDecimal("2.5");
    stat.creative_stats.push_back({14, 5000000000LL, -11, 12, 13});
    stat.publisher_amounts.push_back({15, StatDecimal("-123.45678901")});
    stat.tag_amounts.push_back(
      {16, StatDecimal("1.2"), StatDecimal("-3.4"), StatDecimal("5.6")});
    stat.ctr_reset_stats.push_back({17, 6000000000ULL});
    result.campaign_stats.push_back(stat);
    return result;
  }

  void same(const StatInfo& actual, const StatInfo& expected)
  {
    require(actual.campaign_stats.size() == expected.campaign_stats.size(), "campaign count");
    for (std::size_t i = 0; i < actual.campaign_stats.size(); ++i)
    {
      const auto& a = actual.campaign_stats[i];
      const auto& b = expected.campaign_stats[i];
      require(a.sdate == b.sdate && a.adv_sdate == b.adv_sdate, "timestamps");
      require(a.adv_account_id == b.adv_account_id && a.campaign_id == b.campaign_id &&
        a.ccg_id == b.ccg_id, "identifiers");
      require(a.adv_account_amount == b.adv_account_amount && a.adv_amount == b.adv_amount &&
        a.adv_comm_amount == b.adv_comm_amount &&
        a.adv_payable_comm_amount == b.adv_payable_comm_amount, "campaign amounts");
      require(a.creative_stats.size() == 1 && a.publisher_amounts.size() == 1 &&
        a.tag_amounts.size() == 1 && a.ctr_reset_stats.size() == 1, "nested counts");
      const auto& ac = a.creative_stats.front();
      const auto& bc = b.creative_stats.front();
      require(ac.cc_id == bc.cc_id && ac.requests == bc.requests &&
        ac.impressions == bc.impressions && ac.clicks == bc.clicks && ac.actions == bc.actions,
        "creative stats");
      require(a.publisher_amounts[0].publisher_account_id ==
        b.publisher_amounts[0].publisher_account_id &&
        a.publisher_amounts[0].adv_amount == b.publisher_amounts[0].adv_amount, "publishers");
      require(a.tag_amounts[0].tag_id == b.tag_amounts[0].tag_id &&
        a.tag_amounts[0].pub_isp_amount == b.tag_amounts[0].pub_isp_amount &&
        a.tag_amounts[0].adv_amount == b.tag_amounts[0].adv_amount &&
        a.tag_amounts[0].adv_comm_amount == b.tag_amounts[0].adv_comm_amount, "tags");
      require(a.ctr_reset_stats[0].ctr_reset_id == b.ctr_reset_stats[0].ctr_reset_id &&
        a.ctr_reset_stats[0].impressions == b.ctr_reset_stats[0].impressions, "CTR reset stats");
    }
  }

  class MalformedService final : public lg_rpc::LogGeneralizer::Service
  {
  public:
    grpc::Status get_stat_info(
      grpc::ServerContext*, const lg_rpc::StatRequest*, lg_rpc::StatInfo*) override;
  };

  grpc::Status MalformedService::get_stat_info(
    grpc::ServerContext*, const lg_rpc::StatRequest* request, lg_rpc::StatInfo* response)
  {
    auto& value = *response->add_campaign_stats();
    if (request->client_id() == 1)
    {
      value.set_sdate(GrpcAlgs::pack_time(Generics::Time::ZERO));
      value.set_adv_sdate(value.sdate());
      value.set_adv_account_amount("invalid");
    }

    return grpc::Status::OK;
  }

  void malformed()
  {
    MalformedService service;
    grpc::ServerBuilder builder;
    int port = 0;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
    builder.RegisterService(&service);
    const auto server = builder.BuildAndStart();
    require(server != nullptr, "malformed test server");
    auto shutdown = AdServer::Commons::make_scope_guard([&]
    {
      server->Shutdown();
      server->Wait();
    });
    LogGeneralizerClient client("127.0.0.1:" + std::to_string(port));
    expect<LogGeneralizerError>([&] { client.get_stat_info(0, false); });
    expect<LogGeneralizerError>([&] { client.get_stat_info(1, false); });
  }

  void test()
  {
    std::map<std::uint32_t, StatInfo> collections;
    std::mutex mutex;
    std::atomic<unsigned int> calls{0};
    LogGeneralizerGrpcService service(
      [&](auto id)
      {
        if (id == 0)
        {
          throw NotSupported("not supported");
        }

        std::lock_guard lock(mutex);
        collections.try_emplace(id, sample());
      },
      [&](auto id, bool clear)
      {
        std::lock_guard lock(mutex);
        const auto it = collections.find(id);
        if (it == collections.end())
        {
          throw CollectionNotStarted("collection is not started");
        }

        if (clear)
        {
          it->second.campaign_stats.clear();
        }
      },
      [&](auto id, bool clear)
      {
        ++calls;
        if (id == 99)
        {
          throw std::runtime_error("implementation failure");
        }

        if (id == 98)
        {
          std::this_thread::sleep_for(150ms);
        }

        std::lock_guard lock(mutex);
        const auto it = collections.find(id);
        if (it == collections.end())
        {
          throw CollectionNotStarted("collection is not started");
        }

        auto result = it->second;
        if (clear)
        {
          it->second.campaign_stats.clear();
        }

        return result;
      });

    AdServer::Grpc::DbAccessService db_access(
      [] { return std::optional<bool>{true}; }, [](bool) {},
      [](auto& response) { response.set_ready(true); });
    grpc::ServerBuilder builder;
    int port = 0;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
    builder.SetMaxSendMessageSize(-1);
    builder.RegisterService(&service);
    builder.RegisterService(&db_access);
    const auto server = builder.BuildAndStart();
    require(server != nullptr, "server startup");
    auto shutdown = AdServer::Commons::make_scope_guard([&]
    {
      server->Shutdown();
      server->Wait();
    });
    const auto endpoint = "127.0.0.1:" + std::to_string(port);
    LogGeneralizerClient client(endpoint);
    expect<NotSupported>([&] { client.stop_stat_upload(0); });
    expect<CollectionNotStarted>([&] { client.start_stat_upload(1, false); });
    expect<CollectionNotStarted>([&] { client.get_stat_info(1, false); });
    client.stop_stat_upload(1);
    client.stop_stat_upload(2);
    same(client.get_stat_info(1, false), sample());
    same(client.get_stat_info(1, false), sample());
    same(client.get_stat_info(1, true), sample());
    require(client.get_stat_info(1, false).campaign_stats.empty(), "clear on get");
    same(client.get_stat_info(2, false), sample());
    client.start_stat_upload(2, false);
    same(client.get_stat_info(2, false), sample());
    client.start_stat_upload(2, true);
    require(client.get_stat_info(2, false).campaign_stats.empty(), "clear on start");
    const auto before = calls.load();
    expect<LogGeneralizerError>([&] { client.get_stat_info(99, true); });
    require(calls.load() == before + 1, "unexpected retry");
    client.stop_stat_upload(98);
    LogGeneralizerClient short_client(endpoint, 10ms);
    expect<LogGeneralizerError>([&] { short_client.get_stat_info(98, false); });

    {
      std::lock_guard lock(mutex);
      collections[3].campaign_stats.assign(30000, sample().campaign_stats.front());
    }
    const auto large = client.get_stat_info(3, true);
    require(large.campaign_stats.size() == 30000, "large response truncated");
    same(StatInfo{{large.campaign_stats.back()}}, sample());

    Logging::Logger_var logger(new Logging::OStream::Logger(Logging::OStream::Config(std::cerr)));
    using AdServer::CampaignSvcs::StatLogGeneralizerSource;
    ReferenceCounting::SmartPtr<StatLogGeneralizerSource> source(
      new StatLogGeneralizerSource(logger, 4, {endpoint}));
    bool sync_required = false;
    source->update(nullptr, sync_required, Generics::Time(1700000000));
    require(sync_required, "CampaignServer lost CollectionNotStarted");
    client.stop_stat_upload(4);
    {
      std::lock_guard lock(mutex);
      collections[4].campaign_stats[0].creative_stats[0].impressions = 11;
    }
    const auto stats = source->update(nullptr, sync_required, Generics::Time(1700000000));
    require(!sync_required && stats->campaign_stats.size() == 1, "CampaignServer conversion");
    require(stats->campaign_stats.at(12).amount == sample().campaign_stats[0].adv_amount,
      "CampaignServer amount precision");
    require(stats->account_amounts.at(4000000000U).amount ==
      sample().campaign_stats[0].adv_account_amount, "CampaignServer account amount");
    require(client.get_stat_info(4, false).campaign_stats.empty(), "CampaignServer must clear");
    server->Shutdown();
    server->Wait();
    shutdown.release();
    expect<LogGeneralizerError>([&] { short_client.get_stat_info(1, false); });
    expect<LogGeneralizerError>([] { LogGeneralizerClient invalid("corbaloc:iiop:x:1/LG"); });
    expect<LogGeneralizerError>([&] { LogGeneralizerClient invalid(endpoint, 0ms); });
  }
}

int main()
{
  try
  {
    test();
    malformed();
    std::cout << "PASS: statistics, precision, clear, isolation, errors, deadline, large response, "
      "CampaignServer conversion\n";
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
