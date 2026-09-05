#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <any>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>

#include <sys/resource.h>

#include <Generics/AppUtils.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/StreamLogger.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/UserIdBlackList.hpp>
#include <xsd/CampaignSvcs/DomainConfig.hpp>

#define private public
#include <Frontends/CommonModule/CommonModule.hpp>
#undef private

#include <Frontends/Modules/BiddingFrontend/BiddingFrontendCore.hpp>

namespace
{
  namespace CM = adserver::campaign_svcs::campaign_manager;
  namespace CH = adserver::channel_svcs::channel_server;
  namespace UIB = adserver::user_info_svcs::user_bind;
  namespace UIM = adserver::user_info_svcs::user_info_manager;

  constexpr const char DEFAULT_URL[] = "http://localhost:25080/openrtb?aid=9803&src=msc";

  constexpr const char DEFAULT_BODY[] = R"JSON({"id":"b7495c64-edfc-48b7-5c3b-99fd7e401730","imp":[{"id":"1","banner":{"format":[{"w":300,"h":250}],"mimes":["image/jpg","image/jpeg","image/png","text/html","image/gif","text/javascript"],"api":[1,2]},"bidfloor":0.0,"bidfloorcur":"RUB","secure":1}],"site":{"id":"11","name":"rutube.ru","domain": "rutube.ru","page":"http://rutube.ru","publisher":{"id":"1"}},"device":{"ua":"Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:73.0) Gecko/20100101 Firefox/73.0","geo":{"country":"RUS"},"ip":"213.33.171.240","devicetype":2,"os":"Ubuntu","osv":"Ubuntu"},"user":{"id":"TEST30"},"at":2,"tmax":150,"cur":["RUB"],"badv":["switch-to-juul.ru","olimp.bet","myglo.ru","juul.ru","iqos.ru"]})JSON";

  struct Options
  {
    std::string command;
    std::uint64_t count = 1;
    std::string url = DEFAULT_URL;
    std::string body = DEFAULT_BODY;
    std::string body_file;
    std::string record_dir;
    std::string fe_config;
    std::string domain_config;
    bool print_calls = false;
  };

  struct CpuTimes
  {
    double user = 0.0;
    double sys = 0.0;
  };

  void
  print_usage()
  {
    std::cerr
      << "Usage: BiddingFrontendCoreTest perf [OPTIONS]\n"
      << "Options:\n"
      << "  --count <N>          handle_request calls count (default: 1)\n"
      << "  --url <URL>          request URL\n"
      << "  --body <JSON>        OpenRTB request body\n"
      << "  --body-file <PATH>   OpenRTB request body file\n"
      << "  --record-dir <PATH>  binary protobuf responses directory\n"
      << "  --fe-config <PATH>   FeConfig.xml for UserIdConfig/domain_config_path\n"
      << "  --domain-config <P>  domain config for referer parsing\n"
      << "  --print-calls        print mock client calls\n";
  }

  Options
  parse_options(int argc, char** argv)
  {
    using namespace Generics::AppUtils;

    if (argc < 2)
    {
      print_usage();
      throw std::runtime_error("command is required");
    }

    Options options;
    options.command = argv[1];

    Option<unsigned long> opt_count(1);
    StringOption opt_url(DEFAULT_URL);
    StringOption opt_body(DEFAULT_BODY);
    StringOption opt_body_file;
    StringOption opt_record_dir;
    StringOption opt_fe_config;
    StringOption opt_domain_config;
    CheckOption opt_print_calls;
    CheckOption opt_help;

    Args args(-1);
    args.add(equal_name("count"), opt_count);
    args.add(equal_name("url"), opt_url);
    args.add(equal_name("body"), opt_body);
    args.add(equal_name("body-file"), opt_body_file);
    args.add(equal_name("record-dir"), opt_record_dir);
    args.add(equal_name("fe-config"), opt_fe_config);
    args.add(equal_name("domain-config"), opt_domain_config);
    args.add(equal_name("print-calls"), opt_print_calls);
    args.add(equal_name("help") || short_name("h"), opt_help);
    args.parse(argc - 2, argv + 2);

    if (opt_help.enabled())
    {
      print_usage();
      std::exit(0);
    }

    options.count = *opt_count;
    options.url = *opt_url;
    options.body = *opt_body;
    options.body_file = *opt_body_file;
    options.record_dir = *opt_record_dir;
    options.fe_config = *opt_fe_config;
    options.domain_config = *opt_domain_config;
    options.print_calls = opt_print_calls.enabled();

    if (options.command != "perf")
    {
      throw std::runtime_error("unknown command '" + options.command + "'");
    }

    if (options.count == 0)
    {
      throw std::runtime_error("--count must be > 0");
    }

    if (options.fe_config.empty() && options.domain_config.empty())
    {
      throw std::runtime_error(
        "--fe-config or --domain-config is required for OpenRTB referer parsing");
    }

    return options;
  }

  std::string
  read_file(const std::string& path)
  {
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
      throw std::runtime_error("can't open '" + path + "'");
    }

    std::ostringstream out;
    out << file.rdbuf();
    return out.str();
  }

  template<typename Message>
  void
  load_binary_proto(Message& message, const std::string& record_dir, const char* file_name)
  {
    if (record_dir.empty())
    {
      return;
    }

    const std::string path = record_dir + "/" + file_name;
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
      return;
    }

    if (!message.ParseFromIstream(&file))
    {
      throw std::runtime_error("can't parse protobuf fixture '" + path + "'");
    }
  }

  CpuTimes
  current_cpu_times()
  {
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0)
    {
      throw std::runtime_error("getrusage failed");
    }
    return {
      usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0,
      usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0
    };
  }

  std::string
  format_float(double value)
  {
    std::ostringstream out;
    out.setf(std::ios::fixed);
    out.precision(6);
    out << value;
    return out.str();
  }

  void
  init_common_module(
    AdServer::CommonModule& common_module,
    Logging::Logger* logger,
    const std::string& fe_config_path,
    const std::string& domain_config_path)
  {
    std::string actual_domain_config_path = domain_config_path;

    if (!fe_config_path.empty())
    {
      Config::ErrorHandler error_handler;
      std::unique_ptr<xsd::AdServer::Configuration::FeConfigurationType>
        fe_config(xsd::AdServer::Configuration::FeConfiguration(
          fe_config_path.c_str(),
          error_handler).release());

      if (error_handler.has_errors())
      {
        std::string error;
        throw std::runtime_error(error_handler.text(error));
      }

      if (!fe_config->CommonFeConfiguration().present())
      {
        throw std::runtime_error("CommonFeConfiguration not presented.");
      }

      const auto& common_config = fe_config->CommonFeConfiguration().get();
      const auto& user_id_config = common_config.UserIdConfig();

      AdServer::Commons::UserIdBlackList uid_blacklist;
      uid_blacklist.load(user_id_config, logger, "CommonModule");

      common_module.user_id_controller_ = new AdServer::UserIdController(
        user_id_config.public_key().c_str(),
        user_id_config.temp_public_key().c_str(),
        user_id_config.private_key().c_str(),
        user_id_config.ssp_public_key().c_str(),
        user_id_config.ssp_private_key().c_str(),
        user_id_config.ssp_uid_key(),
        user_id_config.cache_size(),
        user_id_config.temp_cache_size(),
        user_id_config.ssp_cache_size(),
        uid_blacklist);

      if (actual_domain_config_path.empty())
      {
        actual_domain_config_path = common_config.domain_config_path();
      }
    }

    if (actual_domain_config_path.empty())
    {
      return;
    }

    Config::ErrorHandler error_handler;
    std::unique_ptr<xsd::AdServer::Configuration::DomainConfigurationType>
      domain_config(xsd::AdServer::Configuration::DomainConfiguration(
        actual_domain_config_path.c_str(),
        error_handler).release());

    if (error_handler.has_errors())
    {
      std::string error;
      throw std::runtime_error(error_handler.text(error));
    }

    common_module.domain_parser_ = new AdServer::CampaignSvcs::DomainParser(*domain_config);
  }

  AdServer::Bidding::SourceTraits
  make_default_source_traits()
  {
    AdServer::Bidding::SourceTraits result{};
    result.instantiate_type = AdServer::CampaignSvcs::AIT_BODY;
    result.notice_instantiate_type = AdServer::Bidding::SourceTraits::NIT_NONE;
    result.vast_instantiate_type = AdServer::CampaignSvcs::AIT_BODY;
    result.vast_notice_instantiate_type = AdServer::Bidding::SourceTraits::NIT_NONE;
    result.native_notice_instantiate_type = AdServer::Bidding::SourceTraits::NIT_NONE;
    result.ipw_extension = false;
    result.truncate_domain = false;
    result.fill_adid = false;
    result.native_ads_instantiate_type = AdServer::Bidding::SourceTraits::NAIT_NONE;
    result.skip_ext_category = false;
    result.erid_return_type = AdServer::Bidding::SourceTraits::ERIDRT_EXT_BUZSAPE;
    return result;
  }

  class TestHttpRequestHolder final : public FCGI::HttpRequestHolder
  {
  public:
    TestHttpRequestHolder(std::string url, std::string body)
      : url_(std::move(url)),
        body_(std::move(body))
    {
      const auto scheme_pos = url_.find("://");
      const auto path_pos = scheme_pos == std::string::npos ? 0 : url_.find('/', scheme_pos + 3);
      std::string path = path_pos == std::string::npos ? "/" : url_.substr(path_pos);
      const auto query_pos = path.find('?');
      uri_ = query_pos == std::string::npos ? path : path.substr(0, query_pos);
      args_ = query_pos == std::string::npos ? std::string() :
        path.substr(query_pos + 1);

      HTTP::ParamList params;
      FCGI::HttpRequest::parse_params(args_, params);

      http_request_.set_method(FCGI::HttpRequest::RM_POST);
      http_request_.set_uri(uri_);
      http_request_.set_args(args_);
      http_request_.set_params(std::move(params));
      http_request_.set_body(body_);
    }

  private:
    std::string url_;
    std::string body_;
    std::string uri_;
    std::string args_;
  };

  class WaitResponseWriter final : public FCGI::BaseHttpResponseWriter
  {
  public:
    void
    write(FCGI::HttpResponse_var response) override
    {
      {
        std::lock_guard<std::mutex> guard(lock_);
        response_ = std::move(response);
        done_ = true;
      }
      cond_.notify_one();
    }

    FCGI::HttpResponse_var
    wait()
    {
      std::unique_lock<std::mutex> guard(lock_);
      cond_.wait(guard, [this]() { return done_; });
      return response_;
    }

  private:
    std::mutex lock_;
    std::condition_variable cond_;
    FCGI::HttpResponse_var response_;
    bool done_ = false;
  };

  template<typename Response>
  AdServer::Grpc::ResponseHolder<Response>
  make_response_holder(const Response& response)
  {
    Response copy(response);
    return AdServer::Grpc::ResponseHolder<Response>::make_value(std::move(copy));
  }

  bool print_mock_calls = false;

  void print_mock_call(const char* name)
  {
    if (print_mock_calls)
    {
      std::cout << "Mock call: " << name << std::endl;
    }
  }

  class MockChannelClient final :
    public AdServer::ChannelSvcs::ChannelServerGrpcAsyncClient
  {
  public:
    explicit MockChannelClient(const std::string& record_dir)
    {
      load_binary_proto(match_response_, record_dir, "channel_match.pb");
      match_response_.set_full_loaded(true);
      load_binary_proto(ccg_traits_response_, record_dir, "channel_get_ccg_traits.pb");
    }

    void match(const CH::MatchRequest&, MatchCallback callback) override
    { print_mock_call("ChannelServer.match"); callback(grpc::Status::OK, make_response_holder(match_response_)); }

    void get_ccg_traits(const CH::GetCcgTraitsRequest&, GetCcgTraitsCallback callback) override
    { print_mock_call("ChannelServer.get_ccg_traits"); callback(grpc::Status::OK, make_response_holder(ccg_traits_response_)); }

    void check_configuration(const CH::CheckConfigurationRequest&, CheckConfigurationCallback callback) override
    { print_mock_call("ChannelServer.check_configuration"); callback(grpc::Status::OK, make_response_holder(CH::CheckConfigurationResponse{})); }

    void set_sources(const CH::SetSourcesRequest&, SetSourcesCallback callback) override
    { print_mock_call("ChannelServer.set_sources"); callback(grpc::Status::OK, make_response_holder(CH::SetSourcesResponse{})); }

    void set_proxy_sources(const CH::SetProxySourcesRequest&, SetProxySourcesCallback callback) override
    { print_mock_call("ChannelServer.set_proxy_sources"); callback(grpc::Status::OK, make_response_holder(CH::SetProxySourcesResponse{})); }

  private:
    CH::MatchResponse match_response_;
    CH::GetCcgTraitsResponse ccg_traits_response_;
  };

  class MockUserInfoClient final :
    public AdServer::UserInfoSvcs::UserInfoManagerGrpcAsyncClient
  {
  public:
    explicit MockUserInfoClient(const std::string& record_dir)
    {
      load_binary_proto(match_response_, record_dir, "history_match.pb");
      load_binary_proto(update_response_, record_dir, "update_user_freq_caps.pb");
    }

    void match(const UIM::MatchRequest&, MatchCallback callback) override
    { print_mock_call("UserInfoManager.match"); callback(grpc::Status::OK, make_response_holder(match_response_)); }

    void update_user_freq_caps(const UIM::UpdateUserFreqCapsRequest&, UpdateUserFreqCapsCallback callback) override
    { print_mock_call("UserInfoManager.update_user_freq_caps"); callback(grpc::Status::OK, make_response_holder(update_response_)); }

    void get_source(const UIM::GetSourceRequest&, GetSourceCallback callback) override
    { print_mock_call("UserInfoManager.get_source"); callback(grpc::Status::OK, make_response_holder(UIM::GetSourceResponse{})); }
    void get_master_stamp(const UIM::GetMasterStampRequest&, GetMasterStampCallback callback) override
    { print_mock_call("UserInfoManager.get_master_stamp"); callback(grpc::Status::OK, make_response_holder(UIM::GetMasterStampResponse{})); }
    void get_user_profile(const UIM::GetUserProfileRequest&, GetUserProfileCallback callback) override
    { print_mock_call("UserInfoManager.get_user_profile"); callback(grpc::Status::OK, make_response_holder(UIM::GetUserProfileResponse{})); }
    void confirm_user_freq_caps(const UIM::ConfirmUserFreqCapsRequest&, ConfirmUserFreqCapsCallback callback) override
    { print_mock_call("UserInfoManager.confirm_user_freq_caps"); callback(grpc::Status::OK, make_response_holder(UIM::ConfirmUserFreqCapsResponse{})); }
    void fraud_user(const UIM::FraudUserRequest&, FraudUserCallback callback) override
    { print_mock_call("UserInfoManager.fraud_user"); callback(grpc::Status::OK, make_response_holder(UIM::FraudUserResponse{})); }
    void remove_user_profile(const UIM::RemoveUserProfileRequest&, RemoveUserProfileCallback callback) override
    { print_mock_call("UserInfoManager.remove_user_profile"); callback(grpc::Status::OK, make_response_holder(UIM::RemoveUserProfileResponse{})); }
    void merge(const UIM::MergeRequest&, MergeCallback callback) override
    { print_mock_call("UserInfoManager.merge"); callback(grpc::Status::OK, make_response_holder(UIM::MergeResponse{})); }
    void consider_publishers_optin(const UIM::ConsiderPublishersOptinRequest&, ConsiderPublishersOptinCallback callback) override
    { print_mock_call("UserInfoManager.consider_publishers_optin"); callback(grpc::Status::OK, make_response_holder(UIM::ConsiderPublishersOptinResponse{})); }
    void uim_ready(const UIM::UimReadyRequest&, UimReadyCallback callback) override
    { print_mock_call("UserInfoManager.uim_ready"); callback(grpc::Status::OK, make_response_holder(UIM::UimReadyResponse{})); }
    void get_progress(const UIM::GetProgressRequest&, GetProgressCallback callback) override
    { print_mock_call("UserInfoManager.get_progress"); callback(grpc::Status::OK, make_response_holder(UIM::GetProgressResponse{})); }
    void clear_expired(const UIM::ClearExpiredRequest&, ClearExpiredCallback callback) override
    { print_mock_call("UserInfoManager.clear_expired"); callback(grpc::Status::OK, make_response_holder(UIM::ClearExpiredResponse{})); }

  private:
    UIM::MatchResponse match_response_;
    UIM::UpdateUserFreqCapsResponse update_response_;
  };

  class MockUserBindClient final :
    public AdServer::UserInfoSvcs::UserBindServerGrpcAsyncClient
  {
  public:
    explicit MockUserBindClient(const std::string& record_dir)
    {
      load_binary_proto(get_user_id_response_, record_dir, "user_bind_get_user_id.pb");
      load_binary_proto(add_user_id_response_, record_dir, "user_bind_add_user_id.pb");
    }

    void get_user_id(const UIB::GetUserIdRequest&, GetUserIdCallback callback) override
    { print_mock_call("UserBindServer.get_user_id"); callback(grpc::Status::OK, make_response_holder(get_user_id_response_)); }

    void add_user_id(const UIB::AddUserIdRequest&, AddUserIdCallback callback) override
    { print_mock_call("UserBindServer.add_user_id"); callback(grpc::Status::OK, make_response_holder(add_user_id_response_)); }

    void get_bind_request(const UIB::GetBindRequestRequest&, GetBindRequestCallback callback) override
    { print_mock_call("UserBindServer.get_bind_request"); callback(grpc::Status::OK, make_response_holder(UIB::GetBindRequestResponse{})); }
    void add_bind_request(const UIB::AddBindRequestRequest&, AddBindRequestCallback callback) override
    { print_mock_call("UserBindServer.add_bind_request"); callback(grpc::Status::OK, make_response_holder(UIB::AddBindRequestResponse{})); }
    void get_source(const UIB::GetSourceRequest&, GetSourceCallback callback) override
    { print_mock_call("UserBindServer.get_source"); callback(grpc::Status::OK, make_response_holder(UIB::GetSourceResponse{})); }

  private:
    UIB::GetUserIdResponse get_user_id_response_;
    UIB::AddUserIdResponse add_user_id_response_;
  };

  class MockCampaignManager final :
    public AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient
  {
  public:
    explicit MockCampaignManager(const std::string& record_dir)
    {
      load_binary_proto(get_campaign_creative_response_, record_dir, "campaign_get_campaign_creative.pb");
    }

    void get_campaign_creative(const CM::GetCampaignCreativeRequest&, GetCampaignCreativeCallback callback) override
    { print_mock_call("CampaignManager.get_campaign_creative"); callback(grpc::Status::OK, make_response_holder(get_campaign_creative_response_)); }

    void ready(const CM::ReadyRequest&, ReadyCallback callback) override
    { print_mock_call("CampaignManager.ready"); callback(grpc::Status::OK, make_response_holder(CM::ReadyResponse{})); }
    void progress_comment(const CM::ProgressCommentRequest&, ProgressCommentCallback callback) override
    { print_mock_call("CampaignManager.progress_comment"); callback(grpc::Status::OK, make_response_holder(CM::ProgressCommentResponse{})); }
    void get_file(const CM::GetFileRequest&, GetFileCallback callback) override
    { print_mock_call("CampaignManager.get_file"); callback(grpc::Status::OK, make_response_holder(CM::GetFileResponse{})); }
    void process_match_request(const CM::ProcessMatchRequestRequest&, ProcessMatchRequestCallback callback) override
    { print_mock_call("CampaignManager.process_match_request"); callback(grpc::Status::OK, make_response_holder(CM::ProcessMatchRequestResponse{})); }
    void instantiate_ad(const CM::InstantiateAdRequest&, InstantiateAdCallback callback) override
    { print_mock_call("CampaignManager.instantiate_ad"); callback(grpc::Status::OK, make_response_holder(CM::InstantiateAdResponse{})); }
    void trace_campaign_selection_index(const CM::TraceCampaignSelectionIndexRequest&, TraceCampaignSelectionIndexCallback callback) override
    { print_mock_call("CampaignManager.trace_campaign_selection_index"); callback(grpc::Status::OK, make_response_holder(CM::TraceCampaignSelectionIndexResponse{})); }
    void get_campaign_creative_by_ccid(const CM::GetCampaignCreativeByCcidRequest&, GetCampaignCreativeByCcidCallback callback) override
    { print_mock_call("CampaignManager.get_campaign_creative_by_ccid"); callback(grpc::Status::OK, make_response_holder(CM::GetCampaignCreativeByCcidResponse{})); }
    void get_channel_links(const CM::GetChannelLinksRequest&, GetChannelLinksCallback callback) override
    { print_mock_call("CampaignManager.get_channel_links"); callback(grpc::Status::OK, make_response_holder(CM::GetChannelLinksResponse{})); }
    void get_category_channels(const CM::GetCategoryChannelsRequest&, GetCategoryChannelsCallback callback) override
    { print_mock_call("CampaignManager.get_category_channels"); callback(grpc::Status::OK, make_response_holder(CM::GetCategoryChannelsResponse{})); }
    void get_colocation_flags(const CM::GetColocationFlagsRequest&, GetColocationFlagsCallback callback) override
    { print_mock_call("CampaignManager.get_colocation_flags"); callback(grpc::Status::OK, make_response_holder(CM::GetColocationFlagsResponse{})); }
    void get_pub_pixels(const CM::GetPubPixelsRequest&, GetPubPixelsCallback callback) override
    { print_mock_call("CampaignManager.get_pub_pixels"); callback(grpc::Status::OK, make_response_holder(CM::GetPubPixelsResponse{})); }
    void consider_passback(const CM::ConsiderPassbackRequest&, ConsiderPassbackCallback callback) override
    { print_mock_call("CampaignManager.consider_passback"); callback(grpc::Status::OK, make_response_holder(CM::ConsiderPassbackResponse{})); }
    void consider_passback_track(const CM::ConsiderPassbackTrackRequest&, ConsiderPassbackTrackCallback callback) override
    { print_mock_call("CampaignManager.consider_passback_track"); callback(grpc::Status::OK, make_response_holder(CM::ConsiderPassbackTrackResponse{})); }
    void get_click_url(const CM::GetClickUrlRequest&, GetClickUrlCallback callback) override
    { print_mock_call("CampaignManager.get_click_url"); callback(grpc::Status::OK, make_response_holder(CM::GetClickUrlResponse{})); }
    void verify_impression(const CM::VerifyImpressionRequest&, VerifyImpressionCallback callback) override
    { print_mock_call("CampaignManager.verify_impression"); callback(grpc::Status::OK, make_response_holder(CM::VerifyImpressionResponse{})); }
    void action_taken(const CM::ActionTakenRequest&, ActionTakenCallback callback) override
    { print_mock_call("CampaignManager.action_taken"); callback(grpc::Status::OK, make_response_holder(CM::ActionTakenResponse{})); }
    void verify_opt_operation(const CM::VerifyOptOperationRequest&, VerifyOptOperationCallback callback) override
    { print_mock_call("CampaignManager.verify_opt_operation"); callback(grpc::Status::OK, make_response_holder(CM::VerifyOptOperationResponse{})); }
    void consider_web_operation(const CM::ConsiderWebOperationRequest&, ConsiderWebOperationCallback callback) override
    { print_mock_call("CampaignManager.consider_web_operation"); callback(grpc::Status::OK, make_response_holder(CM::ConsiderWebOperationResponse{})); }
    void get_config(const CM::GetConfigRequest&, GetConfigCallback callback) override
    { print_mock_call("CampaignManager.get_config"); callback(grpc::Status::OK, make_response_holder(CM::GetConfigResponse{})); }

  private:
    CM::GetCampaignCreativeResponse get_campaign_creative_response_;
  };

  class MockUserIdController final :
    public AdServer::UserIdControllerBase
  {
  public:
    Generics::SignedUuid verify(std::string_view, KeyType = PERSISTENT) const override
    {
      throw std::runtime_error("MockUserIdController::verify is not implemented");
    }

    bool null_blacklisted(AdServer::Commons::UserId&) const noexcept override
    {
      return false;
    }

    Generics::Uuid ssp_uuid(const Generics::Uuid& uuid, const String::SubString&) override
    {
      return uuid;
    }

  protected:
    ~MockUserIdController() noexcept override = default;
  };

  class TestContext
  {
  public:
    explicit TestContext(const Options& options)
      : logger(new Logging::Null::Logger),
        callback(new Logging::ActiveObjectCallbackImpl(logger)),
        common_module(new AdServer::CommonModule(logger)),
        bid_workers(std::make_shared<AdServer::Commons::ExecutorPool>(
          callback,
          1,
          AdServer::Commons::ExecutorPool::ResumeStrategy::CurrentContext,
          "bf-test-bid")),
        timeout_scheduler(std::make_shared<AdServer::Commons::FastScheduler>(1)),
        sources(std::make_shared<AdServer::Bidding::RequestInfoFiller::SourceMap>()),
        account_traits(std::make_shared<AdServer::Bidding::RequestInfoFiller::AccountTraitsById>()),
        stats(new AdServer::StatHolder())
    {
      print_mock_calls = options.print_calls;
      init_common_module(*common_module, logger, options.fe_config, options.domain_config);
      (*sources)["msc"] = make_default_source_traits();

      AdServer::Bidding::RequestInfoFiller::ExternalUserIdSet skip_external_ids;
      request_info_filler = std::make_shared<AdServer::Bidding::RequestInfoFiller>(
        logger,
        1,
        common_module,
        std::shared_ptr<GeoIPMapping::IPMapCity2>(),
        "",
        skip_external_ids,
        false,
        "",
        *sources,
        true,
        *account_traits);

      auto user_bind_client = std::make_shared<MockUserBindClient>(options.record_dir);
      auto user_info_client = std::make_shared<MockUserInfoClient>(options.record_dir);
      auto campaign_manager = std::make_shared<MockCampaignManager>(options.record_dir);
      auto channel_client = std::make_shared<MockChannelClient>(options.record_dir);

      AdServer::Bidding::BiddingFrontendCore::InitParams params;
      params.logger = logger;
      params.common_module = common_module;
      params.user_id_controller = user_id_controller;
      params.request_info_filler = request_info_filler;
      params.sources = sources;
      params.account_traits = account_traits;
      params.stats = stats;
      params.bid_workers = bid_workers;
      params.timeout_scheduler = timeout_scheduler;
      params.user_bind_client = user_bind_client;
      params.user_info_client = user_info_client;
      params.campaign_manager = campaign_manager;
      params.channel_client = channel_client;
      params.google_uris = {"/google"};
      params.adfox_uris = {"/adfox"};
      params.adxml_uris = {"/adxml"};
      params.clickstar_uris = {"/clickstar"};
      params.dao_uris = {"/dao"};
      params.max_pending_tasks = 1000000;
      params.threads = 1;
      params.request_timeout = Generics::Time(10);
      params.server_id = "BiddingFrontendCoreTest";
      params.colo_id = 1;
      params.trace_mapping = false;
      params.enable_profile_referer = true;

      ext_config = new AdServer::Bidding::BiddingFrontendCore::ExtConfig;
      core = std::make_unique<AdServer::Bidding::BiddingFrontendCore>(params);
      core->set_config(ext_config);

      bid_workers->activate_object();
    }

    ~TestContext()
    {
      bid_workers->deactivate_object();
      bid_workers->wait_object();
    }

    Logging::Logger_var logger;
    Logging::ActiveObjectCallbackImpl_var callback;
    AdServer::CommonModule_var common_module;
    AdServer::UserIdControllerBase_var user_id_controller = new MockUserIdController;
    std::shared_ptr<AdServer::Commons::ExecutorPool> bid_workers;
    std::shared_ptr<AdServer::Commons::FastScheduler> timeout_scheduler;
    std::shared_ptr<AdServer::Bidding::RequestInfoFiller::SourceMap> sources;
    std::shared_ptr<AdServer::Bidding::RequestInfoFiller::AccountTraitsById>
      account_traits;
    AdServer::StatHolder_var stats;
    std::shared_ptr<AdServer::Bidding::RequestInfoFiller> request_info_filler;
    AdServer::Bidding::BiddingFrontendCore::ExtConfig_var ext_config;
    std::unique_ptr<AdServer::Bidding::BiddingFrontendCore> core;
  };

  std::uint64_t
  run_perf(TestContext& context, const Options& options)
  {
    std::uint64_t checksum = 0;
    for (std::uint64_t i = 0; i < options.count; ++i)
    {
      FCGI::HttpRequestHolder_var request_holder(
        new TestHttpRequestHolder(options.url, options.body));
      ReferenceCounting::SmartPtr<WaitResponseWriter> response_writer(new WaitResponseWriter);

      context.core->handle_request(request_holder, response_writer);

      FCGI::HttpResponse_var response = response_writer->wait();
      checksum += response ? static_cast<std::uint64_t>(response->status()) : 0;
      checksum += response ? response->body().size() : 0;
    }
    return checksum;
  }
}

int
main(int argc, char** argv)
{
  try
  {
    Options options = parse_options(argc, argv);
    if (!options.body_file.empty())
    {
      options.body = read_file(options.body_file);
    }

    TestContext context(options);

    const auto started_at = std::chrono::steady_clock::now();
    const CpuTimes cpu_started = current_cpu_times();

    const std::uint64_t checksum = run_perf(context, options);

    const CpuTimes cpu_finished = current_cpu_times();
    const auto finished_at = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(finished_at - started_at).count();
    const double rate = static_cast<double>(options.count) / elapsed;

    std::cout
      << "command=perf\n"
      << "count=" << options.count << '\n'
      << "elapsed_sec=" << format_float(elapsed) << '\n'
      << "rate_per_sec=" << format_float(rate) << '\n'
      << "user_cpu_sec=" << format_float(cpu_finished.user - cpu_started.user) << '\n'
      << "sys_cpu_sec=" << format_float(cpu_finished.sys - cpu_started.sys) << '\n'
      << "checksum=" << checksum << '\n';

    return 0;
  }
  catch(const std::exception& ex)
  {
    std::cerr << "std::exception: " << ex.what() << std::endl;
  }

  return 1;
}
