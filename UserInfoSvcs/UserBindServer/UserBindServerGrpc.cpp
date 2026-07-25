#include "UserBindServerGrpc.hpp"

#include <grpcpp/grpcpp.h>

#include <algorithm>
#include <chrono>
#include <thread>
#include <utility>
#include <vector>
#include <unistd.h>

#include <Generics/HashTableAdapters.hpp>
#include <Generics/Time.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Commons/ExecutorPool.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/Grpc/GrpcServer.hpp>

#include <Commons/Grpc/Batch.grpc.pb.h>
#include <UserInfoSvcs/UserBindServer/UserBindServerGrpc.grpc.pb.h>

#include "UserBindServerCore.hpp"

namespace AdServer::UserInfoSvcs
{
  namespace
  {
    constexpr const char user_bind_server_grpc_aspect[] = "UserBindServerGrpc";

    const std::string&
    service_hostname_()
    {
      static const std::string hostname = []()
      {
        char buffer[256];
        if (::gethostname(buffer, sizeof(buffer)) != 0)
        {
          return std::string();
        }
        buffer[sizeof(buffer) - 1] = 0;
        return std::string(buffer);
      }();
      return hostname;
    }

    class InProgressGuard final
    {
    public:
      InProgressGuard(
        std::atomic<std::uint64_t>& call_total,
        std::atomic<std::uint64_t>& call_total_time,
        std::atomic<std::uint64_t>& call_counter,
        std::atomic<std::uint64_t>& method_total,
        std::atomic<std::uint64_t>& method_total_time,
        std::atomic<std::uint64_t>& method_counter) noexcept
        : call_total_(call_total),
          call_total_time_(call_total_time),
          call_counter_(call_counter),
          method_total_(method_total),
          method_total_time_(method_total_time),
          method_counter_(method_counter)
      {
        call_total_.fetch_add(1, std::memory_order_relaxed);
        call_counter_.fetch_add(1, std::memory_order_relaxed);
        method_total_.fetch_add(1, std::memory_order_relaxed);
        method_counter_.fetch_add(1, std::memory_order_relaxed);
      }

      ~InProgressGuard()
      {
        const auto elapsed_us =
          (Generics::Time::get_time_of_day() - start_time_).microseconds();
        call_total_time_.fetch_add(elapsed_us, std::memory_order_relaxed);
        method_total_time_.fetch_add(elapsed_us, std::memory_order_relaxed);
        method_counter_.fetch_sub(1, std::memory_order_relaxed);
        call_counter_.fetch_sub(1, std::memory_order_relaxed);
      }

      InProgressGuard(const InProgressGuard&) = delete;
      InProgressGuard& operator=(const InProgressGuard&) = delete;

    private:
      std::atomic<std::uint64_t>& call_total_;
      std::atomic<std::uint64_t>& call_total_time_;
      std::atomic<std::uint64_t>& call_counter_;
      std::atomic<std::uint64_t>& method_total_;
      std::atomic<std::uint64_t>& method_total_time_;
      std::atomic<std::uint64_t>& method_counter_;
      const Generics::Time start_time_ = Generics::Time::get_time_of_day();
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
        const auto elapsed_us =
          (Generics::Time::get_time_of_day() - start_time_).microseconds();
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

#ifdef MOCK_USER_BIND_SERVER_FAST_GET_USER_ID
    void
    maybe_sleep_mock_response(
      const std::shared_ptr<std::atomic_uint>& response_sleep_ms)
    {
      if (!response_sleep_ms)
      {
        return;
      }

      const auto delay_ms = response_sleep_ms->load(std::memory_order_acquire);
      if (delay_ms != 0)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
      }
    }
#endif

    std::size_t
    resolve_max_batch_split(
      std::size_t configured,
      std::size_t process_threads)
    {
      return std::max<std::size_t>(
        1,
        configured != 0 ? configured : process_threads);
    }
  }

  struct UserBindServerGrpc::AtomicStats
  {
    std::atomic<std::uint64_t> call_total{0};
    std::atomic<std::uint64_t> call_total_time{0};
    std::atomic<std::uint64_t> call_in_progress{0};
    std::atomic<std::uint64_t> get_bind_request_total{0};
    std::atomic<std::uint64_t> get_bind_request_total_time{0};
    std::atomic<std::uint64_t> get_bind_request_in_progress{0};
    std::atomic<std::uint64_t> add_bind_request_total{0};
    std::atomic<std::uint64_t> add_bind_request_total_time{0};
    std::atomic<std::uint64_t> add_bind_request_in_progress{0};
    std::atomic<std::uint64_t> get_user_id_total{0};
    std::atomic<std::uint64_t> get_user_id_total_time{0};
    std::atomic<std::uint64_t> get_user_id_in_progress{0};
    std::atomic<std::uint64_t> add_user_id_total{0};
    std::atomic<std::uint64_t> add_user_id_total_time{0};
    std::atomic<std::uint64_t> add_user_id_in_progress{0};
    std::atomic<std::uint64_t> batch_total{0};
    std::atomic<std::uint64_t> batch_total_time{0};
    std::atomic<std::uint64_t> batch_in_progress{0};
  };

  class UserBindServerGrpc::ServiceImpl final:
    public AdServer::Grpc::GrpcAsyncServiceBase<
      UserBindServerGrpc::ServiceImpl,
      adserver::user_info_svcs::user_bind::UserBindServerGrpc,
      adserver::user_info_svcs::user_bind::UserBindServerGrpc::AsyncService>
  {
    using AsyncService =
      adserver::user_info_svcs::user_bind::UserBindServerGrpc::AsyncService;

  public:
    ServiceImpl(
      UserBindServerCore* core,
      std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
      std::size_t max_batch_split,
      std::shared_ptr<AtomicStats> stats,
      std::shared_ptr<std::atomic_uint> response_sleep_ms = nullptr);

    static auto grpc_calls()
    {
      return std::make_tuple(
        MAKE_DISTRIBUTED_GRPC_CORO_CALL(
          adserver::user_info_svcs::user_bind::GetBindRequestRequest,
          adserver::user_info_svcs::user_bind::GetBindRequestResponse,
          get_bind_request,
          co_get_bind_request,
          &ServiceImpl::hash_get_bind_request),
        MAKE_DISTRIBUTED_GRPC_CORO_CALL(
          adserver::user_info_svcs::user_bind::AddBindRequestRequest,
          adserver::user_info_svcs::user_bind::AddBindRequestResponse,
          add_bind_request,
          co_add_bind_request,
          &ServiceImpl::hash_add_bind_request),
        MAKE_DISTRIBUTED_GRPC_CORO_CALL(
          adserver::user_info_svcs::user_bind::GetUserIdRequest,
          adserver::user_info_svcs::user_bind::GetUserIdResponse,
          get_user_id,
          co_get_user_id,
          &ServiceImpl::hash_get_user_id),
        MAKE_DISTRIBUTED_GRPC_CORO_CALL(
          adserver::user_info_svcs::user_bind::AddUserIdRequest,
          adserver::user_info_svcs::user_bind::AddUserIdResponse,
          add_user_id,
          co_add_user_id,
          &ServiceImpl::hash_add_user_id),
        MAKE_DISTRIBUTED_GRPC_CORO_CALL(
          adserver::user_info_svcs::user_bind::GetSourceRequest,
          adserver::user_info_svcs::user_bind::GetSourceResponse,
          get_source,
          co_get_source));
    }

    std::size_t distributed_batch_max_split() const noexcept override;

    std::shared_ptr<AdServer::Commons::ExecutorPool>
    batch_processing_executor_pool() const noexcept override;

    AdServer::Grpc::GrpcCoroutine co_handle_batch_request(
      const adserver::grpc::BatchRequest& batch_request,
      adserver::grpc::BatchResponse& batch_response) const override;

    AdServer::Grpc::GrpcCoroutine co_get_bind_request(
      adserver::user_info_svcs::user_bind::GetBindRequestRequest&& request,
      adserver::user_info_svcs::user_bind::GetBindRequestResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_add_bind_request(
      adserver::user_info_svcs::user_bind::AddBindRequestRequest&& request,
      adserver::user_info_svcs::user_bind::AddBindRequestResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_get_user_id(
      adserver::user_info_svcs::user_bind::GetUserIdRequest&& request,
      adserver::user_info_svcs::user_bind::GetUserIdResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_add_user_id(
      adserver::user_info_svcs::user_bind::AddUserIdRequest&& request,
      adserver::user_info_svcs::user_bind::AddUserIdResponse& response,
      ::grpc::Status& result_status) const;

    AdServer::Grpc::GrpcCoroutine co_get_source(
      adserver::user_info_svcs::user_bind::GetSourceRequest&& request,
      adserver::user_info_svcs::user_bind::GetSourceResponse& response,
      ::grpc::Status& result_status) const;

    static std::size_t hash_get_user_id(
      const adserver::user_info_svcs::user_bind::GetUserIdRequest& request);

    static std::size_t hash_add_user_id(
      const adserver::user_info_svcs::user_bind::AddUserIdRequest& request);

    static std::size_t hash_get_bind_request(
      const adserver::user_info_svcs::user_bind::GetBindRequestRequest& request);

    static std::size_t hash_add_bind_request(
      const adserver::user_info_svcs::user_bind::AddBindRequestRequest& request);

  private:
    const UserBindServerCore_var core_;
    const std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool_;
    const std::size_t max_batch_split_;
    const std::shared_ptr<AtomicStats> stats_;
#ifdef MOCK_USER_BIND_SERVER_FAST_GET_USER_ID
    const std::shared_ptr<std::atomic_uint> response_sleep_ms_;
#endif
  };

  // UserBindServerGrpc::ServiceImpl
  UserBindServerGrpc::ServiceImpl::ServiceImpl(
    UserBindServerCore* core,
    std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
    std::size_t max_batch_split,
    std::shared_ptr<AtomicStats> stats,
    std::shared_ptr<std::atomic_uint> response_sleep_ms)
    : core_(ReferenceCounting::add_ref(core)),
      executor_pool_(std::move(executor_pool)),
      max_batch_split_(max_batch_split),
      stats_(std::move(stats))
#ifdef MOCK_USER_BIND_SERVER_FAST_GET_USER_ID
      ,
      response_sleep_ms_(std::move(response_sleep_ms))
#endif
  {
#ifndef MOCK_USER_BIND_SERVER_FAST_GET_USER_ID
    (void)response_sleep_ms;
#endif
  }

  std::size_t
  UserBindServerGrpc::ServiceImpl::distributed_batch_max_split()
    const noexcept
  {
    return max_batch_split_;
  }

  std::shared_ptr<AdServer::Commons::ExecutorPool>
  UserBindServerGrpc::ServiceImpl::batch_processing_executor_pool()
    const noexcept
  {
    return executor_pool_;
  }

  AdServer::Grpc::GrpcCoroutine
  UserBindServerGrpc::ServiceImpl::co_handle_batch_request(
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

  AdServer::Grpc::GrpcCoroutine
  UserBindServerGrpc::ServiceImpl::co_get_bind_request(
    adserver::user_info_svcs::user_bind::GetBindRequestRequest&& request,
    adserver::user_info_svcs::user_bind::GetBindRequestResponse& response,
    ::grpc::Status& result_status) const
  {
    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);
    InProgressGuard in_progress(
      stats_->call_total,
      stats_->call_total_time,
      stats_->call_in_progress,
      stats_->get_bind_request_total,
      stats_->get_bind_request_total_time,
      stats_->get_bind_request_in_progress);

    response.set_hostname(service_hostname_());

#ifdef MOCK_USER_BIND_SERVER_FAST_GET_USER_ID
    maybe_sleep_mock_response(response_sleep_ms_);
    result_status = ::grpc::Status::OK;
    co_return;
#endif

    try
    {
      const auto result = co_await core_->co_get_bind_request(
        request.request_id(),
        GrpcAlgs::unpack_time(request.timestamp()));

      response.mutable_bind_user_ids()->Add(
        result.bind_user_ids.begin(),
        result.bind_user_ids.end());

      result_status = ::grpc::Status::OK;
    }
    catch (const UserBindServerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch (const UserBindServerCore::ChunkNotFound& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::NOT_FOUND,
        ex.what());
    }
    catch (const UserBindServerCore::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  AdServer::Grpc::GrpcCoroutine
  UserBindServerGrpc::ServiceImpl::co_add_bind_request(
    adserver::user_info_svcs::user_bind::AddBindRequestRequest&& request,
    adserver::user_info_svcs::user_bind::AddBindRequestResponse& response,
    ::grpc::Status& result_status) const
  {
    (void)response;
    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);
    InProgressGuard in_progress(
      stats_->call_total,
      stats_->call_total_time,
      stats_->call_in_progress,
      stats_->add_bind_request_total,
      stats_->add_bind_request_total_time,
      stats_->add_bind_request_in_progress);

    response.set_hostname(service_hostname_());

#ifdef MOCK_USER_BIND_SERVER_FAST_GET_USER_ID
    maybe_sleep_mock_response(response_sleep_ms_);
    result_status = ::grpc::Status::OK;
    co_return;
#endif

    try
    {
      UserBindServerCore::BindRequestInfo bind_request;
      bind_request.bind_user_ids.reserve(request.bind_user_ids_size());
      for (int i = 0; i < request.bind_user_ids_size(); ++i)
      {
        bind_request.bind_user_ids.push_back(request.bind_user_ids(i));
      }

      core_->add_bind_request(
        request.request_id(),
        bind_request,
        GrpcAlgs::unpack_time(request.timestamp()));

      result_status = ::grpc::Status::OK;
    }
    catch (const UserBindServerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch (const UserBindServerCore::ChunkNotFound& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::NOT_FOUND,
        ex.what());
    }
    catch (const UserBindServerCore::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  AdServer::Grpc::GrpcCoroutine
  UserBindServerGrpc::ServiceImpl::co_get_user_id(
    adserver::user_info_svcs::user_bind::GetUserIdRequest&& request,
    adserver::user_info_svcs::user_bind::GetUserIdResponse& response,
    ::grpc::Status& result_status) const
  {
    InProgressGuard in_progress(
      stats_->call_total,
      stats_->call_total_time,
      stats_->call_in_progress,
      stats_->get_user_id_total,
      stats_->get_user_id_total_time,
      stats_->get_user_id_in_progress);

    response.set_hostname(service_hostname_());

#ifdef MOCK_USER_BIND_SERVER_FAST_GET_USER_ID
    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);
    maybe_sleep_mock_response(response_sleep_ms_);
    response.set_user_id(request.current_user_id());
    response.set_min_age_reached(true);
    response.set_created(false);
    response.set_invalid_operation(false);
    response.set_user_found(false);
    result_status = ::grpc::Status::OK;
    co_return;
#endif

    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);

    try
    {
      UserBindServerCore::GetUserRequestInfo req_info;
      req_info.id = request.id();
      req_info.silent = request.silent();
      req_info.generate_user_id = request.generate_user_id();
      req_info.for_set_cookie = request.for_set_cookie();

      req_info.timestamp = GrpcAlgs::unpack_time(request.timestamp());
      req_info.create_timestamp = GrpcAlgs::unpack_time(request.create_timestamp());
      req_info.current_user_id = GrpcAlgs::unpack_user_id(request.current_user_id());

      const auto result = co_await core_->co_get_user_id(req_info);
      response.set_user_id(GrpcAlgs::pack_user_id(result.user_id));
      response.set_min_age_reached(result.min_age_reached);
      response.set_created(result.created);
      response.set_invalid_operation(result.invalid_operation);
      response.set_user_found(result.user_found);

      result_status = ::grpc::Status::OK;
    }
    catch (const UserBindServerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch (const UserBindServerCore::ChunkNotFound& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::NOT_FOUND,
        ex.what());
    }
    catch (const UserBindServerCore::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  AdServer::Grpc::GrpcCoroutine
  UserBindServerGrpc::ServiceImpl::co_add_user_id(
    adserver::user_info_svcs::user_bind::AddUserIdRequest&& request,
    adserver::user_info_svcs::user_bind::AddUserIdResponse& response,
    ::grpc::Status& result_status) const
  {
    InProgressGuard in_progress(
      stats_->call_total,
      stats_->call_total_time,
      stats_->call_in_progress,
      stats_->add_user_id_total,
      stats_->add_user_id_total_time,
      stats_->add_user_id_in_progress);

    response.set_hostname(service_hostname_());

#ifdef MOCK_USER_BIND_SERVER_FAST_GET_USER_ID
    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);
    maybe_sleep_mock_response(response_sleep_ms_);
    response.set_merge_user_id(request.user_id());
    response.set_invalid_operation(false);
    result_status = ::grpc::Status::OK;
    co_return;
#endif

    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);

    try
    {
      UserBindServerCore::AddUserRequestInfo req_info;
      req_info.id = request.id();

      req_info.timestamp = GrpcAlgs::unpack_time(request.timestamp());
      req_info.user_id = GrpcAlgs::unpack_user_id(request.user_id());

      const auto result = co_await core_->co_add_user_id(req_info);

      response.set_merge_user_id(GrpcAlgs::pack_user_id(result.merge_user_id));
      response.set_invalid_operation(result.invalid_operation);

      result_status = ::grpc::Status::OK;
    }
    catch (const UserBindServerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch (const UserBindServerCore::ChunkNotFound& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::NOT_FOUND,
        ex.what());
    }
    catch (const UserBindServerCore::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  AdServer::Grpc::GrpcCoroutine
  UserBindServerGrpc::ServiceImpl::co_get_source(
    adserver::user_info_svcs::user_bind::GetSourceRequest&& request,
    adserver::user_info_svcs::user_bind::GetSourceResponse& response,
    ::grpc::Status& result_status) const
  {
    (void)request;
    co_await AdServer::Commons::ExecutorPool::yield(executor_pool_);
#ifdef MOCK_USER_BIND_SERVER_FAST_GET_USER_ID
    maybe_sleep_mock_response(response_sleep_ms_);
    response.set_chunks_number(1);
    response.add_chunks(0);
    result_status = ::grpc::Status::OK;
    co_return;
#endif

    try
    {
      const auto result = core_->get_source();
      response.mutable_chunks()->Add(
        result.chunks.begin(),
        result.chunks.end());
      response.set_chunks_number(result.chunks_number);

      result_status = ::grpc::Status::OK;
    }
    catch (const UserBindServerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch (const UserBindServerCore::ChunkNotFound& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::NOT_FOUND,
        ex.what());
    }
    catch (const UserBindServerCore::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  std::size_t
  UserBindServerGrpc::ServiceImpl::hash_get_user_id(
    const adserver::user_info_svcs::user_bind::GetUserIdRequest& request)
  {
    return Generics::StringHashAdapter(request.id()).hash();
  }

  std::size_t
  UserBindServerGrpc::ServiceImpl::hash_add_user_id(
    const adserver::user_info_svcs::user_bind::AddUserIdRequest& request)
  {
    return Generics::StringHashAdapter(request.id()).hash();
  }

  std::size_t
  UserBindServerGrpc::ServiceImpl::hash_get_bind_request(
    const adserver::user_info_svcs::user_bind::GetBindRequestRequest& request)
  {
    return Generics::StringHashAdapter(request.request_id()).hash();
  }

  std::size_t
  UserBindServerGrpc::ServiceImpl::hash_add_bind_request(
    const adserver::user_info_svcs::user_bind::AddBindRequestRequest& request)
  {
    return Generics::StringHashAdapter(request.request_id()).hash();
  }

  // UserBindServerGrpc impl
  UserBindServerGrpc::UserBindServerGrpc(
    UserBindServerCore* core,
    Logging::Logger* logger,
    std::string_view bind_address,
    unsigned int bind_port,
    std::size_t process_threads,
    std::size_t cq_threads,
    std::size_t max_split,
    std::shared_ptr<std::atomic_uint> response_sleep_ms)
    : bind_address_(std::string(bind_address) + ":" + std::to_string(bind_port)),
      max_batch_split_(resolve_max_batch_split(max_split, process_threads)),
      stats_(std::make_shared<AtomicStats>()),
      executor_pool_(std::make_shared<AdServer::Commons::ExecutorPool>(
        Generics::ActiveObjectCallback_var(
          new Logging::ActiveObjectCallbackImpl(
            logger,
            "",
            user_bind_server_grpc_aspect)),
        std::max<std::size_t>(1, process_threads),
        AdServer::Commons::ExecutorPool::ResumeStrategy::CurrentContext,
        "ca:ub-grpc-p")),
      impl_(std::make_shared<Impl>(
        logger,
        user_bind_server_grpc_aspect,
        bind_address_,
        cq_threads != 0 ? cq_threads : 16,
        std::make_unique<ServiceImpl>(
          core,
          executor_pool_,
          max_batch_split_,
          stats_,
          std::move(response_sleep_ms))))
  {
    add_child_object(executor_pool_);
    add_child_object(impl_);
  }

  UserBindServerGrpc::Stats
  UserBindServerGrpc::stats() const noexcept
  {
    const auto lifecycle_stats = impl_->service().lifecycle_stats();
    return Stats{
      stats_->call_total.load(std::memory_order_relaxed),
      stats_->call_total_time.load(std::memory_order_relaxed),
      stats_->call_in_progress.load(std::memory_order_relaxed),
      stats_->get_bind_request_total.load(std::memory_order_relaxed),
      stats_->get_bind_request_total_time.load(std::memory_order_relaxed),
      stats_->get_bind_request_in_progress.load(std::memory_order_relaxed),
      stats_->add_bind_request_total.load(std::memory_order_relaxed),
      stats_->add_bind_request_total_time.load(std::memory_order_relaxed),
      stats_->add_bind_request_in_progress.load(std::memory_order_relaxed),
      stats_->get_user_id_total.load(std::memory_order_relaxed),
      stats_->get_user_id_total_time.load(std::memory_order_relaxed),
      stats_->get_user_id_in_progress.load(std::memory_order_relaxed),
      stats_->add_user_id_total.load(std::memory_order_relaxed),
      stats_->add_user_id_total_time.load(std::memory_order_relaxed),
      stats_->add_user_id_in_progress.load(std::memory_order_relaxed),
      stats_->batch_total.load(std::memory_order_relaxed),
      stats_->batch_total_time.load(std::memory_order_relaxed),
      stats_->batch_in_progress.load(std::memory_order_relaxed),
      lifecycle_stats
    };
  }

  UserBindServerGrpc::~UserBindServerGrpc() noexcept
  {}
}
