
#include <arpa/inet.h>

#include <sstream>
#include <vector>

#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>
#include <Generics/Rand.hpp>
#include <Generics/Time.hpp>
#include <Generics/Uuid.hpp>
#include <String/StringManip.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Commons/Algs.hpp>
#include <Commons/ExternalUserIdUtils.hpp>
#include <LogCommons/AdRequestLogger.hpp>

#include "CampaignManagerImpl.hpp"
#include "CampaignManagerLogger.hpp"
#include "CreativeTextGenerator.hpp"

namespace
{
  const char EQL[] = "*eql*";
  const char AMP[] = "*amp*";
  
  const size_t MAX_UA_TOKEN_SIZE = 500;
  const String::SubString MAIN_IMAGE("ADIMAGE");

  const String::AsciiStringManip::CharCategory SEP_AMP("&");

  using String::StringManip::IntToStr;

  template <typename T>
  std::string
  to_string(T i)
  {
    return IntToStr(i).str().str();
  }

  template<typename Map>
  void
  replace_insert(
    const Map& source,
    Map& target)
    noexcept
  {
    for (auto it = source.begin(); it != source.end(); ++it)
    {
      target[it->first] = it->second;
    }
  }

  void
  encode_parameter(
    std::string& result,
    const String::SubString& value,
    bool data_encoding = false)
  {
    if(!data_encoding)
    {
      String::StringManip::mime_url_encode(value, result);
    }
    else
    {
      String::AsciiStringManip::flatten(
        result, value, String::SubString("&&", 2), SEP_AMP);
    }
  }

  void
  find_token(
    const AdServer::CampaignSvcs::OptionTokenValueMap& tokens,
    const std::string& name,
    unsigned long& value)
    noexcept
  {
    const auto it = tokens.find(name);

    if (it != tokens.end())
    {
      unsigned long parsed_value = 0;

      if (String::StringManip::str_to_int(it->second.value, parsed_value))
      {
        value = parsed_value;
      }
    }
  }

  const char ADRIVER_CLICKPIXEL[] =
    "https://![rhost]/cgi-bin/eclick.cgi?xpid=![xpid]";

  const char ADRIVER_MIME_CLICKPIXEL[] =
    "https%3A%2F%2F![rhost]%2Fcgi-bin%2Feclick.cgi%3Fxpid%3D![xpid]";
}

namespace AdProtocol
{
  const char COLO_ID[] = "colo";
  const char TAG_ID[] = "tid";
  const char TAG_SIZE_ID[] = "tsid";
  const char CC_ID[] = "ccid";
  const char GLOBAL_REQUEST_ID[] = "rid";
  const char REQUEST_ID[] = "requestid";
  const char USER_ID_DISTRIBUTION_HASH[] = "h";
  const char RANDOM[] = "random";
  const char CAMPAIGN_ID[] = "cid";
  const char CCG_KEYWORD_ID[] = "ccgkeyword";
  const char CLICK_RATE[] = "cr";
  const char PRECLICK[] = "preclick";
  const char OPEN_SETTLE_PRICE[] = "p";
  const char OPEN_MIL_SETTLE_PRICE[] = "p2"; // open price multiplied by 1000
  const char OPENX_SETTLE_PRICE[] = "oxp";
  const char LIVERAIL_SETTLE_PRICE[] = "lrp";
  const char GOOGLE_SETTLE_PRICE[] = "gp";
  const char PUBLISHER_ACCOUNT_ID[] = "aid";
  const char PUBLISHER_SITE_ID[] = "sid";
  const char SOURCE_ID[] = "src";
  const char EXTERNAL_USER_ID[] = "id";
  const char USER_ID[] = "u";
  const char VERIFY_TYPE[] = "t";
  const char CAMPAIGN_MANAGER_INDEX[] = "cmi";
  const char CLICK_PREFIX[] = "clickpref";
  const char EXTERNAL_USER_ID2[] = "xid";
  const char ENCRYPTED_USER_IP[] = "euip";
  const char PUB_PIXEL_ACCOUNTS[] = "paid";
  const char BID_TIME[] = "bt";
  const char PUB_POSITION_BOTTOM[] = "hpos";
  const char VIDEO_WIDTH[] = "vw";
  const char VIDEO_HEIGHT[] = "vh";
  const char SET_COOKIE[] = "sc";
}

namespace AdServer
{
  namespace CampaignSvcs
  {
    namespace
    {
      std::string
      append_mime_url_encoded(
        const String::SubString& prefix,
        const String::SubString& to_encode_suffix)
        noexcept
      {
        if(!prefix.empty())
        {
          std::string encoded_suffix;
          String::StringManip::mime_url_encode(
            to_encode_suffix,
            encoded_suffix);
          return prefix.str() + encoded_suffix;
        }

        return to_encode_suffix.str();
      }
    }

    const char* instantiate_user_status(UserStatus user_status) noexcept
    {
      if (user_status == US_OPTIN ||
          user_status == US_TEMPORARY)
      {
        return "1";
      }

      if (user_status == US_OPTOUT ||
          user_status == US_EXTERNALPROBE ||
          user_status == US_NOEXTERNALID)
      {
        return "-1";
      }

      return "0";
    }

    void
    CampaignManagerImpl::instantiate_click_url(
      const CampaignConfig* const campaign_config,
      const OptionValue& click_url,
      std::string& result_click_url,
      const unsigned long* colo_id,
      const Tag* tag,
      const Tag::Size* tag_size,
      const Creative* creative,
      const CampaignKeywordBase* campaign_keyword,
      const TokenValueMap& tokens)
      /*throw(CreativeOptionsProblem, eh::Exception)*/
    {
      const std::string& keyword =
        campaign_keyword ? campaign_keyword->original_keyword : std::string();

      unsigned long random = Generics::safe_rand();

      try
      {
        TemplateParams_var args_ptr = new TemplateParams();
        OptionTokenValueMap args;
        args[CreativeTokens::ADV_CLICK_URL] = click_url;
        args[CreativeTokens::RANDOM] = OptionValue(
          0, IntToStr(random));
        args[CreativeTokens::KEYWORD] = OptionValue(0, keyword);

        for(auto token_it = tokens.begin(); token_it != tokens.end(); ++token_it)
        {
          args[token_it->first] = OptionValue(0, token_it->second);
        }
        
        if(colo_id)
        {
          args[CreativeTokens::COLOCATION] = OptionValue(
            0, IntToStr(*colo_id));
        }

        if (creative)
        {
          args.insert(creative->tokens.begin(), creative->tokens.end());

          args[CreativeTokens::CCID] = OptionValue(
            0, IntToStr(creative->ccid));
          args[CreativeTokens::ADVERTISER_ID] = OptionValue(
            0,
            creative->campaign->advertiser ?
              IntToStr(creative->campaign->advertiser->account_id).str() :
              String::SubString());
          args[CreativeTokens::CGID] = OptionValue(
            0, IntToStr(creative->campaign->campaign_id));
          args[CreativeTokens::CID] = OptionValue(
            0, IntToStr(creative->campaign->campaign_group_id));
        }

        if(tag)
        {
          args[CreativeTokens::TAGID] = OptionValue(
            0, IntToStr(tag->tag_id));
          args[CreativeTokens::SITE_ID] = OptionValue(
            0, IntToStr(tag->site->site_id));
          args[CreativeTokens::PUBLISHER_ID] = OptionValue(
            0, IntToStr(tag->site->account->account_id));
        }

        if(tag_size)
        {
          args[CreativeTokens::CREATIVE_SIZE] = OptionValue(
            0, tag_size->size->protocol_name);
        }
        else if(creative && !creative->sizes.empty())
        {
          args[CreativeTokens::CREATIVE_SIZE] = OptionValue(
            0,
            creative->sizes.begin()->second.size->protocol_name);
        }

        BaseTokenProcessor* token_processor = 0;

        const TokenProcessorMap::const_iterator it =
          campaign_config->token_processors.find(
          click_url.option_id);

        if(it != campaign_config->token_processors.end())
        {
          token_processor = it->second;
        }
        else
        {
          token_processor = campaign_config->default_click_token_processor;
        }

        std::string click_url_str;

        token_processor->instantiate(
          args,
          campaign_config->token_processors,
          CreativeInstantiateRule(),
          CreativeInstantiateArgs(),
          click_url_str);

        HTTP::BrowserAddress click_url_addr(click_url_str);
        click_url_addr.get_view(
          HTTP::HTTPAddress::VW_FULL,
          result_click_url);
      }
      catch(const eh::Exception& ex)
      {
        unsigned long ccg_keyword_id =
          campaign_keyword ? campaign_keyword->ccg_keyword_id : 0;

        Stream::Error ostr;
        ostr << "Can't instantiate creative "
              "(ccid = " << creative->ccid <<
              ", creative_id = " << creative->creative_id <<
              ", ccgkeywordid = " << ccg_keyword_id <<
              ", tag_id = " << (tag ? tag->tag_id : 0) <<
              ") click url: " << ex.what();
        throw CreativeOptionsProblem(ostr);
      }
    }

    bool
    CampaignManagerImpl::instantiate_creative_preview(
      const AdServer::CampaignSvcs::CampaignManager::CreativeParams& params,
      const CampaignConfig* const campaign_config,
      const Campaign* campaign,
      const Creative* creative,
      const Tag* tag,
      const Tag::Size& tag_size,
      CORBA::String_out creative_body)
      /*throw(CORBA::SystemException, eh::Exception)*/
    {
      static const char* FUN = "CampaignManagerImpl::instantiate_creative_preview()";

      unsigned long random = Generics::safe_rand();

      CreativeInstantiateRuleMap::iterator
        instantiate_rule_it = creative_instantiate_.creative_rules.find(
          AdInstantiateRule::UNSECURE.str());

      if(instantiate_rule_it == creative_instantiate_.creative_rules.end())
      {
        Stream::Error ostr;
        ostr << FUN << ": can't find creative instantiate rule: " <<
          AdInstantiateRule::UNSECURE;
        throw Exception(ostr);
      }

      Creative::SizeMap::const_iterator cr_size_it =
        creative->sizes.find(tag_size.size->size_id);

      if(cr_size_it == creative->sizes.end())
      {
        Stream::Error ostr;
        ostr << FUN << ": can't find creative size: " << tag_size.size->size_id;
        throw Exception(ostr);
      }

      const Creative::Size& cr_size = cr_size_it->second;
      const CreativeInstantiateRule&
        creative_instantiate_rule = instantiate_rule_it->second;

      ClickParams click_params;

      // calculating click url
      std::ostringstream ostr_click_url;
      ostr_click_url << creative_instantiate_rule.click_url <<
        "/ccid" << EQL << params.ccid << AMP <<
        "requestid" << EQL;

      if(!creative->click_url.value.empty())
      {
        click_params.click_url = ostr_click_url.str();
      }

      if(!click_params.click_url.empty())
      {
        click_params.click_url_f = click_params.click_url + AMP + "m" + EQL + "f";
      }

      // Always initilize preclick (same in instantiate and preinstantiate)
      click_params.preclick_url = ostr_click_url.str() + AMP + "relocate" + EQL;
      click_params.preclick_url_f = ostr_click_url.str() + AMP + "m" + EQL + "f" + AMP + "relocate" + EQL;

      CreativeInstantiateArgs creative_instantiate_args;
      fill_creative_instantiate_args_(
        creative_instantiate_args,
        creative_instantiate_rule,
        creative,
        click_params,
        random);

      try
      {
        // getting template
        CreativeTemplateKey key(
          creative->creative_format.c_str(),
          tag_size.size->protocol_name.c_str(),
          params.format.in());

        Template* creative_template =
          campaign_config->creative_templates.get(key);

        if(!creative_template)
        {
          Stream::Error ostr;
          ostr << "Can't find creative template for (type='" <<
            creative->creative_format <<
            "', size='" << tag_size.size->protocol_name <<
            "', app_format='" << params.format.in() << "').";

          throw Exception(ostr);
        }

        std::string peer_ip = params.peer_ip.in() && *params.peer_ip ?
          params.peer_ip.in() :
          "127.0.0.1";

        TemplateParams_var request_args_ptr = new TemplateParams();

        {
          TokenValueMap& request_args = *request_args_ptr;

          if(tag)
          {
            request_args[CreativeTokens::TAGWIDTH] = to_string(tag_size.size->width);
            request_args[CreativeTokens::TAGHEIGHT] = to_string(tag_size.size->height);
            request_args[CreativeTokens::TAGSIZE] = tag_size.size->protocol_name;
            request_args[CreativeTokens::TAGID] = to_string(tag->tag_id);
            request_args[CreativeTokens::SITE_ID] = to_string(tag->site->site_id);
            request_args[CreativeTokens::PUBLISHER_ID] = to_string(tag->site->account->account_id);
            for(OptionTokenValueMap::const_iterator token_it = tag->tokens.begin();
                token_it != tag->tokens.end(); ++token_it)
            {
              request_args[token_it->first] = token_it->second.value;
            }

            if(!tag->sizes.empty())
            {
              const Tag::Size* tag_size = tag->sizes.begin()->second;
              for(OptionTokenValueMap::const_iterator token_it = tag_size->tokens.begin();
                  token_it != tag_size->tokens.end(); ++token_it)
              {
                request_args[token_it->first] = token_it->second.value;
              }
            }
          }

          request_args[CreativeTokens::PUB_PIXELS_OPTIN] = "";
          request_args[CreativeTokens::PUB_PIXELS_OPTOUT] = "";
          request_args[CreativeTokens::DNS_ENCODED_UIDS] = "";

          request_args[CreativeTokens::CRVSERVER] =
          request_args[CreativeTokens::ADIMAGE_SERVER] =
            creative_instantiate_rule.ad_image_server;
          request_args[CreativeTokens::AD_SERVER] =
            creative_instantiate_rule.ad_server;
          request_args[CreativeTokens::RANDOM] = to_string(random);

          request_args[CreativeTokens::REFERER] = "";
          request_args[CreativeTokens::REFERER_DOMAIN] = "";
          request_args[CreativeTokens::REFERER_DOMAIN_HASH] = "";
          request_args[CreativeTokens::ETID] = "";
          request_args[CreativeTokens::SOURCE_ID] = "";
          request_args[CreativeTokens::EXTERNAL_USER_ID] = "";

          request_args[CreativeTokens::USER_STATUS] = "0";

          request_args[CreativeTokens::ORIGLINK] = params.original_url.in();
          request_args[CreativeTokens::REQUEST_TOKEN] = "";
          request_args[CreativeTokens::COLOCATION] = "0";

          request_args[CreativeTokens::REFERER_KW_MATCH] = "";
          request_args[CreativeTokens::CONTEXT_KW_MATCH] = "";
          request_args[CreativeTokens::SEARCH_TR_MATCH] = "";

          request_args[CreativeTokens::REFERER_KW] = "";
          request_args[CreativeTokens::CONTEXT_KW] = "";
          request_args[CreativeTokens::SEARCH_TR] = "";
          request_args[CreativeTokens::TRACKPIXEL] = "";
          request_args[CreativeTokens::TRACKHTMLURL] = "";
          request_args[CreativeTokens::ACTIONPIXEL] = "";

          request_args[CreativeTokens::TAG_TRACK_PIXEL] = "";
          request_args[CreativeTokens::COHORT] = "";
          request_args[CreativeTokens::TEST_REQUEST] = "0";

          request_args[CreativeTokens::APP_FORMAT] = params.format.in();
          request_args[CreativeTokens::TEMPLATE_FORMAT] = creative->creative_format;

          request_args[CreativeTokens::PUBPIXELS] = (
            tag->site->account->use_pub_pixels ? "1" : "0");
          request_args[CreativeTokens::PUB_POSITION_BOTTOM] = "";
          request_args[CreativeTokens::TNS_COUNTER_DEVICE_TYPE] = "0";
        }

        OptionTokenValueMap creative_args;

        {
          creative_args = creative->tokens;
          replace_insert(cr_size.tokens, creative_args);

          for(OptionTokenValueMap::const_iterator token_it =
                creative_instantiate_rule.tokens.begin();
              token_it != creative_instantiate_rule.tokens.end(); ++token_it)
          {
            creative_args[token_it->first] = token_it->second;
          }

          creative_args[CreativeTokens::CCID] = OptionValue(
            0, IntToStr(creative->ccid));
          creative_args[CreativeTokens::ADVERTISER_ID] = OptionValue(
            0, IntToStr(creative->campaign->advertiser->account_id));
          creative_args[CreativeTokens::CGID] = OptionValue(
            0, IntToStr(creative->campaign->campaign_id));
          creative_args[CreativeTokens::CID] = OptionValue(
            0, IntToStr(creative->campaign->campaign_group_id));

          creative_args[CreativeTokens::CREATIVE_SIZE] = OptionValue(
            0, tag_size.size->protocol_name);

          // ##PUBPRECLICK## empty so click0 == click, preclick0 == preclick
          if(!click_params.click_url.empty())
          {
            creative_args[CreativeTokens::CLICKURL] = OptionValue(
              0, click_params.click_url);
            creative_args[CreativeTokens::CLICKF] = OptionValue(
              0, click_params.click_url_f);
            creative_args[CreativeTokens::CLICK0] = OptionValue(
              0, click_params.click_url);
            creative_args[CreativeTokens::CLICKF0] = OptionValue(
              0, click_params.click_url_f);
          }

          // preclick_url not empty.
          {
            creative_args[CreativeTokens::PRECLICKURL] = OptionValue(
              0, click_params.preclick_url);
            creative_args[CreativeTokens::PRECLICKF] = OptionValue(
              0, click_params.preclick_url_f);
            creative_args[CreativeTokens::PRECLICK0] = OptionValue(
              0, click_params.preclick_url);
            creative_args[CreativeTokens::PRECLICKF0] = OptionValue(
              0, click_params.preclick_url_f);
          }
        }

        TemplateParams_var result_creative_args = new TemplateParams();

        try
        {
          CreativeTextGenerator::init_creative_tokens(
            creative_instantiate_rule,
            creative_instantiate_args,
            campaign_config->token_processors,
            *request_args_ptr,
            creative_args,
            *result_creative_args);
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << "Can't instantiate creative options. Caught eh::Exception: " <<
            ex.what();

          throw CreativeOptionsProblem(ostr);
        }

        {
          TemplateParamsList creative_args_list;
          creative_args_list.push_back(result_creative_args);

          std::ostringstream creative_body_str;
          creative_template->instantiate(
            request_args_ptr, creative_args_list, creative_body_str);
          creative_body << creative_body_str.str();
        }

        return true;
      }
      catch(const Template::InvalidTemplate& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": eh::Exception while instantiating creative ccid=" <<
          creative->ccid <<
          ", cmpid=" << campaign->campaign_id <<
          ", app_format='" << params.format.in() <<
          "': " << ex.what();

        logger_->log(ostr.str(),
          Logging::Logger::NOTICE,
          Aspect::TRAFFICKING_PROBLEM);

        throw Exception(ostr);
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception "
          "while instantiating creative ccid=" << creative->ccid <<
          ", cmpid=" << campaign->campaign_id << ": " << e.what();

        throw Exception(ostr);
      }
    }

    void
    CampaignManagerImpl::fill_instantiate_request_params_(
      TokenValueMap& request_args,
      AccountIdList* consider_pub_pixel_accounts,
      const CampaignConfig* const campaign_config,
      const Colocation* colocation,
      const Tag* tag,
      const Tag::Size* tag_size,
      const char* app_format,
      const AdServer::CampaignSvcs::CampaignManager::
        CommonAdRequestInfo& request_params,
      const AccountIdList* pubpixel_accounts,
      const AdServer::CampaignSvcs::
        PublisherAccountIdSeq* exclude_pubpixel_accounts,
      const CreativeInstantiateRule& instantiate_info,
      const AdSlotContext& ad_slot_context,
      const String::SubString& ext_tag_id)
      /*throw(eh::Exception)*/
    {
      //static const char* FUN = "CampaignManagerImpl::fill_instantiate_request_params_()";

      for(CORBA::ULong tok_i = 0; tok_i < request_params.tokens.length(); ++tok_i)
      {
        request_args[request_params.tokens[tok_i].name.in()] =
          request_params.tokens[tok_i].value.in();
      }

      request_args[CreativeTokens::PUB_PIXELS_OPTIN] = "";
      request_args[CreativeTokens::PUB_PIXELS_OPTOUT] = "";
      const char* const COUNTRY = request_params.location.length() ?
        request_params.location[0].country.in() : "";

      const AccountIdList* actual_pubpixel_accounts;
      AccountIdList optin_pubpixel_accounts_holder;

      if(pubpixel_accounts)
      {
        actual_pubpixel_accounts = pubpixel_accounts;
      }
      else
      {
        assert(exclude_pubpixel_accounts);

        get_inst_optin_pub_pixel_account_ids_(
          optin_pubpixel_accounts_holder,
          campaign_config,
          tag,
          request_params,
          *exclude_pubpixel_accounts);
        actual_pubpixel_accounts = &optin_pubpixel_accounts_holder;
      }

      // use empty PUB_PIXELS_OPTIN if all publishers mapped or here no pixels
      if(!actual_pubpixel_accounts->empty())
      {
        std::ostringstream pub_pixel_url_str;
        pub_pixel_url_str << instantiate_info.pub_pixels_optin << "&aid=";

        for(AccountIdList::const_iterator acc_it =
              actual_pubpixel_accounts->begin();
            acc_it != actual_pubpixel_accounts->end(); ++acc_it)
        {
          if(acc_it != actual_pubpixel_accounts->begin())
          {
            pub_pixel_url_str << ",";
          }

          pub_pixel_url_str << *acc_it;
        }

        if(consider_pub_pixel_accounts)
        {
          *consider_pub_pixel_accounts = *actual_pubpixel_accounts;
        }

        request_args[CreativeTokens::PUB_PIXELS_OPTIN] = pub_pixel_url_str.str();
      }

      if (find_pub_pixel_accounts_(campaign_config, COUNTRY, US_OPTOUT) !=
            campaign_config->pub_pixel_accounts.end())
      {
        request_args[CreativeTokens::PUB_PIXELS_OPTOUT] =
          instantiate_info.pub_pixels_optout;
      }

      {
        AdServer::Commons::ExternalUserIdArray user_ids;
        Commons::UserId req_user_id = CorbaAlgs::unpack_user_id(request_params.user_id);
        if(request_params.external_user_id[0])
        {
          user_ids.push_back(request_params.external_user_id.in());
        }

        if(!req_user_id.is_null())
        {
          user_ids.push_back(std::string("/") + req_user_id.to_string());
        }

        if(!user_ids.empty())
        {
          std::string dns_enc_str;
          AdServer::Commons::dns_encode_external_user_ids(dns_enc_str, user_ids);
          request_args[CreativeTokens::DNS_ENCODED_UIDS].swap(dns_enc_str);
        }
      }

      request_args[CreativeTokens::CRVSERVER] =
      request_args[CreativeTokens::ADIMAGE_SERVER] =
        instantiate_info.ad_image_server;
      request_args[CreativeTokens::AD_SERVER] = instantiate_info.ad_server;
      request_args[CreativeTokens::RANDOM] = to_string(request_params.random);
      request_args[CreativeTokens::PP] = request_params.pub_param;
      request_args[CreativeTokens::REFERER] = request_params.full_referer[0] == '\0' ?
        request_params.referer : request_params.full_referer;

      {
        const char* referer = request_params.full_referer[0] == '\0' ?
          request_params.referer : request_params.full_referer;

        if(referer[0])
        {
          try
          {
            HTTP::BrowserAddress referer_url = HTTP::BrowserAddress(String::SubString(referer));
            std::string domain_value = referer_url.host().str();
            request_args[CreativeTokens::REFERER_DOMAIN] = domain_value;
            request_args[CreativeTokens::REFERER_DOMAIN_HASH] = std::to_string(Generics::CRC::quick(
              0, domain_value.data(), domain_value.size()));
          }
          catch(const eh::Exception&)
          {}
        }
      }

      AdServer::LogProcessing::undisplayable_mime_encode(
        request_args[CreativeTokens::ETID], ext_tag_id);
      if(request_params.source_id[0])
      {
        request_args[CreativeTokens::SOURCE_ID] = request_params.source_id;
      }

      if(request_params.external_user_id[0])
      {
        request_args[CreativeTokens::EXTERNAL_USER_ID] = request_params.external_user_id;
      }

      request_args[CreativeTokens::UID] = request_params.signed_user_id;
      const Commons::UserId user_id = CorbaAlgs::unpack_user_id(request_params.track_user_id);
      if(!user_id.is_null())
      {
        request_args[CreativeTokens::UNSIGNEDUID] = user_id.to_string();
      }

      request_args[CreativeTokens::USER_STATUS] =
        instantiate_user_status(
          static_cast<UserStatus>(request_params.user_status));

      request_args[CreativeTokens::ORIGLINK] = request_params.original_url.in();
      request_args[CreativeTokens::COLOCATION] = to_string(
        colocation ? colocation->colo_id : 0);
      request_args[CreativeTokens::COHORT] = request_params.cohort.in();
      request_args[CreativeTokens::TEST_REQUEST] =
        to_string(ad_slot_context.test_request ? 1 : 0);
      if(request_params.ext_track_params[0])
      {
        request_args[CreativeTokens::EXT_TRACK_PARAMS] =
          request_params.ext_track_params;
      }
      request_args[CreativeTokens::TNS_COUNTER_DEVICE_TYPE] =
        ad_slot_context.tns_counter_device_type;

      /*
      std::string security_token(request_params.security_token.in());

      if (sec_token_generator_.in() && security_token.empty())
      {
        uint32_t addr;

        if (inet_pton(AF_INET, request_params.peer_ip.in(), &addr))
        {
          security_token = sec_token_generator_->get_text_token(addr);
        }
        else
        {
          Stream::Error ostr;
          ostr << FUN << ": can't parse ip address ='" <<
              request_params.peer_ip.in() << "'";
          logger_->log(ostr.str(),
            Logging::Logger::ERROR,
            Aspect::TRAFFICKING_PROBLEM);
        }
      }

      request_args[CreativeTokens::REQUEST_TOKEN] = security_token;
      */

      request_args[CreativeTokens::REFERER_KW].clear();
      request_args[CreativeTokens::CONTEXT_KW].clear();
      request_args[CreativeTokens::SEARCH_TR].clear();
      request_args[CreativeTokens::GREQUESTID] =
        rid_signer_.sign(
          CorbaAlgs::unpack_request_id(request_params.request_id)).str();

      request_args[CreativeTokens::APP_FORMAT] = app_format;

      /*
      // USERBIND deprecated to remove in 3.5
      if(!instantiate_info.user_bind_url.empty() &&
         request_params.external_user_id[0])
      {
        std::ostringstream ostr_user_bind_url;
        ostr_user_bind_url << instantiate_info.user_bind_url << "?";

        std::string mime_external_user_id;
        String::StringManip::mime_url_encode(
          String::SubString(request_params.external_user_id.in()),
          mime_external_user_id);
        ostr_user_bind_url << AdProtocol::EXTERNAL_USER_ID << "=" <<
          mime_external_user_id;

        if(request_params.source_id[0])
        {
          std::string mime_source_id;
          String::StringManip::mime_url_encode(
            String::SubString(request_params.source_id.in()),
            mime_source_id);
          ostr_user_bind_url << "&" << AdProtocol::SOURCE_ID << "=" << mime_source_id;
        }
        request_args[CreativeTokens::USER_BIND] = ostr_user_bind_url.str();
      }
      */

      if(tag)
      {
        request_args[CreativeTokens::TAGID] = to_string(tag->tag_id);
        request_args[CreativeTokens::SITE_ID] = to_string(tag->site->site_id);
        request_args[CreativeTokens::PUBLISHER_ID] = to_string(tag->site->account->account_id);
        request_args[CreativeTokens::PUBPIXELS] = (
          tag->site->account->use_pub_pixels ? "1" : "0");

        if(tag_size)
        {
          request_args[CreativeTokens::TAGWIDTH] = to_string(tag_size->size->width);
          request_args[CreativeTokens::TAGHEIGHT] = to_string(tag_size->size->height);
          request_args[CreativeTokens::TAGSIZE] = tag_size->size->protocol_name;
        }
      }

      request_args[CreativeTokens::PUBPRECLICK] =
        request_params.request_type == AR_ADRIVER ?
        ADRIVER_CLICKPIXEL :
        request_params.preclick_url.in();
    }

    void
    CampaignManagerImpl::fill_instantiate_passback_params_(
      TokenValueMap& request_args,
      const CampaignConfig* const campaign_config,
      const Tag* tag,
      const InstantiateParams& inst_params,
      const AdServer::CampaignSvcs::CampaignManager::CommonAdRequestInfo& request_params,
      const CreativeInstantiateRule& instantiate_info,
      const AdSlotContext& ad_slot_context,
      const AccountIdList* consider_pub_pixel_accounts)
      /*throw(eh::Exception)*/
    {
      if(ad_slot_context.passback_url[0])
      {
        request_args[CreativeTokens::PASSBACK_URL] = ad_slot_context.passback_url;
      }

      if(request_params.passback_type[0])
      {
        request_args[CreativeTokens::PASSBACK_TYPE] =
          request_params.passback_type.in();
      }
      else if(tag)
      {
        request_args[CreativeTokens::PASSBACK_TYPE] =
          tag->passback_type;
      }

      if(tag)
      {
        {
          // init passback pixel
          std::ostringstream passback_imp_url_ostr;

          passback_imp_url_ostr << instantiate_info.passback_pixel_url <<
            "?requestid=" << CorbaAlgs::unpack_request_id(
              request_params.request_id).to_string() <<
            "&random=" << request_params.random;

          if(ad_slot_context.test_request)
          {
            passback_imp_url_ostr << "&testrequest=1";
          }

          if(inst_params.user_id_hash_mod.present())
          {
            passback_imp_url_ostr << "&" <<
              AdProtocol::USER_ID_DISTRIBUTION_HASH << "=" <<
              *inst_params.user_id_hash_mod;
          }

          if(consider_pub_pixel_accounts && !consider_pub_pixel_accounts->empty())
          {
            passback_imp_url_ostr << '&' << AdProtocol::PUB_PIXEL_ACCOUNTS << '=';
            Algs::print(passback_imp_url_ostr,
              consider_pub_pixel_accounts->begin(),
              consider_pub_pixel_accounts->end(),
              ",");
          }

          request_args[CreativeTokens::PASSBACK_PIXEL] =
            passback_imp_url_ostr.str();
        }

        if(request_params.passback_url[0] == 0)
        {
          CreativeTextGenerator::init_creative_tokens(
            instantiate_info,
            CreativeInstantiateArgs(), // no creative args for passback tokens
            campaign_config->token_processors,
            request_args,
            tag->passback_tokens,
            request_args);
        }
        else
        {
          request_args[CreativeTokens::PASSBACK_CODE] = "";
        }
      }
    }

    void
    CampaignManagerImpl::fill_track_urls_(
      const AdSelectionResult& ad_selection_result,
      RequestResultParams& request_result_params,
      const AdServer::CampaignSvcs::CampaignManager::CommonAdRequestInfo& request_params,
      bool fill_track_url,
      const InstantiateParams& inst_params,
      const CreativeInstantiateRule& instantiate_info,
      const AccountIdList* consider_pub_pixel_accounts)
      noexcept
    {
      std::string request_ids;
      std::ostringstream ad_param_ostr;

      for(CampaignSelectionDataList::const_iterator it =
            ad_selection_result.selected_campaigns.begin();
          it != ad_selection_result.selected_campaigns.end(); ++it)
      {
        /*
        if(it->request_id.is_null())
        {
          // REVIEW: now generated for all branches in SelectCreative.cpp
          it->request_id = Generics::Uuid::create_random_based();
        }
        */

        std::string request_id_str;
        std::string mime_request_id_str;

        it->request_id.to_string().swap(request_id_str);
        String::StringManip::mime_url_encode(request_id_str, mime_request_id_str);

        if(it != ad_selection_result.selected_campaigns.begin())
        {
          request_ids += ",";
          ad_param_ostr << ",";
        }

        request_ids += mime_request_id_str;

        if(fill_track_url)
        {
          ad_param_ostr << it->creative->ccid << ':' << it->ctr;
        }
      }

      if(request_result_params.request_id.is_null())
      {
        request_result_params.request_id =
          Commons::RequestId::create_random_based();
      }

      if(!request_ids.empty())
      {
        std::ostringstream ostr;

        ostr << AdProtocol::REQUEST_ID << "=" << request_ids << "&" <<
          AdProtocol::GLOBAL_REQUEST_ID << "=" <<
          request_result_params.request_id.to_string() << "&" <<
          AdProtocol::BID_TIME << "=" << CorbaAlgs::unpack_time(request_params.time).tv_sec;

        if(inst_params.publisher_site_id)
        {
          ostr << "&" << AdProtocol::PUBLISHER_SITE_ID << "=" <<
            inst_params.publisher_site_id;
        }
        else if(inst_params.publisher_account_id)
        {
          ostr << "&" << AdProtocol::PUBLISHER_ACCOUNT_ID << "=" <<
            inst_params.publisher_account_id;
        }

        if(inst_params.user_id_hash_mod.present())
        {
          ostr << "&" << AdProtocol::USER_ID_DISTRIBUTION_HASH << "=" <<
            *inst_params.user_id_hash_mod;
        }

        AdServer::Commons::UserId track_user_id = CorbaAlgs::unpack_user_id(
          request_params.track_user_id);

        if(!track_user_id.is_null())
        {
          ostr << "&" << AdProtocol::USER_ID << "=" << track_user_id;
        }

        if(!request_params.set_cookie)
        {
          ostr << "&" << AdProtocol::SET_COOKIE << "=0";
        }

        std::ostringstream notice_url_ostr;
        std::ostringstream* url_for_cost_ostr = &ostr;

        if(inst_params.enabled_notice)
        {
          // fill notice url
          notice_url_ostr << ostr.str();
          url_for_cost_ostr = &notice_url_ostr;
        }

        // add cost delivering
        if(!inst_params.google_price.empty())
        {
          std::string mime_google_price;
          String::StringManip::mime_url_encode(
            inst_params.google_price,
            mime_google_price);
          *url_for_cost_ostr << "&" << AdProtocol::GOOGLE_SETTLE_PRICE <<
            "=" << mime_google_price;
        }
        else if(!inst_params.open_price.empty())
        {
          std::string mime_open_price;
          String::StringManip::mime_url_encode(
            inst_params.open_price,
            mime_open_price);
          *url_for_cost_ostr << "&" << AdProtocol::OPEN_SETTLE_PRICE <<
            "=" << mime_open_price;
        }
        else if(!inst_params.openx_price.empty())
        {
          std::string mime_openx_price;
          String::StringManip::mime_url_encode(
            inst_params.openx_price,
            mime_openx_price);
          *url_for_cost_ostr << "&" << AdProtocol::OPENX_SETTLE_PRICE <<
            "=" << mime_openx_price;
        }
        else if(!inst_params.liverail_price.empty())
        {
          std::string mime_liverail_price;
          String::StringManip::mime_url_encode(
            inst_params.liverail_price,
            mime_liverail_price);
          *url_for_cost_ostr << "&" << AdProtocol::LIVERAIL_SETTLE_PRICE <<
            "=" << mime_liverail_price;
        }
        else if(request_params.request_type == AR_GOOGLE)
        {
          *url_for_cost_ostr << "&" << AdProtocol::GOOGLE_SETTLE_PRICE <<
            "=%%WINNING_PRICE%%";
        }
        else if(request_params.request_type == AR_OPENRTB ||
          request_params.request_type == AR_OPENRTB_WITH_CLICKURL ||
          request_params.request_type == AR_ADRIVER)
        {
          *url_for_cost_ostr << "&" << AdProtocol::OPEN_SETTLE_PRICE <<
            "=${AUCTION_PRICE}";
        }
        else if(request_params.request_type == AR_OPENX)
        {
          *url_for_cost_ostr << "&" << AdProtocol::OPENX_SETTLE_PRICE <<
            "={winning_price}";
        }
        else if(request_params.request_type == AR_LIVERAIL)
        {
          *url_for_cost_ostr << "&" << AdProtocol::LIVERAIL_SETTLE_PRICE <<
            "=$WINNING_PRICE";
        }
        else if(request_params.request_type == AR_YANDEX)
        {
          *url_for_cost_ostr << "&" << AdProtocol::OPEN_MIL_SETTLE_PRICE <<
            "=${AUCTION_PRICE}";
        }

        if(inst_params.enabled_notice)
        {
          notice_url_ostr << "&" << AdProtocol::VERIFY_TYPE << "=n";
          request_result_params.notice_url =
            instantiate_info.track_pixel_url + "?" + notice_url_ostr.str();
        }

        if(request_params.source_id[0])
        {
          std::string mime_source_id;
          String::StringManip::mime_url_encode(
            String::SubString(request_params.source_id.in()),
            mime_source_id);
          ostr << "&" << AdProtocol::SOURCE_ID << "=" << mime_source_id;
        }

        if(request_params.external_user_id[0])
        {
          std::string mime_external_user_id;
          String::StringManip::mime_url_encode(
            String::SubString(request_params.external_user_id.in()),
            mime_external_user_id);
          ostr << "&" << AdProtocol::EXTERNAL_USER_ID2 << "=" <<
            mime_external_user_id;
        }

        if(fill_track_url)
        {
          // if impression tracking disabled confirm amount on selection
          ostr << "&ad=" << ad_param_ostr.str();

          if(consider_pub_pixel_accounts && !consider_pub_pixel_accounts->empty())
          {
            ostr << '&' << AdProtocol::PUB_PIXEL_ACCOUNTS << '=';
            Algs::print(ostr,
              consider_pub_pixel_accounts->begin(),
              consider_pub_pixel_accounts->end(),
              ",");
          }

          request_result_params.track_pixel_url =
            instantiate_info.track_pixel_url + "?" + ostr.str();
        } // fill_track_url

        request_result_params.track_html_url =
          instantiate_info.track_pixel_url + "?" + ostr.str() + "&t=b";
      }
    }

    void
    CampaignManagerImpl::fill_iurl_(
      std::string& iurl,
      const CampaignConfig* const campaign_config,
      const CreativeInstantiateRule& instantiate_info,
      const AdServer::CampaignSvcs::CampaignManager::CommonAdRequestInfo& request_params,
      const Creative* creative,
      const Size* size)
      noexcept
    {
      // find size specific
      Creative::SizeMap::const_iterator size_it =
        creative->sizes.find(size->size_id);

      Commons::Optional<OptionValue> adimage_url;

      if(size_it != creative->sizes.end())
      {
        OptionTokenValueMap::const_iterator ad_image_it =
          size_it->second.tokens.find(CreativeTokens::ADIMAGE);
        if(ad_image_it != size_it->second.tokens.end())
        {
          adimage_url = ad_image_it->second;
        }
      }

      if(!adimage_url.present())
      {
        OptionTokenValueMap::const_iterator ad_image_it =
          creative->tokens.find(CreativeTokens::ADIMAGE);
        if(ad_image_it != creative->tokens.end())
        {
          adimage_url = ad_image_it->second;
        }
      }

      if(adimage_url.present())
      {
        OptionTokenValueMap args;
        args[CreativeTokens::RANDOM] = OptionValue(
          0, IntToStr(request_params.random));
        args[CreativeTokens::ADIMAGE] = *adimage_url;

        const TokenProcessorMap::const_iterator it =
          campaign_config->token_processors.find(
            adimage_url->option_id);

        if(it != campaign_config->token_processors.end())
        {
          try
          {
            it->second->instantiate(
              args,
              campaign_config->token_processors,
              instantiate_info,
              CreativeInstantiateArgs(),
              iurl);
          }
          catch(const eh::Exception&)
          {
            // don't use ADIMAGE if it contains some non RANDOM token
          }
        }
      }
    }

    /* CampaignManagerImpl::fill_instantiate_params(..) */
    void
    CampaignManagerImpl::fill_instantiate_params_(
      const AdServer::CampaignSvcs::CampaignManager::CommonAdRequestInfo& request_params,
      const CampaignConfig* const campaign_config,
      const Colocation* const colocation,
      const CreativeTemplate& template_descr,
      const Template* creative_template,
      const InstantiateParams& inst_params,
      const CreativeInstantiateRule& instantiate_info,
      const char* app_format,
      AdSelectionResult& ad_selection_result,
      RequestResultParams& request_result_params,
      CreativeParamsList& creative_params_list,
      TemplateParams_var& request_template_params,
      TemplateParamsList& creative_template_params,
      const AdSlotContext& ad_slot_context,
      const AdServer::CampaignSvcs::
        PublisherAccountIdSeq* exclude_pubpixel_accounts)
      /*throw(eh::Exception)*/
    {
      static const char* FUN = "CampaignManagerImpl::fill_instantiate_params_()";

      const Tag* tag = ad_selection_result.tag;

      TemplateParams_var request_args_ptr = new TemplateParams();
      TokenValueMap& request_args = *request_args_ptr;

      // init request system tokens (no recursive usage)
      {
        AccountIdList consider_pub_pixel_accounts;

        fill_instantiate_request_params_(
          request_args,
          &consider_pub_pixel_accounts,
          campaign_config,
          colocation,
          tag,
          ad_selection_result.tag_size,
          app_format,
          request_params,
          (inst_params.generate_pubpixel_accounts ?
            nullptr : &inst_params.pubpixel_accounts),
          exclude_pubpixel_accounts,
          instantiate_info,
          ad_slot_context,
          inst_params.ext_tag_id);

        for(CampaignSelectionDataList::iterator it =
          ad_selection_result.selected_campaigns.begin();
          it != ad_selection_result.selected_campaigns.end(); ++it)
        {
          it->track_impr = template_descr.track_impressions;
        }

        fill_track_urls_(
          ad_selection_result,
          request_result_params,
          request_params,
          template_descr.track_impressions,
          inst_params,
          instantiate_info,
          &consider_pub_pixel_accounts);
        
        fill_instantiate_passback_params_(
          request_args,
          campaign_config,
          tag,
          inst_params,
          request_params,
          instantiate_info,
          ad_slot_context,
          &consider_pub_pixel_accounts);

        fill_instantiate_url_(
          request_args[CreativeTokens::EXTDATA],
          AIT_DATA_PARAM_VALUE,
          creative_params_list,
          request_result_params,
          inst_params,
          instantiate_info,
          request_params,
          ad_selection_result,
          ad_slot_context,
          app_format,
          consider_pub_pixel_accounts,
          false,
          false);
        
        //filled in fill_track_urls_
        request_args[CreativeTokens::TRACKPIXEL] = request_result_params.track_pixel_url;
        request_args[CreativeTokens::TRACKHTMLURL] = request_result_params.track_html_url;

        if (inst_params.video_width)
        {
          request_args[CreativeTokens::VIDEOW] = to_string(inst_params.video_width);
        }

        if (inst_params.video_height)
        {
          request_args[CreativeTokens::VIDEOH] = to_string(inst_params.video_height);
        }

        request_args[CreativeTokens::IP] = request_params.peer_ip;

        if (request_params.user_agent[0] != 0)
        {
          request_args[CreativeTokens::UA].assign(
            request_params.user_agent.in(), 0, MAX_UA_TOKEN_SIZE);
        }

        // Fill video place traits

        request_template_params = request_args_ptr;
      }

      unsigned long position = 1;

      for(CampaignSelectionDataList::iterator it =
            ad_selection_result.selected_campaigns.begin();
          it != ad_selection_result.selected_campaigns.end();
          ++it, ++position)
      {
        CampaignSelectionData& select_params = *it;

        try
        {
          const Creative* creative = select_params.creative;

          creative_params_list.push_back(CreativeParams());
          CreativeParams& creative_params = creative_params_list.back();
          creative_params.click_url = instantiate_info.click_url;
          creative_params.action_adv_url =
            instantiate_info.action_pixel_url.substr(
              0,
              instantiate_info.action_pixel_url.find_last_of("/"));

          std::string mime_request_id_str;
          std::string base64_request_id =
            select_params.request_id.to_string();
          String::StringManip::mime_url_encode(
            base64_request_id, mime_request_id_str);

          ClickParams click_params;
          std::string* click_url = 0;
          OptionValue adv_click_url;

          if(select_params.campaign_keyword.in() &&
             !select_params.campaign_keyword->click_url.empty())
          {
            adv_click_url = OptionValue(
              creative->click_url.option_id,
              select_params.campaign_keyword->click_url);
          }
          else
          {
            adv_click_url = creative->click_url;
          }

          {
            bool init_creative_click_url = !adv_click_url.value.empty();

            init_click_url_(
              click_params,
              colocation,
              tag,
              ad_selection_result.tag_size,
              inst_params,
              request_params,
              ad_slot_context,
              select_params,
              creative_params.click_url);

            if(init_creative_click_url)
            {
              click_url = &click_params.click_url;
            }

            if(position == 1)
            {
              request_result_params.click_params = click_params.params;
            }
          }

          CreativeInstantiateArgs creative_instantiate_args;
          fill_creative_instantiate_args_(
            creative_instantiate_args,
            instantiate_info,
            creative,
            click_params,
            request_params.random);

          const std::string& keyword =
            select_params.campaign_keyword.in() ?
              select_params.campaign_keyword->original_keyword :
              std::string();

          if(click_url)
          {
            OptionValue click_url_in =
              OptionValue(creative->click_url.option_id, *click_url);

            instantiate_click_url(
              configuration(),
              click_url_in,
              creative_params.click_url, /* out */
              colocation ? &colocation->colo_id : 0,
              tag,
              ad_selection_result.tag_size,
              creative,
              select_params.campaign_keyword.in() ? select_params.campaign_keyword.in() : 0,
              request_result_params.ext_tokens);
          }

          {
            OptionTokenValueMap creative_args(creative->tokens);
            Creative::SizeMap::const_iterator cr_size_it =
              creative->sizes.find(ad_selection_result.tag_size->size->size_id);

            if(cr_size_it == creative->sizes.end())
            {
              Stream::Error ostr;
              ostr << FUN << ": Cannot find creative size: " <<
                ad_selection_result.tag_size->size->size_id;
              throw Exception(ostr);
            }

            replace_insert(cr_size_it->second.tokens, creative_args);

            if (select_params.campaign->keyword_based())
            {
              creative_args[CreativeTokens::KEYWORD] = OptionValue(0, keyword);
            }

            for(OptionTokenValueMap::const_iterator token_it =
                  instantiate_info.tokens.begin();
                token_it != instantiate_info.tokens.end(); ++token_it)
            {
              creative_args[token_it->first] = token_it->second;
            }

            creative_args[CreativeTokens::ADV_CLICK_URL] = adv_click_url;

            if((request_params.request_type == AR_OPENRTB_WITH_CLICKURL ||
              request_params.request_type == AR_OPENX) &&
              position == 1)
            {
              // init click prefix if it defined in source (preclick can be defined too)
              std::string click_url_prefix;

              if(!creative_params.click_url.empty() ||
                 !click_params.preclick_url.empty())
              {
                CreativeInstantiate::SourceRuleMap::const_iterator source_rule_it =
                  creative_instantiate_.source_rules.find(request_params.source_id.in());

                if(source_rule_it != creative_instantiate_.source_rules.end() &&
                   source_rule_it->second.click_prefix.present())
                {
                  click_url_prefix = *source_rule_it->second.click_prefix;
                }
                else
                {
                  if(request_params.request_type == AR_OPENRTB_WITH_CLICKURL)
                  {
                    click_url_prefix = "${CLICK_URL}";
                  }
                  else // AR_OPENX 
                  {
                    click_url_prefix = "{clickurl}";
                  }
                }
              }

              if(click_url && !creative_params.click_url.empty())
              {
                {
                  creative_args[CreativeTokens::CLICKURL] = OptionValue(
                    0,
                    append_mime_url_encoded(click_url_prefix, click_params.click_url));
                }

                if(!click_params.click_url_f.empty())
                {
                  creative_args[CreativeTokens::CLICKF] = OptionValue(
                    0,
                    append_mime_url_encoded(click_url_prefix, click_params.click_url_f));
                }

                if(!click_params.click0_url.empty())
                {
                  creative_args[CreativeTokens::CLICK0] = OptionValue(
                    0,
                    append_mime_url_encoded(click_url_prefix, click_params.click0_url));
                }

                if(!click_params.click0_url_f.empty())
                {
                  creative_args[CreativeTokens::CLICKF0] = OptionValue(
                    0,
                    append_mime_url_encoded(click_url_prefix, click_params.click0_url_f));
                }
              }

              // click_params.preclick_url and click_params.preclick0_url could not be empty here
              creative_args[CreativeTokens::PRECLICKURL] = OptionValue(
                0,
                append_mime_url_encoded(click_url_prefix, click_params.preclick_url));

              creative_args[CreativeTokens::PRECLICK0] = OptionValue(
                0,
                append_mime_url_encoded(click_url_prefix, click_params.preclick0_url));

              creative_args[CreativeTokens::PRECLICKF0] = OptionValue(
                0,
                append_mime_url_encoded(click_url_prefix, click_params.preclick0_url_f));
            }
            else
            {
              // position != 1 || AR_NORMAL, AR_OPENRTB, AR_GOOGLE
              if(click_url)
              {
                creative_args[CreativeTokens::CLICKURL] = OptionValue(
                  0, *click_url);

                if(!click_params.click_url_f.empty())
                {
                  creative_args[CreativeTokens::CLICKF] = OptionValue(
                    0, click_params.click_url_f);
                }

                if(!click_params.click0_url.empty())
                {
                  creative_args[CreativeTokens::CLICK0] = OptionValue(
                    0, click_params.click0_url);
                }

                if(!click_params.click0_url_f.empty())
                {
                  creative_args[CreativeTokens::CLICKF0] = OptionValue(
                    0, click_params.click0_url_f);
                }
              }

              // click_params.preclick_url and click_params.preclick0_url not empty
              {
                creative_args[CreativeTokens::PRECLICKURL] = OptionValue(
                  0, click_params.preclick_url);

                creative_args[CreativeTokens::PRECLICKF] = OptionValue(
                  0, click_params.preclick_url_f);

                creative_args[CreativeTokens::PRECLICK0] = OptionValue(
                  0, click_params.preclick0_url);

                creative_args[CreativeTokens::PRECLICKF0] = OptionValue(
                  0, click_params.preclick0_url_f);
              }
            }

            creative_args[CreativeTokens::REQUEST_ID] = OptionValue(
              0, select_params.request_id.to_string());
            creative_args[CreativeTokens::CCID] = OptionValue(
              0, IntToStr(creative->ccid));
            creative_args[CreativeTokens::ADVERTISER_ID] = OptionValue(
              0, IntToStr(creative->campaign->advertiser->account_id));
            creative_args[CreativeTokens::CGID] = OptionValue(
              0, IntToStr(creative->campaign->campaign_id));
            creative_args[CreativeTokens::CID] = OptionValue(
              0, IntToStr(creative->campaign->campaign_group_id));

            creative_args[CreativeTokens::CREATIVE_SIZE] = OptionValue(
              0, ad_selection_result.tag_size->size->protocol_name);
            creative_args[CreativeTokens::TEMPLATE_FORMAT] = OptionValue(
              0, creative->creative_format);
            creative_args[CreativeTokens::ACTIONPIXEL] = OptionValue(
              0, ""); // stub for old token

            if(request_params.log_as_test)
            {
              OptionTokenValueMap::iterator tok_it =
                creative_args.lower_bound(CreativeTokens::ADV_TRACK_PIXEL);
              while(tok_it != creative_args.end() && tok_it->first.compare(
                      0,
                      CreativeTokens::ADV_TRACK_PIXEL.size(),
                      CreativeTokens::ADV_TRACK_PIXEL) == 0)
              {
                // override advertiser track pixel for log as test requests
                tok_it->second.value = instantiate_info.track_pixel_url;
                ++tok_it;
              }
            }

            TemplateParams_var result_creative_args = new TemplateParams();

            try
            {
              // initialize tokens in priority order (from lower to higher)
              CreativeTextGenerator::init_creative_tokens(
                instantiate_info,
                creative_instantiate_args,
                campaign_config->token_processors,
                *request_template_params,
                creative_args,
                *result_creative_args);

              CreativeTextGenerator::init_creative_tokens(
                instantiate_info,
                creative_instantiate_args,
                campaign_config->token_processors,
                *request_template_params,
                *template_descr.tokens,
                request_args);

              Tag::TemplateOptionTokenValueMap::const_iterator templ_tokens_it =
                tag->template_tokens.find(creative->creative_format);

              if(templ_tokens_it != tag->template_tokens.end())
              {
                CreativeTextGenerator::init_creative_tokens(
                  instantiate_info,
                  CreativeInstantiateArgs(),
                  campaign_config->token_processors,
                  *request_template_params,
                  templ_tokens_it->second,
                  request_args);
              }

              CreativeTextGenerator::init_creative_tokens(
                instantiate_info,
                CreativeInstantiateArgs(),
                campaign_config->token_processors,
                *request_template_params,
                tag->tokens,
                request_args);

              CreativeTextGenerator::init_creative_tokens(
                instantiate_info,
                CreativeInstantiateArgs(),
                campaign_config->token_processors,
                request_args,
                ad_selection_result.tag_size->tokens,
                request_args);

              if (request_params.pub_impr_track_url[0])
              {
                (*request_template_params)[CreativeTokens::TAG_TRACK_PIXEL] =
                  request_params.pub_impr_track_url;
              }
              else if (request_params.log_as_test)
              {
                // override tag track pixel for test requests
                (*request_template_params)[CreativeTokens::TAG_TRACK_PIXEL] =
                  instantiate_info.track_pixel_url;
              }

              if (request_params.hpos != CampaignSvcs::UNDEFINED_PUB_POSITION_BOTTOM)
              {
                (*request_template_params)[CreativeTokens::PUB_POSITION_BOTTOM] =
                  to_string(request_params.hpos);
              }

              CreativeTextGenerator::init_creative_tokens(
                instantiate_info,
                CreativeInstantiateArgs(),
                campaign_config->token_processors,
                *request_template_params,
                *template_descr.hidden_tokens,
                request_args);

              CreativeTextGenerator::init_creative_tokens(
                instantiate_info,
                CreativeInstantiateArgs(),
                campaign_config->token_processors,
                *request_template_params,
                tag->hidden_tokens,
                request_args);

              CreativeTextGenerator::init_creative_tokens(
                instantiate_info,
                CreativeInstantiateArgs(),
                campaign_config->token_processors,
                request_args,
                ad_selection_result.tag_size->hidden_tokens,
                request_args);

              if(colocation)
              {
                // colocation tokens have access (recursive substitution) to tag tokens
                CreativeTextGenerator::init_creative_tokens(
                  instantiate_info,
                  CreativeInstantiateArgs(),
                  campaign_config->token_processors,
                  *request_template_params,
                  colocation->tokens,
                  request_args);
              }

              // ADSC-10601
              if(request_params.location.length())
              {
                // country tokens

                CampaignConfig::CountryMap::const_iterator country_it =
                  campaign_config->countries.find(
                    request_params.location[0].country.in());

                if (country_it != campaign_config->countries.end())
                {
                  CreativeTextGenerator::init_creative_tokens(
                    instantiate_info,
                    CreativeInstantiateArgs(),
                    campaign_config->token_processors,
                    *request_template_params,
                    country_it->second->tokens,
                    request_args);
                }
              }
              
              // fill overlay sizes
              TokenValueMap::const_iterator overlay_width_it =
                request_args.find(CreativeTokens::OVERLAY_WIDTH);

              if(overlay_width_it != request_args.end())
              {
                unsigned long overlay_width;
                if(String::StringManip::str_to_int(
                    overlay_width_it->second,
                    overlay_width))
                {
                  request_result_params.overlay_width = overlay_width;
                }
              }

              TokenValueMap::const_iterator overlay_height_it =
                request_args.find(CreativeTokens::OVERLAY_HEIGHT);

              if(overlay_height_it != request_args.end())
              {
                unsigned long overlay_height;
                if(String::StringManip::str_to_int(
                    overlay_height_it->second,
                    overlay_height))
                {
                  request_result_params.overlay_height = overlay_height;
                }
              }
            }
            catch(const eh::Exception& ex)
            {
              Stream::Error ostr;
              ostr << "Can't instantiate creative options. Caught eh::Exception: " <<
                ex.what();

              throw CreativeOptionsProblem(ostr);
            }

            creative_template_params.push_back(result_creative_args);
          }
        }
        catch(const CreativeOptionsProblem&)
        {
          throw;
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": eh::Exception while instantiating creative "
            "ccid=" << select_params.creative->ccid <<
            ", cmpid=" << select_params.campaign->campaign_id <<
            ": " << e.what();

          throw Exception(ostr);
        }
      }

      // fill CREATIVES_JSON
      if(creative_template->key_used(CreativeTokens::CREATIVES_JSON))
      {
        std::ostringstream creatives_json_ostr;
        creatives_json_ostr << "[";
        for(TemplateParamsList::const_iterator cr_it =
              creative_template_params.begin();
            cr_it != creative_template_params.end(); ++cr_it)
        {
          const TokenValueMap& creative_tokens = **cr_it;
          creatives_json_ostr << (cr_it != creative_template_params.begin() ? ",{" : "{");
          for(TokenValueMap::const_iterator token_it = creative_tokens.begin();
              token_it != creative_tokens.end(); ++token_it)
          {
            std::string js_token_name;
            String::StringManip::js_encode(token_it->first.c_str(), js_token_name);
            std::string js_token_value;
            String::StringManip::js_encode(token_it->second.c_str(), js_token_value);
            creatives_json_ostr <<
              (token_it != creative_tokens.begin() ? ",\"" : "\"") <<
              js_token_name << "\":\"" <<
              js_token_value << "\"";
          }
          creatives_json_ostr << "}";
        }
        creatives_json_ostr << "]";
        request_args[CreativeTokens::CREATIVES_JSON] = creatives_json_ostr.str();
      }
    }

    std::string
    CampaignManagerImpl::init_click_params0_(
      const AdServer::Commons::RequestId& request_id,
      const Colocation* colocation,
      const Creative* creative,
      const Tag* tag,
      const Tag::Size* tag_size,
      const CampaignKeyword* campaign_keyword,
      const RevenueDecimal& ctr,
      const InstantiateParams& inst_params,
      const AdServer::CampaignSvcs::CampaignManager::CommonAdRequestInfo& request_params,
      const AdSlotContext& ad_slot_context)
      noexcept
    {
      const char* LOCAL_EQL = (
        request_params.request_type == AR_YANDEX ? "=" : EQL);
      const char* LOCAL_AMP = (
        request_params.request_type == AR_YANDEX ? "&" : AMP);
      std::string mime_request_id_str;

      String::StringManip::mime_url_encode(
        request_id.to_string(),
        mime_request_id_str);

      std::ostringstream ostr_click_url;
      ostr_click_url <<
        AdProtocol::CC_ID << LOCAL_EQL << creative->ccid << LOCAL_AMP <<
        AdProtocol::REQUEST_ID << LOCAL_EQL << mime_request_id_str << LOCAL_AMP <<
        AdProtocol::CAMPAIGN_MANAGER_INDEX << LOCAL_EQL <<
          campaign_manager_config_.service_index() << LOCAL_AMP <<
        AdProtocol::BID_TIME << LOCAL_EQL << CorbaAlgs::unpack_time(request_params.time).tv_sec;

      if(colocation)
      {
        ostr_click_url << LOCAL_AMP << AdProtocol::COLO_ID << LOCAL_EQL <<
          colocation->colo_id;
      }

      if(tag)
      {
        ostr_click_url << LOCAL_AMP << AdProtocol::TAG_ID << LOCAL_EQL <<
          tag->tag_id;

        if(tag_size)
        {
          ostr_click_url << LOCAL_AMP << AdProtocol::TAG_SIZE_ID << LOCAL_EQL <<
            tag_size->size->size_id;
        }
      }
      if(inst_params.user_id_hash_mod.present())
      {
        ostr_click_url << LOCAL_AMP << AdProtocol::USER_ID_DISTRIBUTION_HASH <<
          LOCAL_EQL << *inst_params.user_id_hash_mod;
      }

      if(campaign_keyword)
      {
        ostr_click_url << LOCAL_AMP << AdProtocol::CCG_KEYWORD_ID << LOCAL_EQL <<
          campaign_keyword->ccg_keyword_id;
      }

      ostr_click_url << LOCAL_AMP << AdProtocol::CLICK_RATE << LOCAL_EQL << ctr;

      const AdServer::Commons::UserId track_user_id(
        CorbaAlgs::unpack_user_id(request_params.track_user_id));
      const AdServer::Commons::UserId user_id(
        CorbaAlgs::unpack_user_id(request_params.user_id));

      if (!track_user_id.is_null())
      {
        ostr_click_url << LOCAL_AMP << AdProtocol::USER_ID << LOCAL_EQL <<
          track_user_id;
      }
      else if (request_params.user_status == US_OPTIN &&
          !user_id.is_null())
      {
        ostr_click_url << LOCAL_AMP << AdProtocol::USER_ID << LOCAL_EQL <<
          user_id;
      }

      for(CORBA::ULong i = 0; i < request_params.tokens.length(); ++i)
      {
        std::string mime_token_name;
        String::StringManip::mime_url_encode(
          String::SubString(request_params.tokens[i].name),
          mime_token_name);

        std::string mime_token_value;
        String::StringManip::mime_url_encode(
          String::SubString(request_params.tokens[i].value),
          mime_token_value);

        ostr_click_url << LOCAL_AMP << "t." << mime_token_name << LOCAL_EQL <<
          mime_token_value;
      }

      if(ad_slot_context.tns_counter_device_type[0])
      {
        std::string mime_token_value;
        String::StringManip::mime_url_encode(
          String::SubString(ad_slot_context.tns_counter_device_type),
          mime_token_value);

        ostr_click_url << LOCAL_AMP << "t." << CreativeTokens::TNS_COUNTER_DEVICE_TYPE << LOCAL_EQL <<
          mime_token_value;
      }

      const char* referer = request_params.full_referer[0] == '\0' ?
        request_params.referer : request_params.full_referer;

      if(referer[0])
      {
        try
        {
          HTTP::BrowserAddress referer_url = HTTP::BrowserAddress(String::SubString(referer));
          ostr_click_url << LOCAL_AMP << "t." << CreativeTokens::REFERER_DOMAIN << LOCAL_EQL <<
            referer_url.host();
        }
        catch(const eh::Exception&)
        {}
      }

      return ostr_click_url.str();
    }

    void
    CampaignManagerImpl::init_click_url_(
      ClickParams& click_params,
      const Colocation* colocation,
      const Tag* tag,
      const Tag::Size* tag_size,
      const InstantiateParams& inst_params,
      const AdServer::CampaignSvcs::CampaignManager::CommonAdRequestInfo& request_params,
      const AdSlotContext& ad_slot_context,
      const CampaignSelectionData& select_params,
      const std::string& base_click_url)
    {
      const char* LOCAL_EQL = EQL;
      const char* LOCAL_AMP = AMP;

      const Creative* creative = select_params.creative;
      std::ostringstream ostr_click_url;

      ostr_click_url << init_click_params0_(
        select_params.request_id,
        colocation,
        creative,
        tag,
        tag_size,
        select_params.campaign_keyword.in(),
        select_params.ctr,
        inst_params,
        request_params,
        ad_slot_context);

      std::string res_click0_url = base_click_url + "/" + ostr_click_url.str();

      std::string mime_preclick_url;

      if(request_params.preclick_url[0])
      {
        String::StringManip::mime_url_encode(
          String::SubString(request_params.preclick_url.in()),
          mime_preclick_url);
      }
      else if(inst_params.init_source_macroses)
      {
        CreativeInstantiate::SourceRuleMap::const_iterator source_rule_it =
          creative_instantiate_.source_rules.find(request_params.source_id.in());

        if(source_rule_it != creative_instantiate_.source_rules.end())
        {
          mime_preclick_url = source_rule_it->second.mime_encoded_preclick;
        }
      }

      if(request_params.request_type == AR_ADRIVER)
      {
        std::string adriver_mime_preclick_url = ADRIVER_MIME_CLICKPIXEL;

        if(!mime_preclick_url.empty())
        {
          std::string adriver_mime2_preclick_url;

          String::StringManip::mime_url_encode(
            adriver_mime_preclick_url,
            adriver_mime2_preclick_url);

          mime_preclick_url += adriver_mime2_preclick_url;
        }
        else
        {
          mime_preclick_url.swap(adriver_mime_preclick_url);
        }
      }

      if(!mime_preclick_url.empty())
      {
        ostr_click_url << LOCAL_AMP <<
          AdProtocol::PRECLICK << LOCAL_EQL << mime_preclick_url;
      }

      if (request_params.request_type == AR_GOOGLE)
      {
        ostr_click_url << LOCAL_AMP <<
          AdProtocol::PRECLICK << LOCAL_EQL << "%%CLICK_URL_ESC%%";
      }

      std::string res_click_url = base_click_url + "/" + ostr_click_url.str();

      if(request_params.click_prefix_url[0])
      {
        std::string mimed_click_url;
        std::string mimed_click0_url;
        String::StringManip::mime_url_encode(
          res_click_url,
          mimed_click_url);
        String::StringManip::mime_url_encode(
          res_click0_url,
          mimed_click0_url);
        res_click_url = request_params.click_prefix_url;
        res_click0_url = res_click_url;
        res_click_url += mimed_click_url;
        res_click0_url += mimed_click0_url;
      }

      const std::string f_marker = std::string(LOCAL_AMP) + "m" + LOCAL_EQL + "f";
      const std::string relocate_suffix = std::string(LOCAL_AMP) + "relocate" + LOCAL_EQL;

      click_params.mime_pub_preclick_url.swap(mime_preclick_url);
      click_params.click_url = res_click_url;
      click_params.click_url_f = res_click_url + f_marker;
      click_params.preclick_url = res_click_url + relocate_suffix;
      click_params.preclick_url_f = res_click_url + f_marker + relocate_suffix;

      click_params.click0_url = res_click0_url;
      click_params.click0_url_f = res_click0_url + f_marker;
      click_params.preclick0_url = res_click0_url + relocate_suffix;
      click_params.preclick0_url_f = res_click0_url + f_marker + relocate_suffix;
    }

    void
    CampaignManagerImpl::fill_instantiate_url_(
      std::string& instantiate_url,
      AdInstantiateType ad_instantiate_type,
      CreativeParamsList& creative_params_list,
      const RequestResultParams& request_result_params,
      const InstantiateParams& inst_params,
      const CreativeInstantiateRule& instantiate_info,
      const AdServer::CampaignSvcs::CampaignManager::CommonAdRequestInfo& request_params,
      const AdSelectionResult& ad_selection_result,
      const AdSlotContext& ad_slot_context,
      const char* app_format,
      const AccountIdList& pub_pixel_accounts,
      bool fill_auction_price,
      bool fill_creative_params)
      /*throw(CreativeTemplateProblem, CreativeOptionsProblem, eh::Exception)*/
    {
      std::ostringstream url_ostr;
      bool data_encoding = false;

      if(ad_instantiate_type == AIT_URL || ad_instantiate_type == AIT_URL_IN_BODY)
      {
        url_ostr << instantiate_info.direct_instantiate_url <<
          "format=" << app_format << "&";
      }
      else if(ad_instantiate_type == AIT_NONSECURE_URL)
      {
        url_ostr << instantiate_info.nonsecure_direct_instantiate_url <<
          "format=" << app_format << "&";
      }
      else if(ad_instantiate_type == AIT_VIDEO_URL ||
        ad_instantiate_type == AIT_VIDEO_URL_IN_BODY)
      {
        url_ostr << instantiate_info.video_instantiate_url <<
          "format=" << app_format << "&";
      }
      else if(ad_instantiate_type == AIT_VIDEO_NONSECURE_URL)
      {
        url_ostr << instantiate_info.nonsecure_video_instantiate_url <<
          "format=" << app_format << "&";
      }
      else if(ad_instantiate_type == AIT_SCRIPT_WITH_URL)
      {
        url_ostr << instantiate_info.script_instantiate_url;
      }
      else if(ad_instantiate_type == AIT_IFRAME_WITH_URL)
      {
        url_ostr << instantiate_info.iframe_instantiate_url;
      }
      else if(ad_instantiate_type == AIT_DATA_URL_PARAM ||
        ad_instantiate_type == AIT_DATA_PARAM_VALUE)
      {
        data_encoding = true;
      }
      // AIT_URL_PARAMS - do nothing
      
      url_ostr <<
        AdProtocol::BID_TIME << "=" << CorbaAlgs::unpack_time(request_params.time).tv_sec <<
        "&rid=" << request_result_params.request_id <<
        "&" << AdProtocol::TAG_ID << "=" <<
        ad_selection_result.tag->tag_id <<
        "&" << AdProtocol::TAG_SIZE_ID << "=" <<
        ad_selection_result.tag_size->size->size_id <<
        "&r=" << request_params.random <<
        "&colo=" << request_params.colo_id;

      if(!request_params.set_cookie)
      {
        url_ostr << "&" << AdProtocol::SET_COOKIE << "=0";
      }

      if (inst_params.video_width)
      {
        url_ostr << "&" << AdProtocol::VIDEO_WIDTH << "=" << inst_params.video_width;
      }

      if (inst_params.video_height)
      {
        url_ostr << "&" << AdProtocol::VIDEO_HEIGHT << "=" << inst_params.video_height;
      }
      
      if(ip_crypter_ && request_params.request_type != AR_NORMAL)
      {
        // delegate euip:
        //  - if ip crypter enabled;
        //  - request is not nslookup (AR_NORMAL) - ADSC-10456
        std::string encrypted_user_ip;
        ip_crypter_->encrypt(encrypted_user_ip, request_params.peer_ip.in());
        url_ostr << "&" << AdProtocol::ENCRYPTED_USER_IP << "=" << encrypted_user_ip;
      }
      
      if(inst_params.user_id_hash_mod.present())
      {
        url_ostr << "&" <<
          AdProtocol::USER_ID_DISTRIBUTION_HASH << "=" <<
          *inst_params.user_id_hash_mod;
      }
      
      AdServer::Commons::UserId user_id = CorbaAlgs::unpack_user_id(
        request_params.user_id);
      
      if(!user_id.is_null())
      {
        url_ostr << "&" << AdProtocol::USER_ID << "=" << user_id;
      }

      if(fill_auction_price)
      {
        if(request_params.request_type == AR_GOOGLE)
        {
          url_ostr << "&" << AdProtocol::GOOGLE_SETTLE_PRICE <<
            "=%%WINNING_PRICE%%";
        }
        else if(request_params.request_type == AR_OPENRTB ||
          request_params.request_type == AR_OPENRTB_WITH_CLICKURL ||
          request_params.request_type == AR_ADRIVER)
        {
          url_ostr << "&" << AdProtocol::OPEN_SETTLE_PRICE <<
            "=${AUCTION_PRICE}";
        }
        else if(request_params.request_type == AR_OPENX)
        {
          url_ostr << "&" << AdProtocol::OPENX_SETTLE_PRICE <<
            "={winning_price}";
        }
        else if(request_params.request_type == AR_LIVERAIL)
        {
          url_ostr << "&" << AdProtocol::LIVERAIL_SETTLE_PRICE <<
            "=$WINNING_PRICE";
        }
        // AR_APPNEXUS uses p=${PRICE_PAID} defined in uploaded link
        // AR_YANDEX can pass price only in notice
      }

      if(request_params.original_url[0])
      {
        std::string mime_original_url;
        encode_parameter(
          mime_original_url,
          String::SubString(request_params.original_url.in()),
          data_encoding);
        url_ostr << "&orig=" << mime_original_url;
      }
      
      if(ad_slot_context.test_request || request_params.log_as_test)
      {
        url_ostr << "&test=" << (ad_slot_context.test_request ? "1" : "2");
      }

      if(request_params.pub_impr_track_url[0])
      {
        std::string mime_pub_impr_track_url;
        encode_parameter(
          mime_pub_impr_track_url,
          String::SubString(request_params.pub_impr_track_url.in()),
          data_encoding);
        url_ostr << "&imptrck=" << mime_pub_impr_track_url;
      }

      if(request_params.ext_track_params[0])
      {
        std::string mime_ext_track_params;
        encode_parameter(
          mime_ext_track_params,
          String::SubString(request_params.ext_track_params.in()),
          data_encoding);
        url_ostr << "&ep=" << mime_ext_track_params;
      }

      if(request_params.source_id[0])
      {
        std::string mime_source_id;
        encode_parameter(
          mime_source_id,
          String::SubString(request_params.source_id.in()),
          data_encoding);
        url_ostr << "&src=" << mime_source_id;
      }
      
      if(request_params.external_user_id[0])
      {
        std::string mime_external_user_id;
        encode_parameter(
          mime_external_user_id,
          String::SubString(request_params.external_user_id.in()),
          data_encoding);
        url_ostr << "&eid=" << mime_external_user_id;
      }

      if(inst_params.publisher_site_id)
      {
        url_ostr << "&sid=" << inst_params.publisher_site_id;
      }
      else if(inst_params.publisher_account_id)
      {
        url_ostr << "&aid=" << inst_params.publisher_account_id;
      }

      if(request_params.referer[0] != '\0')
      {
        std::string mime_request_referer;
        encode_parameter(
          mime_request_referer,
          String::SubString(request_params.referer.in()),
          data_encoding);
        url_ostr << "&referer=" << mime_request_referer;
      }

      if(!inst_params.ext_tag_id.empty())
      {
        std::string mime_ext_tag_id;
        encode_parameter(
          mime_ext_tag_id,
          inst_params.ext_tag_id,
          data_encoding);
        url_ostr << "&etid=" << mime_ext_tag_id;
      }
      
      // pass only passback url defined in parameters
      if(request_params.passback_url[0])
      {
        std::string mime_passback_url;
        encode_parameter(
          mime_passback_url,
          String::SubString(request_params.passback_url.in()),
          data_encoding);
        url_ostr << "&pb=" << mime_passback_url;
      }
      
      if(request_params.passback_type[0])
      {
        std::string mime_passback_type;
        encode_parameter(
          mime_passback_type,
            String::SubString(request_params.passback_type.in()),
          data_encoding);
        url_ostr << "&pt=" << mime_passback_type;
      }
      
      url_ostr << "&irid=";
      for(CampaignSelectionDataList::const_iterator cs_it =
            ad_selection_result.selected_campaigns.begin();
          cs_it != ad_selection_result.selected_campaigns.end();
          ++cs_it)
      {
        if (fill_creative_params)
        {
          CreativeParams creative_params;

          if (request_params.request_type == AR_GOOGLE)
          {
            const Creative* creative = cs_it->creative;
            const CampaignKeyword_var& ckw = cs_it->campaign_keyword;
            const unsigned long colo_id = request_params.colo_id;
            
            OptionValue click_url_in = (
              ckw.in() && !ckw->click_url.empty()) ?
              OptionValue(
                creative->click_url.option_id,
                ckw->click_url) :
              creative->click_url;
            
            instantiate_click_url(
              configuration(),
              click_url_in,
              creative_params.click_url, /* out */
              &colo_id,
              ad_selection_result.tag,
              ad_selection_result.tag_size,
              creative,
              ckw.in() ? ckw.in() : 0,
              request_result_params.ext_tokens);
          }
          
          creative_params_list.push_back(creative_params);
        }

        if(cs_it != ad_selection_result.selected_campaigns.begin())
        {
          url_ostr << ",";
        }
        url_ostr << cs_it->request_id;
      }

      url_ostr << "&ad=";
      for(CampaignSelectionDataList::const_iterator cs_it =
            ad_selection_result.selected_campaigns.begin();
            cs_it != ad_selection_result.selected_campaigns.end();
          ++cs_it)
      {
        if(cs_it != ad_selection_result.selected_campaigns.begin())
        {
          url_ostr << ",";
        }
        url_ostr << cs_it->creative->ccid << ":";
        if(cs_it->campaign_keyword.in())
        {
          url_ostr << cs_it->campaign_keyword->ccg_keyword_id;
        }
        url_ostr << ":" << cs_it->ctr;
      }
      url_ostr << '&' << AdProtocol::CAMPAIGN_MANAGER_INDEX << '=' <<
        campaign_manager_config_.service_index();
      
      {
        // fill pub optin pixel accounts
        if(!pub_pixel_accounts.empty())
        {
          url_ostr << "&" << AdProtocol::PUB_PIXEL_ACCOUNTS << "=";
          Algs::print(url_ostr,
            pub_pixel_accounts.begin(),
            pub_pixel_accounts.end(),
            ",");
        }
      }
      
      if (request_params.hpos != CampaignSvcs::UNDEFINED_PUB_POSITION_BOTTOM)
      {
        url_ostr << '&' << AdProtocol::PUB_POSITION_BOTTOM << '=' <<
          request_params.hpos;
      }

      if(request_params.tokens.length())
      {
        for(CORBA::ULong tok_i = 0; tok_i < request_params.tokens.length(); ++tok_i)
        {
          std::string mime_enc_tok_value;

          encode_parameter(
            mime_enc_tok_value,
            String::SubString(request_params.tokens[tok_i].value.in()),
            data_encoding);

          url_ostr << "&t." << request_params.tokens[tok_i].name.in() << '=' <<
            mime_enc_tok_value;
        }
      }

      {
        std::string mime_enc_tok_value;

        encode_parameter(
          mime_enc_tok_value,
          ad_slot_context.tns_counter_device_type,
          data_encoding);

        url_ostr << "&t." << CreativeTokens::TNS_COUNTER_DEVICE_TYPE << '=' <<
          mime_enc_tok_value;
      }
 
      // preclick MUST be last
      if(request_params.request_type == AR_ADRIVER)
      {
        url_ostr << "&preclick=" << ADRIVER_MIME_CLICKPIXEL;
      }
      else if(request_params.preclick_url[0] ||
        request_params.request_type == AR_OPENRTB_WITH_CLICKURL ||
        request_params.request_type == AR_OPENX ||
        request_params.request_type == AR_GOOGLE)
      {
        std::string mime_preclick_url;
        encode_parameter(
          mime_preclick_url,
          String::SubString(request_params.preclick_url.in()),
          data_encoding);

        CreativeInstantiate::SourceRuleMap::const_iterator source_rule_it =
          creative_instantiate_.source_rules.find(request_params.source_id.in());

        if(source_rule_it != creative_instantiate_.source_rules.end())
        {
          const std::string& mime_encoded_preclick =
            ad_instantiate_type == AIT_VIDEO_URL || ad_instantiate_type == AIT_VIDEO_NONSECURE_URL ||
            ad_instantiate_type == AIT_VIDEO_URL_IN_BODY ?
            source_rule_it->second.mime_encoded_vast_preclick :
            source_rule_it->second.mime_encoded_preclick;

          if(!mime_encoded_preclick.empty())
          {
            url_ostr << "&preclick=" << mime_encoded_preclick;

            if(!source_rule_it->second.mime_encoded_click_prefix.empty())
            {
              url_ostr << "&" << AdProtocol::CLICK_PREFIX << "=" <<
                source_rule_it->second.mime_encoded_click_prefix;
            }
          }
        }
        else
        {
          url_ostr << "&preclick=";
          if(request_params.request_type == AR_OPENRTB_WITH_CLICKURL)
          {
            url_ostr << "${CLICK_URL_ENC}";
          }
          else if(request_params.request_type == AR_OPENX)
          {
            url_ostr << "{clickurl_enc}";
          }
          else if(request_params.request_type == AR_GOOGLE)
          {
            url_ostr << "%%CLICK_URL_ESC%%";
          }
        }

        url_ostr << mime_preclick_url;
      }

      if(data_encoding)
      {
        std::string url_base64;
        std::string url_str = url_ostr.str();
        String::StringManip::base64mod_encode(
          url_base64,
          url_str.data(),
          url_str.size());
        if(ad_instantiate_type != AIT_DATA_PARAM_VALUE)
        {
          instantiate_url = "d=";
        }
        instantiate_url += url_base64;
      }
      else
      {
        instantiate_url = url_ostr.str();
      }
    }

    void
    CampaignManagerImpl::init_instantiate_url_(
      std::string& instantiate_url,
      AdInstantiateType ad_instantiate_type,
      CreativeParamsList& creative_params_list,
      RequestResultParams& request_result_params,
      const CampaignConfig* const campaign_config,
      const Tag* tag,
      const InstantiateParams& inst_params,
      const CreativeInstantiateRule& instantiate_info,
      const AdServer::CampaignSvcs::CampaignManager::CommonAdRequestInfo& request_params,
      const AdSelectionResult& ad_selection_result,
      const AdSlotContext& ad_slot_context,
      const char* app_format,
      const AdServer::CampaignSvcs::
        PublisherAccountIdSeq& exclude_pubpixel_accounts,
      bool fill_auction_price)
      /*throw(CreativeTemplateProblem, CreativeOptionsProblem, eh::Exception)*/
    {
      static const char* FUN = "CampaignManagerImpl::init_instantiate_url_()";

      request_result_params.request_id = CorbaAlgs::unpack_request_id(
        request_params.request_id);
      try
      {

        AccountIdList optin_pubpixel_accounts;

        get_inst_optin_pub_pixel_account_ids_(
          optin_pubpixel_accounts,
          campaign_config,
          tag,
          request_params,
          exclude_pubpixel_accounts);
   
        fill_instantiate_url_(
          instantiate_url,
          ad_instantiate_type,
          creative_params_list,
          request_result_params,
          inst_params,
          instantiate_info,
          request_params,
          ad_selection_result,
          ad_slot_context,
          app_format,
          optin_pubpixel_accounts,
          fill_auction_price,
          true);

        find_token(
          tag->tokens,
          CreativeTokens::OVERLAY_HEIGHT,
          request_result_params.overlay_height);

        find_token(
          tag->tokens,
          CreativeTokens::OVERLAY_WIDTH,
          request_result_params.overlay_width);

        find_token(
          ad_selection_result.tag_size->tokens,
          CreativeTokens::OVERLAY_HEIGHT,
          request_result_params.overlay_height);

        find_token(
          ad_selection_result.tag_size->tokens,
          CreativeTokens::OVERLAY_WIDTH,
          request_result_params.overlay_width);

        find_token(
          tag->hidden_tokens,
          CreativeTokens::OVERLAY_HEIGHT,
          request_result_params.overlay_height);

        find_token(
          tag->hidden_tokens,
          CreativeTokens::OVERLAY_WIDTH,
          request_result_params.overlay_width);

        find_token(
          ad_selection_result.tag_size->hidden_tokens,
          CreativeTokens::OVERLAY_HEIGHT,
          request_result_params.overlay_height);

        find_token(
          ad_selection_result.tag_size->hidden_tokens,
          CreativeTokens::OVERLAY_WIDTH,
          request_result_params.overlay_width);
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught eh::Exception on instantiate ";

        for(CampaignSelectionDataList::const_iterator cs_it =
              ad_selection_result.selected_campaigns.begin();
            cs_it != ad_selection_result.selected_campaigns.end();
            ++cs_it)
        {
          ostr << "( " << cs_it->creative->ccid << ", " <<
            cs_it->campaign->campaign_id << ") ";
        }

        ostr << ": " << e.what();

        throw Exception(ostr);
      }
    }

    void
    CampaignManagerImpl::instantiate_url_creative_(
      CORBA::String_out creative_body,
      RequestResultParams& request_result_params,
      const AdSelectionResult& ad_selection_result,
      const String::SubString& instantiate_url,
      AdInstantiateType ad_instantiate_type)
      /*throw(CreativeTemplateProblem)*/
    {
      static const char* FUN = "CampaignManagerImpl::instantiate_url_creative_()";

      request_result_params.mime_format = (
        ad_instantiate_type == AIT_SCRIPT_WITH_URL ?
        campaign_manager_config_.Creative().post_instantiate_script_mime_format() :
        campaign_manager_config_.Creative().post_instantiate_iframe_mime_format());

      try
      {
        PassbackTemplate_var templ = passback_templates_.get(
          ad_instantiate_type == AIT_SCRIPT_WITH_URL ?
          campaign_manager_config_.Creative().post_instantiate_script_template_file() :
          campaign_manager_config_.Creative().post_instantiate_iframe_template_file());

        TokenValueMap args;
        args[CreativeTokens::TAGWIDTH] = to_string(ad_selection_result.tag_size->size->width);
        args[CreativeTokens::TAGHEIGHT] = to_string(ad_selection_result.tag_size->size->height);
        args[CreativeTokens::URL] = instantiate_url.str();

        std::ostringstream ostr;
        templ->instantiate(ostr, args);
        creative_body << ostr.str();
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught eh::Exception: " << ex.what();
        throw CreativeTemplateProblem(ostr);
      }
    }

    void
    CampaignManagerImpl::instantiate_creative_(
      const AdServer::CampaignSvcs::CampaignManager::CommonAdRequestInfo& request_params,
      const CampaignConfig* const campaign_config,
      const Colocation* const colocation,
      const InstantiateParams& inst_params,
      const char* app_format,
      AdSelectionResult& ad_selection_result,
      RequestResultParams& request_result_params,
      CreativeParamsList& creative_params_list,
      CORBA::String_out creative_body,
      const AdSlotContext& ad_slot_context,
      const AdServer::CampaignSvcs::
        PublisherAccountIdSeq* exclude_pubpixel_accounts)
      /*throw(CreativeTemplateProblem, CreativeOptionsProblem, eh::Exception)*/
    {
      static const char* FUN = "CampaignManagerImpl::instantiate_creative_()";

      assert(!ad_selection_result.selected_campaigns.empty());
      assert(ad_selection_result.tag);
      assert(ad_selection_result.tag_size);
      assert(ad_selection_result.selected_campaigns.begin()->creative);

      try
      {

        const Creative* creative =
          ad_selection_result.selected_campaigns.front().creative;
        
        const char* cr_format = creative->creative_format.c_str();

        const std::string& cr_size =
          ad_selection_result.tag_size->size->protocol_name;

        std::string inst_rule(
          request_params.request_type == AR_GOOGLE &&
            creative->https_safe_flag ?
              AdInstantiateRule::SECURE.str() : request_params.creative_instantiate_type.in());

        CreativeInstantiateRuleMap::iterator rule_it =
          creative_instantiate_.creative_rules.find(inst_rule);

        if(rule_it == creative_instantiate_.creative_rules.end())
        {
          Stream::Error ostr;
          ostr << "Cannot find creative instantiate rule with name: " << inst_rule;

          throw Exception(ostr);
        }
        const CreativeInstantiateRule& instantiate_info = rule_it->second;

        CreativeTemplateKey key(
          cr_format,
          cr_size.c_str(),
          app_format);

        Template_var creative_template;
        CreativeTemplate creative_template_descr;

        creative_template =
          campaign_config->creative_templates.get(
            key, creative_template_descr);

        if(!creative_template.in())
        {
          Stream::Error ostr;
          ostr << "Cannot find corresponding template for instantiate: "
            "creative_format = '" << cr_format <<
            "', size = '" << cr_size << "', app_format= '" <<
            app_format << "'";

          throw CreativeTemplateProblem(ostr);
        }

        TemplateParams_var template_request_params;
        TemplateParamsList template_creative_params;

        fill_instantiate_params_(
          request_params,
          campaign_config,
          colocation,
          creative_template_descr,
          creative_template,
          inst_params,
          instantiate_info,
          app_format,
          ad_selection_result,
          request_result_params,
          creative_params_list,
          template_request_params,
          template_creative_params,
          ad_slot_context,
          exclude_pubpixel_accounts);

        std::ostringstream creative_body_str;

        // try to init current template (but body isn't required)
        creative_template->instantiate(
          template_request_params.in(),
          template_creative_params,
          creative_body_str);

        request_result_params.mime_format = creative_template_descr.mime_format;
        creative_body << creative_body_str.str();
      }
      catch(const Template::InvalidTemplate& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Template::InvalidTemplate caught "
          "while instantiating creative (ccid, cmpid): ";

        for(CampaignSelectionDataList::iterator cs_it =
              ad_selection_result.selected_campaigns.begin();
            cs_it != ad_selection_result.selected_campaigns.end();
            ++cs_it)
        {
          ostr << "( " << cs_it->creative->ccid << ", " << cs_it->campaign->campaign_id << ") ";
        }

        ostr << " for app_format='" << app_format <<
          "' for tag.id=" << ad_selection_result.tag->tag_id <<
          " for tag_size.id=" << ad_selection_result.tag_size->size->size_id <<
          ": " << ex.what();

        throw CreativeTemplateProblem(ostr);
      }
      catch(const CreativeTemplateProblem&)
      {
        throw;
      }
      catch(const CreativeOptionsProblem&)
      {
        throw;
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception "
          "while instantiating creative (ccid, cmp_id): ";

        for(CampaignSelectionDataList::iterator cs_it =
              ad_selection_result.selected_campaigns.begin();
            cs_it != ad_selection_result.selected_campaigns.end();
            ++cs_it)
        {
          ostr << "( " << cs_it->creative->ccid << ", " << cs_it->campaign->campaign_id << ") ";
        }

        ostr << " for app_format='" << app_format <<
          "' for tag.id=" << ad_selection_result.tag->tag_id <<
          " for tag_size.id=" << ad_selection_result.tag_size->size->size_id <<
          ": " << e.what();

        throw CreativeInstantiateProblem(ostr);
      }
    }

    bool
    CampaignManagerImpl::instantiate_passback(
      CORBA::String_out mime_format,
      CORBA::String_out passback_body,
      const CampaignConfig* const campaign_config,
      const Colocation* colocation,
      const Tag* tag,
      const char* app_format,
      const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      const AdSlotContext& ad_slot_context,
      const String::SubString& ext_tag_id)
      /*throw(eh::Exception)*/
    {
      static const char* FUN = "CampaignManagerImpl::instantiate_passback()";

      try
      {
        CreativeInstantiateRuleMap::iterator rule_it =
          creative_instantiate_.creative_rules.find(
            request_params.common_info.creative_instantiate_type.in());

        if(rule_it == creative_instantiate_.creative_rules.end())
        {
          return false;
        }

        const CreativeInstantiateRule& instantiate_info = rule_it->second;

        // getting passback template
        AppFormatMap::const_iterator app_format_it =
          campaign_config->app_formats.find(app_format);
        if(app_format_it != campaign_config->app_formats.end())
        {
          Generics::StringHashAdapter key =
            rule_it->second.passback_template_path_prefix + app_format_it->first;

          PassbackTemplate_var passback_template =
            passback_templates_.get(key);

          if(passback_template.in())
          {
            // instantiate passback template
            TokenValueMap request_args;
            AccountIdList consider_pub_pixel_accounts;

            fill_instantiate_request_params_(
              request_args,
              &consider_pub_pixel_accounts,
              campaign_config,
              colocation,
              tag,
              !tag->sizes.empty() ? tag->sizes.begin()->second.in() : 0,
              app_format,
              request_params.common_info,
              0, // pubpixel_accounts
              &request_params.exclude_pubpixel_accounts,
              instantiate_info,
              ad_slot_context,
              ext_tag_id);

            fill_instantiate_passback_params_(
              request_args,
              campaign_config,
              tag,
              InstantiateParams(
                request_params.common_info.user_id,
                false // enabled_notice : don't used here
                ),
              request_params.common_info,
              instantiate_info,
              ad_slot_context,
              &consider_pub_pixel_accounts);

            CreativeTextGenerator::init_creative_tokens(
              instantiate_info,
              CreativeInstantiateArgs(),
              campaign_config->token_processors,
              request_args,
              tag->tokens,
              request_args);

            if(!tag->sizes.empty())
            {
              // use size with lower id
              CreativeTextGenerator::init_creative_tokens(
                instantiate_info,
                CreativeInstantiateArgs(),
                campaign_config->token_processors,
                request_args,
                tag->sizes.begin()->second->tokens,
                request_args);
            }

            CreativeTextGenerator::init_creative_tokens(
              instantiate_info,
              CreativeInstantiateArgs(),
              campaign_config->token_processors,
              request_args,
              tag->hidden_tokens,
              request_args);

            std::ostringstream passback_body_ostr;
            passback_template->instantiate(passback_body_ostr, request_args);
            passback_body << passback_body_ostr.str();
            mime_format << app_format_it->second.mime_format;
          }
          else
          {
            Stream::Error ostr;
            ostr << FUN << ": can't open passback template for appformat='" <<
              app_format << "', file '" <<
              key.text() << "'";
            logger_->log(ostr.str(),
              Logging::Logger::ERROR,
              Aspect::TRAFFICKING_PROBLEM);
          }
        }
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception "
          "while instantiating passback tag_id=" << (tag ? tag->tag_id : 0) <<
          ": " << e.what();
        logger_->log(ostr.str(),
          Logging::Logger::NOTICE,
          Aspect::TRAFFICKING_PROBLEM);
        return false;
      }

      return true;
    }

    void
    CampaignManagerImpl::fill_creative_instantiate_args_(
      CreativeInstantiateArgs& creative_instantiate_args,
      const CreativeInstantiateRule& creative_instantiate_rule,
      const Creative* creative,
      const ClickParams& click_params,
      unsigned long random)
      noexcept
    {
      std::string mime_click0_url;
      String::StringManip::mime_url_encode(
        click_params.click0_url, mime_click0_url);

      std::string mime_resource_url_suffix;
      OptionTokenValueMap::const_iterator tok_it = creative->tokens.find(
        "ADIMAGE-PATH-SUFFIX");
      if(tok_it != creative->tokens.end())
      {
        String::StringManip::mime_url_encode(
          tok_it->second.value, mime_resource_url_suffix);
      }

      std::ostringstream url_ostr;

      url_ostr << creative_instantiate_rule.dynamic_creative_prefix <<
        "?rs=" << mime_resource_url_suffix <<
        "&r=" << random <<
        '&' << AdProtocol::CAMPAIGN_MANAGER_INDEX << '=' <<
          campaign_manager_config_.service_index();

      if(!click_params.mime_pub_preclick_url.empty())
      {
        url_ostr << "&prck=" << click_params.mime_pub_preclick_url;
      }

      creative_instantiate_args.full_dynamic_creative_prefix = url_ostr.str();
      creative_instantiate_args.last_c_param = std::string("&c=") + mime_click0_url;
    }

    void
    CampaignManagerImpl::fill_yandex_track_params_(
      std::string& yandex_track_params,
      const AdServer::CampaignSvcs::CampaignManager::
        CommonAdRequestInfo& request_params,
      const AdSlotContext& ad_slot_context)
      noexcept
    {
      // fill yandex_track_params for fill tracking in vast case
      std::ostringstream yandex_track_params_ostr;

      CORBA::ULong tok_i = 0;
      for(; tok_i < request_params.tokens.length(); ++tok_i)
      {
        String::SubString tok_name(request_params.tokens[tok_i].name.in());
        String::SubString param_name = tok_name;

        std::string enc_tok_value;
        encode_parameter(
          enc_tok_value,
          String::SubString(request_params.tokens[tok_i].value.in()),
          true);

        auto tok_map_it = token_to_parameters_.find(tok_name);
        if(tok_map_it != token_to_parameters_.end())
        {
          param_name = tok_map_it->second;
        }

        yandex_track_params_ostr << (tok_i != 0 ? "&" : "") << param_name <<
          "=" << enc_tok_value;
      }

      if(ad_slot_context.tns_counter_device_type[0])
      {
        String::SubString tok_name(CreativeTokens::TNS_COUNTER_DEVICE_TYPE);
        String::SubString param_name = tok_name;

        std::string enc_tok_value;
        encode_parameter(
          enc_tok_value,
          String::SubString(ad_slot_context.tns_counter_device_type),
          true);

        auto tok_map_it = token_to_parameters_.find(tok_name);
        if(tok_map_it != token_to_parameters_.end())
        {
          param_name = tok_map_it->second;
        }

        yandex_track_params_ostr << (tok_i != 0 ? "&" : "") << param_name <<
          "=" << enc_tok_value;
        ++tok_i;
      }

      yandex_track_params = yandex_track_params_ostr.str();
    }

    void
    CampaignManagerImpl::init_yandex_tokens_(
      const CampaignConfig* campaign_config,
      const CreativeInstantiateRule& instantiate_info,
      RequestResultParams& request_result_params,
      const AdServer::CampaignSvcs::CampaignManager::
        CommonAdRequestInfo& request_params,
      const AdSlotContext& ad_slot_context,
      const Creative* creative)
      /*throw(CreativeInstantiateProblem)*/
    {
      struct YandexTokenMapping
      {
        const char* name;
        const char* result_name;
      };

      const YandexTokenMapping YANDEX_TOKENS[] = {
        {"YANDEX_TOKEN", "token"},
        {"YANDEX_PROP", "properties"},
        {"YANDEX_ADM", "adm"}
      };

      for(unsigned long i = 0; i < sizeof(YANDEX_TOKENS) / sizeof(YANDEX_TOKENS[0]); ++i)
      {
        OptionTokenValueMap::const_iterator token_it = creative->tokens.find(
          YANDEX_TOKENS[i].name);
        if(token_it == creative->tokens.end() || token_it->second.value.empty())
        {
          Stream::Error ostr;
          ostr << "CampaignManagerImpl::init_yandex_tokens_(): yandex token " <<
            YANDEX_TOKENS[i].result_name << " has empty value";
          throw CreativeInstantiateProblem(ostr);
        }
        request_result_params.tokens.insert(std::make_pair(
          YANDEX_TOKENS[i].result_name,
          token_it->second.value));
      }

      fill_yandex_track_params_(
        request_result_params.yandex_track_params,
        request_params,
        ad_slot_context);

      // fill add_track_pixel_urls for fill tracking in banner case
      init_track_pixels_(
        campaign_config,
        request_result_params,
        request_params,
        ad_slot_context,
        creative,
        instantiate_info,
        false);
    }

    void
    CampaignManagerImpl::init_track_pixels_(
      const CampaignConfig* campaign_config,
      RequestResultParams& request_result_params,
      const AdServer::CampaignSvcs::CampaignManager::
        CommonAdRequestInfo& request_params,
      const AdSlotContext& ad_slot_context,
      const Creative* creative,
      const CreativeInstantiateRule& instantiate_info,
      bool need_absolute_urls)
    {
      (void)need_absolute_urls;

      // fill additional track pixels for Yandex
      // trust that these options defined as url and checked by https safety
      const char* TRACK_TOKENS[] = {
        "CRADVTRACKPIXEL",
        "CRADVTRACKPIXEL2",
        "CRADVTRACKPIXEL3",
        "CRADVTRACKPIXEL4",
        "CRADVTRACKPIXEL5",
        "CRADVTRACKPIXEL6",
        "CRADVTRACKPIXEL7"
      };

      // fill add_track_pixel_urls for fill tracking in banner case
      for(unsigned long i = 0;
        i < sizeof(TRACK_TOKENS) / sizeof(TRACK_TOKENS[0]); ++i)
      {
        OptionTokenValueMap::const_iterator token_it = creative->tokens.find(
          TRACK_TOKENS[i]);
        // relative prefix requirement allow to ignore token type 'file'
        if(token_it != creative->tokens.end() &&
          !token_it->second.value.empty()
          // && (need_absolute_urls || RELATIVE_URL_PREFIX.start(token_it->second.value))
          )
        {
          OptionTokenValueMap args;
          args[CreativeTokens::RANDOM] = OptionValue(
            0, String::StringManip::IntToStr(request_params.random).str().str().c_str());
          if(request_params.ext_track_params[0])
          {
            args[CreativeTokens::EXT_TRACK_PARAMS] = OptionValue(
              0, request_params.ext_track_params);
          }
          
          args[CreativeTokens::TNS_COUNTER_DEVICE_TYPE] =
            OptionValue(0, ad_slot_context.tns_counter_device_type);
          
          for(CORBA::ULong tok_i = 0; tok_i < request_params.tokens.length(); ++tok_i)
          {
            args[request_params.tokens[tok_i].name.in()] = OptionValue(
              0, request_params.tokens[tok_i].value.in());
          }
          
          args[TRACK_TOKENS[i]] = token_it->second;
          
          const TokenProcessorMap::const_iterator it =
          campaign_config->token_processors.find(token_it->second.option_id);
          
          if(it != campaign_config->token_processors.end())
          {
            try
            {
              std::string track_pixel_url;
              it->second->instantiate(
                args,
                campaign_config->token_processors,
                instantiate_info,
                CreativeInstantiateArgs(),
                track_pixel_url);

              request_result_params.add_track_pixel_urls.push_back(
                track_pixel_url);
            }
            catch(const eh::Exception&)
            {
              // don't use track pixel if it contains some non RANDOM token
            }
          }
        }
      }
    }
    
    void
    CampaignManagerImpl::init_native_tokens_(
      const CampaignConfig* campaign_config,
      const CreativeInstantiateRule& instantiate_info,
      RequestResultParams& request_result_params,
      const AdServer::CampaignSvcs::CampaignManager::
        CommonAdRequestInfo& request_params,
      const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
      const AdSlotContext& ad_slot_context,
      const Creative* creative)
      /*throw(CreativeInstantiateProblem)*/
    {

      static const char* FUN = "CampaignManagerImpl::init_native_tokens_()";

      // Data tokens
      
      for(unsigned long i = 0; i < ad_slot.native_data_tokens.length(); ++i)
      {
        const AdServer::CampaignSvcs::CampaignManager::NativeDataToken& token =
          ad_slot.native_data_tokens[i];
        OptionTokenValueMap::const_iterator token_it = creative->tokens.find(
          token.name.in());
        if (token_it == creative->tokens.end() || token_it->second.value.empty())
        {
          if (token.required)
          {
            Stream::Error ostr;
            ostr << FUN << ": data token '" <<
              token.name << "' is absent or has empty value";
            throw CreativeInstantiateProblem(ostr);
          }
        }
        else
        {
          request_result_params.native_data_tokens.insert(
            std::make_pair(
              token.name.in(),
              token_it->second.value));
        }
      }

      // Image tokens
      OptionTokenValueMap args;

      if(request_params.external_user_id[0])
      {
        args[CreativeTokens::EXTERNAL_USER_ID] = OptionValue(
          0, request_params.external_user_id);
      }

      for(CORBA::ULong tok_i = 0; tok_i < request_params.tokens.length(); ++tok_i)
      {
        args[request_params.tokens[tok_i].name.in()] = OptionValue(
          0, request_params.tokens[tok_i].value.in());
      }

      for(unsigned long i = 0; i < ad_slot.native_image_tokens.length(); ++i)
      {
        const AdServer::CampaignSvcs::CampaignManager::NativeImageToken& token =
          ad_slot.native_image_tokens[i];

        unsigned long width = token.width;
        unsigned long height = token.height;

        auto token_it = creative->tokens.find(token.name.in());
        if (token_it == creative->tokens.end() || token_it->second.value.empty())
        {
          if (token.required)
          {
            Stream::Error ostr;
            ostr << FUN << ": image token '" <<
              token.name << "' is absent or has empty value";
            throw CreativeInstantiateProblem(ostr);
          }
        }
        else
        {
          if (token.name != MAIN_IMAGE)
          {
            std::ostringstream width_name_ostr, height_name_ostr;
            width_name_ostr << token.name << "_WIDTH";
            height_name_ostr << token.name << "_HEIGHT";
            find_token(
              creative->tokens, width_name_ostr.str(), width);
            find_token(
              creative->tokens, height_name_ostr.str(), height);
            if (!(width && height))
            {
              if (token.required)
              {
                Stream::Error ostr;
                ostr << FUN << ": can't detect width or height for image token '" <<
                  token.name << "'";
                throw CreativeInstantiateProblem(ostr);
              }
              continue;
            }
          }

          args[token.name.in()] = token_it->second;

          try
          {
            BaseTokenProcessor* token_processor = 0;
            
            const TokenProcessorMap::const_iterator it =
              campaign_config->token_processors.find(
                token_it->second.option_id);
            
            if(it != campaign_config->token_processors.end())
            {
              token_processor = it->second;
            }
            else
            {
              token_processor = campaign_config->default_click_token_processor;
            }
            
            std::string image_url_str;
            
            token_processor->instantiate(
              args,
              campaign_config->token_processors,
              instantiate_info,
              CreativeInstantiateArgs(),
              image_url_str);          
         
            ImageToken image;

            HTTP::BrowserAddress image_url_addr(image_url_str);
            image.width = width;
            image.height = height;
            image_url_addr.get_view(HTTP::HTTPAddress::VW_FULL, image.value);

            if(ad_slot.native_ads_impression_tracker_type == AdServer::CampaignSvcs::NAITT_RESOURCES)
            {
              std::string mime_image_value;
              String::StringManip::mime_url_encode(image.value, mime_image_value);

              std::string image_value = request_result_params.track_pixel_url;
              image_value += "&r=";
              image_value += mime_image_value;
              image.value.swap(image_value);
            }

            request_result_params.native_image_tokens.insert(
              std::make_pair(token.name.in(), image));
          }
          catch(const eh::Exception& ex)
          {
            if (token.required)
            {
              Stream::Error ostr;
              ostr << FUN << ": can't instantiate image token '" <<
                token.name << "': " << ex.what();
              throw CreativeInstantiateProblem(ostr);
            }
            continue;
          }
        }
      }

      init_track_pixels_(
        campaign_config,
        request_result_params,
        request_params,
        ad_slot_context,
        creative,
        instantiate_info,
        true);

      if (ad_slot.fill_track_html)
      {
        PassbackTemplate_var templ = passback_templates_.get(
          campaign_manager_config_.Creative().instantiate_track_html_file());

        TokenValueMap args;
        args[CreativeTokens::TRACKHTMLURL] = request_result_params.track_html_url;

        std::ostringstream ostr;
        templ->instantiate(ostr, args);
        request_result_params.track_html_body = ostr.str();
      }
    }
    
    void
    CampaignManagerImpl::init_vast_tokens_(
      RequestResultParams& request_result_params,
      const Creative* creative)
      /*throw(CreativeInstantiateProblem)*/
    {
      OptionTokenValueMap::const_iterator token_it =
        creative->tokens.find("MP4_DURATION");

      if(token_it != creative->tokens.end())
      {
        int duration;
        if(String::StringManip::str_to_int(
             token_it->second.value,
             duration))
        {
          request_result_params.ext_tokens.insert(std::make_pair(
            "duration",
            token_it->second.value));
        }
      }

      token_it = creative->tokens.find("VIDEO_TYPE");

      if(token_it != creative->tokens.end())
      {
        request_result_params.ext_tokens.insert(std::make_pair(
          "crtype",
          token_it->second.value));
      }
      else
      {
        request_result_params.ext_tokens.insert(std::make_pair(
          "crtype",
          "VAST 2.0"));
      }
    }
  } // namespace CampaignSvcs
} // namespace AdServer

