#include <map>
#include <set>
#include <list>
#include <string>

#include <eh/Exception.hpp>
#include <Generics/SimpleDecimal.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <Commons/Algs.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#include "CampaignServerImpl_v350.hpp"

namespace
{
  template<class IN, class OUT> 
  void
  copy_deleted_sequence(
    const IN& deleted_id_seq,
    OUT& result_deleted_id_seq,
    const Generics::Time& request_timestamp)
  {
    CORBA::ULong di = result_deleted_id_seq.length();
    result_deleted_id_seq.length(di + deleted_id_seq.length());
    for(CORBA::ULong i = 0; i < deleted_id_seq.length(); ++i)
    {
      if(CorbaAlgs::unpack_time(deleted_id_seq[i].timestamp) > request_timestamp)
      {
        result_deleted_id_seq[di].id = deleted_id_seq[i].id;
        result_deleted_id_seq[di].timestamp = deleted_id_seq[i].timestamp;
        ++di;
      }
    }
    result_deleted_id_seq.length(di);
  }

  void
  copy_option_values(
    AdServer::CampaignSvcs_v350::OptionValueSeq& tgt,
    const AdServer::CampaignSvcs_v360::OptionValueSeq& src)
  {
    tgt.length(src.length());
    for(CORBA::ULong i = 0; i < src.length(); ++i)
    {
      tgt[i].option_id = src[i].option_id;
      tgt[i].value = src[i].value;
    }
  }
  
  void
  copy_creative_size_info(
    AdServer::CampaignSvcs_v350::CreativeSizeSeq& tgt,
    const AdServer::CampaignSvcs_v360::CreativeSizeSeq& src)
  {
    tgt.length(src.length());
    for(CORBA::ULong i = 0; i < src.length(); ++i)
    {
      tgt[i].size_id = src[i].size_id;
      tgt[i].up_expand_space = src[i].up_expand_space;
      tgt[i].right_expand_space = src[i].right_expand_space;
      tgt[i].down_expand_space = src[i].down_expand_space;
      tgt[i].left_expand_space = src[i].left_expand_space;
      copy_option_values(tgt[i].tokens, src[i].tokens);
    }
  }

  void
  copy_tag_size_info(
    AdServer::CampaignSvcs_v350::TagSizeSeq& tgt,
    const AdServer::CampaignSvcs_v360::TagSizeSeq& src)
  {
    tgt.length(src.length());
    for(CORBA::ULong i = 0; i < src.length(); ++i)
    {
      tgt[i].size_id = src[i].size_id;
      tgt[i].max_text_creatives = src[i].max_text_creatives;
      copy_option_values(tgt[i].tokens, src[i].tokens);
    }
  }

  void
  copy_channel_expr(
    AdServer::CampaignSvcs_v350::ExpressionInfo& result_expr,
    const AdServer::CampaignSvcs_v360::ExpressionInfo& expr)
  {
    result_expr.operation = expr.operation;
    result_expr.channel_id = expr.channel_id;
    result_expr.sub_channels.length(expr.sub_channels.length());

    for(CORBA::ULong i = 0; i < expr.sub_channels.length(); ++i)
    {
      copy_channel_expr(
        result_expr.sub_channels[i],
        expr.sub_channels[i]);
    }
  }
  
  void copy_channel(
    AdServer::CampaignSvcs_v350::ExpressionChannelInfo& result_channel_info,
    const AdServer::CampaignSvcs_v360::ExpressionChannelInfo& channel_info)
  {
    result_channel_info.channel_id = channel_info.channel_id;
    result_channel_info.name = channel_info.name;
    result_channel_info.account_id = channel_info.account_id;
    result_channel_info.country_code = channel_info.country_code;
    result_channel_info.flags = channel_info.flags;
    result_channel_info.status = channel_info.status;
    result_channel_info.type = channel_info.type;
    result_channel_info.is_public = channel_info.is_public;
    result_channel_info.language = channel_info.language;
    result_channel_info.freq_cap_id = channel_info.freq_cap_id;
    result_channel_info.parent_channel_id = channel_info.parent_channel_id;
    result_channel_info.timestamp = channel_info.timestamp;
    result_channel_info.discover_query = channel_info.discover_query;
    result_channel_info.discover_annotation = channel_info.discover_annotation;
    result_channel_info.channel_rate_id = channel_info.channel_rate_id;
    result_channel_info.imp_revenue = channel_info.imp_revenue;
    result_channel_info.click_revenue = channel_info.click_revenue;
    copy_channel_expr(result_channel_info.expression, channel_info.expression);
    result_channel_info.threshold = channel_info.threshold;
  }

  void
  copy_simple_channels(
    const AdServer::CampaignSvcs_v360::SimpleChannelKeySeq& simple_channel_seq,
    AdServer::CampaignSvcs_v350::SimpleChannelKeySeq& result_simple_channel_seq,
    const Generics::Time& request_timestamp)
  {
    result_simple_channel_seq.length(simple_channel_seq.length());

    CORBA::ULong res_i = 0;
    for(CORBA::ULong i = 0; i < simple_channel_seq.length(); ++i)
    {
      const AdServer::CampaignSvcs_v360::SimpleChannelKey&
        simple_channel_info = simple_channel_seq[i];
      if(CorbaAlgs::unpack_time(simple_channel_info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::SimpleChannelKey&
          result_simple_channel_info = result_simple_channel_seq[res_i++];

        result_simple_channel_info.channel_id = simple_channel_info.channel_id;
        result_simple_channel_info.country_code = simple_channel_info.country_code;
        result_simple_channel_info.status = simple_channel_info.status;
        result_simple_channel_info.behav_param_list_id = simple_channel_info.behav_param_list_id;
        result_simple_channel_info.str_behav_param_list_id = simple_channel_info.str_behav_param_list_id;
        result_simple_channel_info.threshold = simple_channel_info.threshold;
        result_simple_channel_info.discover = simple_channel_info.discover;
        result_simple_channel_info.timestamp = simple_channel_info.timestamp;

        CorbaAlgs::copy_sequence(
          simple_channel_info.categories,
          result_simple_channel_info.categories);

        CorbaAlgs::copy_sequence(
          simple_channel_info.page_triggers,
          result_simple_channel_info.page_triggers);
        CorbaAlgs::copy_sequence(
          simple_channel_info.search_triggers,
          result_simple_channel_info.search_triggers);
        CorbaAlgs::copy_sequence(
          simple_channel_info.url_triggers,
          result_simple_channel_info.url_triggers);
        CorbaAlgs::copy_sequence(
          simple_channel_info.url_keyword_triggers,
          result_simple_channel_info.url_keyword_triggers);
      }
    }

    result_simple_channel_seq.length(res_i);
  }

  void convert_bp_seq(
    AdServer::CampaignSvcs_v350::BehavParameterSeq& result,
    const AdServer::CampaignSvcs_v360::BehavParameterSeq& src)
  {
    result.length(src.length());
    
    CORBA::ULong res_el_i = 0;
    for(CORBA::ULong bp_el_i = 0; bp_el_i < src.length(); ++bp_el_i)
    {
      const AdServer::CampaignSvcs_v360::BehavParameter& bp_el = src[bp_el_i];
      AdServer::CampaignSvcs_v350::BehavParameter& result_bp_el = result[res_el_i++];
      result_bp_el.min_visits = bp_el.min_visits;
      result_bp_el.time_from = bp_el.time_from;
      result_bp_el.time_to = bp_el.time_to;
      result_bp_el.weight = bp_el.weight;
      result_bp_el.trigger_type = bp_el.trigger_type;
    }
    result.length(res_el_i);
  }
  
  void convert_ext_categories(
    AdServer::CampaignSvcs_v350::ExternalCategorySeq& result,
    const AdServer::CampaignSvcs_v360::ExternalCategorySeq& src)
  {
    result.length(src.length());
    for(CORBA::ULong i = 0; i < src.length(); ++i)
    {
      const AdServer::CampaignSvcs_v360::ExternalCategoryInfo& cat = src[i];
      AdServer::CampaignSvcs_v350::ExternalCategoryInfo& res = result[i];
      res.ad_request_type = cat.ad_request_type;
      res.names = cat.names;
    }
  }

}

namespace AdServer
{
namespace CampaignSvcs
{
  namespace Aspect
  {
    const char CAMPAIGN_SERVER_v350[] = "CampaignServer_v350";
  }

  CampaignServerImpl_v350::CampaignServerImpl_v350(
    Logging::Logger* logger,
    POA_AdServer::CampaignSvcs_v360::CampaignServer* campaign_server)
    /*throw(InvalidArgument, Exception, eh::Exception)*/
    : logger_(ReferenceCounting::add_ref(logger))
  {
    campaign_server_ = campaign_server;
    campaign_server_->_add_ref();
  }

  CampaignServerImpl_v350::~CampaignServerImpl_v350() noexcept
  {}

  void
  CampaignServerImpl_v350::convert_config_(
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const Generics::Time& request_timestamp)
    /*throw(Exception, eh::Exception)*/
  {
    /* fill global params */
    result_update_info.server_id = update_info.server_id;
    result_update_info.master_stamp = update_info.master_stamp;
    result_update_info.first_load_stamp = update_info.first_load_stamp;
    result_update_info.finish_load_stamp = update_info.finish_load_stamp;
    result_update_info.current_time = update_info.current_time;

    result_update_info.currency_exchange_id = update_info.currency_exchange_id;
    result_update_info.max_keyword_ecpm = update_info.max_keyword_ecpm;
    result_update_info.tanx_publisher_account_id = 0;
    result_update_info.google_publisher_account_id = update_info.google_publisher_account_id;
    result_update_info.fraud_user_deactivate_period =
      update_info.fraud_user_deactivate_period;
    result_update_info.global_params_timestamp = update_info.global_params_timestamp;
    result_update_info.cost_limit = update_info.cost_limit;


    convert_sizes_(result_update_info, update_info, request_timestamp);

    convert_currencies_(result_update_info, update_info, request_timestamp);

    convert_accounts_(result_update_info, update_info, request_timestamp);

    convert_creative_options_(result_update_info, update_info, request_timestamp);

    convert_campaigns_(result_update_info, update_info, request_timestamp);

    convert_expression_channels_(result_update_info, update_info, request_timestamp);

    convert_ecpms_(result_update_info, update_info, request_timestamp);

    convert_sites_(result_update_info, update_info, request_timestamp);

    convert_tags_(result_update_info, update_info, request_timestamp);

    convert_frequency_caps_(result_update_info, update_info, request_timestamp);

    convert_simple_channels_(result_update_info, update_info, request_timestamp);

    convert_geo_channels_(result_update_info, update_info, request_timestamp);

    convert_geo_coord_channels_(result_update_info, update_info, request_timestamp);

    convert_block_channels_(result_update_info, update_info, request_timestamp);

    convert_behav_params_(result_update_info, update_info, request_timestamp);

    convert_colocations_(result_update_info, update_info, request_timestamp);

    convert_creative_templates_(result_update_info, update_info, request_timestamp);

    convert_campaign_keywords_(result_update_info, update_info, request_timestamp);

    convert_creative_categories_(result_update_info, update_info, request_timestamp);

    convert_adv_actions_(result_update_info, update_info, request_timestamp);

    convert_category_channels_(result_update_info, update_info, request_timestamp);

    convert_fraud_conditions_(result_update_info, update_info, request_timestamp);

    convert_search_engines_(result_update_info, update_info, request_timestamp);

    convert_web_browsers_(result_update_info, update_info, request_timestamp);

    convert_platforms_(result_update_info, update_info, request_timestamp);

    convert_string_dictionaries_(result_update_info, update_info, request_timestamp);

    convert_app_formats_(result_update_info, update_info, request_timestamp);
  }

  void
  CampaignServerImpl_v350::convert_get_config_settings_(
    const AdServer::CampaignSvcs_v350::CampaignGetConfigSettings& settings,
    AdServer::CampaignSvcs_v360::CampaignGetConfigSettings& result_settings)
    /*throw(Exception, eh::Exception)*/
  {
    result_settings.timestamp = settings.timestamp;
    result_settings.server_id = settings.server_id;
    result_settings.portion = settings.portion;
    result_settings.portions_number = settings.portions_number;

    result_settings.colo_id = settings.colo_id;
    result_settings.version = settings.version;

    result_settings.campaign_statuses = settings.campaign_statuses;
    result_settings.channel_statuses = settings.channel_statuses;
    result_settings.country = settings.country;

    result_settings.no_deleted = settings.no_deleted;
    result_settings.provide_only_tags = settings.provide_only_tags;
    result_settings.provide_channel_triggers = settings.provide_channel_triggers;

    result_settings.geo_channels_timestamp = settings.geo_channels_timestamp;
  }

  AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo*
  CampaignServerImpl_v350::get_config(
    const AdServer::CampaignSvcs_v350::CampaignGetConfigSettings& settings)
    /*throw(AdServer::CampaignSvcs_v350::CampaignServer::ImplementationException,
      AdServer::CampaignSvcs_v350::CampaignServer::NotReady)*/
  {
    static const char* FUN = "CampaignServerImpl_v350::get_config()";

    try
    {
      AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo_var campaign_config;
      AdServer::CampaignSvcs_v360::CampaignGetConfigSettings conv_settings;

      convert_get_config_settings_(settings, conv_settings);

      campaign_config = campaign_server_->get_config(conv_settings);

      AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo_var result_campaign_config =
        new AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo();

      Generics::Time filter_timestamp = CorbaAlgs::unpack_time(settings.timestamp);

      if(filter_timestamp < CorbaAlgs::unpack_time(campaign_config->first_load_stamp) ||
         (settings.server_id != 0 &&
           settings.server_id != campaign_config->server_id))
      {
        filter_timestamp = Generics::Time::ZERO;
      }

      convert_config_(
        campaign_config.in(),
        *result_campaign_config,
        filter_timestamp);

      return result_campaign_config._retn();
    }
    catch(const AdServer::CampaignSvcs_v360::CampaignServer::ImplementationException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": AdServer::CampaignSvcs_v360::CampaignServer::"
        "ImplementationException caught: " <<
        ex.description;

      CORBACommons::throw_desc<
        CampaignSvcs_v350::CampaignServer::ImplementationException>(
          ostr.str());
    }
    catch(const AdServer::CampaignSvcs_v360::CampaignServer::NotReady& ex)
    {
      throw AdServer::CampaignSvcs_v350::CampaignServer::NotReady(ex.description);
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": CORBA::SystemException caught: " << ex;

      logger_->log(
        ostr.str(),
        Logging::Logger::ERROR,
        Aspect::CAMPAIGN_SERVER_v350);

      CORBACommons::throw_desc<
        CampaignSvcs_v350::CampaignServer::ImplementationException>(
          ostr.str());
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << e.what();

      logger_->log(
        ostr.str(),
        Logging::Logger::ERROR,
        Aspect::CAMPAIGN_SERVER_v350);

      CORBACommons::throw_desc<
        CampaignSvcs_v350::CampaignServer::ImplementationException>(
          ostr.str());
    }
    return 0; // never reach
  }

  AdServer::CampaignSvcs_v350::ColocationFlagsSeq*
  CampaignServerImpl_v350::get_colocation_flags()
  {
    try
    {
      AdServer::CampaignSvcs_v360::ColocationFlagsSeq_var flags_src =
        campaign_server_->get_colocation_flags();

      AdServer::CampaignSvcs_v350::ColocationFlagsSeq_var flags_dest =
        new AdServer::CampaignSvcs_v350::ColocationFlagsSeq;

      flags_dest->length(flags_src->length());
      for(CORBA::ULong i = 0; i < flags_dest->length(); ++i)
      {
        const AdServer::CampaignSvcs_v360::ColocationFlags& flag_src =
          flags_src[i];
        AdServer::CampaignSvcs_v350::ColocationFlags& flag_dst = 
          flags_dest[i];
        flag_dst.colo_id = flag_src.colo_id;
        flag_dst.flags = flag_src.flags;
      }
      return flags_dest._retn();
    }
    catch(const AdServer::CampaignSvcs_v360::CampaignServer::ImplementationException& ex)
    {
      Stream::Error ostr;
      ostr << __func__ << ": AdServer::CampaignSvcs_v360::CampaignServer::"
        "ImplementationException caught: " <<
        ex.description;

      CORBACommons::throw_desc<
        CampaignSvcs_v350::CampaignServer::ImplementationException>(
          ostr.str());
    }
    catch(const AdServer::CampaignSvcs_v360::CampaignServer::NotReady& ex)
    {
      throw AdServer::CampaignSvcs_v350::CampaignServer::NotReady(ex.description);
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << __func__ << ": CORBA::SystemException caught: " << ex;

      logger_->log(
        ostr.str(),
        Logging::Logger::ERROR,
        Aspect::CAMPAIGN_SERVER_v350);

      CORBACommons::throw_desc<
        CampaignSvcs_v350::CampaignServer::ImplementationException>(
          ostr.str());
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exception caught: " << e.what();

      logger_->log(
        ostr.str(),
        Logging::Logger::ERROR,
        Aspect::CAMPAIGN_SERVER_v350);

      CORBACommons::throw_desc<
        CampaignSvcs_v350::CampaignServer::ImplementationException>(
          ostr.str());
    }
    return 0; // never reach
  }


  AdServer::CampaignSvcs_v350::EcpmSeq*
  CampaignServerImpl_v350::get_ecpms(
    const AdServer::CampaignSvcs_v350::TimestampInfo& request_timestamp)
    /*throw(AdServer::CampaignSvcs_v350::CampaignServer::ImplementationException)*/
  {
    static const char* FUN = "CampaignServerImpl_v350::get_ecpms()";
      
    try
    {
      AdServer::CampaignSvcs_v360::EcpmSeq_var ecpms  =
        campaign_server_->get_ecpms(request_timestamp);
      AdServer::CampaignSvcs_v350::EcpmSeq_var result_ecpms =
        new AdServer::CampaignSvcs_v350::EcpmSeq();

      convert_ecpm_seq_(
        result_ecpms.inout(),
        ecpms.in(),
        CorbaAlgs::unpack_time(request_timestamp));

      return result_ecpms._retn();
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << e.what();

      CORBACommons::throw_desc<
        CampaignSvcs_v350::CampaignServer::ImplementationException>(
          ostr.str());
    }
    catch(const AdServer::CampaignSvcs_v360::CampaignServer::ImplementationException& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": CampaignSvcs_v360::CampaignServer::"
        "ImplementationException caught: " << e.description;
      CORBACommons::throw_desc<
        CampaignSvcs_v350::CampaignServer::ImplementationException>(
          ostr.str());
    }
    catch(const AdServer::CampaignSvcs_v360::CampaignServer::NotReady& )
    {
      throw AdServer::CampaignSvcs_v350::CampaignServer::NotReady();
    }
    return 0; // never reach
  }

  AdServer::CampaignSvcs_v350::BillStatInfo*
  CampaignServerImpl_v350::get_bill_stat()
    /*throw(AdServer::CampaignSvcs_v350::CampaignServer::NotSupport,
      AdServer::CampaignSvcs_v350::CampaignServer::ImplementationException)*/
  {
    static const char* FUN = "CampaignServerImpl_v350::get_bill_stat()";
      
    try
    {
      AdServer::CampaignSvcs_v360::BillStatInfo_var bill_stat =
        campaign_server_->get_bill_stat();
      AdServer::CampaignSvcs_v350::BillStatInfo_var result_bill_stat =
        new AdServer::CampaignSvcs_v350::BillStatInfo();

      return result_bill_stat._retn();
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << e.what();

      CORBACommons::throw_desc<
        CampaignSvcs_v350::CampaignServer::ImplementationException>(
          ostr.str());
    }
    catch(const AdServer::CampaignSvcs_v360::CampaignServer::ImplementationException& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": CampaignSvcs_v360::CampaignServer::"
        "ImplementationException caught: " << e.description;
      CORBACommons::throw_desc<
        CampaignSvcs_v350::CampaignServer::ImplementationException>(
          ostr.str());
    }
    catch(const AdServer::CampaignSvcs_v360::CampaignServer::NotReady& )
    {
      throw AdServer::CampaignSvcs_v350::CampaignServer::NotReady();
    }
    
    return 0; // never reach
  }

  AdServer::CampaignSvcs_v350::BriefSimpleChannelAnswer*
  CampaignServerImpl_v350::brief_simple_channels(
    const AdServer::CampaignSvcs_v350::CampaignServer::
    GetSimpleChannelsInfo& /*settings*/)
    /*throw(AdServer::CampaignSvcs_v350::CampaignServer::ImplementationException,
      AdServer::CampaignSvcs_v350::CampaignServer::NotReady)*/
  {
    throw AdServer::CampaignSvcs_v350::CampaignServer::ImplementationException(
      "CampaignServerImpl_v350::brief_simple_channels(): Not supported.");
  }

  AdServer::CampaignSvcs_v350::SimpleChannelAnswer*
  CampaignServerImpl_v350::simple_channels(
    const AdServer::CampaignSvcs_v350::CampaignServer::GetSimpleChannelsInfo& /*settings*/)
    /*throw(AdServer::CampaignSvcs_v350::CampaignServer::ImplementationException,
      AdServer::CampaignSvcs_v350::CampaignServer::NotReady)*/
  {
    throw AdServer::CampaignSvcs_v350::CampaignServer::ImplementationException(
      "CampaignServerImpl_v350::simple_channels(): Not supported.");
  }

  AdServer::CampaignSvcs_v350::ChannelServerChannelAnswer*
  CampaignServerImpl_v350::chsv_simple_channels(
    const AdServer::CampaignSvcs_v350::CampaignServer::GetSimpleChannelsInfo& /*settings*/)
    /*throw(AdServer::CampaignSvcs_v350::CampaignServer::ImplementationException,
      AdServer::CampaignSvcs_v350::CampaignServer::NotReady)*/
  {
    throw AdServer::CampaignSvcs_v350::CampaignServer::ImplementationException(
      "CampaignServerImpl_v350::chsv_simple_channels(): Not supported.");
  }

  AdServer::CampaignSvcs_v350::ExpressionChannelsInfo*
  CampaignServerImpl_v350::get_expression_channels(
    const AdServer::CampaignSvcs_v350::CampaignServer::GetExpressionChannelsInfo&)
    /*throw(AdServer::CampaignSvcs_v350::CampaignServer::ImplementationException,
      AdServer::CampaignSvcs_v350::CampaignServer::NotReady)*/
  {
    throw AdServer::CampaignSvcs_v350::CampaignServer::ImplementationException(
      "CampaignServerImpl_v350::get_expression_channels(): Not supported.");
  }

  AdServer::CampaignSvcs_v350::DetectorsConfig*
  CampaignServerImpl_v350::detectors(
    const AdServer::CampaignSvcs_v350::TimestampInfo& /*request_timestamp*/)
    /*throw(AdServer::CampaignSvcs_v350::CampaignServer::ImplementationException,
      AdServer::CampaignSvcs_v350::CampaignServer::NotReady)*/
  {
    throw AdServer::CampaignSvcs_v350::CampaignServer::ImplementationException(
      "CampaignServerImpl_v350::search_engines(): Not supported.");
  }

  AdServer::CampaignSvcs_v350::FreqCapConfigInfo*
  CampaignServerImpl_v350::freq_caps()
    /*throw(AdServer::CampaignSvcs_v350::CampaignServer::ImplementationException,
      AdServer::CampaignSvcs_v350::CampaignServer::NotReady)*/
  {
    throw AdServer::CampaignSvcs_v350::CampaignServer::ImplementationException(
      "CampaignServerImpl_v350::freq_caps(): Not supported.");
  }

  AdServer::CampaignSvcs_v350::StatInfo*
  CampaignServerImpl_v350::get_stat()
    /*throw(
      AdServer::CampaignSvcs_v350::CampaignServer::NotSupport,
      AdServer::CampaignSvcs_v350::CampaignServer::NotReady)*/
  {
    throw AdServer::CampaignSvcs_v350::CampaignServer::NotSupport(
      "CampaignServerImpl_v350::get_stat(): Not supported.");
  }

  void
  CampaignServerImpl_v350::update_stat()
    /*throw(
      AdServer::CampaignSvcs_v350::CampaignServer::NotSupport,
      AdServer::CampaignSvcs_v350::CampaignServer::ImplementationException)*/
  {
    throw AdServer::CampaignSvcs_v350::CampaignServer::NotSupport(
      "CampaignServerImpl_v350::update_stat(): Not supported.");
  }

  CORBA::Boolean
  CampaignServerImpl_v350::need_config(
    const CORBACommons::TimestampInfo& master_stamp)
    /*throw(AdServer::CampaignSvcs_v350::CampaignServer::ImplementationException)*/
  {
    static const char* FUN = "CampaignServerImpl_v350::need_config()";

    try
    {
      return campaign_server_->need_config(master_stamp);
    }
    catch(const AdServer::CampaignSvcs_v350::
          CampaignServer::ImplementationException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught ImplementationException: " <<
        ex.description;
      CORBACommons::throw_desc<
        CampaignSvcs_v350::CampaignServer::ImplementationException>(
          ostr.str());
    }
    return 0; // never reach
  }

  AdServer::CampaignSvcs_v350::DiscoverSourceInfo*
  CampaignServerImpl_v350::get_discover_channels(
    CORBA::ULong, CORBA::ULong)
  {
    throw AdServer::CampaignSvcs_v350::CampaignServer::ImplementationException(
      "CampaignServerImpl_v350::get_discover_channels(): Not supported.");
  }

  AdServer::CampaignSvcs_v350::CampaignServer::PassbackInfo*
  CampaignServerImpl_v350::get_tag_passback(CORBA::ULong, const char*)
  {
    throw AdServer::CampaignSvcs_v350::CampaignServer::ImplementationException(
      "CampaignServerImpl_v350::get_tag_passback(): Not supported.");
  }

  AdServer::CampaignSvcs_v350::FraudConditionConfig*
  CampaignServerImpl_v350::fraud_conditions()
  {
    throw AdServer::CampaignSvcs_v350::CampaignServer::ImplementationException(
      "CampaignServerImpl_v350::fraud_conditions(): Not supported.");
  }

  void
  CampaignServerImpl_v350::convert_campaigns_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    const
    noexcept
  {
    /* fill campaigns */
    CORBA::ULong camp_len = update_info.campaigns.length();
    result_update_info.campaigns.length(camp_len);

    CORBA::ULong res_i = 0;
    for (CORBA::ULong i = 0; i < camp_len; ++i)
    {
      const AdServer::CampaignSvcs_v360::CampaignInfo& campaign_info =
        update_info.campaigns[i];

      if(CorbaAlgs::unpack_time(campaign_info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::CampaignInfo& result_campaign_info =
          result_update_info.campaigns[res_i++];

        result_campaign_info.start_user_group_id =
          campaign_info.start_user_group_id;
        result_campaign_info.end_user_group_id =
          campaign_info.end_user_group_id;

        result_campaign_info.status = campaign_info.status;
        result_campaign_info.eval_status = campaign_info.eval_status;
        result_campaign_info.timestamp = campaign_info.timestamp;

        result_campaign_info.campaign_id = campaign_info.campaign_id;
        result_campaign_info.campaign_group_id = campaign_info.campaign_group_id;
        result_campaign_info.ccg_rate_id = campaign_info.ccg_rate_id;
        result_campaign_info.ccg_rate_type = campaign_info.ccg_rate_type;
        result_campaign_info.fc_id = campaign_info.fc_id;
        result_campaign_info.group_fc_id = campaign_info.group_fc_id;

        result_campaign_info.priority = 0;
        result_campaign_info.flags = campaign_info.flags;
        result_campaign_info.account_id = campaign_info.account_id;
        result_campaign_info.advertiser_id = campaign_info.advertiser_id;

        copy_channel_expr(result_campaign_info.expression, campaign_info.expression);
        copy_channel_expr(result_campaign_info.stat_expression, campaign_info.stat_expression);

        result_campaign_info.imp_revenue = campaign_info.imp_revenue;
        result_campaign_info.click_revenue = campaign_info.click_revenue;
        result_campaign_info.action_revenue = campaign_info.action_revenue;
        result_campaign_info.commision = campaign_info.commision;
        result_campaign_info.ccg_type = campaign_info.ccg_type;
        result_campaign_info.target_type = campaign_info.target_type;
        result_campaign_info.country = campaign_info.country;
        result_campaign_info.marketplace = campaign_info.marketplace;
        result_campaign_info.ctr_reset_id = campaign_info.ctr_reset_id;
        result_campaign_info.random_imps = campaign_info.random_imps;
        result_campaign_info.mode = campaign_info.mode;
        result_campaign_info.seq_set_rotate_imps = campaign_info.seq_set_rotate_imps;
        result_campaign_info.min_uid_age = campaign_info.min_uid_age;
        result_campaign_info.max_pub_share = campaign_info.max_pub_share;
        result_campaign_info.min_ctr_goal = campaign_info.min_ctr_goal;

        result_campaign_info.campaign_delivery_limits.date_start =
          campaign_info.campaign_delivery_limits.date_start;
        result_campaign_info.campaign_delivery_limits.date_end =
          campaign_info.campaign_delivery_limits.date_end;
        result_campaign_info.campaign_delivery_limits.delivery_pacing =
          campaign_info.campaign_delivery_limits.delivery_pacing;
        result_campaign_info.campaign_delivery_limits.budget =
          campaign_info.campaign_delivery_limits.budget;
        result_campaign_info.campaign_delivery_limits.daily_budget =
          campaign_info.campaign_delivery_limits.daily_budget;

        result_campaign_info.ccg_delivery_limits.date_start =
          campaign_info.ccg_delivery_limits.date_start;
        result_campaign_info.ccg_delivery_limits.date_end =
          campaign_info.ccg_delivery_limits.date_end;
        result_campaign_info.ccg_delivery_limits.delivery_pacing =
          campaign_info.ccg_delivery_limits.delivery_pacing;
        result_campaign_info.ccg_delivery_limits.budget =
          campaign_info.ccg_delivery_limits.budget;
        result_campaign_info.ccg_delivery_limits.daily_budget =
          campaign_info.ccg_delivery_limits.daily_budget;

        result_campaign_info.weekly_run_intervals.length(
          campaign_info.weekly_run_intervals.length());

        for(CORBA::ULong wr_i = 0;
            wr_i < campaign_info.weekly_run_intervals.length();
            ++wr_i)
        {
          result_campaign_info.weekly_run_intervals[wr_i].min =
            campaign_info.weekly_run_intervals[wr_i].min;
          result_campaign_info.weekly_run_intervals[wr_i].max =
            campaign_info.weekly_run_intervals[wr_i].max;
        }

        CorbaAlgs::copy_sequence(campaign_info.sites, result_campaign_info.sites);

        CORBA::ULong creatives_count = campaign_info.creatives.length();
        result_campaign_info.creatives.length(creatives_count);

        CORBA::ULong res_creative_i = 0;
        for(unsigned int j = 0; j < creatives_count; j++)
        {
          const AdServer::CampaignSvcs_v360::CreativeInfo& creative_info =
            campaign_info.creatives[j];

          AdServer::CampaignSvcs_v350::CreativeInfo& result_creative_info =
            result_campaign_info.creatives[res_creative_i++];

          result_creative_info.ccid = creative_info.ccid;
          result_creative_info.creative_id = creative_info.creative_id;
          result_creative_info.fc_id = creative_info.fc_id;
          result_creative_info.weight = creative_info.weight;
          result_creative_info.creative_format = creative_info.creative_format;
          result_creative_info.click_url.option_id = creative_info.click_url.option_id;
          result_creative_info.click_url.value = creative_info.click_url.value;
          result_creative_info.html_url.option_id = creative_info.html_url.option_id;
          result_creative_info.html_url.value = creative_info.html_url.value;
          result_creative_info.order_set_id = creative_info.order_set_id;
          result_creative_info.status = creative_info.status;
          result_creative_info.version_id = creative_info.version_id;

          CorbaAlgs::copy_sequence(creative_info.categories,
            result_creative_info.categories);

          //copy sizes
          copy_creative_size_info(
            result_creative_info.sizes, creative_info.sizes);
          copy_option_values(
            result_creative_info.tokens, creative_info.tokens);

        }
        //result_campaign_info.creatives.length(res_creative_i);

        CorbaAlgs::copy_sequence(
          campaign_info.colocations,
          result_campaign_info.colocations);

        result_campaign_info.exclude_tags.length(
          campaign_info.exclude_tags.length());
        for(CORBA::ULong i = 0; i < campaign_info.exclude_tags.length(); ++i)
        {
          result_campaign_info.exclude_tags[i].tag_id =
            campaign_info.exclude_tags[i].tag_id;
          result_campaign_info.exclude_tags[i].delivery_value =
            campaign_info.exclude_tags[i].delivery_value;
        }

        CorbaAlgs::copy_sequence(
          campaign_info.exclude_pub_accounts,
          result_campaign_info.exclude_pub_accounts);
      }
    }
    result_update_info.campaigns.length(res_i);

    copy_deleted_sequence(
      update_info.deleted_campaigns,
      result_update_info.deleted_campaigns,
      request_timestamp);
  }

  void
  CampaignServerImpl_v350::convert_accounts_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    /* fill accounts */
    result_update_info.accounts.length(update_info.accounts.length());

    CORBA::ULong res_i = 0;
    for(CORBA::ULong i = 0; i < update_info.accounts.length(); ++i)
    {
      const AdServer::CampaignSvcs_v360::AccountInfo& account_info =
        update_info.accounts[i];

      if(CorbaAlgs::unpack_time(account_info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::AccountInfo& result_account_info =
          result_update_info.accounts[res_i++];

        result_account_info.account_id = account_info.account_id;
        result_account_info.agency_account_id = account_info.agency_account_id;
        result_account_info.internal_account_id = account_info.internal_account_id;
        result_account_info.role_id = account_info.role_id;
        result_account_info.legal_name = account_info.legal_name;
        result_account_info.flags = account_info.flags;
        result_account_info.at_flags = account_info.at_flags;
        result_account_info.text_adserving = account_info.text_adserving;
        result_account_info.country = account_info.country;
        result_account_info.currency_id = account_info.currency_id;
        result_account_info.time_offset = account_info.time_offset;
        result_account_info.commision = account_info.commision;
        result_account_info.budget = account_info.budget;
        result_account_info.paid_amount = account_info.paid_amount;
        result_account_info.auction_rate = account_info.auction_rate;
        result_account_info.pub_pixel_optin = account_info.pub_pixel_optin;
        result_account_info.pub_pixel_optout = account_info.pub_pixel_optout;
        result_account_info.status = account_info.status;
        result_account_info.eval_status = account_info.eval_status;
        CorbaAlgs::copy_sequence(
          account_info.walled_garden_accounts,
          result_account_info.walled_garden_accounts);
        result_account_info.use_pub_pixels = account_info.use_pub_pixels;
        result_account_info.pub_pixel_optin = account_info.pub_pixel_optin;
        result_account_info.pub_pixel_optout = account_info.pub_pixel_optout;
        result_account_info.timestamp = account_info.timestamp;
      }
    }

    result_update_info.accounts.length(res_i);

    copy_deleted_sequence(
      update_info.deleted_accounts,
      result_update_info.deleted_accounts,
      request_timestamp);
  }

  void
  CampaignServerImpl_v350::convert_creative_options_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    result_update_info.activate_creative_options.length(
      update_info.activate_creative_options.length());

    CORBA::ULong res_i = 0;
    for(CORBA::ULong i = 0; i < update_info.activate_creative_options.length(); ++i)
    {
      const AdServer::CampaignSvcs_v360::CreativeOptionInfo& option_info =
        update_info.activate_creative_options[i];

      if(CorbaAlgs::unpack_time(option_info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::CreativeOptionInfo& result_option_info =
          result_update_info.activate_creative_options[res_i++];
        result_option_info.option_id = option_info.option_id;
        result_option_info.token = option_info.token;
        result_option_info.type = option_info.type;
        result_option_info.timestamp = option_info.timestamp;

        CorbaAlgs::copy_sequence(
          option_info.token_relations,
          result_option_info.token_relations);
      }
    }

    result_update_info.activate_creative_options.length(res_i);

    copy_deleted_sequence(
      update_info.delete_creative_options,
      result_update_info.delete_creative_options,
      request_timestamp);
  }

  void
  CampaignServerImpl_v350::convert_expression_channels_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    /* fill channels */
    result_update_info.expression_channels.length(
      update_info.expression_channels.length());
    CORBA::ULong res_i = 0;
    for(CORBA::ULong ch_i = 0;
        ch_i < update_info.expression_channels.length(); ++ch_i)
    {
      if(CorbaAlgs::unpack_time(
           update_info.expression_channels[ch_i].timestamp) > request_timestamp)
      {
        copy_channel(
          result_update_info.expression_channels[res_i++],
          update_info.expression_channels[ch_i]);
      }
    }

    result_update_info.expression_channels.length(res_i);

    copy_deleted_sequence(
      update_info.deleted_expression_channels,
      result_update_info.deleted_expression_channels,
      request_timestamp);
  }

  void
  CampaignServerImpl_v350::convert_geo_coord_channels_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    result_update_info.activate_geo_coord_channels.length(
      update_info.activate_geo_coord_channels.length());
    CORBA::ULong res_i = 0;
    for(CORBA::ULong ch_i = 0;
        ch_i < update_info.activate_geo_coord_channels.length(); ++ch_i)
    {
      const AdServer::CampaignSvcs_v360::GeoCoordChannelInfo& channel_info =
        update_info.activate_geo_coord_channels[ch_i];

      if(CorbaAlgs::unpack_time(channel_info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::GeoCoordChannelInfo& result_channel_info =
          result_update_info.activate_geo_coord_channels[res_i++];
        result_channel_info.channel_id = channel_info.channel_id;
        result_channel_info.longitude = channel_info.longitude;
        result_channel_info.latitude = channel_info.latitude;
        result_channel_info.radius = channel_info.radius;
        result_channel_info.timestamp = channel_info.timestamp;
      }
    }
    result_update_info.activate_geo_coord_channels.length(res_i);

    copy_deleted_sequence(
      update_info.delete_geo_coord_channels,
      result_update_info.delete_geo_coord_channels,
      request_timestamp);
  }

  void
  CampaignServerImpl_v350::convert_block_channels_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    result_update_info.activate_block_channels.length(
      update_info.activate_block_channels.length());
    CORBA::ULong res_i = 0;
    for(CORBA::ULong ch_i = 0;
        ch_i < update_info.activate_block_channels.length(); ++ch_i)
    {
      const AdServer::CampaignSvcs_v360::BlockChannelInfo& channel_info =
        update_info.activate_block_channels[ch_i];

      if(CorbaAlgs::unpack_time(channel_info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::BlockChannelInfo& result_channel_info =
          result_update_info.activate_block_channels[res_i++];
        result_channel_info.channel_id = channel_info.channel_id;
        result_channel_info.size_id = channel_info.size_id;
        result_channel_info.timestamp = channel_info.timestamp;
      }
    }

    result_update_info.activate_block_channels.length(res_i);

    copy_deleted_sequence(
      update_info.delete_block_channels,
      result_update_info.delete_block_channels,
      request_timestamp);
  }

  void
  CampaignServerImpl_v350::convert_geo_channels_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    /* fill channels */
    result_update_info.geo_channels_timestamp = update_info.geo_channels_timestamp;
    result_update_info.activate_geo_channels.length(
      update_info.activate_geo_channels.length());
    CORBA::ULong res_i = 0;
    for(CORBA::ULong ch_i = 0;
        ch_i < update_info.activate_geo_channels.length(); ++ch_i)
    {
      const AdServer::CampaignSvcs_v360::GeoChannelInfo& channel_info =
        update_info.activate_geo_channels[ch_i];

      if(CorbaAlgs::unpack_time(channel_info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::GeoChannelInfo& result_channel_info =
          result_update_info.activate_geo_channels[res_i++];
        result_channel_info.channel_id = channel_info.channel_id;
        result_channel_info.country = channel_info.country;
        result_channel_info.timestamp = channel_info.timestamp;
        result_channel_info.geoip_targets.length(channel_info.geoip_targets.length());
        for(CORBA::ULong t_i = 0; t_i < channel_info.geoip_targets.length(); ++t_i)
        {
          result_channel_info.geoip_targets[t_i].region =
            channel_info.geoip_targets[t_i].region;
          result_channel_info.geoip_targets[t_i].city =
            channel_info.geoip_targets[t_i].city;
        }
      }
    }

    result_update_info.activate_geo_channels.length(res_i);

    copy_deleted_sequence(
      update_info.delete_geo_channels,
      result_update_info.delete_geo_channels,
      request_timestamp);
  }

  void
  CampaignServerImpl_v350::convert_ecpm_seq_(
    AdServer::CampaignSvcs_v350::EcpmSeq& result_ecpms,
    const AdServer::CampaignSvcs_v360::EcpmSeq& ecpms,
    const Generics::Time& request_timestamp)
    noexcept
  {
    /* fill ecpms */
    result_ecpms.length(ecpms.length());

    CORBA::ULong res_i = 0;
    for(CORBA::ULong i = 0; i < ecpms.length(); ++i)
    {
      const AdServer::CampaignSvcs_v360::CampaignEcpmInfo& ecpm_info = ecpms[i];

      if(CorbaAlgs::unpack_time(ecpm_info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::CampaignEcpmInfo& res_ecpm_info = result_ecpms[res_i++];
        res_ecpm_info.ccg_id = ecpm_info.ccg_id;
        res_ecpm_info.timestamp = ecpm_info.timestamp;
        res_ecpm_info.ecpm = ecpm_info.ecpm;
        res_ecpm_info.ctr = ecpm_info.ctr;
      }
    }
    result_ecpms.length(res_i);
  }
  
  void
  CampaignServerImpl_v350::convert_ecpms_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    convert_ecpm_seq_(
      result_update_info.ecpms,
      update_info.ecpms,
      request_timestamp);

    copy_deleted_sequence(
      update_info.deleted_ecpms,
      result_update_info.deleted_ecpms,
      request_timestamp);
  }
  
  void
  CampaignServerImpl_v350::convert_sites_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    result_update_info.sites.length(update_info.sites.length());

    CORBA::ULong res_i = 0;

    for(CORBA::ULong i = 0; i < update_info.sites.length(); ++i)
    {
      const AdServer::CampaignSvcs_v360::SiteInfo& site_info = update_info.sites[i];

      if(CorbaAlgs::unpack_time(site_info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::SiteInfo& result_site_info =
          result_update_info.sites[res_i++];

        result_site_info.site_id = site_info.site_id;
        result_site_info.freq_cap_id = site_info.freq_cap_id;
        result_site_info.noads_timeout = site_info.noads_timeout;
        result_site_info.status = site_info.status;
        result_site_info.flags = site_info.flags;
        result_site_info.account_id = site_info.account_id;
        result_site_info.timestamp = site_info.timestamp;

        CorbaAlgs::copy_sequence(
          site_info.approved_creative_categories,
          result_site_info.approved_creative_categories);

        CorbaAlgs::copy_sequence(
          site_info.rejected_creative_categories,
          result_site_info.rejected_creative_categories);

        CorbaAlgs::copy_sequence(
          site_info.approved_creatives,
          result_site_info.approved_creatives);

        CorbaAlgs::copy_sequence(
          site_info.rejected_creatives,
          result_site_info.rejected_creatives);
      }
    }

    result_update_info.sites.length(res_i);

    copy_deleted_sequence(
      update_info.deleted_sites,
      result_update_info.deleted_sites,
      request_timestamp);
  }

  void
  CampaignServerImpl_v350::convert_sizes_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    result_update_info.sizes.length(update_info.sizes.length());
    CORBA::ULong res_i = 0;
    for(CORBA::ULong i = 0; i < update_info.sizes.length(); ++i)
    {
      const AdServer::CampaignSvcs_v360::SizeInfo& size_info =
        update_info.sizes[i];
      if(CorbaAlgs::unpack_time(size_info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::SizeInfo& result_size_info =
          result_update_info.sizes[res_i++];
        result_size_info.size_id = size_info.size_id;
        result_size_info.protocol_name = size_info.protocol_name;
        result_size_info.size_type_id = size_info.size_type_id;
        result_size_info.width = size_info.width;
        result_size_info.height = size_info.height;
        result_size_info.timestamp = size_info.timestamp;
      }
    }
    result_update_info.sizes.length(res_i);
  }

  void
  CampaignServerImpl_v350::convert_tags_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    result_update_info.tags.length(update_info.tags.length());

    CORBA::ULong res_i = 0;

    for(CORBA::ULong tag_i = 0; tag_i < update_info.tags.length(); ++tag_i)
    {
      const AdServer::CampaignSvcs_v360::TagInfo& tag_info =
        update_info.tags[tag_i];

      if(CorbaAlgs::unpack_time(tag_info.timestamp) > request_timestamp &&
         tag_info.sizes.length() > 0)
      {

        AdServer::CampaignSvcs_v350::TagInfo& result_tag_info =
          result_update_info.tags[res_i++];
        result_tag_info.timestamp = tag_info.timestamp;
        result_tag_info.tag_id = tag_info.tag_id;
        result_tag_info.site_id = tag_info.site_id;
        result_tag_info.status = tag_info.status;
        result_tag_info.marketplace = tag_info.marketplace;
        result_tag_info.passback = tag_info.passback;
        result_tag_info.passback_type = tag_info.passback_type;
        result_tag_info.imp_track_pixel = tag_info.imp_track_pixel;
        result_tag_info.flags = tag_info.flags;
        result_tag_info.adjustment = tag_info.adjustment;
        result_tag_info.auction_max_ecpm_share = tag_info.auction_max_ecpm_share;
        result_tag_info.auction_prop_probability_share =
          tag_info.auction_prop_probability_share;
        result_tag_info.auction_random_share = tag_info.auction_random_share;
        result_tag_info.tag_pricings_timestamp = tag_info.tag_pricings_timestamp;
        result_tag_info.allow_expandable = tag_info.allow_expandable;

        copy_tag_size_info(
          result_tag_info.sizes, tag_info.sizes);

        CorbaAlgs::copy_sequence(
          tag_info.accepted_categories, result_tag_info.accepted_categories);

        CorbaAlgs::copy_sequence(
          tag_info.rejected_categories, result_tag_info.rejected_categories);

        result_tag_info.tokens.length(tag_info.tokens.length() + tag_info.sizes[0].tokens.length());

        CORBA::ULong res_tok_i = 0;

        for(CORBA::ULong tok_i = 0; tok_i < tag_info.tokens.length(); ++tok_i, ++res_tok_i)
        {
          result_tag_info.tokens[res_tok_i].option_id = tag_info.tokens[tok_i].option_id;
          result_tag_info.tokens[res_tok_i].value = tag_info.tokens[tok_i].value;
        }

        for(CORBA::ULong tok_i = 0; tok_i < tag_info.sizes[0].tokens.length(); ++tok_i, ++res_tok_i)
        {
          result_tag_info.tokens[res_tok_i].option_id = tag_info.sizes[0].tokens[tok_i].option_id;
          result_tag_info.tokens[res_tok_i].value = tag_info.sizes[0].tokens[tok_i].value;
        }

        result_tag_info.hidden_tokens.length(tag_info.hidden_tokens.length());
        for(CORBA::ULong tok_i = 0;
            tok_i < tag_info.hidden_tokens.length(); ++tok_i)
        {
          result_tag_info.hidden_tokens[tok_i].option_id =
            tag_info.hidden_tokens[tok_i].option_id;
          result_tag_info.hidden_tokens[tok_i].value =
            tag_info.hidden_tokens[tok_i].value;
        }

        result_tag_info.passback_tokens.length(
          tag_info.passback_tokens.length());
        for(CORBA::ULong tok_i = 0;
            tok_i < tag_info.passback_tokens.length(); ++tok_i)
        {
          result_tag_info.passback_tokens[tok_i].option_id =
            tag_info.passback_tokens[tok_i].option_id;
          result_tag_info.passback_tokens[tok_i].value =
            tag_info.passback_tokens[tok_i].value;
        }

        result_tag_info.template_tokens.length(tag_info.template_tokens.length());
        for(CORBA::ULong templ_i = 0;
            templ_i < tag_info.template_tokens.length(); ++templ_i)
        {
          result_tag_info.template_tokens[templ_i].template_name =
            tag_info.template_tokens[templ_i].template_name;
          result_tag_info.template_tokens[templ_i].tokens.length(
            tag_info.template_tokens[templ_i].tokens.length());
          for(CORBA::ULong tok_i = 0;
              tok_i < tag_info.template_tokens[templ_i].tokens.length(); ++tok_i)
          {
            result_tag_info.template_tokens[templ_i].tokens[tok_i].option_id =
              tag_info.template_tokens[templ_i].tokens[tok_i].option_id;
            result_tag_info.template_tokens[templ_i].tokens[tok_i].value =
              tag_info.template_tokens[templ_i].tokens[tok_i].value;
          }
        }

        /* fill tag pricings */
        CORBA::ULong tag_pricing_count = tag_info.tag_pricings.length();
        result_tag_info.tag_pricings.length(tag_pricing_count);

        CORBA::ULong result_tag_pricing_i = 0;
        for(CORBA::ULong j = 0; j < tag_pricing_count; ++j)
        {
          const AdServer::CampaignSvcs_v360::TagPricingInfo& tag_pricing_info =
            tag_info.tag_pricings[j];
          AdServer::CampaignSvcs_v350::TagPricingInfo& result_tag_pricing_info =
            result_tag_info.tag_pricings[result_tag_pricing_i++];

          result_tag_pricing_info.ccg_type = tag_pricing_info.ccg_type;
          result_tag_pricing_info.ccg_rate_type = tag_pricing_info.ccg_rate_type;
          result_tag_pricing_info.country_code = tag_pricing_info.country_code;
          result_tag_pricing_info.site_rate_id = tag_pricing_info.site_rate_id;
          result_tag_pricing_info.imp_revenue = tag_pricing_info.imp_revenue;
          result_tag_pricing_info.revenue_share =
            tag_pricing_info.revenue_share;
        }

        result_tag_info.tag_pricings.length(result_tag_pricing_i);
      }
    }

    result_update_info.tags.length(res_i);

    copy_deleted_sequence(
      update_info.deleted_tags,
      result_update_info.deleted_tags,
      request_timestamp);
  }
  
  void
  CampaignServerImpl_v350::convert_frequency_caps_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    /* fill freq caps */
    result_update_info.frequency_caps.length(
      update_info.frequency_caps.length());

    CORBA::ULong res_i = 0;

    for(CORBA::ULong i = 0; i < update_info.frequency_caps.length(); ++i)
    {
      const AdServer::CampaignSvcs_v360::FreqCapInfo& freq_cap =
        update_info.frequency_caps[i];

      if(CorbaAlgs::unpack_time(freq_cap.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::FreqCapInfo& result_freq_cap =
          result_update_info.frequency_caps[res_i++];

        result_freq_cap.timestamp = freq_cap.timestamp;
        result_freq_cap.fc_id = freq_cap.fc_id;
        result_freq_cap.lifelimit = freq_cap.lifelimit;
        result_freq_cap.period = freq_cap.period;
        result_freq_cap.window_limit = freq_cap.window_limit;
        result_freq_cap.window_time = freq_cap.window_time;
      }
    }

    result_update_info.frequency_caps.length(res_i);

    copy_deleted_sequence(
      update_info.deleted_freq_caps,
      result_update_info.deleted_freq_caps,
      request_timestamp);
  }

  void
  CampaignServerImpl_v350::convert_simple_channels_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    /* fill simple channels */
    copy_simple_channels(
      update_info.simple_channels,
      result_update_info.simple_channels,
      request_timestamp);

    copy_deleted_sequence(
      update_info.deleted_simple_channels,
      result_update_info.deleted_simple_channels,
      request_timestamp);
  }
  
  void
  CampaignServerImpl_v350::convert_behav_params_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    {
      /* fill behav params */
      result_update_info.behav_params.length(
        update_info.behav_params.length());

      CORBA::ULong res_bp_i = 0;
      for(CORBA::ULong i = 0; i < update_info.behav_params.length(); ++i)
      {
        const AdServer::CampaignSvcs_v360::BehavParamInfo& bp =
          update_info.behav_params[i];

        if(CorbaAlgs::unpack_time(bp.timestamp) > request_timestamp)
        {
          AdServer::CampaignSvcs_v350::BehavParamInfo& result_bp =
            result_update_info.behav_params[res_bp_i];

          convert_bp_seq(result_bp.bp_seq, bp.bp_seq);
          if(result_bp.bp_seq.length())
          {
            result_bp.timestamp = bp.timestamp;
            result_bp.id = bp.id;
            result_bp.threshold = bp.threshold;
            res_bp_i++;
          }
        }
      }

      result_update_info.behav_params.length(res_bp_i);

      copy_deleted_sequence(
        update_info.deleted_behav_params,
        result_update_info.deleted_behav_params,
        request_timestamp);
    }
      
    /* fill key behav params */
    result_update_info.key_behav_params.length(
      update_info.key_behav_params.length());

    CORBA::ULong res_bp_i = 0;
    for(CORBA::ULong i = 0; i < update_info.key_behav_params.length(); ++i)
    {
      const AdServer::CampaignSvcs_v360::KeyBehavParamInfo& bp =
        update_info.key_behav_params[i];
      if(CorbaAlgs::unpack_time(bp.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::KeyBehavParamInfo& result_bp =
          result_update_info.key_behav_params[res_bp_i];

        convert_bp_seq(result_bp.bp_seq, bp.bp_seq);
        if(result_bp.bp_seq.length())
        {
          result_bp.timestamp = bp.timestamp;
          result_bp.id = bp.id;
          result_bp.threshold = bp.threshold;
          res_bp_i++;
        }
      }
    }

    result_update_info.key_behav_params.length(res_bp_i);

    result_update_info.deleted_key_behav_params.length(
      update_info.deleted_key_behav_params.length());

    CORBA::ULong dbp_i = 0;
    for(CORBA::ULong i = 0;
        i < update_info.deleted_key_behav_params.length(); ++i)
    {
      result_update_info.deleted_key_behav_params[dbp_i].id =
        update_info.deleted_key_behav_params[i].id;
      result_update_info.deleted_key_behav_params[dbp_i].timestamp =
        update_info.deleted_key_behav_params[i].timestamp;
      ++dbp_i;
    }

    result_update_info.deleted_key_behav_params.length(dbp_i);
  }
  
  void
  CampaignServerImpl_v350::convert_colocations_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    /* fill colocations */
    result_update_info.colocations.length(
      update_info.colocations.length());

    CORBA::ULong res_i = 0;
    for(CORBA::ULong i = 0; i < update_info.colocations.length(); ++i)
    {
      const AdServer::CampaignSvcs_v360::ColocationInfo& colo_info =
        update_info.colocations[i];
      if(CorbaAlgs::unpack_time(colo_info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::ColocationInfo& result_colo_info =
          result_update_info.colocations[res_i++];
        result_colo_info.timestamp = colo_info.timestamp;
        result_colo_info.colo_id = colo_info.colo_id;
        result_colo_info.colo_name = colo_info.colo_name;
        result_colo_info.colo_rate_id = colo_info.colo_rate_id;
        result_colo_info.at_flags = colo_info.at_flags;
        result_colo_info.account_id = colo_info.account_id;
        result_colo_info.revenue_share = colo_info.revenue_share;
        result_colo_info.ad_serving = colo_info.ad_serving;
        result_colo_info.hid_profile = colo_info.hid_profile;
        result_colo_info.tokens.length(colo_info.tokens.length());
        for(CORBA::ULong i = 0; i < colo_info.tokens.length(); ++i)
        {
          result_colo_info.tokens[i].option_id = colo_info.tokens[i].option_id;
          result_colo_info.tokens[i].value = colo_info.tokens[i].value;
        }
      }
    }

    result_update_info.colocations.length(res_i);

    copy_deleted_sequence(
      update_info.deleted_colocations,
      result_update_info.deleted_colocations,
      request_timestamp);
  }
  
  void
  CampaignServerImpl_v350::convert_creative_templates_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    /* fill creative templates */
    CORBA::ULong creative_templates_count = update_info.creative_templates.length();
    result_update_info.creative_templates.length(creative_templates_count);

    CORBA::ULong res_i = 0;
    for(CORBA::ULong i = 0; i < creative_templates_count; ++i)
    {
      const AdServer::CampaignSvcs_v360::CreativeTemplateInfo& cr_templ_info =
        update_info.creative_templates[i];
      if(CorbaAlgs::unpack_time(cr_templ_info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::CreativeTemplateInfo& result_cr_templ_info =
          result_update_info.creative_templates[res_i++];

        result_cr_templ_info.id = cr_templ_info.id;
        result_cr_templ_info.timestamp = cr_templ_info.timestamp;

        CORBA::ULong ctf_count = cr_templ_info.files.length();
        result_cr_templ_info.files.length(ctf_count);

        for(CORBA::ULong ctf_i = 0; ctf_i < ctf_count; ++ctf_i)
        {
          const AdServer::CampaignSvcs_v360::CreativeTemplateFileInfo& ctf_info =
            cr_templ_info.files[ctf_i];
          AdServer::CampaignSvcs_v350::CreativeTemplateFileInfo& result_ctf_info =
            result_cr_templ_info.files[ctf_i];

          result_ctf_info.creative_format = ctf_info.creative_format;
          result_ctf_info.creative_size = ctf_info.creative_size;
          result_ctf_info.app_format = ctf_info.app_format;
          result_ctf_info.mime_format = ctf_info.mime_format;
          result_ctf_info.track_impr = ctf_info.track_impr;
          result_ctf_info.template_file = ctf_info.template_file;
          result_ctf_info.type = static_cast<
            AdServer::CampaignSvcs_v350::CreativeTemplateType>(
              ctf_info.type);
        }

        result_cr_templ_info.tokens.length(
          cr_templ_info.tokens.length());
        for(CORBA::ULong i = 0; i < cr_templ_info.tokens.length(); ++i)
        {
          result_cr_templ_info.tokens[i].option_id =
            cr_templ_info.tokens[i].option_id;
          result_cr_templ_info.tokens[i].value =
            cr_templ_info.tokens[i].value;
        }

        result_cr_templ_info.hidden_tokens.length(
          cr_templ_info.hidden_tokens.length());
        for(CORBA::ULong i = 0; i < cr_templ_info.hidden_tokens.length(); ++i)
        {
          result_cr_templ_info.hidden_tokens[i].option_id =
            cr_templ_info.hidden_tokens[i].option_id;
          result_cr_templ_info.hidden_tokens[i].value =
            cr_templ_info.hidden_tokens[i].value;
        }
      }
    }

    result_update_info.creative_templates.length(res_i);

    copy_deleted_sequence(
      update_info.deleted_templates,
      result_update_info.deleted_templates,
      request_timestamp);
  }

  void
  CampaignServerImpl_v350::convert_currencies_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    /* fill currencies */
    result_update_info.currencies.length(update_info.currencies.length());
    CORBA::ULong res_i = 0;
    for(CORBA::ULong i = 0; i < update_info.currencies.length(); ++i)
    {
      const AdServer::CampaignSvcs_v360::CurrencyInfo& currency_info =
        update_info.currencies[i];

      if(CorbaAlgs::unpack_time(currency_info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::CurrencyInfo& result_currency_info =
          result_update_info.currencies[res_i++];

        result_currency_info.rate = currency_info.rate;
        result_currency_info.currency_id = currency_info.currency_id;
        result_currency_info.currency_exchange_id = currency_info.currency_exchange_id;
        result_currency_info.effective_date = currency_info.effective_date;
        result_currency_info.fraction_digits = currency_info.fraction_digits;
        result_currency_info.currency_code = currency_info.currency_code;
        result_currency_info.timestamp = currency_info.timestamp;
      }
    }

    result_update_info.currencies.length(res_i);
  }
  
  void
  CampaignServerImpl_v350::convert_campaign_keywords_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    /* fill campaign keywords */
    result_update_info.campaign_keywords.length(
      update_info.campaign_keywords.length());
    CORBA::ULong res_i = 0;
    for(CORBA::ULong i = 0; i < update_info.campaign_keywords.length(); ++i)
    {
      const AdServer::CampaignSvcs_v360::CampaignKeywordInfo& keyword_info =
        update_info.campaign_keywords[i];

      if(CorbaAlgs::unpack_time(keyword_info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::CampaignKeywordInfo& res_keyword_info =
          result_update_info.campaign_keywords[res_i++];

        res_keyword_info.ccg_keyword_id = keyword_info.ccg_keyword_id;
        res_keyword_info.original_keyword = keyword_info.original_keyword;
        res_keyword_info.click_url = keyword_info.click_url;
        res_keyword_info.timestamp = keyword_info.timestamp;
      }
    }

    result_update_info.campaign_keywords.length(res_i);

    copy_deleted_sequence(
      update_info.deleted_keywords,
      result_update_info.deleted_keywords,
      request_timestamp);
  }
  
  void
  CampaignServerImpl_v350::convert_creative_categories_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    /* fill creative categories */
    result_update_info.creative_categories.length(
      update_info.creative_categories.length());

    CORBA::ULong res_i = 0;
    for(CORBA::ULong i = 0; i < update_info.creative_categories.length(); ++i)
    {
      const AdServer::CampaignSvcs_v360::CreativeCategoryInfo& category_info =
        update_info.creative_categories[i];
      if(CorbaAlgs::unpack_time(category_info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::CreativeCategoryInfo& res_category_info =
          result_update_info.creative_categories[res_i++];
        res_category_info.creative_category_id = category_info.creative_category_id;
        res_category_info.cct_id = category_info.cct_id;
        res_category_info.name = category_info.name;
        res_category_info.timestamp = category_info.timestamp;
        convert_ext_categories(
          res_category_info.external_categories,
          category_info.external_categories);
      }
    }

    result_update_info.creative_categories.length(res_i);

    copy_deleted_sequence(
      update_info.deleted_creative_categories,
      result_update_info.deleted_creative_categories,
      request_timestamp);
  }

  void
  CampaignServerImpl_v350::convert_adv_actions_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    /* fill adv actions */
    result_update_info.adv_actions.length(update_info.adv_actions.length());

    CORBA::ULong res_i = 0;
    for(CORBA::ULong i = 0; i < update_info.adv_actions.length(); ++i)
    {
      if(CorbaAlgs::unpack_time(update_info.adv_actions[i].timestamp) > request_timestamp)
      {
        result_update_info.adv_actions[res_i].action_id =
          update_info.adv_actions[i].action_id;
        result_update_info.adv_actions[res_i].timestamp =
          update_info.adv_actions[i].timestamp;
        CorbaAlgs::copy_sequence(
          update_info.adv_actions[i].ccg_ids,
          result_update_info.adv_actions[res_i].ccg_ids);
        ++res_i;
      }
    }
    
    result_update_info.adv_actions.length(res_i);

    copy_deleted_sequence(
      update_info.deleted_adv_actions,
      result_update_info.deleted_adv_actions,
      request_timestamp);
  }
  
  void
  CampaignServerImpl_v350::convert_category_channels_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    // fill category channels
    result_update_info.category_channels.length(
      update_info.category_channels.length());

    CORBA::ULong res_i = 0;
      
    for(CORBA::ULong cat_i = 0;
        cat_i < update_info.category_channels.length(); ++cat_i)
    {
      const AdServer::CampaignSvcs_v360::CategoryChannelInfo& channel_category_info =
        update_info.category_channels[cat_i];
      if(CorbaAlgs::unpack_time(channel_category_info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::CategoryChannelInfo& result_channel_category_info =
          result_update_info.category_channels[res_i++];

        result_channel_category_info.channel_id =
          channel_category_info.channel_id;
        result_channel_category_info.name =
          channel_category_info.name;
        result_channel_category_info.newsgate_name =
          channel_category_info.newsgate_name;
        result_channel_category_info.timestamp =
          channel_category_info.timestamp;
        result_channel_category_info.parent_channel_id =
          channel_category_info.parent_channel_id;
        result_channel_category_info.flags =
          channel_category_info.flags;

        result_channel_category_info.localizations.length(
          channel_category_info.localizations.length());
        for(CORBA::ULong li = 0;
            li < channel_category_info.localizations.length(); ++li)
        {
          result_channel_category_info.localizations[li].language =
            channel_category_info.localizations[li].language;
          result_channel_category_info.localizations[li].name =
            channel_category_info.localizations[li].name;
        }
      }
    }

    result_update_info.category_channels.length(res_i);
      
    copy_deleted_sequence(
      update_info.deleted_category_channels,
      result_update_info.deleted_category_channels,
      request_timestamp);
  }

  void
  CampaignServerImpl_v350::convert_search_engines_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    result_update_info.search_engines.length(
      update_info.search_engines.length());
    CORBA::ULong res_i = 0;

    for(CORBA::ULong i = 0;
        i < update_info.search_engines.length(); ++i)
    {
      const AdServer::CampaignSvcs_v360::SearchEngineInfo& s_info =
        update_info.search_engines[i];
      if(CorbaAlgs::unpack_time(s_info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::SearchEngineInfo& result_s_info =
          result_update_info.search_engines[res_i++];
        result_s_info.id = s_info.id;
        result_s_info.name = s_info.name;
        result_s_info.regexps.length(s_info.regexps.length());
        for(CORBA::ULong j = 0; j < s_info.regexps.length(); j++)
        {
          result_s_info.regexps[j].host_postfix =
            s_info.regexps[j].host_postfix;
          result_s_info.regexps[j].regexp =
            s_info.regexps[j].regexp;
          result_s_info.regexps[j].encoding =
            s_info.regexps[j].encoding;
          result_s_info.regexps[j].post_encoding =
            s_info.regexps[j].post_encoding;
          result_s_info.regexps[j].decoding_depth =
            s_info.regexps[j].decoding_depth;
        }
        result_s_info.timestamp = s_info.timestamp;
      }
    }
    result_update_info.search_engines.length(res_i);

    result_update_info.deleted_search_engines.length(
      update_info.deleted_search_engines.length());
    for(CORBA::ULong i = 0;
        i < update_info.deleted_search_engines.length(); ++i)
    {
      result_update_info.deleted_search_engines[i].id =
        update_info.deleted_search_engines[i].id;
      result_update_info.deleted_search_engines[i].timestamp =
        update_info.deleted_search_engines[i].timestamp;
    }
  }

  void
  CampaignServerImpl_v350::convert_web_browsers_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    result_update_info.web_browsers.length(update_info.web_browsers.length());
    CORBA::ULong res_i = 0;

    for(CORBA::ULong i = 0;
        i < update_info.web_browsers.length(); ++i)
    {
      const AdServer::CampaignSvcs_v360::WebBrowserInfo& b_info =
        update_info.web_browsers[i];
      if(CorbaAlgs::unpack_time(b_info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::WebBrowserInfo& result_b_info =
          result_update_info.web_browsers[res_i++];
        result_b_info.name = b_info.name;
        result_b_info.detectors.length(b_info.detectors.length());
        for(CORBA::ULong j = 0; j < b_info.detectors.length(); j++)
        {
          const AdServer::CampaignSvcs_v360::WebBrowserDetectorInfo& d_info =
            b_info.detectors[j];
          AdServer::CampaignSvcs_v350::WebBrowserDetectorInfo& result_d_info =
            result_b_info.detectors[j];
          result_d_info.marker = d_info.marker;
          result_d_info.regexp = d_info.regexp;
          result_d_info.regexp_required = d_info.regexp_required;
          result_d_info.priority = d_info.priority;
        }
        result_b_info.timestamp = b_info.timestamp;
      }
    }
    result_update_info.web_browsers.length(res_i);

    copy_deleted_sequence(
      update_info.deleted_web_browsers,
      result_update_info.deleted_web_browsers,
      request_timestamp);
  }

  void
  CampaignServerImpl_v350::convert_platforms_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    result_update_info.platforms.length(update_info.platforms.length());
    CORBA::ULong res_i = 0;

    for(CORBA::ULong i = 0;
        i < update_info.platforms.length(); ++i)
    {
      const AdServer::CampaignSvcs_v360::PlatformInfo& p_info =
        update_info.platforms[i];
      if(CorbaAlgs::unpack_time(p_info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::PlatformInfo& result_p_info =
          result_update_info.platforms[res_i++];
        result_p_info.platform_id = p_info.platform_id;
        result_p_info.name = p_info.name;
        result_p_info.type = p_info.type;
        result_p_info.detectors.length(p_info.detectors.length());
        for(CORBA::ULong j = 0; j < p_info.detectors.length(); j++)
        {
          const AdServer::CampaignSvcs_v360::PlatformDetectorInfo& pd_info =
            p_info.detectors[j];
          AdServer::CampaignSvcs_v350::PlatformDetectorInfo& result_pd_info =
            result_p_info.detectors[j];
          result_pd_info.use_name = pd_info.use_name;
          result_pd_info.marker = pd_info.marker;
          result_pd_info.output_regexp = pd_info.output_regexp;
          result_pd_info.match_regexp = pd_info.match_regexp;
          result_pd_info.priority = pd_info.priority;
        }
        result_p_info.timestamp = p_info.timestamp;
      }
    }
    result_update_info.platforms.length(res_i);

    copy_deleted_sequence(
      update_info.deleted_platforms,
      result_update_info.deleted_platforms,
      request_timestamp);
  }

  void
  CampaignServerImpl_v350::convert_fraud_conditions_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    result_update_info.fraud_conditions.length(
      update_info.fraud_conditions.length());

    CORBA::ULong res_i = 0;

    for(CORBA::ULong i = 0;
        i < update_info.fraud_conditions.length(); ++i)
    {
      const AdServer::CampaignSvcs_v360::FraudConditionInfo& fraud_cond_info =
        update_info.fraud_conditions[i];
      if(CorbaAlgs::unpack_time(fraud_cond_info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::FraudConditionInfo& result_fraud_cond_info =
          result_update_info.fraud_conditions[res_i++];

        result_fraud_cond_info.id = fraud_cond_info.id;
        result_fraud_cond_info.type = fraud_cond_info.type;
        result_fraud_cond_info.limit = fraud_cond_info.limit;
        result_fraud_cond_info.period = fraud_cond_info.period;
        result_fraud_cond_info.timestamp = fraud_cond_info.timestamp;
      }
    }

    result_update_info.fraud_conditions.length(res_i);

    copy_deleted_sequence(
      update_info.deleted_fraud_conditions,
      result_update_info.deleted_fraud_conditions,
      request_timestamp);
  }

  void
  CampaignServerImpl_v350::convert_string_dictionaries_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    result_update_info.web_operations.length(
      update_info.web_operations.length());

    CORBA::ULong res_i = 0;
    for(CORBA::ULong i = 0;
        i < update_info.web_operations.length(); ++i)
    {
      const AdServer::CampaignSvcs_v360::WebOperationInfo& info =
        update_info.web_operations[i];
      if(CorbaAlgs::unpack_time(info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::WebOperationInfo& result_info =
          result_update_info.web_operations[res_i++];
        result_info.id = info.id;
        result_info.app = info.app;
        result_info.source = info.source;
        result_info.operation = info.operation;
        result_info.timestamp = info.timestamp;
        result_info.flags = info.flags;
      }
    }
    result_update_info.web_operations.length(res_i);

    copy_deleted_sequence(
      update_info.delete_web_operations,
      result_update_info.delete_web_operations,
      request_timestamp);
  }
  
  void
  CampaignServerImpl_v350::convert_app_formats_(
    AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
    const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
    const Generics::Time& request_timestamp)
    noexcept
  {
    result_update_info.app_formats.length(
      update_info.app_formats.length());

    CORBA::ULong res_i = 0;
    for(CORBA::ULong i = 0;
        i < update_info.app_formats.length(); ++i)
    {
      const AdServer::CampaignSvcs_v360::AppFormatInfo& info =
        update_info.app_formats[i];
      if(CorbaAlgs::unpack_time(info.timestamp) > request_timestamp)
      {
        AdServer::CampaignSvcs_v350::AppFormatInfo& result_info =
          result_update_info.app_formats[res_i++];
        result_info.app_format = info.app_format;
        result_info.mime_format = info.mime_format;
        result_info.timestamp = info.timestamp;
      }
    }

    result_update_info.app_formats.length(res_i);

    copy_deleted_sequence(
      update_info.delete_app_formats,
      result_update_info.delete_app_formats,
      request_timestamp);
  }

  void
  CampaignServerImpl_v350::convert_amount_distribution_(
    AdServer::CampaignSvcs_v350::AmountDistributionInfo& res_amount_distribution,
    const AdServer::CampaignSvcs_v360::AmountDistributionInfo& amount_distribution)
  {
    res_amount_distribution.prev_days_amount.day =
      amount_distribution.prev_days_amount.day;
    res_amount_distribution.prev_days_amount.amount =
      amount_distribution.prev_days_amount.amount;
    res_amount_distribution.day_amounts.length(
      amount_distribution.day_amounts.length());
    for(CORBA::ULong day_i = 0; day_i < amount_distribution.day_amounts.length(); ++day_i)
    {
      res_amount_distribution.day_amounts[day_i].day =
        amount_distribution.day_amounts[day_i].day;
      res_amount_distribution.day_amounts[day_i].amount =
        amount_distribution.day_amounts[day_i].amount;
    }
  }
}
}
