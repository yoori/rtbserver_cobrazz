#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <grpcpp/grpcpp.h>

#include <Generics/ActiveObject.hpp>
#include <Generics/AppUtils.hpp>
#include <Generics/Time.hpp>
#include <Generics/Uuid.hpp>
#include <String/StringManip.hpp>

#include <Commons/GrpcAlgs.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <Commons/Grpc/GrpcSync.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <UtilCommons/Table.hpp>
#include <CampaignManagerGrpc.grpc-client.hpp>
#include <CampaignManagerGrpc.grpc.pb.h>

namespace
{
  namespace cm = adserver::campaign_svcs::campaign_manager;

  const char TIME_FORMAT[] = "%d-%m-%Y:%H-%M-%S.%q";

  const Table::Column CREATIVE_TABLE_COLUMNS[] =
  {
    Table::Column("ccid", Table::Column::NUMBER),
    Table::Column("creative_id", Table::Column::NUMBER),
    Table::Column("campaign_id", Table::Column::NUMBER),
    Table::Column("fc_id", Table::Column::NUMBER),
    Table::Column("weight", Table::Column::NUMBER),
    Table::Column("creative_format", Table::Column::TEXT),
    Table::Column("click_url", Table::Column::TEXT),
    Table::Column("sizes", Table::Column::TEXT),
    Table::Column("version_id", Table::Column::TEXT),
    Table::Column("status", Table::Column::TEXT),
    Table::Column("categories", Table::Column::TEXT),
    Table::Column("order_set_id", Table::Column::NUMBER),
    Table::Column("initial_contract_id", Table::Column::NUMBER),
    Table::Column("options", Table::Column::TEXT)
  };

  const Table::Column GLOBAL_TABLE_COLUMNS[] =
  {
    Table::Column("currency_exchange_id", Table::Column::NUMBER),
    Table::Column("fraud_user_deactivate_period", Table::Column::NUMBER),
    Table::Column("cost_limit", Table::Column::TEXT),
    Table::Column("google_publisher_account_id", Table::Column::NUMBER),
    Table::Column("global_params_timestamp", Table::Column::TEXT),
    Table::Column("master_stamp", Table::Column::TEXT),
    Table::Column("first_load_stamp", Table::Column::TEXT),
    Table::Column("finish_load_stamp", Table::Column::TEXT)
  };

  const Table::Column FREQ_CAPS_TABLE_COLUMNS[] =
  {
    Table::Column("id", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT),
    Table::Column("lifelimit", Table::Column::NUMBER),
    Table::Column("period", Table::Column::NUMBER),
    Table::Column("window_limit", Table::Column::NUMBER),
    Table::Column("window_time", Table::Column::NUMBER)
  };

  const Table::Column APP_FORMAT_COLUMNS[] =
  {
    Table::Column("app_format", Table::Column::TEXT),
    Table::Column("mime_format", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT)
  };

  const Table::Column SIZE_TABLE_COLUMNS[] =
  {
    Table::Column("size_id", Table::Column::NUMBER),
    Table::Column("size_type_id", Table::Column::NUMBER),
    Table::Column("protocol_name", Table::Column::TEXT),
    Table::Column("width", Table::Column::NUMBER),
    Table::Column("height", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT)
  };

  const Table::Column CURRENCY_TABLE_COLUMNS[] =
  {
    Table::Column("currency_id", Table::Column::NUMBER),
    Table::Column("currency_exchange_id", Table::Column::NUMBER),
    Table::Column("effective_date", Table::Column::TEXT),
    Table::Column("rate", Table::Column::TEXT),
    Table::Column("fraction_digits", Table::Column::NUMBER),
    Table::Column("currency_code", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT),
  };

  const Table::Column CREATIVE_OPTION_TABLE_COLUMNS[] =
  {
    Table::Column("option_id", Table::Column::NUMBER),
    Table::Column("token", Table::Column::TEXT),
    Table::Column("type", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT),
    Table::Column("relations", Table::Column::TEXT)
  };

  const Table::Column CAMPAIGN_KEYWORD_TABLE_COLUMNS[] =
  {
    Table::Column("ccg_keyword_id", Table::Column::NUMBER),
    Table::Column("original_keyword", Table::Column::TEXT),
    Table::Column("click_url", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT),
  };

  const Table::Column CREATIVE_CATEGORY_TABLE_COLUMNS[] =
  {
    Table::Column("creative_category_id", Table::Column::NUMBER),
    Table::Column("cct_id", Table::Column::NUMBER),
    Table::Column("name", Table::Column::TEXT),
    Table::Column("external_categories", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT),
  };

  const Table::Column CATEGORY_CHANNEL_TABLE_COLUMNS[] =
  {
    Table::Column("channel_id", Table::Column::NUMBER),
    Table::Column("name", Table::Column::TEXT),
    Table::Column("newsgate_name", Table::Column::TEXT),
    Table::Column("parent_channel_id", Table::Column::NUMBER),
    Table::Column("flags", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT),
    Table::Column("localizations", Table::Column::TEXT)
  };

  const Table::Column GEO_CHANNEL_TABLE_COLUMNS[] =
  {
    Table::Column("channel_id", Table::Column::NUMBER),
    Table::Column("country", Table::Column::TEXT),
    Table::Column("region", Table::Column::TEXT),
    Table::Column("city", Table::Column::TEXT)
  };

  const Table::Column GEO_COORD_CHANNEL_TABLE_COLUMNS[] =
  {
    Table::Column("channel_id", Table::Column::NUMBER),
    Table::Column("longitude", Table::Column::NUMBER),
    Table::Column("latitude", Table::Column::NUMBER),
    Table::Column("radius", Table::Column::NUMBER)
  };

  const Table::Column WEB_APPLICATIONS_TABLE_COLUMNS[] =
  {
    Table::Column("web_operation_id", Table::Column::NUMBER),
    Table::Column("app", Table::Column::TEXT),
    Table::Column("source", Table::Column::TEXT),
    Table::Column("operation", Table::Column::TEXT),
    Table::Column("flags", Table::Column::NUMBER)
  };

  const Table::Column ACCOUNT_TABLE_COLUMNS[] =
  {
    Table::Column("account_id", Table::Column::NUMBER),
    Table::Column("agency_account_id", Table::Column::NUMBER),
    Table::Column("internal_account_id", Table::Column::NUMBER),
    Table::Column("status", Table::Column::TEXT),
    Table::Column("eval_status", Table::Column::TEXT),
    Table::Column("legal_name", Table::Column::TEXT),
    Table::Column("flags", Table::Column::NUMBER),
    Table::Column("at_flags", Table::Column::TEXT),
    Table::Column("text_adserving", Table::Column::TEXT),
    Table::Column("currency_id", Table::Column::NUMBER),
    Table::Column("country", Table::Column::TEXT),
    Table::Column("commision", Table::Column::TEXT),
    Table::Column("budget", Table::Column::TEXT),
    Table::Column("paid_amount", Table::Column::TEXT),
    Table::Column("time_offset", Table::Column::TEXT),
    Table::Column("walled_garden_accounts", Table::Column::TEXT),
    Table::Column("auction_rate", Table::Column::TEXT),
    Table::Column("use_pub_pixels", Table::Column::NUMBER),
    Table::Column("pub_pixel_optin", Table::Column::TEXT),
    Table::Column("pub_pixel_optout", Table::Column::TEXT),
    Table::Column("self_service_commission", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT),
  };

  const Table::Column SITES_TABLE_COLUMNS[] =
  {
    Table::Column("site_id", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT),
    Table::Column("status", Table::Column::TEXT),
    Table::Column("noads_timeout", Table::Column::NUMBER),
    Table::Column("freq_cap_id", Table::Column::NUMBER),
    Table::Column("approved_creative_categories", Table::Column::TEXT),
    Table::Column("rejected_creative_categories", Table::Column::TEXT),
    Table::Column("approved_creatives", Table::Column::TEXT),
    Table::Column("rejected_creatives", Table::Column::TEXT),
    Table::Column("flags", Table::Column::NUMBER),
    Table::Column("account_id", Table::Column::NUMBER)
  };

  const Table::Column COLOCATIONS_TABLE_COLUMNS[] =
  {
    Table::Column("colo_name", Table::Column::TEXT),
    Table::Column("colo_id", Table::Column::NUMBER),
    Table::Column("colo_rate_id", Table::Column::NUMBER),
    Table::Column("account_id", Table::Column::NUMBER),
    Table::Column("revenue_share", Table::Column::TEXT),
    Table::Column("ad_serving", Table::Column::TEXT),
    Table::Column("hid_profile", Table::Column::NUMBER),
    Table::Column("options", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT),
  };

  const Table::Column COUNTRIES_TABLE_COLUMNS[] =
  {
    Table::Column("country_code", Table::Column::TEXT),
    Table::Column("options", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT),
  };

  const Table::Column CREATIVE_TEMPLATES_TABLE_COLUMNS[] =
  {
    Table::Column("creative_format", Table::Column::TEXT),
    Table::Column("creative_size", Table::Column::TEXT),
    Table::Column("app_format", Table::Column::TEXT),
    Table::Column("mime_format", Table::Column::TEXT),
    Table::Column("track_impression", Table::Column::NUMBER),
    Table::Column("template_file", Table::Column::TEXT),
    Table::Column("type", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT),
    Table::Column("status", Table::Column::TEXT),
    Table::Column("tokens", Table::Column::TEXT),
    Table::Column("hidden_tokens", Table::Column::TEXT),
  };

  const Table::Column ADV_ACTION_TABLE_COLUMNS[] =
  {
    Table::Column("action_id", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT),
    Table::Column("cur_value", Table::Column::TEXT),
    Table::Column("ccg_ids", Table::Column::TEXT)
  };

  const Table::Column CONTRACT_TABLE_COLUMNS[] =
  {
    Table::Column("number", Table::Column::TEXT),
    Table::Column("date", Table::Column::TEXT),
    Table::Column("type", Table::Column::TEXT),
    Table::Column("vat_included", Table::Column::NUMBER),
    Table::Column("ord_contract_id", Table::Column::TEXT),
    Table::Column("ord_ado_id", Table::Column::TEXT),
    Table::Column("subject_type", Table::Column::TEXT),
    Table::Column("action_type", Table::Column::TEXT),
    Table::Column("agent_acting_for_publisher", Table::Column::NUMBER),
    Table::Column("parent_contract_id", Table::Column::NUMBER),
    Table::Column("client_id", Table::Column::TEXT),
    Table::Column("client_name", Table::Column::TEXT),
    Table::Column("client_legal_form", Table::Column::TEXT),
    Table::Column("contractor_id", Table::Column::TEXT),
    Table::Column("contractor_name", Table::Column::TEXT),
    Table::Column("contractor_legal_form", Table::Column::TEXT),
    Table::Column("timestamp", Table::Column::TEXT)
  };

  const Table::Column TAGS_TABLE_COLUMNS[] =
  {
    Table::Column("tag_id", Table::Column::NUMBER),
    Table::Column("site_id", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT),
    Table::Column("sizes", Table::Column::TEXT),
    Table::Column("imp_track_pixel", Table::Column::TEXT),
    Table::Column("passback", Table::Column::TEXT),
    Table::Column("passback_type", Table::Column::TEXT),
    Table::Column("flags", Table::Column::NUMBER),
    Table::Column("marketplace", Table::Column::TEXT),
    Table::Column("adjustment", Table::Column::TEXT),
    Table::Column("accepted_categories", Table::Column::TEXT),
    Table::Column("rejected_categories", Table::Column::TEXT),
    Table::Column("allow_expandable", Table::Column::NUMBER),
    Table::Column("auction_max_ecpm_share", Table::Column::TEXT),
    Table::Column("auction_prop_probability_share", Table::Column::TEXT),
    Table::Column("auction_random_share", Table::Column::TEXT),
    Table::Column("cost_coef", Table::Column::TEXT),
    Table::Column("tag_pricings_timestamp", Table::Column::TEXT),
    Table::Column("tag_pricings", Table::Column::TEXT),
    Table::Column("hidden_options", Table::Column::TEXT),
    Table::Column("passback_options", Table::Column::TEXT),
    Table::Column("options", Table::Column::TEXT),
  };

  const Table::Column CAMPAIGN_TABLE_COLUMNS[] =
  {
    Table::Column("id", Table::Column::NUMBER),
    Table::Column("timestamp", Table::Column::TEXT),
    Table::Column("account_id", Table::Column::NUMBER),
    Table::Column("advertiser_id", Table::Column::NUMBER),
    Table::Column("campaign_group_id", Table::Column::NUMBER),
    Table::Column("ccg_rate_id", Table::Column::NUMBER),
    Table::Column("ccg_rate_type", Table::Column::TEXT),
    Table::Column("fc_id", Table::Column::NUMBER),
    Table::Column("group_fc_id", Table::Column::NUMBER),
    Table::Column("weeklyrun", Table::Column::TEXT),
    Table::Column("ecpm", Table::Column::NUMBER),
    Table::Column("ctr", Table::Column::TEXT),
    Table::Column("channel", Table::Column::TEXT),
    Table::Column("stat_channel", Table::Column::TEXT),
    Table::Column("country", Table::Column::TEXT),
    Table::Column("sites", Table::Column::TEXT),
    Table::Column("creatives", Table::Column::TEXT),
    Table::Column("status", Table::Column::TEXT),
    Table::Column("eval_status", Table::Column::TEXT),
    Table::Column("campaign_type", Table::Column::TEXT),
    Table::Column("target_type", Table::Column::TEXT),
    Table::Column("start_user_group_id", Table::Column::NUMBER),
    Table::Column("end_user_group_id", Table::Column::NUMBER),
    Table::Column("ctr_reset_id", Table::Column::NUMBER),
    Table::Column("random_imps", Table::Column::NUMBER),
    Table::Column("mode", Table::Column::TEXT),
    Table::Column("min_uid_age", Table::Column::TEXT),
    Table::Column("seq_set_rotate_imps", Table::Column::NUMBER),
    Table::Column("date_start", Table::Column::TEXT),
    Table::Column("date_end", Table::Column::TEXT),
    Table::Column("budget", Table::Column::TEXT),
    Table::Column("daily_budget", Table::Column::TEXT),
    Table::Column("delivery_pacing", Table::Column::TEXT),
    Table::Column("imps", Table::Column::TEXT),
    Table::Column("daily_imps", Table::Column::TEXT),
    Table::Column("clicks", Table::Column::TEXT),
    Table::Column("daily_clicks", Table::Column::TEXT),
    Table::Column("cmp_date_start", Table::Column::TEXT),
    Table::Column("cmp_date_end", Table::Column::TEXT),
    Table::Column("cmp_budget", Table::Column::TEXT),
    Table::Column("cmp_daily_budget", Table::Column::TEXT),
    Table::Column("cmp_delivery_pacing", Table::Column::TEXT),
    Table::Column("cmp_imps", Table::Column::TEXT),
    Table::Column("cmp_daily_imps", Table::Column::TEXT),
    Table::Column("cmp_clicks", Table::Column::TEXT),
    Table::Column("cmp_daily_clicks", Table::Column::TEXT),
    Table::Column("max_pub_share", Table::Column::TEXT),
    Table::Column("bid_strategy", Table::Column::TEXT),
    Table::Column("min_ctr_goal", Table::Column::TEXT),
    Table::Column("commision", Table::Column::TEXT),
    Table::Column("flags", Table::Column::TEXT),
    Table::Column("marketplace", Table::Column::TEXT)
  };

  const char USAGE[] =
    "Usage:\n"
    "CampaignAdmin2 <command> -r <CampaignManager gRPC endpoint> [options]\n\n"
    "Commands:\n"
    "  globals\n"
    "  app_format\n"
    "  size\n"
    "  creative_categories\n"
    "  sites\n"
    "  colocations\n"
    "  countries\n"
    "  creative_templates\n"
    "  campaign\n"
    "  tags\n"
    "  currency\n"
    "  creative [--id=<ccid>]\n"
    "  freq_cap\n"
    "  creative_option\n"
    "  adv_action\n"
    "  category_channel\n"
    "  geo_channel [--geo_channels]\n"
    "  geo_coord_channel [--geo_channels]\n"
    "  web_operations\n"
    "  keyword\n"
    "  account\n"
    "  expression_channel\n"
    "  contract\n"
    "  ready\n"
    "  progress\n"
    "  select [--uid=<uid>] [--tag_id=<id>] [--format=<format>] "
      "[--referer=<url>] [--peer_ip=<ip>] [--country=<code>] "
      "[--channels=1,2] [--colo_id=<id>] [--random=<n>] [--optout]\n"
    "  trace_index\n"
    "  trace_weight --id=<campaign_id> [--tag_id=<id>] [--format=<format>] "
      "[--referer=<url>] [--country=<code>] [--channels=1,2] "
      "[--colo_id=<id>] [--optout] [--testrequest=<0|1>]\n"
    "  preview --id=<ccid> [--tag_id=<id>] [--format=<format>] "
      "[--referer=<url>] [--peer_ip=<ip>]\n"
    "  colocation_flags\n"
    "  pub_pixels --country=<code> --user_status=<status> "
      "[--publisher_account_ids=1,2]\n"
    "  category_channels [--language=<lang>]\n"
    "  channel_links --channels=1,2 [--match]\n"
    "  discover_channels --channels=1[:weight],2[:weight] "
      "[--country=<code>] [--language=<lang>] [--all]\n"
    "  get_file --file=<name> [--service_index=<index>]\n"
    "  get_config [--geo_channels]\n\n"
    "Options:\n"
    "  -r, --reference=<host:port>  CampaignManager gRPC endpoint\n";

  struct ClientHolder
  {
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor;
    std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcAsyncBatchingClient>
      client;
  };

  std::vector<unsigned long>
  parse_ids(const std::string& value)
  {
    std::vector<unsigned long> result;
    String::StringManip::SplitComma tokenizer(value);
    String::SubString token;
    while(tokenizer.get_token(token))
    {
      unsigned long id = 0;
      if(!String::StringManip::str_to_int(token, id))
      {
        throw std::runtime_error("invalid id: " + token.str());
      }
      result.push_back(id);
    }
    return result;
  }

  std::string
  option_value(const Generics::AppUtils::StringOption& option)
  {
    return option.installed() ? *option : std::string();
  }

  std::string
  pack_uuid(const Generics::Uuid& uuid)
  {
    return std::string(uuid.begin(), uuid.end());
  }

  void
  add_ids(
    const std::vector<unsigned long>& ids,
    google::protobuf::RepeatedField<google::protobuf::uint64>* target)
  {
    for(const auto id : ids)
    {
      target->Add(id);
    }
  }

  void
  fill_geo(
    const Generics::AppUtils::StringOption& country,
    google::protobuf::RepeatedPtrField<cm::GeoInfo>* target)
  {
    if(country.installed())
    {
      auto* location = target->Add();
      location->set_country(*country);
    }
  }

  void
  fill_channels(
    const Generics::AppUtils::StringOption& channels,
    google::protobuf::RepeatedField<google::protobuf::uint64>* target)
  {
    if(channels.installed())
    {
      add_ids(parse_ids(*channels), target);
    }
  }

  void
  fill_common_request(
    cm::CommonAdRequestInfo& common_info,
    const Generics::Time& now,
    const Generics::AppUtils::StringOption& uid,
    const Generics::AppUtils::StringOption& referer,
    const Generics::AppUtils::StringOption& peer_ip,
    const Generics::AppUtils::StringOption& country,
    unsigned long colo_id,
    unsigned long random,
    bool optout)
  {
    const Generics::Uuid request_id =
      Generics::Uuid::create_random_based();
    const Generics::Uuid user_id = uid.installed() && !uid->empty() ?
      Generics::Uuid(*uid, true) :
      Generics::Uuid::create_random_based();

    common_info.set_request_id(pack_uuid(request_id));
    common_info.set_user_id(::GrpcAlgs::pack_user_id(user_id));
    common_info.set_time(::GrpcAlgs::pack_time(now));
    common_info.set_colo_id(colo_id);
    common_info.set_user_status(optout ? 3 : 2);
    common_info.set_referer(option_value(referer));
    common_info.set_original_url(option_value(referer));
    common_info.set_peer_ip(option_value(peer_ip));
    common_info.set_creative_instantiate_type("unsecure");
    common_info.set_random(random);
    fill_geo(country, common_info.mutable_location());
  }

  ClientHolder
  create_client(const std::string& reference)
  {
    if(reference.empty())
    {
      throw std::runtime_error("empty CampaignManager gRPC endpoint");
    }

    ClientHolder result;
    result.grpc_executor = std::make_shared<AdServer::Grpc::GrpcExecutor>(1);
    result.grpc_executor->activate_object();
    result.client = std::make_shared<
      AdServer::CampaignSvcs::CampaignManagerGrpcAsyncBatchingClient>(
        reference,
        result.grpc_executor,
        AdServer::Grpc::BatchingOptions());
    result.client->activate_object();
    return result;
  }

  void
  shutdown_client(ClientHolder& holder) noexcept
  {
    try
    {
      if(holder.client)
      {
        holder.client->deactivate_object();
        holder.client->wait_object();
      }
      if(holder.grpc_executor)
      {
        holder.grpc_executor->deactivate_object();
        holder.grpc_executor->wait_object();
      }
    }
    catch(...)
    {}
  }

  template<typename Response, typename Request, typename Method>
  Response
  call(
    AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient& client,
    const Request& request,
    Method method)
  {
    return AdServer::Grpc::sync_call<Response>(
      [&](auto callback)
      {
        (client.*method)(request, std::move(callback));
      });
  }

  void
  print_colocation_flags(const cm::GetColocationFlagsResponse& response)
  {
    std::cout << "colo_id\tflags\thid_profile\n";
    for(const auto& item : response.colocations())
    {
      std::cout << item.colo_id() << '\t' <<
        item.flags() << '\t' <<
        item.hid_profile() << '\n';
    }
  }

  void
  print_category_channels(
    const google::protobuf::RepeatedPtrField<cm::CategoryChannelNode>& channels,
    const std::string& prefix = std::string())
  {
    for(const auto& channel : channels)
    {
      std::cout << prefix << channel.channel_id() << '\t' <<
        channel.flags() << '\t' <<
        channel.name() << '\n';
      print_category_channels(
        channel.child_category_channels(),
        prefix + "  ");
    }
  }

  void
  add_discover_channel(
    const std::string& token,
    cm::GetDiscoverChannelsRequest& request)
  {
    const auto pos = token.find(':');
    const std::string id_part = token.substr(0, pos);
    const std::string weight_part =
      pos == std::string::npos ? std::string("1") : token.substr(pos + 1);

    unsigned long id = 0;
    unsigned long weight = 0;
    if(!String::StringManip::str_to_int(id_part, id) ||
      !String::StringManip::str_to_int(weight_part, weight))
    {
      throw std::runtime_error("invalid channel[:weight]: " + token);
    }

    auto* channel = request.add_channels();
    channel->set_channel_id(id);
    channel->set_weight(weight);
  }

  std::string
  hex_value(const std::string& value)
  {
    if(value.empty())
    {
      return std::string();
    }

    std::ostringstream out;
    out << "0x";
    static const char HEX[] = "0123456789ABCDEF";
    for(unsigned char ch : value)
    {
      out << HEX[(ch >> 4) & 0x0F] << HEX[ch & 0x0F];
    }
    return out.str();
  }

  std::string
  bytes_value(const std::string& value)
  {
    if(value.empty())
    {
      return std::string();
    }

    if(value.size() != Generics::Time::TIME_PACK_LEN)
    {
      return hex_value(value);
    }

    try
    {
      return GrpcAlgs::unpack_time(value).get_gm_time().format(
        "%Y-%m-%d %H:%M:%S");
    }
    catch(...)
    {}

    return hex_value(value);
  }

  std::string
  decimal_value(const cm::DecimalInfo& value)
  {
    using RevenueDecimal = AdServer::CampaignSvcs::RevenueDecimal;
    if(value.value().size() != RevenueDecimal::PACK_SIZE)
    {
      return std::string();
    }

    RevenueDecimal decimal;
    decimal.unpack(value.value().data());
    return decimal.str();
  }

  std::string
  decimal_integer_value(const cm::DecimalInfo& value)
  {
    using RevenueDecimal = AdServer::CampaignSvcs::RevenueDecimal;
    if(value.value().size() != RevenueDecimal::PACK_SIZE)
    {
      return std::string();
    }

    RevenueDecimal decimal;
    decimal.unpack(value.value().data());
    return std::to_string(decimal.integer<uint64_t>());
  }

  std::string
  time_value(const std::string& value)
  {
    if(value.size() != Generics::Time::TIME_PACK_LEN)
    {
      return std::string();
    }

    return GrpcAlgs::unpack_time(value).get_gm_time().format(TIME_FORMAT);
  }

  std::string
  option_value(const cm::ConfigOptionValue& value)
  {
    return value.value();
  }

  template<typename Repeated>
  std::string
  join_numbers(const Repeated& values)
  {
    std::ostringstream out;
    bool first = true;
    for(const auto value : values)
    {
      if(!first)
      {
        out << ',';
      }
      first = false;
      out << value;
    }
    return out.str();
  }

  Table
  make_table(
    const Table::Column* columns,
    std::size_t columns_count)
  {
    Table table(columns_count);
    for(std::size_t i = 0; i < columns_count; ++i)
    {
      table.column(i, columns[i]);
    }
    return table;
  }

  std::string
  make_prefix(
    const Table::Column* columns,
    std::size_t columns_count)
  {
    std::size_t max_len = 0;
    for(std::size_t i = 0; i < columns_count; ++i)
    {
      max_len = std::max(max_len, columns[i].name.length());
    }
    return std::string(max_len + 3, ' ');
  }

  std::string
  creative_sizes(
    const cm::ConfigCreative& creative,
    const std::map<unsigned long, std::string>& sizes)
  {
    std::ostringstream out;
    for(int i = 0; i < creative.sizes_size(); ++i)
    {
      const auto& size = creative.sizes(i);
      out << '[' << size.size_id() << ",'";
      const auto size_it = sizes.find(size.size_id());
      if(size_it != sizes.end())
      {
        out << size_it->second;
      }
      out << "'," << size.up_expand_space() <<
        ',' << size.right_expand_space() <<
        ',' << size.down_expand_space() <<
        ',' << size.left_expand_space() << ']';
    }
    return out.str();
  }

  std::string
  option_values(
    const google::protobuf::RepeatedPtrField<cm::ConfigOptionValue>& values)
  {
    std::ostringstream out;
    for(int i = 0; i < values.size(); ++i)
    {
      if(i > 0)
      {
        out << ", ";
      }
      out << values.Get(i).option_id();
    }
    return out.str();
  }

  template<typename Repeated>
  std::string
  join_numbers_spaced(const Repeated& values)
  {
    std::ostringstream out;
    for(int i = 0; i < values.size(); ++i)
    {
      if(i > 0)
      {
        out << ", ";
      }
      out << values.Get(i);
    }
    return out.str();
  }

  std::string
  optional_decimal_value(const cm::DecimalInfo& value)
  {
    return value.value().empty() ? std::string("null") : decimal_value(value);
  }

  std::string
  time_offset_value(const std::string& value)
  {
    const Generics::Time time_offset = GrpcAlgs::unpack_time(value);
    std::ostringstream out;
    const Generics::Time::Print& print = time_offset.print();
    out << (print.sign >= 0 ? '+' : '-') <<
      Generics::Time(print.integer_part).get_gm_time().format("%H:%M");
    return out.str();
  }

  std::string
  account_type_flags(unsigned long flags)
  {
    using namespace AdServer::CampaignSvcs;
    std::ostringstream out;
    out << flags << ' ';
    out << (flags & AccountTypeFlags::GROSS ? "(gross)" : "(net)");
    if(flags & AccountTypeFlags::INVOICE_COMMISION)
    {
      out << "(invoice commision)";
    }
    out << (flags & AccountTypeFlags::USE_SELF_BUDGET ?
      "(advertiser budget)" : "(agency budget)");
    out << (flags & AccountTypeFlags::AGENCY_PROFIT_BY_PUB_AMOUNT ?
      "(agency profit by pub amount)" : "(fix price)");
    return out.str();
  }

  std::string
  colocation_ad_serving(unsigned long value)
  {
    using namespace AdServer::CampaignSvcs;
    switch(static_cast<ColocationAdServingType>(value))
    {
    case CS_ALL: return "all";
    case CS_ONLY_OPTIN: return "only optin";
    case CS_NON_OPTOUT: return "non optout";
    case CS_NONE: return "none";
    }
    return "unknown";
  }

  std::string
  creative_template_type(unsigned long value)
  {
    if(value == 1)
    {
      return "Text";
    }
    if(value == 2)
    {
      return "Xslt";
    }
    return "Unknown";
  }

  std::string
  ccg_type_string(unsigned long value)
  {
    using namespace AdServer::CampaignSvcs;
    switch(static_cast<CCGType>(value))
    {
    case CT_DISPLAY: return "display";
    case CT_TEXT: return "text";
    case CT_ALL: return "all";
    }
    return "unk";
  }

  std::string
  ccg_rate_type_string(unsigned long value)
  {
    using namespace AdServer::CampaignSvcs;
    switch(static_cast<CCGRateType>(value))
    {
    case CR_CPM: return "cpm";
    case CR_CPC: return "cpc";
    case CR_CPA: return "cpa";
    case CR_MAXBID: return "max bid";
    case CR_ALL: return "all";
    }
    return "unk";
  }

  std::string
  campaign_mode_string(unsigned long value)
  {
    using namespace AdServer::CampaignSvcs;
    if(value == CM_RANDOM)
    {
      return "random";
    }
    if(value == CM_NON_RANDOM)
    {
      return "normal";
    }
    return "invalid !!!";
  }

  std::string
  bid_strategy_string(unsigned long value)
  {
    using namespace AdServer::CampaignSvcs;
    if(value == BS_MAX_REACH)
    {
      return "max-reach";
    }
    if(value == BS_MIN_CTR_GOAL)
    {
      return "min-ctr-goal";
    }
    return "unknown";
  }

  std::string
  campaign_flags_string(unsigned long flags)
  {
    using namespace AdServer::CampaignSvcs;
    std::ostringstream out;
    out << flags << ' ';
    if(flags & CampaignFlags::INCLUDE_SPECIFIC_SITES)
    {
      out << "(specific sites)";
    }
    if(flags & CampaignFlags::TRACK_ACTIONS)
    {
      out << "(track actions)";
    }
    if(flags & CampaignFlags::US_NONE)
    {
      out << "(us: none)";
    }
    if(flags & CampaignFlags::US_OPTIN)
    {
      out << "(us: optin)";
    }
    if(flags & CampaignFlags::US_OPTOUT)
    {
      out << "(us: optout)";
    }
    if(flags & CampaignFlags::US_UNDEFINED)
    {
      out << "(us: undefined)";
    }
    return out.str();
  }

  std::string
  weekly_run_convert(unsigned long weekly_run)
  {
    std::ostringstream out;
    const Generics::Time time_val(Generics::Time::ONE_MINUTE * weekly_run);
    out << (time_val / Generics::Time::ONE_DAY.tv_sec).tv_sec << ":" <<
      time_val.get_gm_time().format("%H.%M");
    return out.str();
  }

  std::string
  weekly_run_value(const cm::ConfigCampaign& campaign)
  {
    if(campaign.weekly_run_intervals_size() == 0)
    {
      return "none";
    }

    std::ostringstream out;
    for(int i = 0; i < campaign.weekly_run_intervals_size(); ++i)
    {
      const auto& interval = campaign.weekly_run_intervals(i);
      out << (i != 0 ? ",(" : "(") <<
        weekly_run_convert(interval.min()) <<
        "," << weekly_run_convert(interval.max()) << ")";
    }
    return out.str();
  }

  std::string
  creative_ids(const cm::ConfigCampaign& campaign)
  {
    std::ostringstream out;
    for(int i = 0; i < campaign.creatives_size(); ++i)
    {
      out << (i > 0 ? ", " : "") << campaign.creatives(i).ccid();
    }
    return out.str();
  }

  std::string
  decimal_from_ulong_tail(
    const google::protobuf::RepeatedField<google::protobuf::uint64>& values,
    int pos)
  {
    using RevenueDecimal = AdServer::CampaignSvcs::RevenueDecimal;
    const unsigned long count = RevenueDecimal::PACK_SIZE / 4 +
      (RevenueDecimal::PACK_SIZE % 4 ? 1 : 0);
    uint32_t buf[count];
    if(values.size() - pos < static_cast<int>(count))
    {
      return RevenueDecimal::ZERO.str();
    }
    for(unsigned long i = 0; i < count; ++i)
    {
      buf[i] = static_cast<uint32_t>(values.Get(pos + i));
    }
    RevenueDecimal decimal;
    decimal.unpack(buf);
    return decimal.str();
  }

  using ExpressionChannelMap = std::unordered_map<
    unsigned long,
    const cm::ConfigExpressionChannel*>;

  std::string
  expression_value(
    const cm::ConfigExpression& expression,
    const ExpressionChannelMap* expression_channels,
    std::set<unsigned long>& expanded_channels)
  {
    const auto op = static_cast<char>(expression.operation());
    if(op == '-')
    {
      const unsigned long channel_id = expression.channel_id();
      if(channel_id && expression_channels)
      {
        const auto channel_it = expression_channels->find(channel_id);
        if(channel_it != expression_channels->end() &&
          (channel_it->second->expression().sub_channels_size() > 0 ||
            channel_it->second->expression().channel_id() != 0) &&
          expanded_channels.insert(channel_id).second)
        {
          const std::string result = expression_value(
            channel_it->second->expression(),
            expression_channels,
            expanded_channels);
          expanded_channels.erase(channel_id);
          return result;
        }
      }

      return channel_id ? (
        std::string("[") + std::to_string(channel_id) + "]") :
        std::string("NULL");
    }
    if(op == 'T')
    {
      return "TRUE";
    }
    if(op == 0)
    {
      return expression.channel_id() ? (
        std::string("[") + std::to_string(expression.channel_id()) + "]") :
        std::string("NULL");
    }

    std::ostringstream out;
    out << '(';
    for(int i = 0; i < expression.sub_channels_size(); ++i)
    {
      if(i != 0)
      {
        out << ' ' << op << ' ';
      }
      out << expression_value(
        expression.sub_channels(i),
        expression_channels,
        expanded_channels);
    }
    out << ')';
    return out.str();
  }

  std::string
  expression_value(
    const cm::ConfigExpression& expression,
    const ExpressionChannelMap* expression_channels = nullptr)
  {
    std::set<unsigned long> expanded_channels;
    return expression_value(expression, expression_channels, expanded_channels);
  }

  cm::CampaignConfig
  fetch_config(
    const std::string& reference,
    bool geo_channels)
  {
    cm::GetConfigRequest request;
    request.set_geo_channels(geo_channels);

    grpc::ChannelArguments channel_args;
    channel_args.SetMaxReceiveMessageSize(-1);
    channel_args.SetMaxSendMessageSize(-1);
    channel_args.SetInt(GRPC_ARG_ENABLE_HTTP_PROXY, 0);
    auto channel = grpc::CreateCustomChannel(
      reference,
      grpc::InsecureChannelCredentials(),
      channel_args);
    auto stub = cm::CampaignManagerGrpc::NewStub(channel);

    grpc::ClientContext context;
    cm::GetConfigResponse response;
    const auto status = stub->get_config(&context, request, &response);
    if(!status.ok())
    {
      std::ostringstream error;
      error << "gRPC get_config failed: code=" <<
        static_cast<int>(status.error_code()) <<
        ", message=" << status.error_message();
      throw std::runtime_error(error.str());
    }

    return response.config();
  }

  void
  print_campaigns(const cm::CampaignConfig& config)
  {
    Table table = make_table(
      CAMPAIGN_TABLE_COLUMNS,
      sizeof(CAMPAIGN_TABLE_COLUMNS) / sizeof(CAMPAIGN_TABLE_COLUMNS[0]));
    ExpressionChannelMap expression_channels;
    for(const auto& expression_channel : config.expression_channels())
    {
      expression_channels.emplace(
        expression_channel.channel_id(),
        &expression_channel);
    }

    for(const auto& adapted_campaign : config.campaigns())
    {
      const auto& campaign = adapted_campaign.info();
      const auto& ccg_limits = campaign.ccg_delivery_limits();
      const auto& cmp_limits = campaign.campaign_delivery_limits();

      Table::Row row(table.columns());
      row.add_field(campaign.campaign_id());
      row.add_field(time_value(campaign.timestamp()));
      row.add_field(campaign.account_id());
      row.add_field(campaign.advertiser_id());
      row.add_field(campaign.campaign_group_id());
      row.add_field(campaign.ccg_rate_id());
      row.add_field(std::string(1, static_cast<char>(campaign.ccg_rate_type())));
      row.add_field(campaign.fc_id());
      row.add_field(campaign.group_fc_id());
      row.add_field(weekly_run_value(campaign));
      row.add_field(decimal_value(adapted_campaign.ecpm()));
      row.add_field(decimal_value(adapted_campaign.ctr()));
      row.add_field(expression_value(
        campaign.expression(),
        &expression_channels));
      row.add_field(expression_value(
        campaign.stat_expression(),
        &expression_channels));
      row.add_field(campaign.country());
      row.add_field(join_numbers(campaign.sites()));
      row.add_field(creative_ids(campaign));
      row.add_field(std::string(1, static_cast<char>(campaign.status())));
      row.add_field(std::string(1, static_cast<char>(campaign.eval_status())));
      row.add_field(std::string(1, static_cast<char>(campaign.ccg_type())));
      row.add_field(std::string(1, static_cast<char>(campaign.target_type())));
      row.add_field(campaign.start_user_group_id());
      row.add_field(campaign.end_user_group_id());
      row.add_field(campaign.ctr_reset_id());
      row.add_field(campaign.random_imps());
      row.add_field(campaign_mode_string(campaign.mode()));
      row.add_field(GrpcAlgs::unpack_time(campaign.min_uid_age()).gm_ft());
      row.add_field(campaign.seq_set_rotate_imps());
      row.add_field(time_value(ccg_limits.date_start()));
      row.add_field(time_value(ccg_limits.date_end()));
      row.add_field(optional_decimal_value(ccg_limits.budget()));
      row.add_field(optional_decimal_value(ccg_limits.daily_budget()));
      row.add_field(std::string(1, static_cast<char>(ccg_limits.delivery_pacing())));
      row.add_field(ccg_limits.imps_defined() ?
        std::to_string(ccg_limits.imps()) : std::string());
      row.add_field(std::string());
      row.add_field(ccg_limits.clicks_defined() ?
        std::to_string(ccg_limits.clicks()) : std::string());
      row.add_field(std::string());
      row.add_field(time_value(cmp_limits.date_start()));
      row.add_field(time_value(cmp_limits.date_end()));
      row.add_field(optional_decimal_value(cmp_limits.budget()));
      row.add_field(optional_decimal_value(cmp_limits.daily_budget()));
      row.add_field(std::string(1, static_cast<char>(cmp_limits.delivery_pacing())));
      row.add_field(cmp_limits.imps_defined() ?
        std::to_string(cmp_limits.imps()) : std::string());
      row.add_field(std::string());
      row.add_field(cmp_limits.clicks_defined() ?
        std::to_string(cmp_limits.clicks()) : std::string());
      row.add_field(std::string());
      row.add_field(decimal_value(campaign.max_pub_share()));
      row.add_field(bid_strategy_string(campaign.bid_strategy()));
      row.add_field(decimal_value(campaign.min_ctr_goal()));
      row.add_field(decimal_value(campaign.commision()));
      row.add_field(campaign_flags_string(campaign.flags()));
      row.add_field(std::string(1, static_cast<char>(campaign.marketplace())));
      table.add_row(row);
    }
    table.dump(std::cout);
  }

  void
  print_creatives(
    const cm::CampaignConfig& config,
    unsigned long ccid,
    bool filter_by_ccid)
  {
    Table table = make_table(
      CREATIVE_TABLE_COLUMNS,
      sizeof(CREATIVE_TABLE_COLUMNS) / sizeof(CREATIVE_TABLE_COLUMNS[0]));
    const std::string prefix = make_prefix(
      CREATIVE_TABLE_COLUMNS,
      sizeof(CREATIVE_TABLE_COLUMNS) / sizeof(CREATIVE_TABLE_COLUMNS[0]));

    std::map<unsigned long, std::string> sizes;
    for(const auto& size : config.sizes())
    {
      sizes.emplace(size.size_id(), size.protocol_name());
    }

    for(const auto& adapted_campaign : config.campaigns())
    {
      const auto& campaign = adapted_campaign.info();
      for(const auto& creative : campaign.creatives())
      {
        if(filter_by_ccid && creative.ccid() != ccid)
        {
          continue;
        }

        Table::Row row(table.columns());
        row.add_field(creative.ccid());
        row.add_field(creative.creative_id());
        row.add_field(campaign.campaign_id());
        row.add_field(creative.fc_id());
        row.add_field(creative.weight());
        row.add_field(creative.creative_format());
        row.add_field(option_value(creative.click_url()));
        row.add_field(creative_sizes(creative, sizes));
        row.add_field(creative.version_id());
        row.add_field(static_cast<char>(creative.status()));
        row.add_field(join_numbers(creative.categories()));
        row.add_field(creative.order_set_id());
        row.add_field(creative.initial_contract_id());
        std::ostringstream options;
        options << option_values(creative.tokens());
        for(const auto& size : creative.sizes())
        {
          options << '\n' << prefix << '<' << size.size_id() <<
            "(size)>" << option_values(size.tokens());
        }
        row.add_field(options.str());
        table.add_row(row);
      }
    }

    table.dump(std::cout);
  }

  void
  print_tags(const cm::CampaignConfig& config)
  {
    Table table = make_table(
      TAGS_TABLE_COLUMNS,
      sizeof(TAGS_TABLE_COLUMNS) / sizeof(TAGS_TABLE_COLUMNS[0]));
    const std::string prefix = make_prefix(
      TAGS_TABLE_COLUMNS,
      sizeof(TAGS_TABLE_COLUMNS) / sizeof(TAGS_TABLE_COLUMNS[0]));

    std::map<unsigned long, std::string> sizes;
    for(const auto& size : config.sizes())
    {
      sizes.emplace(size.size_id(), size.protocol_name());
    }

    for(const auto& adapted_tag : config.tags())
    {
      const auto& tag = adapted_tag.info();
      Table::Row row(table.columns());
      row.add_field(tag.tag_id());
      row.add_field(tag.site_id());
      row.add_field(time_value(tag.timestamp()));

      std::ostringstream tag_sizes;
      for(const auto& size : tag.sizes())
      {
        tag_sizes << '[' << size.size_id() << ",'";
        const auto size_it = sizes.find(size.size_id());
        if(size_it != sizes.end())
        {
          tag_sizes << size_it->second;
        }
        tag_sizes << "'," << size.max_text_creatives() << ']';
      }
      row.add_field(tag_sizes.str());

      row.add_field(tag.imp_track_pixel());
      row.add_field(tag.passback());
      row.add_field(tag.passback_type());
      row.add_field(tag.flags());
      row.add_field(std::string(1, static_cast<char>(tag.marketplace())));
      row.add_field(decimal_value(tag.adjustment()));
      row.add_field(join_numbers(tag.accepted_categories()));
      row.add_field(join_numbers(tag.rejected_categories()));
      row.add_field(tag.allow_expandable());
      row.add_field(decimal_value(tag.auction_max_ecpm_share()));
      row.add_field(decimal_value(tag.auction_prop_probability_share()));
      row.add_field(decimal_value(tag.auction_random_share()));
      row.add_field(decimal_value(tag.cost_coef()));
      row.add_field(time_value(tag.tag_pricings_timestamp()));

      std::ostringstream pricing;
      pricing << "[country ccg_type ccg_rate_type site_rate_id cpm revenue share]";
      const int field_len = 9;
      for(int i = 0; i < tag.tag_pricings_size(); ++i)
      {
        const auto& tag_pricing = tag.tag_pricings(i);
        pricing << '\n';
        pricing.width(30);
        pricing << ' ';
        pricing.width(field_len);
        pricing << "'" << tag_pricing.country_code() << "'";
        pricing.width(field_len);
        pricing << ccg_type_string(tag_pricing.ccg_type());
        pricing.width(field_len);
        pricing << ccg_rate_type_string(tag_pricing.ccg_rate_type());
        pricing.width(field_len);
        pricing << tag_pricing.site_rate_id();
        pricing.width(field_len);
        pricing << (i < adapted_tag.cpms_size() ?
          decimal_integer_value(adapted_tag.cpms(i)) : std::string());
        pricing.width(field_len);
        pricing << decimal_value(tag_pricing.imp_revenue());
        pricing.width(field_len);
        pricing << decimal_value(tag_pricing.revenue_share());
      }
      row.add_field(pricing.str());

      row.add_field(option_values(tag.hidden_tokens()));
      row.add_field(option_values(tag.passback_tokens()));

      std::ostringstream options;
      options << option_values(tag.tokens());
      for(const auto& template_tokens : tag.template_tokens())
      {
        options << '\n' << prefix << '<' <<
          template_tokens.template_name() << "(template)>" <<
          option_values(template_tokens.tokens());
      }
      for(const auto& size : tag.sizes())
      {
        options << '\n' << prefix << '<' << size.size_id() <<
          "(size)>" << option_values(size.hidden_tokens());
        options << '\n' << prefix << '<' << size.size_id() <<
          "(size)>" << option_values(size.tokens());
      }
      row.add_field(options.str());
      table.add_row(row);
    }

    table.dump(std::cout);
  }

  std::string
  decode_ad_request_type(unsigned long type)
  {
    using namespace AdServer::CampaignSvcs;
    if(type == AR_NORMAL)
    {
      return "normal";
    }
    if(type == AR_OPENRTB)
    {
      return "openrtb";
    }
    if(type == AR_OPENRTB_WITH_CLICKURL)
    {
      return "openrtb-with-click";
    }
    if(type == AR_OPENX)
    {
      return "openx";
    }
    if(type == AR_APPNEXUS)
    {
      return "appnexus";
    }
    if(type == AR_GOOGLE)
    {
      return "google";
    }
    return "unknown";
  }

  std::string
  join_strings(const google::protobuf::RepeatedPtrField<std::string>& values)
  {
    std::ostringstream out;
    for(int i = 0; i < values.size(); ++i)
    {
      if(i > 0)
      {
        out << ',';
      }
      out << values.Get(i);
    }
    return out.str();
  }

  void
  print_globals(const cm::CampaignConfig& config)
  {
    Table table = make_table(
      GLOBAL_TABLE_COLUMNS,
      sizeof(GLOBAL_TABLE_COLUMNS) / sizeof(GLOBAL_TABLE_COLUMNS[0]));
    Table::Row row(table.columns());
    row.add_field(config.currency_exchange_id());
    row.add_field(GrpcAlgs::unpack_time(
      config.fraud_user_deactivate_period()).tv_sec);
    row.add_field(decimal_value(config.cost_limit()));
    row.add_field(config.google_publisher_account_id());
    row.add_field(time_value(config.global_params_timestamp()));
    row.add_field(time_value(config.master_stamp()));
    row.add_field(time_value(config.first_load_stamp()));
    row.add_field(time_value(config.finish_load_stamp()));
    table.add_row(row);
    table.dump(std::cout);
  }

  void
  print_simple_config_table(
    const cm::CampaignConfig& config,
    const std::string& command)
  {
    if(command == "freq_cap" || command == "freq_caps")
    {
      Table table = make_table(
        FREQ_CAPS_TABLE_COLUMNS,
        sizeof(FREQ_CAPS_TABLE_COLUMNS) / sizeof(FREQ_CAPS_TABLE_COLUMNS[0]));
      for(const auto& item : config.frequency_caps())
      {
        Table::Row row(table.columns());
        row.add_field(item.fc_id());
        row.add_field(time_value(item.timestamp()));
        row.add_field(item.lifelimit());
        row.add_field(item.period());
        row.add_field(item.window_limit());
        row.add_field(item.window_time());
        table.add_row(row);
      }
      table.dump(std::cout);
    }
    else if(command == "app_format")
    {
      Table table = make_table(
        APP_FORMAT_COLUMNS,
        sizeof(APP_FORMAT_COLUMNS) / sizeof(APP_FORMAT_COLUMNS[0]));
      for(const auto& item : config.app_formats())
      {
        Table::Row row(table.columns());
        row.add_field(item.app_format());
        row.add_field(item.mime_format());
        row.add_field(time_value(item.timestamp()));
        table.add_row(row);
      }
      table.dump(std::cout);
    }
    else if(command == "size")
    {
      Table table = make_table(
        SIZE_TABLE_COLUMNS,
        sizeof(SIZE_TABLE_COLUMNS) / sizeof(SIZE_TABLE_COLUMNS[0]));
      for(const auto& item : config.sizes())
      {
        Table::Row row(table.columns());
        row.add_field(item.size_id());
        row.add_field(item.size_type_id());
        row.add_field(item.protocol_name());
        row.add_field(item.width());
        row.add_field(item.height());
        row.add_field(time_value(item.timestamp()));
        table.add_row(row);
      }
      table.dump(std::cout);
    }
    else if(command == "currency" || command == "currencies")
    {
      Table table = make_table(
        CURRENCY_TABLE_COLUMNS,
        sizeof(CURRENCY_TABLE_COLUMNS) / sizeof(CURRENCY_TABLE_COLUMNS[0]));
      for(const auto& item : config.currencies())
      {
        Table::Row row(table.columns());
        row.add_field(item.currency_id());
        row.add_field(item.currency_exchange_id());
        row.add_field(
          Generics::Time(item.effective_date()).get_gm_time().format("%F"));
        row.add_field(decimal_value(item.rate()));
        row.add_field(item.fraction_digits());
        row.add_field(item.currency_code());
        row.add_field(time_value(item.timestamp()));
        table.add_row(row);
      }
      table.dump(std::cout);
    }
    else if(command == "creative_option" || command == "creative_options")
    {
      Table table = make_table(
        CREATIVE_OPTION_TABLE_COLUMNS,
        sizeof(CREATIVE_OPTION_TABLE_COLUMNS) /
          sizeof(CREATIVE_OPTION_TABLE_COLUMNS[0]));
      for(const auto& item : config.creative_options())
      {
        Table::Row row(table.columns());
        row.add_field(item.option_id());
        row.add_field(item.token());
        row.add_field(std::string(1, static_cast<char>(item.type())));
        row.add_field(time_value(item.timestamp()));
        row.add_field(join_strings(item.token_relations()));
        table.add_row(row);
      }
      table.dump(std::cout);
    }
    else if(command == "keyword")
    {
      Table table = make_table(
        CAMPAIGN_KEYWORD_TABLE_COLUMNS,
        sizeof(CAMPAIGN_KEYWORD_TABLE_COLUMNS) /
          sizeof(CAMPAIGN_KEYWORD_TABLE_COLUMNS[0]));
      for(const auto& item : config.campaign_keywords())
      {
        Table::Row row(table.columns());
        row.add_field(item.ccg_keyword_id());
        row.add_field(item.original_keyword());
        row.add_field(item.click_url());
        row.add_field(time_value(item.timestamp()));
        table.add_row(row);
      }
      table.dump(std::cout);
    }
    else if(command == "creative_categories")
    {
      Table table = make_table(
        CREATIVE_CATEGORY_TABLE_COLUMNS,
        sizeof(CREATIVE_CATEGORY_TABLE_COLUMNS) /
          sizeof(CREATIVE_CATEGORY_TABLE_COLUMNS[0]));
      for(const auto& item : config.creative_categories())
      {
        Table::Row row(table.columns());
        row.add_field(item.creative_category_id());
        row.add_field(item.cct_id());
        row.add_field(item.name());
        std::ostringstream external_categories;
        for(const auto& external_category : item.external_categories())
        {
          external_categories << "[ " <<
            decode_ad_request_type(external_category.ad_request_type()) <<
            ": " << join_strings(external_category.names()) << " ]";
        }
        row.add_field(external_categories.str());
        row.add_field(time_value(item.timestamp()));
        table.add_row(row);
      }
      table.dump(std::cout);
    }
    else if(command == "category_channel")
    {
      Table table = make_table(
        CATEGORY_CHANNEL_TABLE_COLUMNS,
        sizeof(CATEGORY_CHANNEL_TABLE_COLUMNS) /
          sizeof(CATEGORY_CHANNEL_TABLE_COLUMNS[0]));
      for(const auto& item : config.category_channels())
      {
        Table::Row row(table.columns());
        row.add_field(item.channel_id());
        row.add_field(item.name());
        row.add_field(item.newsgate_name());
        row.add_field(item.parent_channel_id());
        row.add_field(item.flags());
        row.add_field(time_value(item.timestamp()));
        std::ostringstream localizations;
        for(const auto& localization : item.localizations())
        {
          localizations << "(" << localization.language() << ", '" <<
            localization.name() << "') ";
        }
        row.add_field(localizations.str());
        table.add_row(row);
      }
      table.dump(std::cout);
    }
    else if(command == "geo_channel")
    {
      Table table = make_table(
        GEO_CHANNEL_TABLE_COLUMNS,
        sizeof(GEO_CHANNEL_TABLE_COLUMNS) / sizeof(GEO_CHANNEL_TABLE_COLUMNS[0]));
      for(const auto& item : config.geo_channels())
      {
        Table::Row row(table.columns());
        row.add_field(item.channel_id());
        row.add_field(item.country());
        if(item.geoip_targets_size() > 0)
        {
          row.add_field(item.geoip_targets(0).region());
          row.add_field(item.geoip_targets(0).city());
        }
        else
        {
          row.add_field("");
          row.add_field("");
        }
        table.add_row(row);
      }
      table.dump(std::cout);
    }
    else if(command == "geo_coord_channel")
    {
      Table table = make_table(
        GEO_COORD_CHANNEL_TABLE_COLUMNS,
        sizeof(GEO_COORD_CHANNEL_TABLE_COLUMNS) /
          sizeof(GEO_COORD_CHANNEL_TABLE_COLUMNS[0]));
      for(const auto& item : config.geo_coord_channels())
      {
        using CoordDecimal = AdServer::CampaignSvcs::CoordDecimal;
        using AccuracyDecimal = AdServer::CampaignSvcs::AccuracyDecimal;
        auto unpack_coord = [](const cm::DecimalInfo& value) {
          CoordDecimal decimal;
          decimal.unpack(value.value().data());
          return decimal.str();
        };
        auto unpack_accuracy = [](const cm::DecimalInfo& value) {
          AccuracyDecimal decimal;
          decimal.unpack(value.value().data());
          return decimal.str();
        };

        Table::Row row(table.columns());
        row.add_field(item.channel_id());
        row.add_field(unpack_coord(item.longitude()));
        row.add_field(unpack_coord(item.latitude()));
        row.add_field(unpack_accuracy(item.radius()));
        table.add_row(row);
      }
      table.dump(std::cout);
    }
    else if(command == "web_operations")
    {
      Table table = make_table(
        WEB_APPLICATIONS_TABLE_COLUMNS,
        sizeof(WEB_APPLICATIONS_TABLE_COLUMNS) /
          sizeof(WEB_APPLICATIONS_TABLE_COLUMNS[0]));
      for(const auto& item : config.web_operations())
      {
        Table::Row row(table.columns());
        row.add_field(item.id());
        row.add_field(item.app());
        row.add_field(item.source());
        row.add_field(item.operation());
        row.add_field(item.flags());
        table.add_row(row);
      }
      table.dump(std::cout);
    }
    else if(command == "sites")
    {
      Table table = make_table(
        SITES_TABLE_COLUMNS,
        sizeof(SITES_TABLE_COLUMNS) / sizeof(SITES_TABLE_COLUMNS[0]));
      for(const auto& item : config.sites())
      {
        Table::Row row(table.columns());
        row.add_field(item.site_id());
        row.add_field(time_value(item.timestamp()));
        row.add_field(std::string(1, static_cast<char>(item.status())));
        row.add_field(item.noads_timeout());
        row.add_field(item.freq_cap_id());
        row.add_field(join_numbers(item.approved_creative_categories()));
        row.add_field(join_numbers(item.rejected_creative_categories()));
        row.add_field(join_numbers(item.approved_creatives()));
        row.add_field(join_numbers(item.rejected_creatives()));
        row.add_field(item.flags());
        row.add_field(item.account_id());
        table.add_row(row);
      }
      table.dump(std::cout);
    }
    else if(command == "countries")
    {
      Table table = make_table(
        COUNTRIES_TABLE_COLUMNS,
        sizeof(COUNTRIES_TABLE_COLUMNS) / sizeof(COUNTRIES_TABLE_COLUMNS[0]));
      for(const auto& item : config.countries())
      {
        Table::Row row(table.columns());
        row.add_field(item.country_code());
        row.add_field(option_values(item.tokens()));
        row.add_field(time_value(item.timestamp()));
        table.add_row(row);
      }
      table.dump(std::cout);
    }
    else if(command == "colocations")
    {
      Table table = make_table(
        COLOCATIONS_TABLE_COLUMNS,
        sizeof(COLOCATIONS_TABLE_COLUMNS) / sizeof(COLOCATIONS_TABLE_COLUMNS[0]));
      for(const auto& item : config.colocations())
      {
        Table::Row row(table.columns());
        row.add_field(item.colo_name());
        row.add_field(item.colo_id());
        row.add_field(item.colo_rate_id());
        row.add_field(item.account_id());
        row.add_field(decimal_value(item.revenue_share()));
        row.add_field(colocation_ad_serving(item.ad_serving()));
        row.add_field(item.hid_profile());
        row.add_field(option_values(item.tokens()));
        row.add_field(time_value(item.timestamp()));
        table.add_row(row);
      }
      table.dump(std::cout);
    }
    else if(command == "creative_templates")
    {
      Table table = make_table(
        CREATIVE_TEMPLATES_TABLE_COLUMNS,
        sizeof(CREATIVE_TEMPLATES_TABLE_COLUMNS) /
          sizeof(CREATIVE_TEMPLATES_TABLE_COLUMNS[0]));
      for(const auto& item : config.creative_template_files())
      {
        Table::Row row(table.columns());
        row.add_field(item.creative_format());
        row.add_field(item.creative_size());
        row.add_field(item.app_format());
        row.add_field(item.mime_format());
        row.add_field(item.track_impr());
        row.add_field(item.template_file());
        row.add_field(creative_template_type(item.type()));
        row.add_field(time_value(item.timestamp()));
        row.add_field(std::string(1, static_cast<char>(item.status())));
        row.add_field(option_values(item.tokens()));
        row.add_field(option_values(item.hidden_tokens()));
        table.add_row(row);
      }
      table.dump(std::cout);
    }
    else if(command == "account")
    {
      Table table = make_table(
        ACCOUNT_TABLE_COLUMNS,
        sizeof(ACCOUNT_TABLE_COLUMNS) / sizeof(ACCOUNT_TABLE_COLUMNS[0]));
      for(const auto& item : config.accounts())
      {
        Table::Row row(table.columns());
        row.add_field(item.account_id());
        row.add_field(item.agency_account_id());
        row.add_field(item.internal_account_id());
        row.add_field(std::string(1, static_cast<char>(item.status())));
        row.add_field(std::string(1, static_cast<char>(item.eval_status())));
        row.add_field(item.legal_name());
        row.add_field(item.flags());
        row.add_field(account_type_flags(item.at_flags()));
        row.add_field(std::string(1, static_cast<char>(item.text_adserving())));
        row.add_field(item.currency_id());
        row.add_field(item.country());
        row.add_field(decimal_value(item.commision()));
        row.add_field(decimal_value(item.budget()));
        row.add_field(decimal_value(item.paid_amount()));
        row.add_field(time_offset_value(item.time_offset()));
        row.add_field(join_numbers(item.walled_garden_accounts()));
        row.add_field(item.auction_rate() == 0 ? "gross" : "net");
        row.add_field(item.use_pub_pixels());
        row.add_field(item.pub_pixel_optin());
        row.add_field(item.pub_pixel_optout());
        row.add_field(decimal_value(item.self_service_commission()));
        row.add_field(time_value(item.timestamp()));
        table.add_row(row);
      }
      table.dump(std::cout);
    }
    else if(command == "adv_action")
    {
      Table table = make_table(
        ADV_ACTION_TABLE_COLUMNS,
        sizeof(ADV_ACTION_TABLE_COLUMNS) / sizeof(ADV_ACTION_TABLE_COLUMNS[0]));
      for(const auto& item : config.adv_actions())
      {
        Table::Row row(table.columns());
        row.add_field(item.action_id());
        row.add_field(time_value(item.timestamp()));
        std::set<unsigned long> ccg_ids;
        int i = 0;
        for(; i < item.ccg_ids_size() && item.ccg_ids(i) != 0; ++i)
        {
          ccg_ids.insert(item.ccg_ids(i));
        }
        row.add_field(i < item.ccg_ids_size() ?
          decimal_from_ulong_tail(item.ccg_ids(), i + 1) :
          AdServer::CampaignSvcs::RevenueDecimal::ZERO.str());
        std::ostringstream ids;
        for(auto it = ccg_ids.begin(); it != ccg_ids.end(); ++it)
        {
          if(it != ccg_ids.begin())
          {
            ids << ", ";
          }
          ids << *it;
        }
        row.add_field(ids.str());
        table.add_row(row);
      }
      table.dump(std::cout);
    }
    else if(command == "contract" || command == "contracts")
    {
      Table table = make_table(
        CONTRACT_TABLE_COLUMNS,
        sizeof(CONTRACT_TABLE_COLUMNS) / sizeof(CONTRACT_TABLE_COLUMNS[0]));
      for(const auto& item : config.contracts())
      {
        Table::Row row(table.columns());
        row.add_field(item.number());
        row.add_field(item.date());
        row.add_field(item.type());
        row.add_field(item.vat_included());
        row.add_field(item.ord_contract_id());
        row.add_field(item.ord_ado_id());
        row.add_field(item.subject_type());
        row.add_field(item.action_type());
        row.add_field(item.agent_acting_for_publisher());
        row.add_field(item.parent_contract_id());
        row.add_field(item.client_id());
        row.add_field(item.client_name());
        row.add_field(item.client_legal_form());
        row.add_field(item.contractor_id());
        row.add_field(item.contractor_name());
        row.add_field(item.contractor_legal_form());
        row.add_field(time_value(item.timestamp()));
        table.add_row(row);
      }
      table.dump(std::cout);
    }
  }

  std::string
  field_value(
    const google::protobuf::Message& message,
    const google::protobuf::FieldDescriptor& field,
    int index)
  {
    const auto* reflection = message.GetReflection();
    switch(field.cpp_type())
    {
    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
      return std::to_string(field.is_repeated() ?
        reflection->GetRepeatedInt32(message, &field, index) :
        reflection->GetInt32(message, &field));
    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
      return std::to_string(field.is_repeated() ?
        reflection->GetRepeatedInt64(message, &field, index) :
        reflection->GetInt64(message, &field));
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
      return std::to_string(field.is_repeated() ?
        reflection->GetRepeatedUInt32(message, &field, index) :
        reflection->GetUInt32(message, &field));
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
      return std::to_string(field.is_repeated() ?
        reflection->GetRepeatedUInt64(message, &field, index) :
        reflection->GetUInt64(message, &field));
    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
      return std::to_string(field.is_repeated() ?
        reflection->GetRepeatedDouble(message, &field, index) :
        reflection->GetDouble(message, &field));
    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
      return std::to_string(field.is_repeated() ?
        reflection->GetRepeatedFloat(message, &field, index) :
        reflection->GetFloat(message, &field));
    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
      return (field.is_repeated() ?
        reflection->GetRepeatedBool(message, &field, index) :
        reflection->GetBool(message, &field)) ? "1" : "0";
    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
      return field.type() == google::protobuf::FieldDescriptor::TYPE_BYTES ?
        bytes_value(field.is_repeated() ?
          reflection->GetRepeatedString(message, &field, index) :
          reflection->GetString(message, &field)) :
        (field.is_repeated() ?
          reflection->GetRepeatedString(message, &field, index) :
          reflection->GetString(message, &field));
    case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
      return field.is_repeated() ?
        reflection->GetRepeatedMessage(message, &field, index).ShortDebugString() :
        reflection->GetMessage(message, &field).ShortDebugString();
    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
      return field.is_repeated() ?
        reflection->GetRepeatedEnum(message, &field, index)->name() :
        reflection->GetEnum(message, &field)->name();
    }

    return std::string();
  }

  std::string
  field_values(
    const google::protobuf::Message& message,
    const google::protobuf::FieldDescriptor& field)
  {
    const auto* reflection = message.GetReflection();
    if(!field.is_repeated())
    {
      return reflection->HasField(message, &field) ?
        field_value(message, field, 0) :
        std::string();
    }

    std::ostringstream out;
    const int size = reflection->FieldSize(message, &field);
    for(int i = 0; i < size; ++i)
    {
      if(i > 0)
      {
        out << ',';
      }
      out << field_value(message, field, i);
    }
    return out.str();
  }

  void
  print_message_table(
    const google::protobuf::Message& owner,
    const google::protobuf::FieldDescriptor& repeated_field)
  {
    const auto* owner_reflection = owner.GetReflection();
    const int rows_size = owner_reflection->FieldSize(owner, &repeated_field);
    if(rows_size == 0)
    {
      return;
    }

    const auto& first = owner_reflection->GetRepeatedMessage(
      owner,
      &repeated_field,
      0);
    const auto* descriptor = first.GetDescriptor();
    for(int field_i = 0; field_i < descriptor->field_count(); ++field_i)
    {
      if(field_i > 0)
      {
        std::cout << '\t';
      }
      std::cout << descriptor->field(field_i)->name();
    }
    std::cout << '\n';

    for(int row_i = 0; row_i < rows_size; ++row_i)
    {
      const auto& row = owner_reflection->GetRepeatedMessage(
        owner,
        &repeated_field,
        row_i);
      for(int field_i = 0; field_i < descriptor->field_count(); ++field_i)
      {
        if(field_i > 0)
        {
          std::cout << '\t';
        }
        std::cout << field_values(row, *descriptor->field(field_i));
      }
      std::cout << '\n';
    }
  }

  bool
  print_config_command(
    const std::string& reference,
    const std::string& command,
    bool geo_channels,
    unsigned long ccid,
    bool filter_by_ccid)
  {
    static const std::unordered_map<std::string, std::string> FIELD_BY_COMMAND =
    {
      {"app_format", "app_formats"},
      {"size", "sizes"},
      {"creative_categories", "creative_categories"},
      {"sites", "sites"},
      {"colocations", "colocations"},
      {"countries", "countries"},
      {"creative_templates", "creative_template_files"},
      {"tags", "tags"},
      {"currency", "currencies"},
      {"currencies", "currencies"},
      {"freq_cap", "frequency_caps"},
      {"freq_caps", "frequency_caps"},
      {"creative_option", "creative_options"},
      {"creative_options", "creative_options"},
      {"adv_action", "adv_actions"},
      {"category_channel", "category_channels"},
      {"geo_channel", "geo_channels"},
      {"geo_coord_channel", "geo_coord_channels"},
      {"web_operations", "web_operations"},
      {"keyword", "campaign_keywords"},
      {"account", "accounts"},
      {"expression_channel", "expression_channels"},
      {"contract", "contracts"},
      {"contracts", "contracts"}
    };
    static const std::unordered_map<std::string, bool> SIMPLE_TABLE_COMMAND =
    {
      {"app_format", true},
      {"size", true},
      {"creative_categories", true},
      {"currency", true},
      {"currencies", true},
      {"freq_cap", true},
      {"freq_caps", true},
      {"creative_option", true},
      {"creative_options", true},
      {"category_channel", true},
      {"geo_channel", true},
      {"geo_coord_channel", true},
      {"web_operations", true},
      {"keyword", true},
      {"sites", true},
      {"colocations", true},
      {"countries", true},
      {"creative_templates", true},
      {"account", true},
      {"adv_action", true},
      {"contract", true},
      {"contracts", true}
    };

    if(command != "globals" && command != "campaign" &&
      command != "creative" && !FIELD_BY_COMMAND.count(command))
    {
      return false;
    }

    const cm::CampaignConfig config = fetch_config(
      reference,
      geo_channels || command == "geo_channel" ||
        command == "geo_coord_channel");
    if(command == "globals")
    {
      print_globals(config);
    }
    else if(command == "campaign")
    {
      print_campaigns(config);
    }
    else if(command == "creative")
    {
      print_creatives(config, ccid, filter_by_ccid);
    }
    else if(command == "tags")
    {
      print_tags(config);
    }
    else if(SIMPLE_TABLE_COMMAND.count(command))
    {
      print_simple_config_table(config, command);
    }
    else
    {
      const auto* descriptor = config.GetDescriptor();
      const auto* field = descriptor->FindFieldByName(
        FIELD_BY_COMMAND.at(command));
      if(!field)
      {
        throw std::runtime_error(
          "internal error: config field not found for command: " + command);
      }
      print_message_table(config, *field);
    }

    return true;
  }
}

int
main(int argc, char** argv)
{
  try
  {
    Generics::AppUtils::Args args(-1);
    Generics::AppUtils::StringOption reference;
    Generics::AppUtils::StringOption file;
    Generics::AppUtils::StringOption service_index;
    Generics::AppUtils::StringOption channels;
    Generics::AppUtils::StringOption country;
    Generics::AppUtils::StringOption language;
    Generics::AppUtils::StringOption publisher_account_ids;
    Generics::AppUtils::StringOption format("html");
    Generics::AppUtils::StringOption referer;
    Generics::AppUtils::StringOption peer_ip;
    Generics::AppUtils::StringOption uid;
    Generics::AppUtils::StringOption client_name;
    Generics::AppUtils::StringOption client_version;
    Generics::AppUtils::StringOption platform;
    Generics::AppUtils::StringOption browser;
    Generics::AppUtils::CheckOption match;
    Generics::AppUtils::CheckOption all;
    Generics::AppUtils::CheckOption geo_channels;
    Generics::AppUtils::CheckOption optout;
    Generics::AppUtils::Option<unsigned long> ccid(0);
    Generics::AppUtils::Option<unsigned long> tag_id(0);
    Generics::AppUtils::Option<unsigned long> user_status(0);
    Generics::AppUtils::Option<unsigned long> colo_id(0);
    Generics::AppUtils::Option<unsigned long> random(0);
    Generics::AppUtils::Option<unsigned long> testrequest(0);

    args.add(
      Generics::AppUtils::equal_name("reference") ||
      Generics::AppUtils::short_name("r"),
      reference);
    args.add(Generics::AppUtils::equal_name("file"), file);
    args.add(Generics::AppUtils::equal_name("service_index"), service_index);
    args.add(Generics::AppUtils::equal_name("channels"), channels);
    args.add(Generics::AppUtils::equal_name("country"), country);
    args.add(Generics::AppUtils::equal_name("language"), language);
    args.add(
      Generics::AppUtils::equal_name("publisher_account_ids"),
      publisher_account_ids);
    args.add(Generics::AppUtils::equal_name("format"), format);
    args.add(
      Generics::AppUtils::equal_name("referer") ||
      Generics::AppUtils::short_name("R"),
      referer);
    args.add(Generics::AppUtils::equal_name("peer_ip"), peer_ip);
    args.add(Generics::AppUtils::equal_name("uid"), uid);
    args.add(Generics::AppUtils::equal_name("client"), client_name);
    args.add(Generics::AppUtils::equal_name("client_version"), client_version);
    args.add(Generics::AppUtils::equal_name("platform"), platform);
    args.add(Generics::AppUtils::equal_name("browser"), browser);
    args.add(Generics::AppUtils::equal_name("match"), match);
    args.add(Generics::AppUtils::equal_name("all"), all);
    args.add(Generics::AppUtils::equal_name("geo_channels"), geo_channels);
    args.add(Generics::AppUtils::equal_name("optout"), optout);
    args.add(
      Generics::AppUtils::equal_name("id") ||
      Generics::AppUtils::short_name("i"),
      ccid);
    args.add(Generics::AppUtils::equal_name("tag_id"), tag_id);
    args.add(Generics::AppUtils::equal_name("user_status"), user_status);
    args.add(Generics::AppUtils::equal_name("colo_id"), colo_id);
    args.add(Generics::AppUtils::equal_name("random"), random);
    args.add(Generics::AppUtils::equal_name("testrequest"), testrequest);

    args.parse(argc - 1, argv + 1);
    const auto& commands = args.commands();
    if(commands.empty() || commands.front() == "help")
    {
      std::cout << USAGE;
      return 0;
    }
    if(!reference.installed())
    {
      throw std::runtime_error("'reference' option is required");
    }

    const std::string& command = commands.front();

    if(print_config_command(
      *reference,
      command,
      geo_channels.enabled(),
      *ccid,
      ccid.installed()))
    {
      return 0;
    }

    ClientHolder holder = create_client(*reference);
    const auto guard = std::unique_ptr<ClientHolder, void(*)(ClientHolder*)>(
      &holder,
      [](ClientHolder* value)
      {
        shutdown_client(*value);
      });
    auto& client = *holder.client;

    if(command == "ready")
    {
      const auto response = call<cm::ReadyResponse>(
        client,
        cm::ReadyRequest(),
        &AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient::ready);
      std::cout << (response.ready() ? "ready" : "not ready") << '\n';
    }
    else if(command == "progress")
    {
      const auto response = call<cm::ProgressCommentResponse>(
        client,
        cm::ProgressCommentRequest(),
        &AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient::progress_comment);
      std::cout << response.comment() << '\n';
    }
    else if(command == "select")
    {
      const Generics::Time now = Generics::Time::get_time_of_day();
      cm::GetCampaignCreativeRequest request;
      auto* params = request.mutable_request_params();
      fill_common_request(
        *params->mutable_common_info(),
        now,
        uid,
        referer,
        peer_ip,
        country,
        *colo_id,
        *random,
        optout.enabled());
      params->mutable_context_info()->set_client(option_value(client_name));
      params->mutable_context_info()->set_client_version(
        option_value(client_version));
      params->mutable_context_info()->set_platform(option_value(platform));
      params->mutable_context_info()->set_web_browser(option_value(browser));
      fill_channels(channels, params->mutable_channels());
      params->set_need_debug_info(true);
      params->set_session_start(::GrpcAlgs::pack_time(now));
      auto* ad_slot = params->add_ad_slots();
      ad_slot->set_format(*format);
      ad_slot->set_tag_id(*tag_id);

      const auto response = call<cm::GetCampaignCreativeResponse>(
        client,
        request,
        &AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient::get_campaign_creative);

      if(response.request_result().ad_slots().empty())
      {
        std::cout << "creative :\n";
        return 0;
      }

      const auto& result_slot = response.request_result().ad_slots(0);
      const auto& debug_info = result_slot.debug_info();
      for(int i = 0; i < result_slot.selected_creatives_size(); ++i)
      {
        const auto& creative = result_slot.selected_creatives(i);
        std::cout << "\nccid : " << creative.ccid() <<
          "\ncreative_size : " << creative.creative_size() <<
          "\ncampaign_id : " << creative.cmp_id();
        if(i < debug_info.selected_creatives_size())
        {
          std::cout << "\ntriggered_expression : " <<
            debug_info.selected_creatives(i).triggered_expression();
        }
        std::cout << "\nclick_url : " << creative.click_url();
      }
      std::cout << "\ncreative :\n" << result_slot.creative_body() << '\n';
    }
    else if(command == "trace_index")
    {
      const auto response = call<cm::TraceCampaignSelectionIndexResponse>(
        client,
        cm::TraceCampaignSelectionIndexRequest(),
        &AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient::trace_campaign_selection_index);
      std::cout << response.trace_xml() << '\n';
    }
    else if(command == "trace_weight")
    {
      cm::TraceCampaignSelectionRequest request;
      request.set_campaign_id(*ccid);
      request.set_auction_type(0);
      request.set_test_request(
        testrequest.installed() && *testrequest != 0);

      const Generics::Time now = Generics::Time::get_time_of_day();
      auto* params = request.mutable_request_params();
      fill_common_request(
        *params->mutable_common_info(),
        now,
        uid,
        referer,
        peer_ip,
        country,
        *colo_id,
        *random,
        optout.enabled());
      params->mutable_common_info()->set_test_request(request.test_request());
      params->mutable_common_info()->set_log_as_test(true);
      params->set_profiling_available(true);
      params->set_disable_fraud_detection(true);
      fill_channels(channels, params->mutable_channels());
      params->set_client_create_time(::GrpcAlgs::pack_time(now));

      auto* ad_slot = request.mutable_ad_slot();
      ad_slot->set_format(*format);
      ad_slot->set_tag_id(*tag_id);
      ad_slot->set_passback(false);
      ad_slot->set_up_expand_space(0xFFFFFFFF);
      ad_slot->set_right_expand_space(0xFFFFFFFF);
      ad_slot->set_down_expand_space(0xFFFFFFFF);
      ad_slot->set_left_expand_space(0xFFFFFFFF);

      const auto response = call<cm::TraceCampaignSelectionResponse>(
        client,
        request,
        &AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient::trace_campaign_selection);
      std::cout << response.trace_xml() << '\n';
    }
    else if(command == "preview")
    {
      cm::GetCampaignCreativeByCcidRequest request;
      request.set_ccid(*ccid);
      request.set_tag_id(*tag_id);
      request.set_format(*format);
      request.set_original_url(*referer);
      request.set_peer_ip(*peer_ip);
      const auto response = call<cm::GetCampaignCreativeByCcidResponse>(
        client,
        request,
        &AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient::get_campaign_creative_by_ccid);
      if(response.found())
      {
        std::cout << response.creative_body() << '\n';
      }
      else
      {
        std::cerr << "creative not found\n";
        return 1;
      }
    }
    else if(command == "colocation_flags")
    {
      const auto response = call<cm::GetColocationFlagsResponse>(
        client,
        cm::GetColocationFlagsRequest(),
        &AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient::get_colocation_flags);
      print_colocation_flags(response);
    }
    else if(command == "pub_pixels")
    {
      cm::GetPubPixelsRequest request;
      request.set_country(*country);
      request.set_user_status(*user_status);
      if(publisher_account_ids.installed())
      {
        add_ids(
          parse_ids(*publisher_account_ids),
          request.mutable_publisher_account_ids());
      }
      const auto response = call<cm::GetPubPixelsResponse>(
        client,
        request,
        &AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient::get_pub_pixels);
      for(const auto& pixel : response.pixels())
      {
        std::cout << pixel << '\n';
      }
    }
    else if(command == "category_channels")
    {
      cm::GetCategoryChannelsRequest request;
      request.set_language(*language);
      const auto response = call<cm::GetCategoryChannelsResponse>(
        client,
        request,
        &AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient::get_category_channels);
      print_category_channels(response.channels());
    }
    else if(command == "channel_links")
    {
      if(!channels.installed())
      {
        throw std::runtime_error("'channels' option is required");
      }
      cm::GetChannelLinksRequest request;
      add_ids(parse_ids(*channels), request.mutable_channels());
      request.set_match(match.enabled());
      const auto response = call<cm::GetChannelLinksResponse>(
        client,
        request,
        &AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient::get_channel_links);
      std::cout << "channel_id\tuse_count\tlanguage\tdiscover_query\n";
      for(const auto& channel : response.channels())
      {
        std::cout << channel.channel_id() << '\t' <<
          channel.use_count() << '\t' <<
          channel.language() << '\t' <<
          channel.discover_query() << '\n';
      }
    }
    else if(command == "discover_channels")
    {
      if(!channels.installed())
      {
        throw std::runtime_error("'channels' option is required");
      }
      cm::GetDiscoverChannelsRequest request;
      String::StringManip::SplitComma tokenizer(*channels);
      String::SubString token;
      while(tokenizer.get_token(token))
      {
        add_discover_channel(token.str(), request);
      }
      request.set_country(*country);
      request.set_language(*language);
      request.set_all(all.enabled());
      const auto response = call<cm::GetDiscoverChannelsResponse>(
        client,
        request,
        &AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient::get_discover_channels);
      std::cout << "channel_id\tweight\tcountry\tlanguage\tname\tquery\n";
      for(const auto& channel : response.channels())
      {
        std::cout << channel.channel_id() << '\t' <<
          channel.weight() << '\t' <<
          channel.country_code() << '\t' <<
          channel.language() << '\t' <<
          channel.name() << '\t' <<
          channel.query() << '\n';
      }
    }
    else if(command == "get_file")
    {
      if(!file.installed())
      {
        throw std::runtime_error("'file' option is required");
      }
      cm::GetFileRequest request;
      request.set_file_name(*file);
      request.set_service_index(*service_index);
      const auto response = call<cm::GetFileResponse>(
        client,
        request,
        &AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient::get_file);
      std::cout.write(response.file().data(), response.file().size());
    }
    else if(command == "get_config")
    {
      cm::GetConfigRequest request;
      request.set_geo_channels(geo_channels.enabled());
      const auto response = call<cm::GetConfigResponse>(
        client,
        request,
        &AdServer::CampaignSvcs::CampaignManagerGrpcAsyncClient::get_config);
      const auto& config = response.config();
      std::cout <<
        "CampaignConfig:"
        "\n  campaigns=" << config.campaigns_size() <<
        "\n  accounts=" << config.accounts_size() <<
        "\n  tags=" << config.tags_size() <<
        "\n  sites=" << config.sites_size() <<
        "\n  creatives_options=" << config.creative_options_size() <<
        "\n  frequency_caps=" << config.frequency_caps_size() <<
        "\n  currencies=" << config.currencies_size() <<
        "\n  colocations=" << config.colocations_size() <<
        "\n  countries=" << config.countries_size() <<
        "\n  creative_templates=" << config.creative_template_files_size() <<
        "\n  creative_categories=" << config.creative_categories_size() <<
        "\n  category_channels=" << config.category_channels_size() <<
        "\n  geo_channels=" << config.geo_channels_size() <<
        "\n  geo_coord_channels=" << config.geo_coord_channels_size() <<
        "\n  web_operations=" << config.web_operations_size() <<
        "\n  contracts=" << config.contracts_size() << '\n';
    }
    else
    {
      throw std::runtime_error("unsupported CampaignAdmin2 command: " + command);
    }

    return 0;
  }
  catch(const std::exception& ex)
  {
    std::cerr << ex.what() << '\n';
    return 1;
  }
}
