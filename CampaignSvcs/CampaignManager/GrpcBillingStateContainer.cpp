// UNIXCOMMONS
#include <Generics/Rand.hpp>

// THIS
#include <CampaignSvcs/CampaignManager/GrpcBillingStateContainer.hpp>

namespace AdServer::CampaignSvcs
{

namespace Aspect
{

const char BILLING_STATE_CONTAINER[] = "BillingStateContainer";

} // namespace Aspect

namespace
{

const std::size_t MAX_SIZE_SERVER_USE_TIMES = 10;
const Generics::Time MAX_SERVER_USE_TIME = Generics::Time(10);
const Generics::Time MIN_SERVER_USE_TIME = Generics::Time(1) / 100; // 10 ms
const Generics::Time REENABLE_INDEX_TIME = Generics::Time(10);
const bool DEBUG_BILLING_SERVER_CALL_ = false;

} // namespace

class GrpcBillingStateContainer::RecheckCCGTask final
  : public Generics::TaskGoal
{
public:
  explicit RecheckCCGTask(
    GrpcBillingStateContainer* billing_state_container,
    Generics::TaskRunner* task_runner)
    : Generics::TaskGoal(task_runner),
      billing_state_container_(billing_state_container)
  {
  }

  void execute() noexcept override
  {
    billing_state_container_->run_recheck_ccgs();
  }

private:
  GrpcBillingStateContainer* billing_state_container_;
};

GrpcBillingStateContainer::GrpcBillingStateContainer(
  Generics::ActiveObjectCallback* callback,
  Logger* logger,
  const std::size_t max_use_count,
  const bool optimize_campaign_ctr,
  TaskProcessor& task_processor,
  const SchedulerPtr& scheduler,
  const Endpoints& endpoints,
  const ConfigPoolClient& config_pool_client,
  const std::size_t grpc_client_timeout_ms)
  : logger_(ReferenceCounting::add_ref(logger)),
    task_runner_(new Generics::TaskRunner(callback, 1)),
    planner_(new Generics::Planner(callback)),
    task_processor_(task_processor),
    factory_client_holder_(new FactoryClientHolder(
      logger_.in(),
      scheduler,
      config_pool_client,
      task_processor)),
    grpc_client_timeout_ms_(grpc_client_timeout_ms),
    max_use_count_(max_use_count),
    optimize_campaign_ctr_(optimize_campaign_ctr)
{
  if (endpoints.empty())
  {
    Stream::Error stream;
    stream << FNS
           << "endpoints is empty";
    throw Exception(stream);
  }

  const std::size_t size = endpoints.size();
  client_holders_.reserve(size);
  for (const auto& endpoint : endpoints)
  {
    const auto client_holder = factory_client_holder_->create(
      endpoint.host,
      endpoint.port);
    client_holders_.emplace_back(client_holder);
  }

  try
  {
    add_child_object(task_runner_);
    add_child_object(planner_);
  }
  catch (const Generics::CompositeActiveObject::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "CompositeActiveObject::Exception caught: "
           << exc.what();
    throw Exception(stream);
  }

  Generics::Task_var msg = new RecheckCCGTask(this, task_runner_);
  task_runner_->enqueue_task(msg);
}

GrpcBillingStateContainer::BidCheckResult
GrpcBillingStateContainer::reserve_bid(
  const Generics::Time& time,
  std::uint32_t account_id,
  std::uint32_t advertiser_id,
  std::uint32_t campaign_id,
  std::uint32_t ccg_id,
  const RevenueDecimal& amount) noexcept
{
  BidCheckResult result;
  result.deactivate_account = false;
  result.deactivate_advertiser = false;
  result.deactivate_campaign = false;
  result.deactivate_ccg = false;
  result.available = true;

  return result;
}

GrpcBillingStateContainer::BidCheckResult
GrpcBillingStateContainer::check_available_bid(
  const Generics::Time& time,
  const std::uint32_t account_id,
  const std::uint32_t advertiser_id,
  const std::uint32_t campaign_id,
  const std::uint32_t ccg_id,
  const CTRDecimal& ctr,
  const AvailableAndMinCTRSetter* const ccg_setter) noexcept
{
  std::size_t try_i = 0;
  bool full_deactivation_check = false;
  bool available = false;
  RevenueDecimal goal_ctr = RevenueDecimal::ZERO;

  while (true)
  {
    auto service_index = get_service_index(
      nullptr,
      time,
      ccg_id,
      nullptr,
      ccg_setter);

    if (!service_index)
    {
      full_deactivation_check = true;
      break;
    }

    if (try_i >= max_try_count_)
    {
      break;
    }

    auto result = check_available_bid(
      *service_index,
      time,
      account_id,
      advertiser_id,
      campaign_id,
      ccg_id,
      ctr,
      optimize_campaign_ctr_);
    if (!result || result->has_error() ||
      !result->info().return_value().available())
    {
      std::lock_guard lock(cache_mutex_);
      cache_[ccg_id].disabled_indexes.insert(
        std::make_pair(*service_index, time));
    }
    else
    {
      available = true;
      goal_ctr = GrpcAlgs::unpack_decimal<RevenueDecimal>(
        result->info().return_value().goal_ctr());
      break;
    }

    try_i += 1;
  }

  const bool deactivate_ccg = full_deactivation_check;

  BidCheckResult result;
  result.deactivate_account = false;
  result.deactivate_advertiser = false;
  result.deactivate_campaign = false;
  result.deactivate_ccg = deactivate_ccg;
  result.available = available;
  result.goal_ctr = goal_ctr;

  if (deactivate_ccg)
  {
    ccg_set_available(
      ccg_setter,
      ccg_id,
      false,
      goal_ctr,
      time);
  }

  return result;
}

GrpcBillingStateContainer::BidCheckResult
GrpcBillingStateContainer::confirm_bid(
  const Generics::Time& time,
  const std::uint32_t account_id,
  const std::uint32_t advertiser_id,
  const std::uint32_t campaign_id,
  const std::uint32_t ccg_id,
  const RevenueDecimal& account_spent_amount,
  const RevenueDecimal& spent_amount,
  const RevenueDecimal& ctr,
  const RevenueDecimal& imps,
  const RevenueDecimal& clicks,
  const AvailableAndMinCTRSetter* ccg_setter) noexcept
{
  ConfirmBidInfo confirm_bid_info;
  confirm_bid_info.time = time;
  confirm_bid_info.account_id = account_id;
  confirm_bid_info.advertiser_id = advertiser_id;
  confirm_bid_info.campaign_id = campaign_id;
  confirm_bid_info.ccg_id = ccg_id;
  confirm_bid_info.ctr = ctr;
  confirm_bid_info.account_spent_budget = account_spent_amount;
  confirm_bid_info.spent_budget = spent_amount;
  confirm_bid_info.reserved_budget = RevenueDecimal::ZERO;
  confirm_bid_info.imps = imps;
  confirm_bid_info.clicks = clicks;
  confirm_bid_info.forced = false;

  DisabledIndexMap bad_indexes;
  bool available = false;
  RevenueDecimal goal_ctr = RevenueDecimal::ZERO;

  while (true)
  {
    auto service_index = get_service_index(
      nullptr,
      time,
      ccg_id,
      confirm_bid_info.forced ? &bad_indexes : nullptr,
      nullptr);

    if (!service_index)
    {
      if (confirm_bid_info.forced)
      {
        break;
      }
      else
      {
        confirm_bid_info.forced = true;
        continue;
      }
    }

    auto response = confirm_bid(
      *service_index,
      confirm_bid_info);

    if (!response || response->has_error())
    {
      if (confirm_bid_info.forced)
      {
        bad_indexes.insert(
          std::make_pair(*service_index, time));
      }
      else
      {
        std::lock_guard lock(cache_mutex_);
        cache_[ccg_id].disabled_indexes.insert(
          std::make_pair(*service_index, time));
      }
    }
    else
    {
      const auto& return_value = response->info().return_value();
      available = return_value.available();
      goal_ctr = GrpcAlgs::unpack_decimal<RevenueDecimal>(return_value.goal_ctr());
      break;
    }
  }

  BidCheckResult result;
  result.deactivate_account = false;
  result.deactivate_campaign = false;
  result.deactivate_advertiser = false;
  result.deactivate_ccg = !available;
  result.available = available;
  result.goal_ctr = goal_ctr;

  if (!available)
  {
    ccg_set_available(
      ccg_setter,
      ccg_id,
      false,
      goal_ctr,
      time);
  }

  return result;
}

template<class Client, class Request, class Response, class ...Args>
std::unique_ptr<Response>
GrpcBillingStateContainer::do_request_service(
  const ClientHolderPtr& client_holder,
  const Args& ...args) noexcept
{
  for (std::size_t i = 1; i <= 5; i += 1)
  {
    auto response = try_do_request_service<Client, Request, Response, Args...>(
      client_holder,
      args...);
    if (!response)
    {
      continue;
    }

    const auto data_case = response->data_case();
    if (data_case == Response::DataCase::kInfo)
    {
      return response;
    }
    else if (data_case == Response::DataCase::kError)
    {
      std::ostringstream stream;
      stream << FNS
             << "Error type=";

      const auto& error = response->error();
      const auto error_type = error.type();
      switch (error_type)
      {
        case AdServer::CampaignSvcs::Billing::Proto::Error_Type::Error_Type_Implementation:
        {
          stream << "Implementation";
          break;
        }
        case AdServer::CampaignSvcs::Billing::Proto::Error_Type::Error_Type_NotReady:
        {
          stream << "NotReady";
          break;
        }
        default:
        {
          Stream::Error stream;
          stream << FNS
                 << "Unknown error type";
          break;
        }
      }

      stream << ", description="
             << error.description();
      logger_->error(
        stream.str(),
        Aspect::BILLING_STATE_CONTAINER);

      return response;
    }
    else
    {
      Stream::Error stream;
      stream << FNS
             << "Unknown response type";
      logger_->error(
        stream.str(),
        Aspect::BILLING_STATE_CONTAINER);

      return response;
    }
  }

  return {};
}

template<class Client, class Request, class Response, class ...Args>
std::unique_ptr<Response> GrpcBillingStateContainer::try_do_request_service(
  const ClientHolderPtr& client_holder,
  const Args& ...args) noexcept
{
  try
  {
    std::unique_ptr<Request> request;
    if constexpr (std::is_same_v<Request, CheckAvailableBidRequest>)
    {
      request = create_check_available_bid_request(args...);
    }
    else if constexpr (std::is_same_v<Request, ReserveBidRequest>)
    {
      request = create_reserve_bid_request(args...);
    }
    else if constexpr (std::is_same_v<Request, ConfirmBidRequest>)
    {
      request = create_confirm_bid_request(args...);
    }
    else if constexpr (std::is_same_v<Request, AddAmountRequest>)
    {
      request = create_add_amount_request(args...);
    }
    else
    {
      static_assert(GrpcAlgs::AlwaysFalseV<Request>);
    }

    auto response = client_holder->template do_request<Client, Request, Response>(
      std::move(request),
      grpc_client_timeout_ms_);

    return response;
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << exc.what();
    logger_->error(
      stream.str(),
      Aspect::BILLING_STATE_CONTAINER);
  }
  catch (...)
  {
    Stream::Error stream;
    stream << FNS
           << "Unknown error";
    logger_->error(
      stream.str(),
      Aspect::BILLING_STATE_CONTAINER);
  }

  return {};
}

template<class Client, class Request, class Response, class ...Args>
std::unique_ptr<Response>
GrpcBillingStateContainer::do_request(
  const std::size_t index,
  const Args& ...args) noexcept
{
  if (client_holders_.empty())
  {
    Stream::Error stream;
    stream << FNS
           << "client_holders is empty";
    logger_->emergency(
      stream.str(),
      Aspect::BILLING_STATE_CONTAINER);

    return {};
  }

  if (index >= client_holders_.size())
  {
    Stream::Error stream;
    stream << FNS
           << "index must be less then number services";
    logger_->emergency(
      stream.str(),
      Aspect::BILLING_STATE_CONTAINER);

    return {};
  }

  const auto& client_holder = client_holders_[index];
  auto response = do_request_service<Client, Request, Response>(
    client_holder,
    args...);

  return response;
}

GrpcBillingStateContainer::CheckAvailableBidRequestPtr
GrpcBillingStateContainer::create_check_available_bid_request(
  const Generics::Time& time,
  const std::uint32_t account_id,
  const std::uint32_t advertiser_id,
  const std::uint32_t campaign_id,
  const std::uint32_t ccg_id,
  const CTRDecimal& ctr,
  const bool optimize_campaign_ctr)
{
  auto request = std::make_unique<CheckAvailableBidRequest>();
  auto* const requst_info = request->mutable_request_info();
  requst_info->set_time(GrpcAlgs::pack_time(time));
  requst_info->set_account_id(account_id);
  requst_info->set_advertiser_id(advertiser_id);
  requst_info->set_campaign_id(campaign_id);
  requst_info->set_ccg_id(ccg_id);
  requst_info->set_ctr(GrpcAlgs::pack_decimal(ctr));
  requst_info->set_optimize_campaign_ctr(optimize_campaign_ctr);

  return request;
}

GrpcBillingStateContainer::ReserveBidRequestPtr
GrpcBillingStateContainer::create_reserve_bid_request(
  const Generics::Time& time,
  const std::uint32_t account_id,
  const std::uint32_t advertiser_id,
  const std::uint32_t campaign_id,
  const std::uint32_t ccg_id,
  const RevenueDecimal& reserve_budget)
{
  auto request = std::make_unique<ReserveBidRequest>();
  auto* const requset_info = request->mutable_request_info();
  requset_info->set_time(GrpcAlgs::pack_time(time));
  requset_info->set_account_id(account_id);
  requset_info->set_advertiser_id(advertiser_id);
  requset_info->set_campaign_id(campaign_id);
  requset_info->set_ccg_id(ccg_id);
  requset_info->set_reserve_budget(
    GrpcAlgs::pack_decimal(reserve_budget));

  return request;
}

GrpcBillingStateContainer::ConfirmBidRequestPtr
GrpcBillingStateContainer::create_confirm_bid_request(
  const ConfirmBidInfo& request_info)
{
  auto request = std::make_unique<ConfirmBidRequest>();
  auto* const proto_request_info = request->mutable_request_info();
  proto_request_info->set_time(GrpcAlgs::pack_time(request_info.time));
  proto_request_info->set_account_id(request_info.account_id);
  proto_request_info->set_advertiser_id(request_info.advertiser_id);
  proto_request_info->set_campaign_id(request_info.campaign_id);
  proto_request_info->set_ccg_id(request_info.ccg_id);
  proto_request_info->set_ctr(GrpcAlgs::pack_decimal(request_info.ctr));
  proto_request_info->set_account_spent_budget(
    GrpcAlgs::pack_decimal(request_info.account_spent_budget));
  proto_request_info->set_spent_budget(
    GrpcAlgs::pack_decimal(request_info.spent_budget));
  proto_request_info->set_reserved_budget(
    GrpcAlgs::pack_decimal(request_info.reserved_budget));
  proto_request_info->set_imps(
    GrpcAlgs::pack_decimal(request_info.imps));
  proto_request_info->set_clicks(
    GrpcAlgs::pack_decimal(request_info.clicks));
  proto_request_info->set_forced(request_info.forced);

  return request;
}

GrpcBillingStateContainer::AddAmountRequestPtr
GrpcBillingStateContainer::create_add_amount_request(
  const std::vector<ConfirmBidInfo>& requests)
{
  auto request = std::make_unique<AddAmountRequest>();
  auto* const request_seq = request->mutable_request_seq();
  request_seq->Reserve(requests.size());
  for (const auto& request : requests)
  {
    auto* const proto_request_info = request_seq->Add();
    proto_request_info->set_time(GrpcAlgs::pack_time(request.time));
    proto_request_info->set_account_id(request.account_id);
    proto_request_info->set_advertiser_id(request.advertiser_id);
    proto_request_info->set_campaign_id(request.campaign_id);
    proto_request_info->set_ccg_id(request.ccg_id);
    proto_request_info->set_ctr(GrpcAlgs::pack_decimal(request.ctr));
    proto_request_info->set_account_spent_budget(
      GrpcAlgs::pack_decimal(request.account_spent_budget));
    proto_request_info->set_spent_budget(
      GrpcAlgs::pack_decimal(request.spent_budget));
    proto_request_info->set_reserved_budget(
      GrpcAlgs::pack_decimal(request.reserved_budget));
    proto_request_info->set_imps(
      GrpcAlgs::pack_decimal(request.imps));
    proto_request_info->set_clicks(
      GrpcAlgs::pack_decimal(request.clicks));
    proto_request_info->set_forced(request.forced);
  }

  return request;
}

GrpcBillingStateContainer::CheckAvailableBidResponsePtr
GrpcBillingStateContainer::check_available_bid(
  const std::size_t index,
  const Generics::Time& time,
  const std::uint32_t account_id,
  const std::uint32_t advertiser_id,
  const std::uint32_t campaign_id,
  const std::uint32_t ccg_id,
  const CTRDecimal& ctr,
  const bool optimize_campaign_ctr) noexcept
{
  using CheckAvailableBidClient =
    AdServer::CampaignSvcs::Billing::Proto::BillingService_check_available_bid_ClientPool;

  return do_request<
    CheckAvailableBidClient,
    CheckAvailableBidRequest,
    CheckAvailableBidResponse>(
      index,
      time,
      account_id,
      advertiser_id,
      campaign_id,
      ccg_id,
      ctr,
      optimize_campaign_ctr);
}

GrpcBillingStateContainer::ReserveBidResponsePtr
GrpcBillingStateContainer::reserve_bid(
  const std::size_t index,
  const Generics::Time& time,
  const std::uint32_t account_id,
  const std::uint32_t advertiser_id,
  const std::uint32_t campaign_id,
  const std::uint32_t ccg_id,
  const RevenueDecimal& reserve_budget) noexcept
{
  using ReserveBidClient =
    AdServer::CampaignSvcs::Billing::Proto::BillingService_reserve_bid_ClientPool;

  return do_request<
    ReserveBidClient,
    ReserveBidRequest,
    ReserveBidResponse>(
      index,
      time,
      account_id,
      advertiser_id,
      campaign_id,
      ccg_id,
      reserve_budget);
}

GrpcBillingStateContainer::ConfirmBidResponsePtr
GrpcBillingStateContainer::confirm_bid(
  const std::size_t index,
  const ConfirmBidInfo& request_info) noexcept
{
  using ConfirmBidClient =
    AdServer::CampaignSvcs::Billing::Proto::BillingService_confirm_bid_ClientPool;

  return do_request<
    ConfirmBidClient,
    ConfirmBidRequest,
    ConfirmBidResponse>(
      index,
      request_info);
}

GrpcBillingStateContainer::AddAmountResponsePtr
GrpcBillingStateContainer::add_amount(
  const std::size_t index,
  const std::vector<ConfirmBidInfo>& requests) noexcept
{
  using AddAmountClient =
    AdServer::CampaignSvcs::Billing::Proto::BillingService_add_amount_ClientPool;

  return do_request<
    AddAmountClient,
    AddAmountRequest,
    AddAmountResponse>(
      index,
      requests);
}

std::size_t GrpcBillingStateContainer::next_index(
  const std::size_t server_index) const noexcept
{
  return (server_index + 1) % client_holders_.size();
}

void GrpcBillingStateContainer::fill_planned_server_switch_time(
  const std::uint32_t ccg_id,
  const Generics::Time& now,
  CCGState& ccg_state) noexcept
{
  const Generics::Time last_server_switch_time = ccg_state.last_server_switch_time;
  if (!ccg_state.server_use_times.empty())
  {
    Generics::Time sum_time;
    for (const auto& time : ccg_state.server_use_times)
    {
      sum_time += time;
    }

    const Generics::Time avg_time =
      sum_time / ccg_state.server_use_times.size();
    const Generics::Time use_time = std::max(
      std::min(avg_time, MAX_SERVER_USE_TIME),
      MIN_SERVER_USE_TIME);

    ccg_state.last_server_switch_time = now;
    ccg_state.planned_server_switch_time = now + use_time;
  }
  else
  {
    ccg_state.planned_server_switch_time = now + MAX_SERVER_USE_TIME;
  }

  if (last_server_switch_time != Generics::Time::ZERO)
  {
    ccg_state.server_use_times.push_back(now - last_server_switch_time);
    while (ccg_state.server_use_times.size() > MAX_SIZE_SERVER_USE_TIMES)
    {
      ccg_state.server_use_times.pop_front();
    }
  }

  std::lock_guard lock(add_recheck_ccgs_mutex_);
  add_recheck_ccgs_[ccg_id] = ccg_state.planned_server_switch_time;
}

std::optional<std::size_t> GrpcBillingStateContainer::get_service_index(
  bool* switched,
  const Generics::Time& now,
  const std::uint32_t ccg_id,
  const DisabledIndexMap* disabled_indexes_in,
  const AvailableAndMinCTRSetter* ccg_setter)
{
  if (switched)
  {
    *switched = false;
  }

  CAvailableAndMinCTRSetter_var new_ccg_setter =
    ReferenceCounting::add_ref(ccg_setter);

  std::lock_guard lock(cache_mutex_);
  CCGState& ccg_state = cache_[ccg_id];

  if (new_ccg_setter)
  {
    ccg_state.ccg_setter.swap(new_ccg_setter);
  }

  const DisabledIndexMap& disabled_indexes = disabled_indexes_in ?
    *disabled_indexes_in : ccg_state.disabled_indexes;

  if (ccg_state.use_count == -1)
  {
    ccg_state.active_index =
      Generics::safe_rand(client_holders_.size());
    ccg_state.use_count = 0;
    ccg_state.last_server_switch_time = now;

    fill_planned_server_switch_time(
      ccg_id,
      now,
      ccg_state);
  }

  const bool active_index_is_bad =
    disabled_indexes.find(ccg_state.active_index) != std::end(disabled_indexes);

  if (active_index_is_bad ||
    ++ccg_state.use_count > max_use_count_ ||
    now > ccg_state.planned_server_switch_time)
  {
    const std::size_t count_servers = client_holders_.size();
    for (std::size_t i = 0; i < count_servers; i += 1)
    {
       ccg_state.active_index = next_index(ccg_state.active_index);
       ccg_state.use_count = 1;

       if (disabled_indexes.find(ccg_state.active_index) == std::end(disabled_indexes))
       {
         if (switched)
         {
           *switched = true;
         }

         if (ccg_state.last_server_switch_time != Generics::Time::ZERO)
         {
           const Generics::Time server_use_time =
             now - ccg_state.last_server_switch_time;
           ccg_state.server_use_times.push_back(server_use_time);
           while (ccg_state.server_use_times.size() > MAX_SIZE_SERVER_USE_TIMES)
           {
             ccg_state.server_use_times.pop_front();
           }

           fill_planned_server_switch_time(
             ccg_id,
             now,
             ccg_state);
         }

         return ccg_state.active_index;
       }
    }

    return {};
  }

  return ccg_state.active_index;
}

void GrpcBillingStateContainer::ccg_set_available(
  const AvailableAndMinCTRSetter* ccg_setter,
  const std::uint32_t ccg_id,
  const bool available,
  const RevenueDecimal& goal_ctr,
  const Generics::Time& now) noexcept
{
  if (ccg_setter)
  {
    ccg_setter->set_available(available, goal_ctr);
  }

  if (!available)
  {
    std::lock_guard lock(add_recheck_ccgs_mutex_);
    Generics::Time& planned_time = add_recheck_ccgs_[ccg_id];
    if (planned_time != Generics::Time::ZERO)
    {
      planned_time = std::min(planned_time, now + REENABLE_INDEX_TIME);
    }
    else
    {
      planned_time = now + REENABLE_INDEX_TIME;
    }
  }
}

void GrpcBillingStateContainer::run_recheck_ccgs() noexcept
{
  CCGCheckTimes add_recheck_ccgs;

  {
    std::lock_guard lock(add_recheck_ccgs_mutex_);
    add_recheck_ccgs_.swap(add_recheck_ccgs);
  }

  for (auto ccg_it = std::begin(add_recheck_ccgs);
       ccg_it != std::end(add_recheck_ccgs);
       ++ccg_it)
  {
    Generics::Time& check_time = recheck_ccgs_[ccg_it->first];
    check_time = std::max(check_time, ccg_it->second);
  }

  for (auto it = std::begin(add_recheck_ccgs);
       it != std::end(add_recheck_ccgs);
       ++it)
  {
    auto prev_it = recheck_ccgs_.find(it->first);
    if (prev_it != std::end(recheck_ccgs_))
    {
      Generics::Time& target_time = recheck_ccgs_[it->first];
      target_time = std::min(target_time, it->second);
    }
    else
    {
      recheck_ccgs_.insert(*it);
    }
  }

  const Generics::Time now = Generics::Time::get_time_of_day();
  const Generics::Time reenable_time = now - REENABLE_INDEX_TIME;

  for (auto it = std::begin(recheck_ccgs_);
       it != std::end(recheck_ccgs_);)
  {
    if (now >= it->second)
    {
      const auto ccg_id = it->first;
      CAvailableAndMinCTRSetter_var ccg_setter;

      {
        std::lock_guard lock(cache_mutex_);
        CCGState& ccg_state = cache_[ccg_id];
        ccg_setter = ReferenceCounting::add_ref(ccg_state.ccg_setter);
        for (auto disable_index_it = std::begin(ccg_state.disabled_indexes);
            disable_index_it != std::end(ccg_state.disabled_indexes); )
        {
          if (disable_index_it->second < reenable_time)
          {
            ccg_state.disabled_indexes.erase(disable_index_it++);
          }
          else
          {
            ++disable_index_it;
          }
        }
      }

      get_service_index(
        nullptr,
        now,
        ccg_id,
        nullptr,
        nullptr);

      ccg_set_available(
        ccg_setter,
        ccg_id,
        true,
        RevenueDecimal::ZERO,
        now);

      recheck_ccgs_.erase(it++);
    }
    else
    {
      ++it;
    }
  }

  try
  {
    Generics::Goal_var msg = new RecheckCCGTask(
      this,
      task_runner_);
    planner_->schedule(
      msg,
      Generics::Time::get_time_of_day() + Generics::Time::ONE_SECOND / 10);
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "Can't schedule next recheck ccgs task. "
           << "eh::Exception caught:"
           << exc.what();
    logger_->log(stream.str(),
                 Logging::Logger::ERROR,
                 Aspect::BILLING_STATE_CONTAINER,
                 "ADS-IMPL-?");
  }
  catch (...)
  {
    Stream::Error stream;
    stream << FNS
           << "Can't schedule next recheck ccgs task. "
           << "eh::Exception caught: Unknown error";
    logger_->log(stream.str(),
                 Logging::Logger::ERROR,
                 Aspect::BILLING_STATE_CONTAINER,
                 "ADS-IMPL-?");
  }
}

void GrpcBillingStateContainer::clear_cache() noexcept
{
  std::lock_guard lock(cache_mutex_);
  cache_.clear();
}

} // namespace AdServer::CampaignSvcs