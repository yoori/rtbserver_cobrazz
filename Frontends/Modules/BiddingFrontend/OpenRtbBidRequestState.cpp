#include <string_view>
#include <utility>

#include <zlib.h>


#include "OpenRtbBidRequestState.hpp"

#include <Commons/GrpcAlgs.hpp>

namespace AdServer::Bidding
{
  namespace
  {
    const String::AsciiStringManip::Caseless WWW_PREFIX("www.");
    const String::AsciiStringManip::CharCategory SLASH("\\");

    namespace Aspect
    {
      const char BIDDING_FRONTEND[] = "BiddingFrontend";
    }

    String::SubString
    to_sub_string(std::string_view value) noexcept
    {
      return String::SubString(value.data(), value.size());
    }

    namespace Response::Header
    {
      const std::string CONTENT_TYPE("Content-Type");
      const std::string OPENRTB_VERSION("x-openrtb-version");
      const std::string OPENRTB_VERSION_VALUE("2.3");
      const std::string OPENRTB_INTERRUPTED_BID("X-Interrupted-Bid");

      const String::AsciiStringManip::Caseless CONTENT_ENCODING("content-encoding");
      const String::AsciiStringManip::Caseless CONTENT_ENCODING2("content_encoding");
      const String::AsciiStringManip::Caseless GZIP("gzip");
    }

    namespace Response::Type
    {
      const std::string TEXT_HTML("text/html");
      const std::string JSON("application/json");
      const std::string OCTET_STREAM("application/octet-stream");
    }

    namespace Response::OpenRtb
    {
      const std::string ID("id");
      const std::string DEAL_ID("dealid");
      const std::string BID("bid");
      const std::string BID_EXT("ext");
      const std::string BIDID("bidid");
      const std::string CUR("cur");
      const std::string EXT("ext");
      const std::string PROTOCOL("protocol");
      const std::string SEATBID("seatbid");
      const std::string ADID("adid");
      const std::string PRICE("price");
      const std::string IMPID("impid");
      const std::string CRID("crid");
      const std::string ADOMAIN("adomain");
      const std::string ADVERTISER_NAME("advertiser_name");
      const std::string VAST_URL("vast_url");
      const std::string ADM("adm");
      const std::string NURL("nurl");
      const std::string BURL("burl");
      const std::string CAT("cat");
      const std::string IURL("iurl");

      const std::string WIDTH("w");
      const std::string HEIGHT("h");
      const std::string CID("cid");
      const std::string SEAT("seat");
      const std::string ATTR("attr");

      // ADSC-10918 Native ads
      const std::string NATIVE("native");
      const std::string NATIVE_VER("ver");
      const std::string NATIVE_VER_1_0("1");
      const std::string NATIVE_VER_1_2("1.2");

      const std::string NATIVE_LINK("link");
      const std::string NATIVE_LINK_URL("url");
      const std::string NATIVE_LINK_CLICK_TRACKERS("clicktrackers");
      const std::string NATIVE_IMP_TRACKERS("imptrackers");
      const std::string NATIVE_EVENT_TRACKERS("eventtrackers");
      const std::string NATIVE_JS_TRACKER("jstracker");
      const std::string NATIVE_ASSETS("assets");
      const std::string NATIVE_ASSET_ID("id");
      const std::string NATIVE_ASSET_REQUIRED("required");
      const std::string NATIVE_ASSET_IMG("img");
      const std::string NATIVE_ASSET_IMG_URL("url");
      const std::string NATIVE_ASSET_IMG_W("w");
      const std::string NATIVE_ASSET_IMG_H("h");
      const std::string NATIVE_ASSET_DATA("data");
      const std::string NATIVE_ASSET_DATA_VALUE("value");
      const std::string NATIVE_ASSET_TITLE("title");
      const std::string NATIVE_ASSET_TITLE_TEXT("text");
      const std::string NATIVE_ASSET_VIDEO("video");
      const std::string NATIVE_ASSET_VIDEO_TAG("vasttag");

      const std::string EVENT_EVENT("event");
      const std::string EVENT_METHOD("method");
      const std::string EVENT_URL("url");

      const std::string BID_EXT_NROA("nroa");

      const std::string NROA_ERID("erid");
      const std::string NROA_CONTRACTS("contracts");
      const std::string CONTRACT_ORD_ID("id");
      const std::string CONTRACT_ORD_ADO_ID("ado_id");
      const std::string CONTRACT_NUMBER("contract_number");
      const std::string CONTRACT_DATE("contract_date");
      const std::string CONTRACT_TYPE("type");
      const std::string CONTRACT_CLIENT_ID("client_inn");
      const std::string CONTRACT_CLIENT_NAME("client_name");
      const std::string CONTRACT_CONTRACTOR_ID("contractor_inn");
      const std::string CONTRACT_CONTRACTOR_NAME("contractor_name");

      // nroa attributes by buzzula+sape standard
      namespace BuzSapeNroa
      {
        const std::string HASNROAMARKUP("has_nroa_markup");

        const std::string CONTRACTOR("contractor");
        const std::string CONTRACTOR_INN("inn");
        const std::string CONTRACTOR_NAME("name");
        const std::string CONTRACTOR_LEGALFORM("legal_form");

        const std::string CLIENT("client");
        const std::string CLIENT_INN("inn");
        const std::string CLIENT_NAME("name");
        const std::string CLIENT_LEGALFORM("legal_form");

        const std::string INITIALCONTRACT("initial_contract");
        const std::string PARENTCONTRACTS("parent_contracts");

        const std::string CONTRACT_SIGNDATE("sign_date");
        const std::string CONTRACT_NUMBER("number");
        const std::string CONTRACT_TYPE("type");
        const std::string CONTRACT_SUBJECTTYPE("subject_type");
        const std::string CONTRACT_ACTIONTYPE("action_type");
        const std::string CONTRACT_ID("id");
        const std::string CONTRACT_ADOID("ado_id");
        const std::string CONTRACT_VATINCLUDED("vat_included");
        const std::string CONTRACT_PARENTCONTRACTID("parent_contract_id");
      }
    }

    namespace Response::OpenX
    {
      const std::string AD_OX_CATS("ad_ox_cats");
      const std::string MATCHING_AD_ID("matching_ad_id");
    }

    namespace Response::Yandex
    {
      const std::string ID("id");
      const std::string SETUSERDATA("setuserdata");
      const std::string BIDSET("bidset");
      const std::string BID("bid");
      const std::string BIDID("bidid");
      const std::string CUR("cur");
      const std::string UNITS("units");
      const std::string ADID("adid");
      const std::string PRICE("price");
      const std::string ADM("adm");
      const std::string VIEW_NOTICES("view_notices");
      const std::string NURL("nurl");
      const std::string BURL("burl");

      const std::string BANNER("banner");
      const std::string WIDTH("w");
      const std::string HEIGHT("h");
      const std::string SETSKIPTOKEN("setskiptoken");

      const std::string DSP_PARAMS("dsp_params");
      /*
        const std::string URL_PARAM3("url_param3");
        const std::string URL_PARAM4("url_param4");
      */
      const std::string URL_PARAM17("url_param17");
      const std::string URL_PARAM18("url_param18");

      const std::string URL_PARAM_ALL[] = {
        "url_param1",
        "url_param2",
        "url_param3",
        "url_param4",
        "url_param5",
        "url_param6",
        "url_param7",
        "url_param8",
        "url_param9",
        "url_param10",
        "url_param11",
        "url_param12",
        "url_param13",
        "url_param14",
        "url_param15"
      };
    } // namespace Yandex

    struct YandexIdFormatter
    {
      const String::SubString id_str;

      YandexIdFormatter(const String::SubString& id_str_val)
      : id_str(id_str_val)
      {}
    };

    std::ostream& operator<<(std::ostream& os, const YandexIdFormatter& fmt)
    {
      const char* const END = fmt.id_str.end();
      bool only_digits =
        String::AsciiStringManip::NUMBER.find_nonowned(fmt.id_str.begin(), END) == END;

      bool is_number =
        (!fmt.id_str.empty()) &&
        (fmt.id_str[0] != '0') &&
        only_digits;

      if (is_number)
      {
        os << fmt.id_str;
      }
      else
      {
        os << "\"" << fmt.id_str << "\"";
      }
      return os;
    }

    void
    print_int_category_seq(
      AdServer::Commons::JsonObject& parent,
      const String::SubString& seq_name,
      const std::pmr::vector<std::string>& categories)
    {
      AdServer::Commons::JsonObject array(parent.add_array(seq_name));
      for(std::size_t cat_i = 0; cat_i < categories.size(); ++cat_i)
      {
        int cat_id = 0;
        if(String::StringManip::str_to_int(
             String::SubString(categories[cat_i]), cat_id))
        {
          array.add_number(cat_id);
        }
      }
    }
  }

  OpenRtbBidRequestState::OpenRtbBidRequestState(
    BiddingFrontendCore* bid_frontend,
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::BaseHttpResponseWriter_var response_writer,
    const Generics::Time& start_processing_time)
    /*throw(Invalid)*/
    : BidRequestState(
        bid_frontend,
        std::move(request_holder),
        std::move(response_writer),
        start_processing_time),
      uri_(request_holder_->request().uri().str())
  {}

  bool
  OpenRtbBidRequestState::read_request() noexcept
  {
    static const char* FUN = "OpenRtbBidRequestState::read_request()";

    std::string bid_request;

    try
    {
      const FCGI::HttpRequest& request = request_holder_->request();

      Stream::BinaryStreamReader request_reader(
        &request.get_input_stream());

      char buf[1024];

      while(!request_reader.eof() && !request_reader.bad())
      {
        request_reader.read(buf, sizeof(buf));
        bid_request.append(buf, request_reader.gcount());
      }

      // check gzip
      bool apply_unzip = false;
      const HTTP::SubHeaderList& headers = request.headers();

      for (HTTP::SubHeaderList::const_iterator it = headers.begin();
           it != headers.end(); ++it)
      {
        if((Response::Header::CONTENT_ENCODING.compare(it->name) == 0 ||
            Response::Header::CONTENT_ENCODING2.compare(it->name) == 0) &&
           Response::Header::GZIP.compare(it->value) == 0)
        {
          apply_unzip = true;
        }
      }

      if(apply_unzip)
      {
        std::string res;
        res.resize(32 * 1024);

        z_stream zs;
        ::memset(&zs, 0, sizeof(zs));
        ::inflateInit2(&zs, 16 + MAX_WBITS);

        zs.avail_in = bid_request.size();
        zs.next_in = (Bytef*)&bid_request[0];
        zs.avail_out = res.size();
        zs.next_out = (Bytef*)&res[0];

        int err = ::inflate(&zs, Z_NO_FLUSH);
        if(!(err == Z_OK || err == Z_STREAM_END))
        {
          // decompression error
          return false;
        }

        res.resize(res.size() - zs.avail_out);

        ::inflateEnd(&zs);

        bid_request.swap(res);
      }

      bid_frontend_->request_info_filler()->fill_by_openrtb_request(
        request_info_,
        context_,
        std::move(bid_request));

      return true;
    }
    catch(const FrontendCommons::HTTPExceptions::InvalidParamException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": bad request, " << ex.what() <<
        ", request: '" << bid_request << "'" <<
        ", uri: '" << uri_ << "'";

      bid_frontend_->logger()->log(
        ostr.str(),
        Logging::Logger::ERROR,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-7601");
    }

    return false;
  }

  std::string_view
  OpenRtbBidRequestState::channel_keywords() const noexcept
  {
    return std::string_view(
      request_info_.keywords.data(),
      request_info_.keywords.size());
  }

  bool
  OpenRtbBidRequestState::write_response(
    const AdServer::Bidding::CampaignManager::RequestCreativeResult&
      campaign_match_result)
    noexcept
  {
    std::ostringstream response_ostr;
    bool first_is_native = context_.ad_slots.empty() ? false : (context_.ad_slots.begin()->native != 0);

    if(request_info_.request_type != AdServer::CampaignSvcs::AR_YANDEX || first_is_native)
    {
      // standard OpenRTB, OpenX, Yandex native
      fill_openrtb_response_(
        response_ostr,
        request_info_,
        context_,
        campaign_match_result,
        request_info_.request_type == AdServer::CampaignSvcs::AR_YANDEX);
    }
    else
    {
      fill_yandex_response_(
        response_ostr,
        request_info_,
        context_,
        campaign_match_result);
    }

    // write response
    const std::string bid_response = response_ostr.str();

    if(!bid_response.empty())
    {
      FCGI::HttpResponse_var response(new FCGI::HttpResponse());
      response->set_content_type_nocopy(Response::Type::JSON);
      response->add_header_nocopy(
        Response::Header::OPENRTB_VERSION,
        Response::Header::OPENRTB_VERSION_VALUE);

      if (interrupted())
      {
        const auto stage =  convert_stage_to_string(
          get_current_stage());
        response->add_header_nocopy(
          Response::Header::OPENRTB_INTERRUPTED_BID,
          stage);
      }

      auto& output = response->get_output_stream();
      output.write(bid_response.data(), bid_response.size());

      write_response_(200, response);

      return true;
    }

    return false;
  }

  void
  OpenRtbBidRequestState::write_empty_response(unsigned int code, bool response_claimed)
    noexcept
  {
    FCGI::HttpResponse_var response(new FCGI::HttpResponse());

    if (interrupted())
    {
      const auto stage =  convert_stage_to_string(
        get_current_stage());
      response->add_header_nocopy(
        Response::Header::OPENRTB_INTERRUPTED_BID,
        stage);
    }

    if(code < 300)
    {
      // no-bid is No content by OpenRTB 2.0 spec
      response->set_content_type_nocopy(Response::Type::JSON);
      response->add_header_nocopy(
        Response::Header::OPENRTB_VERSION,
        Response::Header::OPENRTB_VERSION_VALUE);
      write_response_(204, response, response_claimed);
    }
    else
    {
      write_response_(code, response, response_claimed);
    }
  }

  void
  OpenRtbBidRequestState::clear() noexcept
  {
    BidRequestState::clear();
    uri_.clear();
    context_.clear();
  }

  void
  OpenRtbBidRequestState::print_request(std::ostream& /*out*/) const noexcept
  {
    //out << bid_request_;
  }

  void
  OpenRtbBidRequestState::add_response_notice_(
    AdServer::Commons::JsonObject& bid_object,
    const String::SubString& url,
    SourceTraits::NoticeInstantiateType notice_instantiate_type)
  {
    if(notice_instantiate_type != SourceTraits::NIT_NURL_AND_BURL)
    {
      bid_object.add_escaped_string(
        notice_instantiate_type != SourceTraits::NIT_BURL ?
          Response::OpenRtb::NURL : Response::OpenRtb::BURL,
        url);
    }
    else
    {
      bid_object.add_escaped_string(Response::OpenRtb::NURL, url);
      bid_object.add_escaped_string(Response::OpenRtb::BURL, url);
    }
  }

  void
  OpenRtbBidRequestState::fill_ext0_nroa_(
    AdServer::Commons::JsonObject& nroa_obj,
    const RequestInfo& /*request_info*/,
    const AdServer::Bidding::CampaignManager::AdSlotResult& ad_slot_result)
    noexcept
  {
    nroa_obj.add_escaped_string_if_non_empty(
      Response::OpenRtb::NROA_ERID, String::SubString(ad_slot_result.erid));

    if(ad_slot_result.contracts.size() > 0)
    {
      AdServer::Commons::JsonObject contracts_array(nroa_obj.add_array(Response::OpenRtb::NROA_CONTRACTS));

      for(std::size_t contract_i = 0; contract_i < ad_slot_result.contracts.size(); ++contract_i)
      {
        const AdServer::Bidding::CampaignManager::ExtContractInfo& contract = ad_slot_result.contracts[contract_i];
        AdServer::Commons::JsonObject contract_obj(contracts_array.add_object());
        contract_obj.add_escaped_string_if_non_empty(
          Response::OpenRtb::CONTRACT_ORD_ID, String::SubString(contract.contract_info.ord_contract_id));
        contract_obj.add_escaped_string_if_non_empty(
          Response::OpenRtb::CONTRACT_ORD_ADO_ID, String::SubString(contract.contract_info.ord_ado_id));
        contract_obj.add_escaped_string_if_non_empty(
          Response::OpenRtb::CONTRACT_NUMBER, String::SubString(contract.contract_info.number));
        contract_obj.add_escaped_string_if_non_empty(
          Response::OpenRtb::CONTRACT_DATE, String::SubString(contract.contract_info.date));
        contract_obj.add_escaped_string_if_non_empty(
          Response::OpenRtb::CONTRACT_TYPE, String::SubString(contract.contract_info.type));
        contract_obj.add_escaped_string_if_non_empty(
          Response::OpenRtb::CONTRACT_CLIENT_ID, String::SubString(contract.contract_info.client_id));
        contract_obj.add_escaped_string_if_non_empty(
          Response::OpenRtb::CONTRACT_CLIENT_NAME, String::SubString(contract.contract_info.client_name));
        contract_obj.add_escaped_string_if_non_empty(
          Response::OpenRtb::CONTRACT_CONTRACTOR_ID, String::SubString(contract.contract_info.contractor_id));
        contract_obj.add_escaped_string_if_non_empty(
          Response::OpenRtb::CONTRACT_CONTRACTOR_NAME, String::SubString(contract.contract_info.contractor_name));
      }
    }
  }

  void
  OpenRtbBidRequestState::fill_buzsape_nroa_contract_(
    AdServer::Commons::JsonObject& contract_obj,
    const AdServer::Bidding::CampaignManager::ExtContractInfo& ext_contract_info)
    noexcept
  {
    contract_obj.add_escaped_string_if_non_empty(
      Response::OpenRtb::BuzSapeNroa::CONTRACT_SIGNDATE,
      String::SubString(ext_contract_info.contract_info.date));
    contract_obj.add_escaped_string_if_non_empty(
      Response::OpenRtb::BuzSapeNroa::CONTRACT_NUMBER,
      String::SubString(ext_contract_info.contract_info.number));
    contract_obj.add_escaped_string_if_non_empty(
      Response::OpenRtb::BuzSapeNroa::CONTRACT_TYPE,
      String::SubString(ext_contract_info.contract_info.type));
    contract_obj.add_escaped_string_if_non_empty(
      Response::OpenRtb::BuzSapeNroa::CONTRACT_SUBJECTTYPE,
      String::SubString(ext_contract_info.contract_info.subject_type));
    contract_obj.add_escaped_string_if_non_empty(
      Response::OpenRtb::BuzSapeNroa::CONTRACT_ACTIONTYPE,
      String::SubString(ext_contract_info.contract_info.action_type));
    contract_obj.add_escaped_string_if_non_empty(
      Response::OpenRtb::BuzSapeNroa::CONTRACT_ID,
      String::SubString(ext_contract_info.contract_info.ord_contract_id));
    contract_obj.add_escaped_string_if_non_empty(
      Response::OpenRtb::BuzSapeNroa::CONTRACT_ADOID,
      String::SubString(ext_contract_info.contract_info.ord_ado_id));
    contract_obj.add_boolean(
      Response::OpenRtb::BuzSapeNroa::CONTRACT_VATINCLUDED,
      ext_contract_info.contract_info.vat_included);
    contract_obj.add_escaped_string_if_non_empty(
      Response::OpenRtb::BuzSapeNroa::CONTRACT_PARENTCONTRACTID,
      String::SubString(ext_contract_info.parent_contract_id));
  }

  void
  OpenRtbBidRequestState::fill_buzsape_nroa_(
    AdServer::Commons::JsonObject& nroa_obj,
    const RequestInfo& /*request_info*/,
    const AdServer::Bidding::CampaignManager::AdSlotResult& ad_slot_result)
    noexcept
  {
    nroa_obj.add_escaped_string_if_non_empty(
      Response::OpenRtb::NROA_ERID, String::SubString(ad_slot_result.erid));
    nroa_obj.add_number(
      Response::OpenRtb::BuzSapeNroa::HASNROAMARKUP, 1);

    if(ad_slot_result.contracts.size() > 0)
    {
      const AdServer::Bidding::CampaignManager::ExtContractInfo& initial_contract = ad_slot_result.contracts[0];

      // contractor
      {
        AdServer::Commons::JsonObject contractor_obj(nroa_obj.add_object(Response::OpenRtb::BuzSapeNroa::CONTRACTOR));
        contractor_obj.add_escaped_string_if_non_empty(
          Response::OpenRtb::BuzSapeNroa::CONTRACTOR_INN,
          String::SubString(initial_contract.contract_info.contractor_id));
        contractor_obj.add_escaped_string_if_non_empty(
          Response::OpenRtb::BuzSapeNroa::CONTRACTOR_NAME,
          String::SubString(initial_contract.contract_info.contractor_name));
        contractor_obj.add_escaped_string_if_non_empty(
          Response::OpenRtb::BuzSapeNroa::CONTRACTOR_LEGALFORM,
          String::SubString(initial_contract.contract_info.contractor_legal_form));
      }

      // client
      {
        AdServer::Commons::JsonObject client_obj(nroa_obj.add_object(Response::OpenRtb::BuzSapeNroa::CLIENT));
        client_obj.add_escaped_string_if_non_empty(
          Response::OpenRtb::BuzSapeNroa::CLIENT_INN,
          String::SubString(initial_contract.contract_info.client_id));
        client_obj.add_escaped_string_if_non_empty(
          Response::OpenRtb::BuzSapeNroa::CLIENT_NAME,
          String::SubString(initial_contract.contract_info.client_name));
        client_obj.add_escaped_string_if_non_empty(
          Response::OpenRtb::BuzSapeNroa::CLIENT_LEGALFORM,
          String::SubString(initial_contract.contract_info.client_legal_form));
      }

      {
        // initial_contract
        AdServer::Commons::JsonObject initial_contract_obj(nroa_obj.add_object(Response::OpenRtb::BuzSapeNroa::INITIALCONTRACT));
        fill_buzsape_nroa_contract_(initial_contract_obj, initial_contract);
      }

      // fill parent contracts
      if(ad_slot_result.contracts.size() > 1)
      {
        AdServer::Commons::JsonObject parent_contracts_array(nroa_obj.add_array(Response::OpenRtb::BuzSapeNroa::PARENTCONTRACTS));
        for(std::size_t contract_i = 1; contract_i < ad_slot_result.contracts.size(); ++contract_i)
        {
          AdServer::Commons::JsonObject contract_obj(parent_contracts_array.add_object());
          fill_buzsape_nroa_contract_(contract_obj, ad_slot_result.contracts[contract_i]);
        }
      }
    }
  }

  void
  OpenRtbBidRequestState::fill_openrtb_response_(
    std::ostream& response_ostr,
    const RequestInfo& request_info,
    const JsonProcessingContext& context,
    const AdServer::Bidding::CampaignManager::RequestCreativeResult& campaign_match_result,
    bool fill_yandex_attributes)
    noexcept
  {
    static const char* FUN = "OpenRtbBidRequestState::fill_openrtb_response_()";

    try
    {
      std::string response;
      {
        AdServer::Commons::JsonFormatter root_json(response);
      root_json.add_escaped_string(
        Response::OpenRtb::ID,
        to_sub_string(context.request_id));
      if(fill_yandex_attributes)
      {
        root_json.add_number(Response::Yandex::UNITS, 2);
      }

      {
        char bid_id[20];
        size_t len = String::StringManip::int_to_str(
          request_info.random, bid_id, sizeof(bid_id));
        root_json.add_string(Response::OpenRtb::BIDID, String::SubString(bid_id, len));
      }

      if (request_info.ipw_extension)
      {
        AdServer::Commons::JsonObject ext_obj(root_json.add_object(Response::OpenRtb::EXT));
        ext_obj.add_string(Response::OpenRtb::PROTOCOL, String::SubString("4.0"));
      }

      std::string pub_currency_code;
      {
        AdServer::Commons::JsonObject seatbid_array(root_json.add_array(Response::OpenRtb::SEATBID));
        AdServer::Commons::JsonObject seatbid_obj(seatbid_array.add_object());
        if(!request_info.seat.empty())
        {
          seatbid_obj.add_string(Response::OpenRtb::SEAT, request_info.seat);
        }
        AdServer::Commons::JsonObject bid_array(seatbid_obj.add_array(Response::OpenRtb::BID));

        if(campaign_match_result.ad_slots.size() != context.ad_slots.size())
        {
          Stream::Error ostr;
          ostr << FUN << ": Error on writing open rtb response(assert): "
            "campaign_match_result.ad_slots.size() != context.ad_slots.size()";
          bid_frontend_->logger()->log(ostr.str(), Logging::Logger::EMERGENCY, Aspect::BIDDING_FRONTEND);

          assert(false);
        }

        JsonAdSlotProcessingContextList::const_iterator slot_it =
          context.ad_slots.begin();

        for(std::size_t ad_slot_i = 0;
            ad_slot_i < campaign_match_result.ad_slots.size();
            ++ad_slot_i, ++slot_it)
        {
          const AdServer::Bidding::CampaignManager::
            AdSlotResult& ad_slot_result = campaign_match_result.ad_slots[ad_slot_i];

          if(pub_currency_code.empty())
          {
            pub_currency_code = ad_slot_result.pub_currency_code;
            String::AsciiStringManip::to_upper(pub_currency_code);
          }

          if(ad_slot_result.selected_creatives.size() > 0)
          {
            // campaigns selected
            CampaignSvcs::RevenueDecimal sum_pub_ecpm = CampaignSvcs::RevenueDecimal::ZERO;
            for(std::size_t creative_i = 0;
                creative_i < ad_slot_result.selected_creatives.size();
                ++creative_i)
            {
              sum_pub_ecpm += GrpcAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
                ad_slot_result.selected_creatives[creative_i].pub_ecpm);
            }

            bid_frontend_->limit_max_cpm_(
              sum_pub_ecpm, request_info.publisher_account_ids);

            // result price in RUB/1000, ecpm is in 0.01/1000
            CampaignSvcs::RevenueDecimal openrtb_price = CampaignSvcs::RevenueDecimal::div(
              sum_pub_ecpm,
              CampaignSvcs::RevenueDecimal(false, 100, 0));

            std::string escaped_creative_url;
            std::string escaped_creative_body;

            if(!ad_slot_result.creative_url.empty())
            {
              escaped_creative_url = String::StringManip::json_escape(
                String::SubString(ad_slot_result.creative_url));
            }

            if(!slot_it->native && !ad_slot_result.creative_body.empty())
            {
              escaped_creative_body = String::StringManip::json_escape(
                String::SubString(ad_slot_result.creative_body));
            }

            AdServer::Commons::JsonObject bid_object(bid_array.add_object());
            bid_object.add_as_string(Response::OpenRtb::ID, ad_slot_i);
            if (!slot_it->deal_id.empty())
            {
              bid_object.add_as_string(Response::OpenRtb::DEAL_ID, slot_it->deal_id);
            }

            bid_object.add_escaped_string(
              Response::OpenRtb::IMPID,
              to_sub_string(slot_it->id));
            bid_object.add_number(Response::OpenRtb::PRICE, openrtb_price);

            if (bid_frontend_->request_info_filler()->fill_adid(
                  request_info.source_id))
            {
              bid_object.add_as_string(Response::OpenRtb::ADID, ad_slot_result.selected_creatives[0].creative_id);
            }

            bid_object.add_as_string(
              Response::OpenRtb::CRID,
              ad_slot_result.selected_creatives[0].creative_version_id);

            {
              AdServer::Commons::JsonObject adomain_obj(bid_object.add_array(Response::OpenRtb::ADOMAIN));

              // standard OpenRTB, Allyes, OpenX
              for(std::size_t creative_i = 0;
                  creative_i < ad_slot_result.selected_creatives.size();
                  ++creative_i)
              {
                try
                {
                  HTTP::BrowserAddress adomain(String::SubString(
                    ad_slot_result.selected_creatives[creative_i].destination_url));

                  String::SubString host = adomain.host();

                  if(request_info.truncate_domain && WWW_PREFIX.start(host))
                  {
                    host = host.substr(WWW_PREFIX.str.size());
                  }

                  if (!host.empty())
                  {
                    adomain_obj.add_escaped_string(host);
                  }
                }
                catch (const HTTP::URLAddress::InvalidURL& ex)
                {
                  Stream::Error ostr;
                  ostr << FUN << ": adomain extract failed from '" <<
                    ad_slot_result.selected_creatives[creative_i].destination_url <<
                    "', " << ex.what();

                  bid_frontend_->logger()->log(
                    ostr.str(),
                    Logging::Logger::ERROR,
                    Aspect::BIDDING_FRONTEND,
                    "ADS-IMPL-7604");
                }
              }
            }

            bool need_ipw_extension = request_info.ipw_extension;
            bool video_url_in_ext = false;
            bool native_in_ext = false;

            {
              bool notice_enabled = false;
              auto notice_instantiate_type = request_info.notice_instantiate_type;
              if(slot_it->video)
              {
                notice_instantiate_type = request_info.vast_notice_instantiate_type;
              }

              // ADSC-10918 Native ads
              if (slot_it->native)
              {
                notice_instantiate_type = request_info.native_notice_instantiate_type;
                notice_enabled = true; // enabled if notice_instantiate_type allow

                if (request_info.native_ads_instantiate_type == SourceTraits::NAIT_ADM ||
                  request_info.native_ads_instantiate_type == SourceTraits::NAIT_ADM_1_2 ||
                  request_info.native_ads_instantiate_type == SourceTraits::NAIT_ADM_NATIVE ||
                  request_info.native_ads_instantiate_type == SourceTraits::NAIT_ADM_NATIVE_1_2 ||
                  request_info.native_ads_instantiate_type == SourceTraits::NAIT_ESCAPE_SLASH_ADM)
                {
                  std::string native_response;

                  {
                    AdServer::Commons::JsonFormatter native_json(native_response);
                    fill_native_response_(
                      &native_json,
                      *slot_it->native,
                      ad_slot_result,
                      true,
                      request_info.native_ads_instantiate_type == SourceTraits::NAIT_ADM_NATIVE ||
                        request_info.native_ads_instantiate_type == SourceTraits::NAIT_ADM_NATIVE_1_2,
                      request_info.native_ads_instantiate_type
                      );
                  }

                  escaped_creative_body = String::StringManip::json_escape(
                    std::move(native_response));

                  if(request_info.native_ads_instantiate_type == SourceTraits::NAIT_ESCAPE_SLASH_ADM)
                  {
                    std::string escape_slash_escaped_creative_body;
                    String::AsciiStringManip::flatten(
                      escape_slash_escaped_creative_body,
                      escaped_creative_body,
                      String::SubString("\\\\", 2),
                      SLASH);
                    escaped_creative_body.swap(escape_slash_escaped_creative_body);
                  }

                  bid_object.add_string(Response::OpenRtb::ADM, escaped_creative_body);
                }
                else if (request_info.native_ads_instantiate_type == SourceTraits::NAIT_EXT)
                {
                  native_in_ext = true;
                  notice_enabled = true;
                }
                else if(request_info.native_ads_instantiate_type == SourceTraits::NAIT_NATIVE_AS_ELEMENT_1_2)
                {
                  fill_native_response_(
                    &bid_object,
                    *slot_it->native,
                    ad_slot_result,
                    true,
                    true, // add native root
                    request_info.native_ads_instantiate_type
                    );
                }
              }
              // banners
              else if(!escaped_creative_body.empty())
              {
                bid_object.add_string(Response::OpenRtb::ADM, escaped_creative_body);
                notice_enabled = true;
              }
              else if(!escaped_creative_url.empty())
              {
                if(request_info.ad_instantiate_type == AdServer::CampaignSvcs::AIT_VIDEO_URL)
                {
                  video_url_in_ext = true;
                  notice_enabled = true;
                }
                else if(request_info.ad_instantiate_type ==
                    AdServer::CampaignSvcs::AIT_VIDEO_URL_IN_BODY ||
                  request_info.ad_instantiate_type ==
                    AdServer::CampaignSvcs::AIT_URL_IN_BODY)
                {
                  bid_object.add_string(Response::OpenRtb::ADM, escaped_creative_url);
                  notice_enabled = true;
                }
                else
                {
                  bid_object.add_string(Response::OpenRtb::NURL, escaped_creative_url);
                }
              }

              if(notice_enabled)
              {
                if(!request_info.notice_url.empty())
                {
                  add_response_notice_(bid_object, request_info.notice_url, notice_instantiate_type);
                }
                else if(!ad_slot_result.notice_url.empty())
                {
                  add_response_notice_(
                    bid_object,
                    String::SubString(ad_slot_result.notice_url),
                    notice_instantiate_type);
                }
              }

              // FIXME: can not find 'bid[attr]' in OpenRTB 2.2/2.3 spec
              print_int_category_seq(
                bid_object,
                Response::OpenRtb::ATTR,
                ad_slot_result.external_visual_categories);
            }

            if(!slot_it->banners.empty())
            {
              auto banner_by_size_it = slot_it->size_banner.find(
                std::string_view(
                  ad_slot_result.tag_size.data(),
                  ad_slot_result.tag_size.size()));

              if(banner_by_size_it != slot_it->size_banner.end())
              {
                const JsonAdSlotProcessingContext::BannerFormat& use_banner_format =
                  *banner_by_size_it->second.banner_format;
                bid_object.add_number(
                  Response::OpenRtb::WIDTH,
                  use_banner_format.width);
                bid_object.add_number(
                  Response::OpenRtb::HEIGHT,
                  use_banner_format.height);
              }
            }

            {
              auto banner_by_size_it = slot_it->size_banner.find(
                std::string_view(
                  ad_slot_result.tag_size.data(),
                  ad_slot_result.tag_size.size()));

              const bool fill_overlay_ext = (!slot_it->banners.empty() && (banner_by_size_it != slot_it->size_banner.end()));
              const bool fill_nroa = (!ad_slot_result.erid.empty() || ad_slot_result.contracts.size() > 0);

              if(fill_overlay_ext || fill_nroa || need_ipw_extension || video_url_in_ext || native_in_ext)
              {
                AdServer::Commons::JsonObject ext_obj(bid_object.add_object(Response::OpenRtb::BID_EXT));

                if(native_in_ext)
                {
                  fill_native_response_(
                    &ext_obj,
                    *slot_it->native,
                    ad_slot_result,
                    true,
                    true, // add native root
                    request_info.native_ads_instantiate_type
                    );
                }

                if(video_url_in_ext)
                {
                  ext_obj.add_string(Response::OpenRtb::VAST_URL, escaped_creative_url);

                  if(ad_slot_result.ext_tokens.size() > 0)
                  {
                    for(std::size_t token_i = 0;
                      token_i < ad_slot_result.ext_tokens.size(); ++token_i)
                    {
                      if(!ad_slot_result.ext_tokens[token_i].name.empty())
                      {
                        std::string escaped_name = String::StringManip::json_escape(
                          String::SubString(ad_slot_result.ext_tokens[token_i].name));

                        ext_obj.add_escaped_string(escaped_name,
                          String::SubString(ad_slot_result.ext_tokens[token_i].value));
                      }
                    }
                  }
                }

                if(need_ipw_extension)
                {
                  ext_obj.add_escaped_string(
                    Response::OpenRtb::ADVERTISER_NAME,
                    String::SubString(ad_slot_result.selected_creatives[0].advertiser_name));

                  need_ipw_extension = false;
                }

                if(fill_overlay_ext)
                {
                  const JsonAdSlotProcessingContext::Banner& use_banner =
                    *banner_by_size_it->second.banner;
                  const JsonAdSlotProcessingContext::BannerFormat& use_banner_format =
                    *banner_by_size_it->second.banner_format;

                  bool add_ext_width_height = (
                    ad_slot_result.selected_creatives.size() == 1 &&
                    use_banner_format.ext_type == "20");

                  if(add_ext_width_height ||
                    request_info.request_type == AdServer::CampaignSvcs::AR_OPENX ||
                    !ad_slot_result.erid.empty() ||
                    ad_slot_result.contracts.size() > 0)
                  {
                    if(add_ext_width_height)
                    {
                      ext_obj.add_as_string(Response::OpenRtb::WIDTH, ad_slot_result.overlay_width);
                      ext_obj.add_as_string(Response::OpenRtb::HEIGHT, ad_slot_result.overlay_height);
                    }

                    if(request_info.request_type == AdServer::CampaignSvcs::AR_OPENX)
                    {
                      if(!use_banner.matching_ad.empty())
                      {
                        ext_obj.add_number(
                          Response::OpenX::MATCHING_AD_ID,
                          use_banner.matching_ad);
                      }

                      print_int_category_seq(
                        ext_obj,
                        Response::OpenX::AD_OX_CATS,
                        ad_slot_result.external_content_categories);
                    }
                  }
                } // fill_overlay_ext

                if(fill_nroa)
                {
                  AdServer::Commons::JsonObject nroa_obj(ext_obj.add_object(Response::OpenRtb::BID_EXT_NROA));

                  if(request_info.erid_return_type == SourceTraits::ERIDRT_ARRAY)
                  {
                    AdServer::Commons::JsonObject array(nroa_obj.add_array(Response::OpenRtb::NROA_ERID));
                    if(!ad_slot_result.erid.empty())
                    {
                      array.add_escaped_string(String::SubString(ad_slot_result.erid));
                    }
                  }
                  else if(request_info.erid_return_type == SourceTraits::ERIDRT_EXT0)
                  {
                    fill_ext0_nroa_(nroa_obj, request_info, ad_slot_result);
                  }
                  else if(request_info.erid_return_type == SourceTraits::ERIDRT_EXT_BUZSAPE)
                  {
                    fill_buzsape_nroa_(nroa_obj, request_info, ad_slot_result);
                  }
                  else // SourceTraits::ERIDRT_SINGLE
                  {
                    nroa_obj.add_escaped_string_if_non_empty(
                      Response::OpenRtb::NROA_ERID, String::SubString(ad_slot_result.erid));
                  }
                } // fill_nroa
              }
            }

            if(!ad_slot_result.iurl.empty())
            {
              bid_object.add_escaped_string(
                Response::OpenRtb::IURL,
                String::SubString(ad_slot_result.iurl));
            }

            bid_object.add_as_string(
              Response::OpenRtb::CID,
              ad_slot_result.selected_creatives[0].campaign_group_id);
          } // if(ad_slot_result.selected_creatives.size() > 0)
        } // for(std::size_t ad_slot_i = 0, ...
      }
      root_json.add_string(Response::OpenRtb::CUR, pub_currency_code);
      }
      response_ostr << response;
    }
    catch(const AdServer::Commons::JsonObject::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Error on formatting Json response: '" << ex.what() << "'";
      bid_frontend_->logger()->log(ostr.str(), Logging::Logger::EMERGENCY, Aspect::BIDDING_FRONTEND);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Error on writing open rtb response: '" << ex.what() << "'";
      bid_frontend_->logger()->log(ostr.str(), Logging::Logger::EMERGENCY, Aspect::BIDDING_FRONTEND);
    }
  }

  void
  OpenRtbBidRequestState::fill_yandex_response_(
    std::ostream& response_ostr,
    const RequestInfo& request_info,
    const JsonProcessingContext& context,
    const AdServer::Bidding::CampaignManager::
      RequestCreativeResult& campaign_match_result)
    noexcept
  {
    static const char* FUN = "OpenRtbBidRequestState::fill_yandex_response_()";

    try
    {
      std::string escaped_request_id =
        String::StringManip::json_escape(to_sub_string(context.request_id));

      std::string response;
      {
        AdServer::Commons::JsonFormatter root_json(response);
      root_json.add(Response::Yandex::ID, YandexIdFormatter(escaped_request_id));
      root_json.add_number(Response::Yandex::UNITS, 2);

      {
        char bid_id[20];
        size_t len = String::StringManip::int_to_str(
          request_info.random, bid_id, sizeof(bid_id));
        root_json.add_string(Response::OpenRtb::BIDID, String::SubString(bid_id, len));
      }

      bool some_campaign_selected = false;

      {
        for(std::size_t ad_slot_i = 0;
          ad_slot_i < campaign_match_result.ad_slots.size();
          ++ad_slot_i)
        {
          some_campaign_selected |= (
            campaign_match_result.ad_slots[ad_slot_i].selected_creatives.size() > 0);
        }
      }

      {
        if(request_info.user_id.has_value() && !request_info.user_id->is_null())
        {
          root_json.add_string(
            Response::Yandex::SETUSERDATA,
            AdServer::UserIdController::ssp_uuid_string(
              bid_frontend_->user_id_controller()->ssp_uuid(
                *request_info.user_id, request_info.source_id)));
        }
        else if(some_campaign_selected && request_info.external_user_id.empty())
        {
          std::string tmp_ssp_user_id = AdServer::UserIdController::ssp_uuid_string(
            bid_frontend_->user_id_controller()->ssp_uuid(
              AdServer::Commons::UserId::create_random_based(),
              request_info.source_id));
          tmp_ssp_user_id[0] = '~';
          root_json.add_string(
            Response::Yandex::SETUSERDATA,
            tmp_ssp_user_id);
        }
      }

      std::string pub_currency_code;

      {
        AdServer::Commons::JsonObject bidset(root_json.add_array(Response::Yandex::BIDSET));
        AdServer::Commons::JsonObject bidsetobject(bidset.add_object());
        AdServer::Commons::JsonObject bidarray(bidsetobject.add_array(Response::Yandex::BID));

        if(campaign_match_result.ad_slots.size() != context.ad_slots.size())
        {
          Stream::Error ostr;
          ostr << FUN << ": Error on writing open rtb response(assert): "
            "campaign_match_result.ad_slots.size() != context.ad_slots.size()";
          bid_frontend_->logger()->log(ostr.str(), Logging::Logger::EMERGENCY, Aspect::BIDDING_FRONTEND);

          assert(false);
        }

        JsonAdSlotProcessingContextList::const_iterator slot_it =
          context.ad_slots.begin();

        for(std::size_t ad_slot_i = 0;
            ad_slot_i < campaign_match_result.ad_slots.size();
            ++ad_slot_i, ++slot_it)
        {
          const AdServer::Bidding::CampaignManager::
            AdSlotResult& ad_slot_result = campaign_match_result.ad_slots[ad_slot_i];

          if(pub_currency_code.empty())
          {
            pub_currency_code = ad_slot_result.pub_currency_code;
            String::AsciiStringManip::to_upper(pub_currency_code);
          }

          if(ad_slot_result.selected_creatives.size() > 0)
          {
            // campaigns selected
            CampaignSvcs::RevenueDecimal sum_pub_ecpm = CampaignSvcs::RevenueDecimal::ZERO;
            for(std::size_t creative_i = 0;
                creative_i < ad_slot_result.selected_creatives.size();
                ++creative_i)
            {
              sum_pub_ecpm += GrpcAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
                ad_slot_result.selected_creatives[creative_i].pub_ecpm);
            }

            bid_frontend_->limit_max_cpm_(
              sum_pub_ecpm, request_info.publisher_account_ids);

            // result price in RUB/1000000, ecpm is in 0.01/1000
            CampaignSvcs::RevenueDecimal openrtb_price = CampaignSvcs::RevenueDecimal::mul(
              sum_pub_ecpm,
              CampaignSvcs::RevenueDecimal(false, 10, 0),
              Generics::DMR_ROUND);

            std::string escaped_creative_url;
            std::string escaped_click_params;
            if(!ad_slot_result.creative_url.empty())
            {
              escaped_creative_url = String::StringManip::json_escape(
                String::SubString(ad_slot_result.creative_url));
            }

            if(!ad_slot_result.click_params.empty())
            {
              std::string base64_encoded_click_params;
              String::SubString click_params(ad_slot_result.click_params);
              String::StringManip::base64mod_encode(
                base64_encoded_click_params,
                click_params.data(),
                click_params.size());
              escaped_click_params = String::StringManip::json_escape(
                std::move(base64_encoded_click_params));
            }

            AdServer::Commons::JsonObject bid_object(bidarray.add_object());

            bid_object.add(Response::Yandex::ID,
              YandexIdFormatter(to_sub_string(slot_it->id)));
            bid_object.add_number(Response::Yandex::PRICE,
              openrtb_price.integer<unsigned long>());
            // Always add adid for yandex (don't check source_id)
            bid_object.add_number(Response::Yandex::ADID,
              ad_slot_result.selected_creatives[0].creative_id);

            // fill view_notices, nurl, dsp_params
            if(ad_slot_result.track_pixel_urls.size() > 0)
            {
              AdServer::Commons::JsonObject track_pixel_urls(
                bid_object.add_array(Response::Yandex::VIEW_NOTICES));

              for(std::size_t i = 0;
                i < ad_slot_result.track_pixel_urls.size(); ++i)
              {
                track_pixel_urls.add_escaped_string(
                  String::SubString(ad_slot_result.track_pixel_urls[i]));
              }
            }

            auto notice_instantiate_type = request_info.notice_instantiate_type;
            if(slot_it->video)
            {
              notice_instantiate_type = request_info.vast_notice_instantiate_type;
            }

            if(!request_info.notice_url.empty())
            {
              add_response_notice_(bid_object, request_info.notice_url, notice_instantiate_type);
            }
            else if(!ad_slot_result.notice_url.empty())
            {
              add_response_notice_(
                bid_object,
                String::SubString(ad_slot_result.notice_url),
                notice_instantiate_type);
            }

            {
              AdServer::Commons::JsonObject banner(bid_object.add_object(Response::Yandex::BANNER));

              auto banner_by_size_it = slot_it->size_banner.find(
                std::string_view(
                  ad_slot_result.tag_size.data(),
                  ad_slot_result.tag_size.size()));

              if(banner_by_size_it != slot_it->size_banner.end())
              {
                banner.add_number(Response::Yandex::WIDTH, banner_by_size_it->second.banner_format->width);
                banner.add_number(Response::Yandex::HEIGHT, banner_by_size_it->second.banner_format->height);
              }
            }

            {
              AdServer::Commons::JsonObject dsp_params(bid_object.add_object(Response::Yandex::DSP_PARAMS));
              for(unsigned int url_param_i = 0;
                url_param_i < sizeof(Response::Yandex::URL_PARAM_ALL) / sizeof(Response::Yandex::URL_PARAM_ALL[0]);
                ++url_param_i)
              {
                dsp_params.add_string(
                  Response::Yandex::URL_PARAM_ALL[url_param_i], escaped_click_params);
              }

              if(!ad_slot_result.yandex_track_params.empty())
              {
                std::string escaped_track_params;
                String::StringManip::base64mod_encode(
                  escaped_track_params,
                  ad_slot_result.yandex_track_params.data(),
                  ad_slot_result.yandex_track_params.size());
                dsp_params.add_escaped_string(
                  Response::Yandex::URL_PARAM17,
                  escaped_track_params);
                dsp_params.add_escaped_string(
                  Response::Yandex::URL_PARAM18,
                  escaped_track_params);
              }
            }

            // fill adm, token, properties
            for(std::size_t token_i = 0; token_i < ad_slot_result.tokens.size(); ++token_i)
            {
              std::string escaped_name = String::StringManip::json_escape(
                String::SubString(ad_slot_result.tokens[token_i].name));

              bid_object.add_escaped_string(escaped_name,
                String::SubString(ad_slot_result.tokens[token_i].value));
            }
            //} //
          } // if(ad_slot_result.selected_creatives.size() > 0)
        } // for(std::size_t ad_slot_i = 0, ...
      } // close bidset oject and array
      root_json.add_string(Response::Yandex::CUR, pub_currency_code);
      }
      response_ostr << response;
    }
    catch(const AdServer::Commons::JsonObject::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << " Error on formatting Json response: '" << ex.what() << "'";
      bid_frontend_->logger()->log(ostr.str(), Logging::Logger::EMERGENCY, Aspect::BIDDING_FRONTEND);
    }
  }

  // ADSC-10918 Native ads
  void
  OpenRtbBidRequestState::fill_native_response_(
    AdServer::Commons::JsonObject* root_json,
    const JsonAdSlotProcessingContext::Native& native_context,
    const AdServer::Bidding::CampaignManager::AdSlotResult& ad_slot_result,
    bool need_escape,
    bool add_root_native,
    SourceTraits::NativeAdsInstantiateType instantiate_type)
  {
    static const char* FUN = "OpenRtbBidRequestState::fill_native_response_()";

    try
    {
      std::unique_ptr<AdServer::Commons::JsonObject> native_obj_holder;

      if(add_root_native)
      {
        native_obj_holder.reset(
          new AdServer::Commons::JsonObject(root_json->add_object(Response::OpenRtb::NATIVE)));
      }

      AdServer::Commons::JsonObject& native_obj =
        add_root_native ? *native_obj_holder : *root_json;

      {
        if(ad_slot_result.selected_creatives.size() == 0)
        {
          Stream::Error ostr;
          ostr << FUN << ": Error on writing open rtb response(assert): "
            "ad_slot_result.selected_creatives.size() == 0";
          bid_frontend_->logger()->log(ostr.str(), Logging::Logger::EMERGENCY, Aspect::BIDDING_FRONTEND);

          assert(false);
        }

        if(instantiate_type == SourceTraits::NAIT_NATIVE_AS_ELEMENT_1_2 ||
          instantiate_type == SourceTraits::NAIT_ADM_1_2 ||
          instantiate_type == SourceTraits::NAIT_ADM_NATIVE_1_2)
        {
          native_obj.add_string(
            Response::OpenRtb::NATIVE_VER,
            Response::OpenRtb::NATIVE_VER_1_2);
        }
        else
        {
          native_obj.add_string(
            Response::OpenRtb::NATIVE_VER,
            Response::OpenRtb::NATIVE_VER_1_0);
        }
        AdServer::Commons::JsonObject link_obj(
          native_obj.add_object(Response::OpenRtb::NATIVE_LINK));
        link_obj.add_opt_escaped_string(
          Response::OpenRtb::NATIVE_LINK_URL,
          String::SubString(
            ad_slot_result.selected_creatives[0].click_url),
          need_escape);
      }

      if (native_context.video_assets.empty())
      {
        if(!ad_slot_result.track_html_body.empty())
        {
          native_obj.add_opt_escaped_string(
            Response::OpenRtb::NATIVE_JS_TRACKER,
            String::SubString(ad_slot_result.track_html_body),
            need_escape);
        }

        if(ad_slot_result.track_pixel_urls.size() > 0)
        {
          if(instantiate_type == SourceTraits::NAIT_NATIVE_AS_ELEMENT_1_2 ||
            instantiate_type == SourceTraits::NAIT_ADM_1_2 ||
            instantiate_type == SourceTraits::NAIT_ADM_NATIVE_1_2)
          {
            AdServer::Commons::JsonObject event_trackers_obj(
              native_obj.add_array(Response::OpenRtb::NATIVE_EVENT_TRACKERS));

            for(std::size_t i = 0;
              i < ad_slot_result.track_pixel_urls.size(); ++i)
            {
              AdServer::Commons::JsonObject event_obj(event_trackers_obj.add_object());
              event_obj.add_number(Response::OpenRtb::EVENT_EVENT, 1);
              event_obj.add_number(Response::OpenRtb::EVENT_METHOD, 1);
              event_obj.add_string(
                Response::OpenRtb::EVENT_URL,
                String::SubString(ad_slot_result.track_pixel_urls[i]));
            }
          }
          else
          {
            AdServer::Commons::JsonObject imp_trackers_obj(
              native_obj.add_array(Response::OpenRtb::NATIVE_IMP_TRACKERS));

            for(std::size_t i = 0;
              i < ad_slot_result.track_pixel_urls.size(); ++i)
            {
              imp_trackers_obj.add_opt_escaped_string(
                String::SubString(ad_slot_result.track_pixel_urls[i]),
                need_escape);
            }
          }
        }
      }

      AdServer::Commons::JsonObject assets_obj(
        native_obj.add_array(Response::OpenRtb::NATIVE_ASSETS));

      // Data assets
      if(native_context.data_assets.size() != ad_slot_result.native_data_tokens.size())
      {
        Stream::Error ostr;
        ostr << FUN << ": Error on writing open rtb response(assert): "
          "native_context.data_assets.size() != ad_slot_result.native_data_tokens.size()";
        throw Exception(ostr);
      }

      size_t data_i = 0;
      for (auto data_it = native_context.data_assets.begin();
        data_it != native_context.data_assets.end(); ++data_it, ++data_i)
      {
        const AdServer::Bidding::CampaignManager::ResultTokenInfo& token =
          ad_slot_result.native_data_tokens[data_i];
        if(!token.value.empty())
        {
          AdServer::Commons::JsonObject asset(
            assets_obj.add_object());
          asset.add_number(
            Response::OpenRtb::NATIVE_ASSET_ID, data_it->id);
          bool is_title =
            data_it->data_type == JsonAdSlotProcessingContext::Native::NDTE_TITLE;

          size_t token_len = token.value.size();
          size_t data_len = static_cast<size_t>(data_it->len);

          String::SubString res_text;
          String::StringManip::utf8_substr(
            String::SubString(token.value),
            data_len && token_len > data_len ? data_len : token_len,
            res_text);

          AdServer::Commons::JsonObject data_asset(
            asset.add_object(
              is_title ? Response::OpenRtb::NATIVE_ASSET_TITLE :
                Response::OpenRtb::NATIVE_ASSET_DATA));
          data_asset.add_opt_escaped_string(
            is_title ? Response::OpenRtb::NATIVE_ASSET_TITLE_TEXT :
              Response::OpenRtb::NATIVE_ASSET_DATA_VALUE,
            res_text,
            need_escape);
        }
      }

      // Image assets
      if(ad_slot_result.native_image_tokens.size() > 0)
      {
        size_t image_i = 0;
        for (auto image_it = native_context.image_assets.begin();
           image_it != native_context.image_assets.end(); ++image_it, ++image_i)
        {
          const AdServer::Bidding::CampaignManager::ResultTokenImageInfo& token =
            ad_slot_result.native_image_tokens[image_i];
          if(!token.value.empty())
          {
            AdServer::Commons::JsonObject asset(assets_obj.add_object());
            asset.add_number(
              Response::OpenRtb::NATIVE_ASSET_ID, image_it->id);
            if (image_it->image_type ==
              JsonAdSlotProcessingContext::Native::NITE_MAIN)
            {
              asset.add_number(
                Response::OpenRtb::NATIVE_ASSET_REQUIRED, 1);
            }

            AdServer::Commons::JsonObject image_asset(
              asset.add_object(Response::OpenRtb::NATIVE_ASSET_IMG));

            image_asset.add_number(
              Response::OpenRtb::NATIVE_ASSET_REQUIRED, 0);

            image_asset.add_opt_escaped_string(
              Response::OpenRtb::NATIVE_ASSET_IMG_URL,
              String::SubString(token.value),
              need_escape);

            image_asset.add_number(
              Response::OpenRtb::NATIVE_ASSET_IMG_W,
              token.width);

            image_asset.add_number(
              Response::OpenRtb::NATIVE_ASSET_IMG_H,
              token.height);
          }
        }
      }

      // Video asset
      if(!ad_slot_result.creative_body.empty())
      {
        if (!native_context.video_assets.empty())
        {
          const JsonAdSlotProcessingContext::Native::Video& video =
            native_context.video_assets[0];
          AdServer::Commons::JsonObject asset(assets_obj.add_object());
          asset.add_number(
            Response::OpenRtb::NATIVE_ASSET_ID, video.id);
          asset.add_number(
            Response::OpenRtb::NATIVE_ASSET_REQUIRED, 1);

          AdServer::Commons::JsonObject video_asset(
            asset.add_object(Response::OpenRtb::NATIVE_ASSET_VIDEO));

          video_asset.add_opt_escaped_string(
            Response::OpenRtb::NATIVE_ASSET_VIDEO_TAG,
            String::SubString(ad_slot_result.creative_body),
            need_escape);
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": error on writing native response: " << ex.what();
      bid_frontend_->logger()->log(
        ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::BIDDING_FRONTEND);
      throw;
    }
  }

}
