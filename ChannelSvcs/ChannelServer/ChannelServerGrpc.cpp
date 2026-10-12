#include "ChannelServerGrpc.hpp"
#include "ChannelServerCore.hpp"

#include <grpcpp/grpcpp.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <stdexcept>

#include <Commons/ExecutorPool.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/Hostname.hpp>
#include <Commons/Grpc/GrpcServer.hpp>
#include <Commons/Grpc/DbAccess.hpp>
#include <Commons/Grpc/ProcessControl.grpc.pb.h>
#include <Generics/Time.hpp>
#include <Logger/ActiveObjectCallback.hpp>

#include <ChannelSvcs/ChannelServer/ChannelServerGrpc.grpc.pb.h>

namespace AdServer::ChannelSvcs
{
  namespace
  {
    constexpr const char channel_server_grpc_aspect[] = "ChannelServerGrpc";
    namespace pc = adserver::grpc::process_control;

    std::size_t
    resolve_max_sequential_ops(std::size_t configured)
    {
      return std::max<std::size_t>(1, configured != 0 ? configured : 4);
    }

    class InProgressGuard final
    {
    public:
      InProgressGuard(
        std::atomic<std::uint64_t>& call_counter,
        std::atomic<std::uint64_t>& method_counter) noexcept
        : call_counter_(call_counter),
          method_counter_(method_counter)
      {
        call_counter_.fetch_add(1, std::memory_order_relaxed);
        method_counter_.fetch_add(1, std::memory_order_relaxed);
      }

      ~InProgressGuard()
      {
        method_counter_.fetch_sub(1, std::memory_order_relaxed);
        call_counter_.fetch_sub(1, std::memory_order_relaxed);
      }

      InProgressGuard(const InProgressGuard&) = delete;
      InProgressGuard& operator=(const InProgressGuard&) = delete;

    private:
      std::atomic<std::uint64_t>& call_counter_;
      std::atomic<std::uint64_t>& method_counter_;
    };

    class BatchStatsGuard final
    {
    public:
      BatchStatsGuard(
        std::atomic<std::uint64_t>& total,
        std::atomic<std::uint64_t>& total_time,
        std::atomic<std::uint64_t>& in_progress) noexcept
        : total_(total),
          total_time_(total_time),
          in_progress_(in_progress)
      {
        total_.fetch_add(1, std::memory_order_relaxed);
        in_progress_.fetch_add(1, std::memory_order_relaxed);
      }

      ~BatchStatsGuard() noexcept
      {
        const auto elapsed_us = (Generics::Time::get_time_of_day() - start_time_).microseconds();
        total_time_.fetch_add(elapsed_us, std::memory_order_relaxed);
        in_progress_.fetch_sub(1, std::memory_order_relaxed);
      }

      BatchStatsGuard(const BatchStatsGuard&) = delete;
      BatchStatsGuard& operator=(const BatchStatsGuard&) = delete;

    private:
      std::atomic<std::uint64_t>& total_;
      std::atomic<std::uint64_t>& total_time_;
      std::atomic<std::uint64_t>& in_progress_;
      const Generics::Time start_time_ = Generics::Time::get_time_of_day();
    };

    void pack_channel_atoms(
      const std::vector<ChannelServerCore::ChannelAtom>& source,
      google::protobuf::RepeatedPtrField<
        adserver::channel_svcs::channel_server::ChannelAtom>* target)
    {
      target->Reserve(source.size());
      for (const auto& source_atom : source)
      {
        auto* atom = target->Add();
        atom->set_id(source_atom.id);
        atom->set_trigger_channel_id(source_atom.trigger_channel_id);
      }
    }

    void pack_match_result(
      const ChannelServerCore::MatchResult& source,
      adserver::channel_svcs::channel_server::MatchResponse& target)
    {
      auto* matched_channels = target.mutable_matched_channels();
      pack_channel_atoms(
        source.matched_channels.page_channels,
        matched_channels->mutable_page_channels());
      pack_channel_atoms(
        source.matched_channels.search_channels,
        matched_channels->mutable_search_channels());
      pack_channel_atoms(
        source.matched_channels.url_channels,
        matched_channels->mutable_url_channels());
      pack_channel_atoms(
        source.matched_channels.url_keyword_channels,
        matched_channels->mutable_url_keyword_channels());

      matched_channels->mutable_uid_channels()->Reserve(
        source.matched_channels.uid_channels.size());
      matched_channels->mutable_uid_channels()->Add(
        source.matched_channels.uid_channels.begin(),
        source.matched_channels.uid_channels.end());

      target.mutable_content_channels()->Reserve(source.content_channels.size());
      for (const auto& source_channel : source.content_channels)
      {
        auto* content_channel = target.add_content_channels();
        content_channel->set_id(source_channel.id);
        content_channel->set_weight(source_channel.weight);
      }

      target.set_no_adv(source.no_adv);
      target.set_no_track(source.no_track);
      target.set_full_loaded(source.full_loaded);
      target.set_match_time(::GrpcAlgs::pack_time(source.match_time));
    }

    void pack_traits_result(
      const ChannelServerCore::TraitsResult& source,
      adserver::channel_svcs::channel_server::GetCcgTraitsResponse& target)
    {
      target.mutable_ccg_keywords()->Reserve(source.ccg_keywords.size());
      for (const auto& source_keyword : source.ccg_keywords)
      {
        auto* keyword = target.add_ccg_keywords();
        keyword->set_ccg_keyword_id(source_keyword.ccg_keyword_id);
        keyword->set_ccg_id(source_keyword.ccg_id);
        keyword->set_channel_id(source_keyword.channel_id);
        keyword->set_max_cpc(
          ::GrpcAlgs::pack_decimal<CampaignSvcs::RevenueDecimal>(source_keyword.max_cpc));
        keyword->set_ctr(::GrpcAlgs::pack_decimal<CampaignSvcs::CTRDecimal>(source_keyword.ctr));
        keyword->set_click_url(source_keyword.click_url);
        keyword->set_original_keyword(source_keyword.original_keyword);
      }

      target.mutable_neg_ccg()->Reserve(source.neg_ccg.size());
      target.mutable_neg_ccg()->Add(source.neg_ccg.begin(), source.neg_ccg.end());
    }

    std::vector<unsigned long>
    unpack_sources(const google::protobuf::RepeatedField<::google::protobuf::uint64>& source)
    {
      std::vector<unsigned long> result;
      result.reserve(source.size());
      for (const auto item : source)
      {
        result.emplace_back(item);
      }
      return result;
    }

    Generics::Time unpack_update_stamp(const std::string& value)
    {
      if (value.empty())
      {
        return Generics::Time::ZERO;
      }

      if (value.size() != Generics::Time::TIME_PACK_LEN)
      {
        throw std::invalid_argument("invalid ChannelUpdate timestamp");
      }

      return GrpcAlgs::unpack_time(value);
    }

    void unpack_local_groups(
      const google::protobuf::RepeatedPtrField<
        adserver::channel_svcs::channel_server::LocalLoadGroup>& groups,
      GroupLoadDescription& result)
    {
      for (const auto& group : groups)
      {
        if (!group.servers_size())
        {
          throw ChannelServerCore::Exception("empty ChannelUpdate source group");
        }

        auto& target_group = result.emplace_back();
        for (const auto& server : group.servers())
        {
          if (server.endpoint().empty() || !server.count_chunks() || !server.sources_size())
          {
            throw ChannelServerCore::Exception("invalid ChannelUpdate source descriptor");
          }

          ChannelLoadDescription target;
          target.load_server = std::make_shared<ChannelUpdateGrpcClient>(server.endpoint());
          target.count_chunks = server.count_chunks();
          for (const auto chunk : server.sources())
          {
            if (chunk >= target.count_chunks)
            {
              throw ChannelServerCore::Exception("ChannelUpdate source chunk is out of range");
            }

            target.chunks.push_back(chunk);
          }

          target_group.push_back(std::move(target));
        }
      }
    }
  }

  struct ChannelServerGrpc::AtomicStats
  {
    std::atomic<std::uint64_t> call_in_progress{0};
    std::atomic<std::uint64_t> match_in_progress{0};
    std::atomic<std::uint64_t> get_ccg_traits_in_progress{0};
    std::atomic<std::uint64_t> check_configuration_in_progress{0};
    std::atomic<std::uint64_t> set_sources_in_progress{0};
    std::atomic<std::uint64_t> set_proxy_sources_in_progress{0};
    std::atomic<std::uint64_t> channel_update_in_progress{0};
    std::atomic<std::uint64_t> batch_total{0};
    std::atomic<std::uint64_t> batch_total_time{0};
    std::atomic<std::uint64_t> batch_in_progress{0};
  };

  class ChannelServerGrpc::ServiceImpl final:
    public AdServer::Grpc::GrpcAsyncServiceBase<
      ChannelServerGrpc::ServiceImpl,
      adserver::channel_svcs::channel_server::ChannelServerGrpc,
      adserver::channel_svcs::channel_server::ChannelServerGrpc::AsyncService>
  {
    using AsyncService = adserver::channel_svcs::channel_server::ChannelServerGrpc::AsyncService;

  public:
    ServiceImpl(
      ChannelServerCorePtr core,
      std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
      std::shared_ptr<AtomicStats> stats,
      std::size_t max_sequential_ops);

    static auto grpc_calls()
    {
      return std::make_tuple(
        MAKE_DISTRIBUTED_GRPC_CORO_CALL(
          adserver::channel_svcs::channel_server::MatchRequest,
          adserver::channel_svcs::channel_server::MatchResponse,
          match,
          co_match,
          true),
        MAKE_DISTRIBUTED_GRPC_CORO_CALL(
          adserver::channel_svcs::channel_server::GetCcgTraitsRequest,
          adserver::channel_svcs::channel_server::GetCcgTraitsResponse,
          get_ccg_traits,
          co_get_ccg_traits,
          true),
        MAKE_DISTRIBUTED_GRPC_CORO_CALL(
          adserver::channel_svcs::channel_server::CheckConfigurationRequest,
          adserver::channel_svcs::channel_server::CheckConfigurationResponse,
          check_configuration,
          co_check_configuration,
          true),
        MAKE_DISTRIBUTED_GRPC_CORO_CALL(
          adserver::channel_svcs::channel_server::SetSourcesRequest,
          adserver::channel_svcs::channel_server::SetSourcesResponse,
          set_sources,
          co_set_sources,
          true),
        MAKE_DISTRIBUTED_GRPC_CORO_CALL(
          adserver::channel_svcs::channel_server::SetProxySourcesRequest,
          adserver::channel_svcs::channel_server::SetProxySourcesResponse,
          set_proxy_sources,
          co_set_proxy_sources,
          true),
        MAKE_DISTRIBUTED_GRPC_CORO_CALL(
          adserver::channel_svcs::channel_server::CheckRequest,
          adserver::channel_svcs::channel_server::CheckResponse,
          check,
          co_check,
          true),
        MAKE_DISTRIBUTED_GRPC_CORO_CALL(
          adserver::channel_svcs::channel_server::UpdateTriggersRequest,
          adserver::channel_svcs::channel_server::UpdateTriggersResponse,
          update_triggers,
          co_update_triggers,
          true),
        MAKE_DISTRIBUTED_GRPC_CORO_CALL(
          adserver::channel_svcs::channel_server::UpdateAllCcgRequest,
          adserver::channel_svcs::channel_server::UpdateAllCcgResponse,
          update_all_ccg,
          co_update_all_ccg,
          true),
        MAKE_DISTRIBUTED_GRPC_CORO_CALL(
          adserver::channel_svcs::channel_server::GetCountChunksRequest,
          adserver::channel_svcs::channel_server::GetCountChunksResponse,
          get_count_chunks,
          co_get_count_chunks,
          true));
    }

    AdServer::Commons::StartableAwaitable<void> co_match(
      adserver::channel_svcs::channel_server::MatchRequest&& request,
      adserver::channel_svcs::channel_server::MatchResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Commons::StartableAwaitable<void> co_get_ccg_traits(
      adserver::channel_svcs::channel_server::GetCcgTraitsRequest&& request,
      adserver::channel_svcs::channel_server::GetCcgTraitsResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Commons::StartableAwaitable<void> co_check_configuration(
      adserver::channel_svcs::channel_server::CheckConfigurationRequest&& request,
      adserver::channel_svcs::channel_server::CheckConfigurationResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Commons::StartableAwaitable<void> co_set_sources(
      adserver::channel_svcs::channel_server::SetSourcesRequest&& request,
      adserver::channel_svcs::channel_server::SetSourcesResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Commons::StartableAwaitable<void> co_set_proxy_sources(
      adserver::channel_svcs::channel_server::SetProxySourcesRequest&& request,
      adserver::channel_svcs::channel_server::SetProxySourcesResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Commons::StartableAwaitable<void> co_check(
      adserver::channel_svcs::channel_server::CheckRequest&& request,
      adserver::channel_svcs::channel_server::CheckResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Commons::StartableAwaitable<void> co_update_triggers(
      adserver::channel_svcs::channel_server::UpdateTriggersRequest&& request,
      adserver::channel_svcs::channel_server::UpdateTriggersResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Commons::StartableAwaitable<void> co_update_all_ccg(
      adserver::channel_svcs::channel_server::UpdateAllCcgRequest&& request,
      adserver::channel_svcs::channel_server::UpdateAllCcgResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Commons::StartableAwaitable<void> co_get_count_chunks(
      adserver::channel_svcs::channel_server::GetCountChunksRequest&& request,
      adserver::channel_svcs::channel_server::GetCountChunksResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Commons::StartableAwaitable<void> co_handle_batch_request(
      const adserver::grpc::BatchRequest& batch_request,
      adserver::grpc::BatchResponse& batch_response) const override;

    void start_handle_batch_request(
      AdServer::Grpc::GrpcServiceBase::BatchProcessingHandle& handle,
      const adserver::grpc::BatchRequest& batch_request,
      adserver::grpc::BatchResponse& batch_response,
      AdServer::Grpc::GrpcServiceBase::BatchCompletion completion)
      const override;

    void start_handle_batch_request(
      AdServer::Grpc::GrpcServiceBase::BatchProcessingHandle& handle,
      const adserver::grpc::BatchRequest& batch_request,
      AdServer::Grpc::GrpcServiceBase::BatchResponsePublisher& response_publisher,
      AdServer::Grpc::GrpcServiceBase::BatchCompletion completion)
      const override;

    std::size_t distributed_batch_max_sequential_ops() const noexcept override;

    std::shared_ptr<AdServer::Commons::ExecutorPool>
    batch_processing_executor_pool() const noexcept override;

    void stop_db_access();

  private:
    void get_status_(pc::GetStatusResponse& response) const;

  private:
    AdServer::Grpc::DbAccessService process_control_service_;
    ChannelServerCorePtr core_;
    const std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool_;
    const std::shared_ptr<AtomicStats> stats_;
    const std::size_t max_sequential_ops_;
  };

  void ChannelServerGrpc::ServiceImpl::stop_db_access()
  {
    process_control_service_.stop();
  }

  ChannelServerGrpc::ServiceImpl::ServiceImpl(
    ChannelServerCorePtr core,
    std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
    std::shared_ptr<AtomicStats> stats,
    std::size_t max_sequential_ops)
    : process_control_service_(
        [this]() -> std::optional<bool>
        {
          try
          {
            return core_->get_db_state();
          }
          catch (const Commons::DbStateChanger::NotSupported&)
          {
            return std::nullopt;
          }
        },
        [this](bool enabled) { core_->change_db_state(enabled); },
        [this](auto& response) { get_status_(response); }),
      core_(std::move(core)),
      executor_pool_(std::move(executor_pool)),
      stats_(std::move(stats)),
      max_sequential_ops_(max_sequential_ops)
  {
    add_grpc_service(&process_control_service_);
  }

  AdServer::Commons::StartableAwaitable<void>
  ChannelServerGrpc::ServiceImpl::co_handle_batch_request(
    const adserver::grpc::BatchRequest& batch_request,
    adserver::grpc::BatchResponse& batch_response) const
  {
    BatchStatsGuard in_progress(
      stats_->batch_total,
      stats_->batch_total_time,
      stats_->batch_in_progress);
    co_await AdServer::Grpc::GrpcServiceBase::co_handle_batch_request(
      batch_request,
      batch_response);
  }

  void
  ChannelServerGrpc::ServiceImpl::start_handle_batch_request(
    AdServer::Grpc::GrpcServiceBase::BatchProcessingHandle& handle,
    const adserver::grpc::BatchRequest& batch_request,
    adserver::grpc::BatchResponse& batch_response,
    AdServer::Grpc::GrpcServiceBase::BatchCompletion completion) const
  {
    auto in_progress = std::make_shared<BatchStatsGuard>(
      stats_->batch_total,
      stats_->batch_total_time,
      stats_->batch_in_progress);
    AdServer::Grpc::GrpcServiceBase::start_handle_batch_request(
      handle,
      batch_request,
      batch_response,
      [
        in_progress = std::move(in_progress),
        completion = std::move(completion)
      ](std::optional<std::exception_ptr> exception) mutable
      {
        in_progress.reset();
        if (completion)
        {
          completion(std::move(exception));
        }
      });
  }

  void
  ChannelServerGrpc::ServiceImpl::start_handle_batch_request(
    AdServer::Grpc::GrpcServiceBase::BatchProcessingHandle& handle,
    const adserver::grpc::BatchRequest& batch_request,
    AdServer::Grpc::GrpcServiceBase::BatchResponsePublisher& response_publisher,
    AdServer::Grpc::GrpcServiceBase::BatchCompletion completion) const
  {
    auto in_progress = std::make_shared<BatchStatsGuard>(
      stats_->batch_total,
      stats_->batch_total_time,
      stats_->batch_in_progress);
    AdServer::Grpc::GrpcServiceBase::start_handle_batch_request(
      handle,
      batch_request,
      response_publisher,
      [
        in_progress = std::move(in_progress),
        completion = std::move(completion)
      ](std::optional<std::exception_ptr> exception) mutable
      {
        in_progress.reset();
        if (completion)
        {
          completion(std::move(exception));
        }
      });
  }

  std::size_t
  ChannelServerGrpc::ServiceImpl::distributed_batch_max_sequential_ops() const noexcept
  {
    return max_sequential_ops_;
  }

  std::shared_ptr<AdServer::Commons::ExecutorPool>
  ChannelServerGrpc::ServiceImpl::batch_processing_executor_pool()
    const noexcept
  {
    return executor_pool_;
  }

  void
  ChannelServerGrpc::ServiceImpl::get_status_(pc::GetStatusResponse& response) const
  {
    try
    {
      const bool ready = core_->ready();
      response.set_ready(ready);
      if (!ready)
      {
        response.set_description(core_->comment());
      }
    }
    catch (const eh::Exception& ex)
    {
      response.set_ready(false);
      response.set_description(ex.what());
    }
  }

  AdServer::Commons::StartableAwaitable<void>
  ChannelServerGrpc::ServiceImpl::co_match(
    adserver::channel_svcs::channel_server::MatchRequest&& request,
    adserver::channel_svcs::channel_server::MatchResponse& response,
    ::grpc::Status& result_status) const
  {
    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);
    InProgressGuard in_progress(stats_->call_in_progress, stats_->match_in_progress);

    response.set_hostname(AdServer::Commons::hostname());

    try
    {
      ChannelServerCore::MatchQuery query;
      query.request_id = request.request_id();
      query.first_url = request.first_url();
      query.first_url_words = request.first_url_words();
      query.urls = request.urls();
      query.urls_words = request.urls_words();
      query.pwords = request.pwords();
      query.swords = request.swords();
      query.uid = GrpcAlgs::unpack_user_id(request.uid());
      query.statuses[0] = request.statuses().empty() ? '\0' : request.statuses()[0];
      query.statuses[1] = request.statuses().size() > 1 ? request.statuses()[1] : '\0';
      query.non_strict_word_match = request.non_strict_word_match();
      query.non_strict_url_match = request.non_strict_url_match();
      query.return_negative = request.return_negative();
      query.simplify_page = request.simplify_page();
      query.fill_content = request.fill_content();

      ChannelServerCore::MatchResult result;
      core_->match(query, result);
      pack_match_result(result, response);

      result_status = ::grpc::Status::OK;
    }
    catch (const ChannelServerCore::NotConfigured& ex)
    {
      result_status = AdServer::Grpc::error_status(::grpc::StatusCode::UNAVAILABLE, ex.what());
    }
    catch (const ChannelServerCore::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(::grpc::StatusCode::INTERNAL, ex.what());
    }
    catch (const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(::grpc::StatusCode::INTERNAL, ex.what());
    }
    co_return;
  }

  AdServer::Commons::StartableAwaitable<void>
  ChannelServerGrpc::ServiceImpl::co_get_ccg_traits(
    adserver::channel_svcs::channel_server::GetCcgTraitsRequest&& request,
    adserver::channel_svcs::channel_server::GetCcgTraitsResponse& response,
    ::grpc::Status& result_status) const
  {
    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);
    InProgressGuard in_progress(stats_->call_in_progress, stats_->get_ccg_traits_in_progress);

    response.set_hostname(AdServer::Commons::hostname());

    try
    {
      std::vector<unsigned long> ids;
      ids.reserve(request.ids_size());
      for (int i = 0; i < request.ids_size(); ++i)
      {
        ids.push_back(request.ids(i));
      }

      ChannelServerCore::TraitsResult result;
      core_->get_ccg_traits(ids, result);
      pack_traits_result(result, response);

      result_status = ::grpc::Status::OK;
    }
    catch (const ChannelServerCore::NotConfigured& ex)
    {
      result_status = AdServer::Grpc::error_status(::grpc::StatusCode::UNAVAILABLE, ex.what());
    }
    catch (const ChannelServerCore::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(::grpc::StatusCode::INTERNAL, ex.what());
    }
    catch (const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(::grpc::StatusCode::INTERNAL, ex.what());
    }
    co_return;
  }

  AdServer::Commons::StartableAwaitable<void>
  ChannelServerGrpc::ServiceImpl::co_check_configuration(
    adserver::channel_svcs::channel_server::CheckConfigurationRequest&&,
    adserver::channel_svcs::channel_server::CheckConfigurationResponse& response,
    ::grpc::Status& result_status) const
  {
    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);
    InProgressGuard in_progress(stats_->call_in_progress, stats_->check_configuration_in_progress);

    response.set_check_sum(core_->check_configuration());
    result_status = ::grpc::Status::OK;
    co_return;
  }

  AdServer::Commons::StartableAwaitable<void>
  ChannelServerGrpc::ServiceImpl::co_set_sources(
    adserver::channel_svcs::channel_server::SetSourcesRequest&& request,
    adserver::channel_svcs::channel_server::SetSourcesResponse&,
    ::grpc::Status& result_status) const
  {
    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);
    InProgressGuard in_progress(stats_->call_in_progress, stats_->set_sources_in_progress);

    if (request.check_sum() != 0 && core_->check_configuration() == request.check_sum())
    {
      result_status = ::grpc::Status::OK;
      co_return;
    }

    try
    {
      ChannelServerCore::DBSourceInfo db_info;
      db_info.pg_connection = request.pg_connection();
      db_info.colo = request.colo();
      db_info.version = request.version();
      db_info.count_chunks = request.count_chunks();
      db_info.check_sum = request.check_sum();

      db_info.campaign_refs.reserve(request.campaign_refs_size());
      for (const auto& ref : request.campaign_refs())
      {
        db_info.campaign_refs.emplace_back(ref);
      }

      core_->set_sources(db_info, unpack_sources(request.sources()));
      result_status = ::grpc::Status::OK;
    }
    catch (const ChannelServerCore::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(::grpc::StatusCode::INTERNAL, ex.what());
    }
    catch (const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(::grpc::StatusCode::INTERNAL, ex.what());
    }
    co_return;
  }

  AdServer::Commons::StartableAwaitable<void>
  ChannelServerGrpc::ServiceImpl::co_set_proxy_sources(
    adserver::channel_svcs::channel_server::SetProxySourcesRequest&& request,
    adserver::channel_svcs::channel_server::SetProxySourcesResponse&,
    ::grpc::Status& result_status) const
  {
    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);
    InProgressGuard in_progress(stats_->call_in_progress, stats_->set_proxy_sources_in_progress);

    if (request.check_sum() != 0 && core_->check_configuration() == request.check_sum())
    {
      result_status = ::grpc::Status::OK;
      co_return;
    }

    try
    {
      ChannelServerCore::ProxySourceInfo proxy_info;
      proxy_info.count_chunks = request.count_chunks();
      proxy_info.colo = request.colo();
      proxy_info.version = request.version();
      proxy_info.check_sum = request.check_sum();
      unpack_local_groups(request.local_groups(), proxy_info.local_descriptor);

      proxy_info.campaign_refs.reserve(request.campaign_refs_size());
      for (const auto& ref : request.campaign_refs())
      {
        proxy_info.campaign_refs.emplace_back(ref);
      }

      proxy_info.proxy_endpoints.reserve(request.proxy_endpoints_size());
      for (const auto& ref : request.proxy_endpoints())
      {
        proxy_info.proxy_endpoints.emplace_back(ref);
      }

      core_->set_proxy_sources(proxy_info, unpack_sources(request.sources()));
      result_status = ::grpc::Status::OK;
    }
    catch (const ImplementationException& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INVALID_ARGUMENT, ex.what());
    }
    catch (const ChannelServerCore::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(::grpc::StatusCode::INTERNAL, ex.what());
    }
    catch (const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(::grpc::StatusCode::INTERNAL, ex.what());
    }
    co_return;
  }

  AdServer::Commons::StartableAwaitable<void>
  ChannelServerGrpc::ServiceImpl::co_check(
    adserver::channel_svcs::channel_server::CheckRequest&& request,
    adserver::channel_svcs::channel_server::CheckResponse& response,
    ::grpc::Status& result_status) const
  {
    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);
    InProgressGuard in_progress(stats_->call_in_progress, stats_->channel_update_in_progress);
    try
    {
      ChannelServerCore::CheckQuery query;
      query.master_stamp = unpack_update_stamp(request.master_stamp());
      query.new_ids.assign(request.new_ids().begin(), request.new_ids().end());
      query.use_only_list = request.use_only_list();
      ChannelServerCore::CheckData data;
      core_->check(query, data);
      response.set_first_stamp(GrpcAlgs::pack_time(data.first_stamp));
      response.set_master_stamp(GrpcAlgs::pack_time(data.master_stamp));
      response.set_max_time(GrpcAlgs::pack_time(data.max_time));
      response.set_source_id(data.source_id);
      response.set_special_adv(data.special_adv);
      response.set_special_track(data.special_track);
      for (const auto& source : data.versions)
      {
        auto* target = response.add_versions();
        target->set_id(source.id);
        target->set_size(source.size);
        target->set_stamp(GrpcAlgs::pack_time(source.stamp));
      }

      result_status = ::grpc::Status::OK;
    }
    catch (const ChannelServerCore::NotConfigured& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::FAILED_PRECONDITION, ex.what());
    }
    catch (const std::invalid_argument& ex)
    {
      result_status = AdServer::Grpc::error_status(::grpc::StatusCode::INVALID_ARGUMENT, ex.what());
    }
    catch (const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(::grpc::StatusCode::INTERNAL, ex.what());
    }
    co_return;
  }

  AdServer::Commons::StartableAwaitable<void>
  ChannelServerGrpc::ServiceImpl::co_update_triggers(
    adserver::channel_svcs::channel_server::UpdateTriggersRequest&& request,
    adserver::channel_svcs::channel_server::UpdateTriggersResponse& response,
    ::grpc::Status& result_status) const
  {
    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);
    InProgressGuard in_progress(stats_->call_in_progress, stats_->channel_update_in_progress);
    try
    {
      ChannelServerCore::UpdateDataResult data;
      core_->update_triggers(unpack_sources(request.ids()), data);
      response.set_source_id(data.source_id);
      for (const auto& source : data.channels)
      {
        auto* target = response.add_channels();
        target->set_channel_id(source.channel_id);
        target->set_stamp(GrpcAlgs::pack_time(source.stamp));
        for (const auto& word : source.words)
        {
          auto* target_word = target->add_words();
          target_word->set_channel_trigger_id(word.channel_trigger_id);
          target_word->set_trigger(word.trigger);
        }
      }

      result_status = ::grpc::Status::OK;
    }
    catch (const ChannelServerCore::NotConfigured& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::FAILED_PRECONDITION, ex.what());
    }
    catch (const std::invalid_argument& ex)
    {
      result_status = AdServer::Grpc::error_status(::grpc::StatusCode::INVALID_ARGUMENT, ex.what());
    }
    catch (const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(::grpc::StatusCode::INTERNAL, ex.what());
    }
    co_return;
  }

  AdServer::Commons::StartableAwaitable<void>
  ChannelServerGrpc::ServiceImpl::co_update_all_ccg(
    adserver::channel_svcs::channel_server::UpdateAllCcgRequest&& request,
    adserver::channel_svcs::channel_server::UpdateAllCcgResponse& response,
    ::grpc::Status& result_status) const
  {
    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);
    InProgressGuard in_progress(stats_->call_in_progress, stats_->channel_update_in_progress);
    try
    {
      if (!request.limit())
      {
        throw std::invalid_argument("CCG limit must be positive");
      }

      ChannelServerCore::CCGQuery query;
      query.master_stamp = unpack_update_stamp(request.master_stamp());
      query.start = request.start();
      query.limit = request.limit();
      query.channel_ids = unpack_sources(request.channel_ids());
      query.use_only_list = request.use_only_list();
      ChannelServerCore::PosCCGResult data;
      core_->update_all_ccg(query, data);
      response.set_start_id(data.start_id);
      response.set_source_id(data.source_id);
      for (const auto& source : data.keywords)
      {
        auto* target = response.add_keywords();
        target->set_ccg_keyword_id(source.ccg_keyword_id);
        target->set_ccg_id(source.ccg_id);
        target->set_channel_id(source.channel_id);
        target->set_max_cpc(GrpcAlgs::pack_decimal(source.max_cpc));
        target->set_ctr(GrpcAlgs::pack_decimal(source.ctr));
        target->set_click_url(source.click_url);
        target->set_original_keyword(source.original_keyword);
      }

      for (const auto& source : data.deleted)
      {
        auto* target = response.add_deleted();
        target->set_id(source.id);
        target->set_stamp(GrpcAlgs::pack_time(source.stamp));
      }

      result_status = ::grpc::Status::OK;
    }
    catch (const ChannelServerCore::NotConfigured& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::FAILED_PRECONDITION, ex.what());
    }
    catch (const std::invalid_argument& ex)
    {
      result_status = AdServer::Grpc::error_status(::grpc::StatusCode::INVALID_ARGUMENT, ex.what());
    }
    catch (const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(::grpc::StatusCode::INTERNAL, ex.what());
    }
    co_return;
  }

  AdServer::Commons::StartableAwaitable<void>
  ChannelServerGrpc::ServiceImpl::co_get_count_chunks(
    adserver::channel_svcs::channel_server::GetCountChunksRequest&&,
    adserver::channel_svcs::channel_server::GetCountChunksResponse& response,
    ::grpc::Status& result_status) const
  {
    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);
    InProgressGuard in_progress(stats_->call_in_progress, stats_->channel_update_in_progress);
    try
    {
      response.set_count_chunks(core_->get_count_chunks());

      result_status = ::grpc::Status::OK;
    }
    catch (const ChannelServerCore::NotConfigured& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::FAILED_PRECONDITION, ex.what());
    }
    catch (const std::invalid_argument& ex)
    {
      result_status = AdServer::Grpc::error_status(::grpc::StatusCode::INVALID_ARGUMENT, ex.what());
    }
    catch (const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(::grpc::StatusCode::INTERNAL, ex.what());
    }
    co_return;
  }

  ChannelServerGrpc::ChannelServerGrpc(
    ChannelServerCorePtr core,
    Logging::Logger* logger,
    std::string_view bind_address,
    unsigned int bind_port,
    std::size_t process_threads,
    std::size_t cq_threads,
    std::size_t threads_per_cq,
    std::size_t max_sequential_ops)
    : bind_address_(std::string(bind_address) + ":" + std::to_string(bind_port)),
      stats_(std::make_shared<AtomicStats>()),
      executor_pool_(std::make_shared<AdServer::Commons::ExecutorPool>(
        Generics::ActiveObjectCallback_var(
          new Logging::ActiveObjectCallbackImpl(logger, "", channel_server_grpc_aspect)),
        std::max<std::size_t>(1, process_threads),
        AdServer::Commons::ExecutorPool::ResumeStrategy::CurrentContext,
        "ca:cs-grpc-p")),
      impl_(std::make_shared<Impl>(
        logger,
        channel_server_grpc_aspect,
        bind_address_,
        cq_threads != 0 ? cq_threads : 16,
        threads_per_cq,
        std::make_unique<ServiceImpl>(
          std::move(core),
          executor_pool_,
          stats_,
          resolve_max_sequential_ops(max_sequential_ops))))
  {
    add_child_object(executor_pool_);
    add_child_object(impl_);
  }

  ChannelServerGrpc::Stats
  ChannelServerGrpc::stats() const noexcept
  {
    const auto lifecycle_stats = impl_->service().lifecycle_stats();
    return Stats{
      stats_->call_in_progress.load(std::memory_order_relaxed),
      stats_->match_in_progress.load(std::memory_order_relaxed),
      stats_->get_ccg_traits_in_progress.load(std::memory_order_relaxed),
      stats_->check_configuration_in_progress.load(std::memory_order_relaxed),
      stats_->set_sources_in_progress.load(std::memory_order_relaxed),
      stats_->set_proxy_sources_in_progress.load(std::memory_order_relaxed),
      stats_->channel_update_in_progress.load(std::memory_order_relaxed),
      stats_->batch_total.load(std::memory_order_relaxed),
      stats_->batch_total_time.load(std::memory_order_relaxed),
      stats_->batch_in_progress.load(std::memory_order_relaxed),
      lifecycle_stats
    };
  }

  void ChannelServerGrpc::deactivate_object_()
  {
    impl_->service().stop_db_access();
    Generics::CompositeActiveObject::deactivate_object_();
  }

  ChannelServerGrpc::~ChannelServerGrpc() noexcept = default;
}
