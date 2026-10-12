#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <Logger/StreamLogger.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <ChannelSvcs/ChannelServer/ChannelUpdateGrpcClient.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelLoadSessionImpl.hpp>

namespace
{
  namespace CS = AdServer::ChannelSvcs;
  namespace Proto = adserver::channel_svcs::channel_server;

  void require(bool value, const char* message)
  {
    if (!value)
    {
      throw std::runtime_error(message);
    }
  }

  class Fixture final : public Proto::ChannelServerGrpc::Service
  {
  public:
    explicit Fixture(unsigned int shard);

    ~Fixture();

    grpc::Status check(
      grpc::ServerContext*, const Proto::CheckRequest* request, Proto::CheckResponse* response)
      override;

    grpc::Status update_triggers(
      grpc::ServerContext*, const Proto::UpdateTriggersRequest* request,
      Proto::UpdateTriggersResponse* response) override;

    grpc::Status update_all_ccg(
      grpc::ServerContext*, const Proto::UpdateAllCcgRequest* request,
      Proto::UpdateAllCcgResponse* response) override;

    grpc::Status get_count_chunks(
      grpc::ServerContext*, const Proto::GetCountChunksRequest*,
      Proto::GetCountChunksResponse* response) override;

    std::string endpoint;
    std::string payload = std::string("a\0b\xff", 4);
    std::atomic<bool> fail{false};
    std::atomic<bool> bad_stamp{false};
    std::atomic<bool> bad_decimal{false};
    std::atomic<int> delay_ms{0};
    std::atomic<unsigned int> checks{0};

  private:
    unsigned int shard_;
    std::unique_ptr<grpc::Server> server_;
  };

  Fixture::Fixture(unsigned int shard) : shard_(shard)
  {
    grpc::ServerBuilder builder;
    builder.SetMaxReceiveMessageSize(-1);
    builder.SetMaxSendMessageSize(-1);
    int port = 0;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
    builder.RegisterService(this);
    server_ = builder.BuildAndStart();
    require(server_ && port, "cannot start gRPC fixture");
    endpoint = "127.0.0.1:" + std::to_string(port);
  }

  Fixture::~Fixture()
  {
    server_->Shutdown();
    server_->Wait();
  }

  grpc::Status Fixture::check(
    grpc::ServerContext*, const Proto::CheckRequest* request, Proto::CheckResponse* response)
  {
    ++checks;
    if (fail)
    {
      return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "not configured");
    }

    if (delay_ms)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms.load()));
    }

    require(request->version() == "test", "version was not transported");
    require(request->colo_id() == 1, "colo id was not transported");
    require(request->master_stamp() == GrpcAlgs::pack_time(Generics::Time::ZERO),
      "request timestamp changed");
    response->set_first_stamp(bad_stamp ? "bad" :
      GrpcAlgs::pack_time(Generics::Time(10 + shard_)));
    response->set_master_stamp(GrpcAlgs::pack_time(Generics::Time(20 + shard_)));
    response->set_max_time(GrpcAlgs::pack_time(Generics::Time(30 + shard_)));
    response->set_source_id(7);
    response->set_special_adv(shard_ == 0);
    response->set_special_track(shard_ == 1);
    auto* version = response->add_versions();
    version->set_id(2 + shard_);
    version->set_size(100 + shard_);
    version->set_stamp(GrpcAlgs::pack_time(Generics::Time(40 + shard_)));
    return grpc::Status::OK;
  }

  grpc::Status Fixture::update_triggers(
    grpc::ServerContext*, const Proto::UpdateTriggersRequest* request,
    Proto::UpdateTriggersResponse* response)
  {
    response->set_source_id(7);
    for (const auto id : request->ids())
    {
      if (id % 2 != shard_)
      {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "wrong shard");
      }

      auto* channel = response->add_channels();
      channel->set_channel_id(id);
      channel->set_stamp(GrpcAlgs::pack_time(Generics::Time(50)));
      auto* word = channel->add_words();
      word->set_channel_trigger_id(id * 10);
      word->set_trigger(payload);
    }

    return grpc::Status::OK;
  }

  grpc::Status Fixture::update_all_ccg(
    grpc::ServerContext*, const Proto::UpdateAllCcgRequest* request,
    Proto::UpdateAllCcgResponse* response)
  {
    response->set_source_id(7);
    response->set_start_id(10);
    for (unsigned int id = 2 + shard_; id < 6; id += 2)
    {
      if (id < request->start())
      {
        continue;
      }

      auto* keyword = response->add_keywords();
      keyword->set_ccg_keyword_id(id * 10);
      keyword->set_ccg_id(id * 100);
      keyword->set_channel_id(id);
      keyword->set_max_cpc(
        bad_decimal ? "bad" :
          GrpcAlgs::pack_decimal(AdServer::CampaignSvcs::RevenueDecimal("1.25")));
      keyword->set_ctr(GrpcAlgs::pack_decimal(AdServer::CampaignSvcs::CTRDecimal("0.125")));
      keyword->set_click_url("http://example.test/");
      keyword->set_original_keyword("test");
      if (response->keywords_size() == static_cast<int>(request->limit()))
      {
        response->set_start_id(id + 1);
        break;
      }
    }

    auto* deleted = response->add_deleted();
    deleted->set_id(100 + shard_);
    deleted->set_stamp(GrpcAlgs::pack_time(Generics::Time(60)));
    return grpc::Status::OK;
  }

  grpc::Status Fixture::get_count_chunks(
    grpc::ServerContext*, const Proto::GetCountChunksRequest*,
    Proto::GetCountChunksResponse* response)
  {
    response->set_count_chunks(2);
    return grpc::Status::OK;
  }

  CS::ChannelLoadDescription source(Fixture& fixture, unsigned int shard)
  {
    return {std::make_shared<CS::ChannelUpdateGrpcClient>(fixture.endpoint), {shard}, 2};
  }

  void test()
  {
    Fixture first(0);
    Fixture second(1);
    Logging::Logger_var logger = new Logging::OStream::Logger(
      Logging::OStream::Config(std::cerr));
    CS::ChannelLoadSessionImpl session({{source(first, 0), source(second, 1)}}, 7, logger);
    CS::ChannelUpdate::CheckQuery query;
    query.colo_id = 1;
    query.version = "test";
    query.master_stamp = Generics::Time::ZERO;
    query.use_only_list = false;
    CS::ChannelUpdate::CheckDataPtr check;
    session.check(query, check);
    require(check->versions.size() == 2, "check did not merge both shards");
    require(check->special_adv && check->special_track, "special flags were not merged");
    require(check->master_stamp == Generics::Time(20), "master min");
    require(check->first_stamp == Generics::Time(11), "first max");
    require(check->max_time == Generics::Time(31), "max time");

    CS::ChannelIdList ids;
    ids.resize(4);
    for (unsigned int i = 0; i < 4; ++i)
    {
      ids[i] = i + 2;
    }

    CS::ChannelUpdate::UpdateDataPtr triggers;
    session.update_triggers(ids, triggers);
    require(triggers->channels.size() == 4, "trigger routing or merge failed");
    for (const auto i : {0, 1, 2, 3})
    {
      const auto& word = triggers->channels[i].words[0];
      require(word.trigger.size() == 4, "binary trigger size changed");
      require(std::string(reinterpret_cast<const char*>(word.trigger.data()), 4) ==
        first.payload, "binary trigger bytes changed");
    }

    CS::ChannelUpdate::CCGQuery ccg;
    ccg.master_stamp = Generics::Time::ZERO;
    ccg.start = 0;
    ccg.limit = 2;
    ccg.use_only_list = false;
    CS::ChannelUpdate::PosCCGResultPtr keywords;
    session.update_all_ccg(ccg, keywords);
    require(keywords->start_id == 5, "CCG pagination boundary changed");
    require(keywords->keywords.size() == 3, "CCG page merge failed");
    require(keywords->deleted.size() == 2, "deleted CCGs were lost");
    require(keywords->keywords[0].max_cpc == AdServer::CampaignSvcs::RevenueDecimal("1.25"),
      "decimal precision changed");
    ccg.start = keywords->start_id;
    session.update_all_ccg(ccg, keywords);
    require(keywords->keywords.size() == 1 && keywords->keywords[0].channel_id == 5,
      "CCG continuation skipped or duplicated a channel");

    first.fail = true;
    CS::ChannelLoadSessionImpl failover({{source(first, 0)}, {source(second, 1)}}, 7, logger);
    bool failed = false;
    try
    {
      failover.check(query, check);
    }
    catch (const CS::ImplementationException&)
    {
      failed = true;
    }

    require(failed, "unconfigured source did not fail");
    failover.check(query, check);
    require(check->versions[0].id == 3, "failed group was not rotated");
    first.fail = false;

    CS::ChannelUpdateGrpcClient direct(first.endpoint);
    require(direct.get_count_chunks() == 2, "chunk count changed");
    direct.update_triggers({}, triggers);
    require(triggers->channels.empty(), "empty trigger request returned channels");
    session.update_triggers({}, triggers);
    require(triggers->channels.empty(), "empty sharded request returned channels");

    first.fail = true;
    failed = false;
    try
    {
      direct.check(query, check);
    }
    catch (const CS::NotConfigured&)
    {
      failed = true;
    }

    require(failed, "direct unconfigured response lost its error type");
    first.fail = false;

    first.bad_stamp = true;
    failed = false;
    try
    {
      direct.check(query, check);
    }
    catch (const CS::ImplementationException&)
    {
      failed = true;
    }

    require(failed, "malformed timestamp accepted");
    first.bad_stamp = false;

    first.bad_decimal = true;
    ccg.start = 0;
    failed = false;
    try
    {
      direct.update_all_ccg(ccg, keywords);
    }
    catch (const CS::ImplementationException&)
    {
      failed = true;
    }

    require(failed, "malformed decimal accepted");
    first.bad_decimal = false;
    ids.resize(1);
    ids[0] = 2;
    first.payload.assign(5 * 1024 * 1024, '\0');
    direct.update_triggers(ids, triggers);
    require(triggers->channels[0].words[0].trigger.size() == first.payload.size(),
      "large update exceeded gRPC receive limit");

    first.delay_ms = 100;
    CS::ChannelUpdateGrpcClient short_timeout(first.endpoint, std::chrono::milliseconds(10));
    failed = false;
    try
    {
      short_timeout.check(query, check);
    }
    catch (const CS::ImplementationException&)
    {
      failed = true;
    }

    require(failed, "RPC deadline was not enforced");

    failed = false;
    try
    {
      CS::ChannelUpdateGrpcClient legacy("corbaloc:iiop:127.0.0.1:1/ChannelUpdate");
    }
    catch (const CS::ImplementationException&)
    {
      failed = true;
    }

    require(failed, "legacy CORBA reference was accepted as gRPC endpoint");
  }
}

int main()
{
  try
  {
    test();
    std::cout << "ChannelUpdateGrpcTest passed\n";
  }
  catch (const std::exception& ex)
  {
    std::cerr << ex.what() << '\n';
    return 1;
  }
}
