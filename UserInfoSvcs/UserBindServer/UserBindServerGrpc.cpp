#include "UserBindServerGrpc.hpp"

#include <grpcpp/grpcpp.h>

#include <algorithm>
#include <functional>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <Commons/GrpcAlgs.hpp>

#include <Commons/Grpc/Batch.grpc.pb.h>
#include <UserInfoSvcs/UserBindServer/UserBindServerGrpc.grpc.pb.h>

#include "UserBindServerCore.hpp"

namespace AdServer::UserInfoSvcs
{
  namespace
  {
    ::grpc::Status make_status_(const UserBindServerCore::NotReady& ex)
    {
      return ::grpc::Status(::grpc::StatusCode::UNAVAILABLE, ex.what());
    }

    ::grpc::Status make_status_(const UserBindServerCore::ChunkNotFound& ex)
    {
      return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, ex.what());
    }

    ::grpc::Status make_status_(const UserBindServerCore::Exception& ex)
    {
      return ::grpc::Status(::grpc::StatusCode::INTERNAL, ex.what());
    }

    constexpr const char* get_user_id_full_method =
      "/adserver.user_info_svcs.user_bind.UserBindServerGrpc/get_user_id";

    constexpr const char* add_user_id_full_method =
      "/adserver.user_info_svcs.user_bind.UserBindServerGrpc/add_user_id";

    class IAsyncCall
    {
    public:
      virtual ~IAsyncCall() noexcept = default;
      virtual void proceed(bool ok) = 0;
    };

    template<typename Request, typename Response>
    class UnaryCallBase
      : public IAsyncCall
    {
    public:
      using Service = adserver::user_info_svcs::user_bind::UserBindServerGrpc::AsyncService;

    public:
      UnaryCallBase(Service* service, ::grpc::ServerCompletionQueue* completion_queue)
        : service_(service),
          completion_queue_(completion_queue),
          responder_(&context_),
          state_(State::Create)
      {}

      virtual ~UnaryCallBase() noexcept = default;

      void proceed(bool ok) override
      {
        if (state_ == State::Create)
        {
          if (!ok)
          {
            delete this;
            return;
          }

          state_ = State::Process;
          request_method_();
          return;
        }

        if (state_ == State::Process)
        {
          if (!ok)
          {
            delete this;
            return;
          }

          state_ = State::Finish;
          spawn_next_();
          process_();
          return;
        }

        delete this;
      }

    protected:
      virtual void request_method_() = 0;

      virtual void spawn_next_() = 0;

      virtual void process_() = 0;

    protected:
      Service* const service_;
      ::grpc::ServerCompletionQueue* const completion_queue_;
      ::grpc::ServerContext context_;
      Request request_;
      Response response_;
      ::grpc::ServerAsyncResponseWriter<Response> responder_;

    private:
      enum class State
      {
        Create,
        Process,
        Finish
      };

      State state_;
    };
  }

  class UserBindServerGrpc::Impl final:
    public Generics::SimpleActiveObject,
    public ReferenceCounting::AtomicImpl
  {
  public:
    Impl(
      Logging::Logger* logger,
      UserBindServerCore* core,
      std::string_view bind_address);

    virtual ~Impl() noexcept = default;

  private:
    class ServiceImpl final
    {
    public:
      ServiceImpl(UserBindServerCore* core);

      using CompletionQueues = std::vector<::grpc::ServerCompletionQueue*>;

      void start(const CompletionQueues& completion_queues);

      void handle_get_bind_request(
        const adserver::user_info_svcs::user_bind::GetBindRequestRequest& request,
        adserver::user_info_svcs::user_bind::GetBindRequestResponse& response,
        ::grpc::Status& result_status) const;

      void handle_add_bind_request(
        const adserver::user_info_svcs::user_bind::AddBindRequestRequest& request,
        adserver::user_info_svcs::user_bind::AddBindRequestResponse&,
        ::grpc::Status& result_status) const;

      void handle_get_user_id(
        const adserver::user_info_svcs::user_bind::GetUserIdRequest& request,
        adserver::user_info_svcs::user_bind::GetUserIdResponse& response,
        ::grpc::Status& result_status) const;

      void handle_add_user_id(
        const adserver::user_info_svcs::user_bind::AddUserIdRequest& request,
        adserver::user_info_svcs::user_bind::AddUserIdResponse& response,
        ::grpc::Status& result_status) const;

      void handle_get_source(
        const adserver::user_info_svcs::user_bind::GetSourceRequest&,
        adserver::user_info_svcs::user_bind::GetSourceResponse& response,
        ::grpc::Status& result_status) const;

      void handle_batch_request(
        const adserver::grpc::BatchRequest& batch_request,
        adserver::grpc::BatchResponse& batch_response) const;

      adserver::user_info_svcs::user_bind::UserBindServerGrpc::AsyncService grpc_service;
      adserver::user_info_svcs::user_bind::UserBindServerBatchTransport::AsyncService batch_transport_service;

    private:
      class MethodRegistry final
      {
      public:
        using BatchRequestItem = adserver::grpc::BatchRequestItem;
        using BatchResponseItem = adserver::grpc::BatchResponseItem;
        using DispatchFn = std::function<void(const BatchRequestItem&, BatchResponseItem&)>;

        template<typename Request, typename Response, typename Handler>
        void register_method(std::string full_method, Handler&& handler)
        {
          std::string full_method_copy(full_method);

          methods_.emplace(
            std::move(full_method),
            [handler = std::forward<Handler>(handler), full_method_copy](
              const BatchRequestItem& batch_request,
              BatchResponseItem& batch_response)
            {
              Request request;
              if (!request.ParseFromString(batch_request.payload()))
              {
                batch_response.set_status_code(::grpc::StatusCode::INVALID_ARGUMENT);
                batch_response.set_status_message("Unable to parse payload");
                return;
              }

              Response response;
              ::grpc::Status status;
              handler(request, response, status);

              batch_response.set_status_code(status.error_code());
              batch_response.set_status_message(status.error_message());
              if (status.ok())
              {
                batch_response.set_payload(response.SerializeAsString());
              }
            });
        }

        void dispatch(const BatchRequestItem& request, BatchResponseItem& response) const
        {
          const auto it = methods_.find(request.full_method());
          if (it == methods_.end())
          {
            response.set_status_code(::grpc::StatusCode::UNIMPLEMENTED);
            response.set_status_message("Unknown method");
            return;
          }

          it->second(request, response);
        }

      private:
        std::unordered_map<std::string, DispatchFn> methods_;
      };

      template<typename Request, typename Response>
      class UnaryCall final : public UnaryCallBase<Request, Response>
      {
      public:
        using Base = UnaryCallBase<Request, Response>;
        using RequestMethod = void (
          adserver::user_info_svcs::user_bind::UserBindServerGrpc::AsyncService::*)(
            ::grpc::ServerContext*,
            Request*,
            ::grpc::ServerAsyncResponseWriter<Response>*,
            ::grpc::CompletionQueue*,
            ::grpc::ServerCompletionQueue*,
            void*);

        using Handler = void (
          ServiceImpl::*)(
            const Request&,
            Response&,
            ::grpc::Status&) const;

      public:
        UnaryCall(
          ServiceImpl* service_impl,
          RequestMethod request_method,
          Handler handler,
          ::grpc::ServerCompletionQueue* completion_queue)
          : Base(&service_impl->grpc_service, completion_queue),
            service_impl_(service_impl),
            request_rpc_(request_method),
            handler_rpc_(handler)
        {}

      protected:
        void request_method_() override
        {
          (this->service_->*request_rpc_)(
            &this->context_,
            &this->request_,
            &this->responder_,
            this->completion_queue_,
            this->completion_queue_,
            this);
        }

        void spawn_next_() override
        {
          auto* next_call = new UnaryCall(
            service_impl_,
            request_rpc_,
            handler_rpc_,
            this->completion_queue_);
          next_call->proceed(true);
        }

        void process_() override
        {
          ::grpc::Status status;
          (service_impl_->*handler_rpc_)(this->request_, this->response_, status);
          this->responder_.Finish(this->response_, status, this);
        }

      private:
        ServiceImpl* const service_impl_;
        const RequestMethod request_rpc_;
        const Handler handler_rpc_;
      };

      class BatchStreamCall final : public IAsyncCall
      {
      public:
        using Request = adserver::grpc::BatchRequest;
        using Response = adserver::grpc::BatchResponse;

      public:
        BatchStreamCall(
          ServiceImpl* service_impl,
          ::grpc::ServerCompletionQueue* completion_queue)
          : service_impl_(service_impl),
            completion_queue_(completion_queue),
            responder_(&context_),
            state_(State::Create)
        {}

        void proceed(bool ok) override
        {
          switch (state_)
          {
            case State::Create:
            {
              if (!ok)
              {
                delete this;
                return;
              }

              state_ = State::Start;
              service_impl_->batch_transport_service.Requeststream_batches(
                &context_,
                &responder_,
                completion_queue_,
                completion_queue_,
                this);
              return;
            }

            case State::Start:
            {
              if (!ok)
              {
                delete this;
                return;
              }

              auto* next_call = new BatchStreamCall(service_impl_, completion_queue_);
              next_call->proceed(true);
              state_ = State::Read;
              responder_.Read(&request_, this);
              return;
            }

            case State::Read:
            {
              if (!ok)
              {
                state_ = State::Finish;
                responder_.Finish(::grpc::Status::OK, this);
                return;
              }

              response_.Clear();
              service_impl_->handle_batch_request(request_, response_);
              state_ = State::Write;
              responder_.Write(response_, this);
              return;
            }

            case State::Write:
            {
              if (!ok)
              {
                state_ = State::Finish;
                responder_.Finish(::grpc::Status::OK, this);
                return;
              }

              state_ = State::Read;
              responder_.Read(&request_, this);
              return;
            }

            case State::Finish:
            {
              delete this;
              return;
            }
          }
        }

      private:
        enum class State
        {
          Create,
          Start,
          Read,
          Write,
          Finish
        };

      private:
        ServiceImpl* const service_impl_;
        ::grpc::ServerCompletionQueue* const completion_queue_;
        ::grpc::ServerContext context_;
        ::grpc::ServerAsyncReaderWriter<Response, Request> responder_;
        Request request_;
        Response response_;
        State state_;
      };

    private:
      const UserBindServerCore_var core_;
      MethodRegistry method_registry_;
    };

  protected:
    void activate_object_() override;

    void deactivate_object_() override;

    void wait_object_() override;

  private:
    void process_queue_loop_(::grpc::ServerCompletionQueue* completion_queue);

  private:
    const Logging::Logger_var logger_;
    const std::string bind_address_;
    const std::size_t worker_threads_;
    const std::unique_ptr<ServiceImpl> service_;

    std::unique_ptr<::grpc::Server> server_;
    std::vector<std::unique_ptr<::grpc::ServerCompletionQueue>> completion_queues_;
    std::vector<std::thread> workers_;
  };

  // UserBindServerGrpc::Impl impl
  UserBindServerGrpc::Impl::Impl(
    Logging::Logger* logger,
    UserBindServerCore* core,
    std::string_view bind_address)
    : logger_(ReferenceCounting::add_ref(logger)),
      bind_address_(bind_address),
      worker_threads_(std::max<std::size_t>(1, std::thread::hardware_concurrency())),
      service_(std::make_unique<ServiceImpl>(core))
  {}

  void
  UserBindServerGrpc::Impl::activate_object_()
  {
    ::grpc::ServerBuilder builder;
    builder.AddListeningPort(bind_address_, ::grpc::InsecureServerCredentials());
    builder.RegisterService(&service_->grpc_service);
    builder.RegisterService(&service_->batch_transport_service);

    completion_queues_.reserve(worker_threads_);
    for (std::size_t i = 0; i < worker_threads_; ++i)
    {
      completion_queues_.emplace_back(builder.AddCompletionQueue());
    }

    server_ = builder.BuildAndStart();

    ServiceImpl::CompletionQueues raw_completion_queues;
    raw_completion_queues.reserve(completion_queues_.size());
    for (auto& completion_queue : completion_queues_)
    {
      raw_completion_queues.push_back(completion_queue.get());
    }
    service_->start(raw_completion_queues);

    workers_.reserve(worker_threads_);
    for (std::size_t i = 0; i < worker_threads_; ++i)
    {
      auto* completion_queue = completion_queues_[i].get();
      workers_.emplace_back([this, completion_queue]() { process_queue_loop_(completion_queue); });
    }
  }

  void
  UserBindServerGrpc::Impl::deactivate_object_()
  {
    if (server_)
    {
      server_->Shutdown();
    }

    for (auto& completion_queue : completion_queues_)
    {
      if (completion_queue)
      {
        completion_queue->Shutdown();
      }
    }
  }

  void
  UserBindServerGrpc::Impl::wait_object_()
  {
    for (auto& worker : workers_)
    {
      if (worker.joinable())
      {
        worker.join();
      }
    }
    workers_.clear();

    if (server_)
    {
      server_->Wait();
      server_.reset();
    }

    completion_queues_.clear();
  }

  void
  UserBindServerGrpc::Impl::process_queue_loop_(::grpc::ServerCompletionQueue* completion_queue)
  {
    void* tag = nullptr;
    bool ok = false;

    while (completion_queue && completion_queue->Next(&tag, &ok))
    {
      auto* abstract_call = static_cast<IAsyncCall*>(tag);
      abstract_call->proceed(ok);
    }
  }

  // UserBindServerGrpc::Impl::ServiceImpl
  UserBindServerGrpc::Impl::ServiceImpl::ServiceImpl(
    UserBindServerCore* core)
    : core_(ReferenceCounting::add_ref(core))
  {
    method_registry_.register_method<
      adserver::user_info_svcs::user_bind::GetUserIdRequest,
      adserver::user_info_svcs::user_bind::GetUserIdResponse>(
      get_user_id_full_method,
      [this](
        const auto& request,
        auto& response,
        auto& status)
      {
        handle_get_user_id(request, response, status);
      });

    method_registry_.register_method<
      adserver::user_info_svcs::user_bind::AddUserIdRequest,
      adserver::user_info_svcs::user_bind::AddUserIdResponse>(
      add_user_id_full_method,
      [this](
        const auto& request,
        auto& response,
        auto& status)
      {
        handle_add_user_id(request, response, status);
      });
  }

  void
  UserBindServerGrpc::Impl::ServiceImpl::start(
    const CompletionQueues& completion_queues)
  {
    constexpr std::size_t registrations_per_queue = 64;

    for (auto* completion_queue : completion_queues)
    {
      for (std::size_t i = 0; i < registrations_per_queue; ++i)
      {
        auto* get_bind_request_call = new UnaryCall<
          adserver::user_info_svcs::user_bind::GetBindRequestRequest,
          adserver::user_info_svcs::user_bind::GetBindRequestResponse>(
          this,
          &adserver::user_info_svcs::user_bind::UserBindServerGrpc::AsyncService::Requestget_bind_request,
          &ServiceImpl::handle_get_bind_request,
          completion_queue);
        get_bind_request_call->proceed(true);

        auto* add_bind_request_call = new UnaryCall<
          adserver::user_info_svcs::user_bind::AddBindRequestRequest,
          adserver::user_info_svcs::user_bind::AddBindRequestResponse>(
          this,
          &adserver::user_info_svcs::user_bind::UserBindServerGrpc::AsyncService::Requestadd_bind_request,
          &ServiceImpl::handle_add_bind_request,
          completion_queue);
        add_bind_request_call->proceed(true);

        auto* get_user_id_call = new UnaryCall<
          adserver::user_info_svcs::user_bind::GetUserIdRequest,
          adserver::user_info_svcs::user_bind::GetUserIdResponse>(
          this,
          &adserver::user_info_svcs::user_bind::UserBindServerGrpc::AsyncService::Requestget_user_id,
          &ServiceImpl::handle_get_user_id,
          completion_queue);
        get_user_id_call->proceed(true);

        auto* add_user_id_call = new UnaryCall<
          adserver::user_info_svcs::user_bind::AddUserIdRequest,
          adserver::user_info_svcs::user_bind::AddUserIdResponse>(
          this,
          &adserver::user_info_svcs::user_bind::UserBindServerGrpc::AsyncService::Requestadd_user_id,
          &ServiceImpl::handle_add_user_id,
          completion_queue);
        add_user_id_call->proceed(true);

        auto* get_source_call = new UnaryCall<
          adserver::user_info_svcs::user_bind::GetSourceRequest,
          adserver::user_info_svcs::user_bind::GetSourceResponse>(
          this,
          &adserver::user_info_svcs::user_bind::UserBindServerGrpc::AsyncService::Requestget_source,
          &ServiceImpl::handle_get_source,
          completion_queue);
        get_source_call->proceed(true);

        auto* batch_stream_call = new BatchStreamCall(
          this,
          completion_queue);
        batch_stream_call->proceed(true);
      }
    }
  }

  void
  UserBindServerGrpc::Impl::ServiceImpl::handle_get_bind_request(
    const adserver::user_info_svcs::user_bind::GetBindRequestRequest& request,
    adserver::user_info_svcs::user_bind::GetBindRequestResponse& response,
    ::grpc::Status& result_status) const
  {
    try
    {
      const auto result = core_->get_bind_request(
        request.request_id(),
        GrpcAlgs::unpack_time(request.timestamp()));

      for (const auto& bind_user_id : result.bind_user_ids)
      {
        response.add_bind_user_ids(bind_user_id);
      }

      result_status = ::grpc::Status::OK;
    }
    catch (const UserBindServerCore::NotReady& ex)
    {
      result_status = make_status_(ex);
    }
    catch (const UserBindServerCore::ChunkNotFound& ex)
    {
      result_status = make_status_(ex);
    }
    catch (const UserBindServerCore::Exception& ex)
    {
      result_status = make_status_(ex);
    }
  }

  void
  UserBindServerGrpc::Impl::ServiceImpl::handle_add_bind_request(
    const adserver::user_info_svcs::user_bind::AddBindRequestRequest& request,
    adserver::user_info_svcs::user_bind::AddBindRequestResponse&,
    ::grpc::Status& result_status) const
  {
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
      result_status = make_status_(ex);
    }
    catch (const UserBindServerCore::ChunkNotFound& ex)
    {
      result_status = make_status_(ex);
    }
    catch (const UserBindServerCore::Exception& ex)
    {
      result_status = make_status_(ex);
    }
  }

  void
  UserBindServerGrpc::Impl::ServiceImpl::handle_get_user_id(
    const adserver::user_info_svcs::user_bind::GetUserIdRequest& request,
    adserver::user_info_svcs::user_bind::GetUserIdResponse& response,
    ::grpc::Status& result_status) const
  {
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

      const auto result = core_->get_user_id(req_info);
      response.set_user_id(GrpcAlgs::pack_user_id(result.user_id));
      response.set_min_age_reached(result.min_age_reached);
      response.set_created(result.created);
      response.set_invalid_operation(result.invalid_operation);
      response.set_user_found(result.user_found);

      result_status = ::grpc::Status::OK;
    }
    catch (const UserBindServerCore::NotReady& ex)
    {
      result_status = make_status_(ex);
    }
    catch (const UserBindServerCore::ChunkNotFound& ex)
    {
      result_status = make_status_(ex);
    }
    catch (const UserBindServerCore::Exception& ex)
    {
      result_status = make_status_(ex);
    }
  }

  void
  UserBindServerGrpc::Impl::ServiceImpl::handle_add_user_id(
    const adserver::user_info_svcs::user_bind::AddUserIdRequest& request,
    adserver::user_info_svcs::user_bind::AddUserIdResponse& response,
    ::grpc::Status& result_status) const
  {
    try
    {
      UserBindServerCore::AddUserRequestInfo req_info;
      req_info.id = request.id();

      req_info.timestamp = GrpcAlgs::unpack_time(request.timestamp());
      req_info.user_id = GrpcAlgs::unpack_user_id(request.user_id());

      const auto result = core_->add_user_id(req_info);

      response.set_merge_user_id(GrpcAlgs::pack_user_id(result.merge_user_id));
      response.set_invalid_operation(result.invalid_operation);

      result_status = ::grpc::Status::OK;
    }
    catch (const UserBindServerCore::NotReady& ex)
    {
      result_status = make_status_(ex);
    }
    catch (const UserBindServerCore::ChunkNotFound& ex)
    {
      result_status = make_status_(ex);
    }
    catch (const UserBindServerCore::Exception& ex)
    {
      result_status = make_status_(ex);
    }
  }

  void
  UserBindServerGrpc::Impl::ServiceImpl::handle_get_source(
    const adserver::user_info_svcs::user_bind::GetSourceRequest&,
    adserver::user_info_svcs::user_bind::GetSourceResponse& response,
    ::grpc::Status& result_status) const
  {
    try
    {
      const auto result = core_->get_source();
      for (const auto chunk_id : result.chunks)
      {
        response.add_chunks(chunk_id);
      }
      response.set_chunks_number(result.chunks_number);

      result_status = ::grpc::Status::OK;
    }
    catch (const UserBindServerCore::NotReady& ex)
    {
      result_status = make_status_(ex);
    }
    catch (const UserBindServerCore::ChunkNotFound& ex)
    {
      result_status = make_status_(ex);
    }
    catch (const UserBindServerCore::Exception& ex)
    {
      result_status = make_status_(ex);
    }
  }

  void
  UserBindServerGrpc::Impl::ServiceImpl::handle_batch_request(
    const adserver::grpc::BatchRequest& batch_request,
    adserver::grpc::BatchResponse& batch_response) const
  {
    for (int i = 0; i < batch_request.items_size(); ++i)
    {
      const auto& request_item = batch_request.items(i);
      auto* response_item = batch_response.add_items();
      response_item->set_request_id(request_item.request_id());
      method_registry_.dispatch(request_item, *response_item);
    }
  }

  // UserBindServerGrpc impl
  UserBindServerGrpc::UserBindServerGrpc(
    UserBindServerCore* core,
    Logging::Logger* logger,
    std::string_view bind_address,
    unsigned int bind_port)
    : bind_address_(std::string(bind_address) + ":" + std::to_string(bind_port)),
      impl_(new Impl(logger, core, bind_address_))
  {
    add_child_object(impl_);
  }

  UserBindServerGrpc::~UserBindServerGrpc() noexcept
  {}
}
