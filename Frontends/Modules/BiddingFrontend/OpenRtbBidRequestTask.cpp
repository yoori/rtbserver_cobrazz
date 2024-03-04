#include <zlib.h>

#include "OpenRtbBidRequestTask.hpp"

namespace AdServer
{
namespace Bidding
{
  namespace
  {
    const String::AsciiStringManip::Caseless WWW_PREFIX("www.");
    const String::AsciiStringManip::CharCategory SLASH("\\");

    namespace Aspect
    {
      const char BIDDING_FRONTEND[] = "BiddingFrontend";
    }

    namespace Response
    {
      namespace Header
      {
        const String::SubString CONTENT_TYPE("Content-Type");
        const String::SubString OPENRTB_VERSION("x-openrtb-version");
        const String::SubString OPENRTB_VERSION_VALUE("2.3");

        const String::AsciiStringManip::Caseless CONTENT_ENCODING("content-encoding");
        const String::AsciiStringManip::Caseless CONTENT_ENCODING2("content_encoding");
        const String::AsciiStringManip::Caseless GZIP("gzip");
      }

      namespace Type
      {
        const String::SubString TEXT_HTML("text/html");
        const String::SubString JSON("application/json");
        const String::SubString OCTET_STREAM("application/octet-stream");
      }

      namespace OpenRtb
      {
        const String::SubString ID("id");
        const String::SubString DEAL_ID("dealid");
        const String::SubString BID("bid");
        const String::SubString BID_EXT("ext");
        const String::SubString BIDID("bidid");
        const String::SubString CUR("cur");
        const String::SubString EXT("ext");
        const String::SubString PROTOCOL("protocol");
        const String::SubString SEATBID("seatbid");
        const String::SubString ADID("adid");
        const String::SubString PRICE("price");
        const String::SubString IMPID("impid");
        const String::SubString CRID("crid");
        const String::SubString ADOMAIN("adomain");
        const String::SubString ADVERTISER_NAME("advertiser_name");
        const String::SubString VAST_URL("vast_url");
        const String::SubString ADM("adm");
        const String::SubString NURL("nurl");
        const String::SubString BURL("burl");
        const String::SubString CAT("cat");
        const String::SubString IURL("iurl");

        const String::SubString WIDTH("w");
        const String::SubString HEIGHT("h");
        const String::SubString CID("cid");
        const String::SubString SEAT("seat");
        const String::SubString ATTR("attr");

        // ADSC-10918 Native ads
        const String::SubString NATIVE("native");
        const String::SubString NATIVE_VER("ver");
        const String::SubString NATIVE_VER_1_0("1");
        const String::SubString NATIVE_VER_1_2("1.2");

        const String::SubString NATIVE_LINK("link");
        const String::SubString NATIVE_LINK_URL("url");
        const String::SubString NATIVE_LINK_CLICK_TRACKERS("clicktrackers");
        const String::SubString NATIVE_IMP_TRACKERS("imptrackers");
        const String::SubString NATIVE_EVENT_TRACKERS("eventtrackers");
        const String::SubString NATIVE_JS_TRACKER("jstracker");
        const String::SubString NATIVE_ASSETS("assets");
        const String::SubString NATIVE_ASSET_ID("id");
        const String::SubString NATIVE_ASSET_REQUIRED("required");
        const String::SubString NATIVE_ASSET_IMG("img");
        const String::SubString NATIVE_ASSET_IMG_URL("url");
        const String::SubString NATIVE_ASSET_IMG_W("w");
        const String::SubString NATIVE_ASSET_IMG_H("h");
        const String::SubString NATIVE_ASSET_DATA("data");
        const String::SubString NATIVE_ASSET_DATA_VALUE("value");
        const String::SubString NATIVE_ASSET_TITLE("title");
        const String::SubString NATIVE_ASSET_TITLE_TEXT("text");
        const String::SubString NATIVE_ASSET_VIDEO("video");
        const String::SubString NATIVE_ASSET_VIDEO_TAG("vasttag");

        const String::SubString EVENT_EVENT("event");
        const String::SubString EVENT_METHOD("method");
        const String::SubString EVENT_URL("url");

        const String::SubString BID_EXT_NROA("nroa");

        const String::SubString NROA_ERID("erid");
        const String::SubString NROA_CONTRACTS("contracts");
        const String::SubString CONTRACT_ORD_ID("id");
        const String::SubString CONTRACT_ORD_ADO_ID("ado_id");
        const String::SubString CONTRACT_NUMBER("contract_number");
        const String::SubString CONTRACT_DATE("contract_date");
        const String::SubString CONTRACT_TYPE("type");
        const String::SubString CONTRACT_CLIENT_ID("client_inn");
        const String::SubString CONTRACT_CLIENT_NAME("client_name");
        const String::SubString CONTRACT_CONTRACTOR_ID("contractor_inn");
        const String::SubString CONTRACT_CONTRACTOR_NAME("contractor_name");

        // nroa attributes by buzzula+sape standard
        namespace BuzSapeNroa
        {
          const String::SubString HASNROAMARKUP("has_nroa_markup");

          const String::SubString CONTRACTOR("contractor");
          const String::SubString CONTRACTOR_INN("inn");
          const String::SubString CONTRACTOR_NAME("name");
          const String::SubString CONTRACTOR_LEGALFORM("legal_form");

          const String::SubString CLIENT("client");
          const String::SubString CLIENT_INN("inn");
          const String::SubString CLIENT_NAME("name");
          const String::SubString CLIENT_LEGALFORM("legal_form");

          const String::SubString INITIALCONTRACT("initial_contract");
          const String::SubString PARENTCONTRACTS("parent_contracts");

          const String::SubString CONTRACT_SIGNDATE("sign_date");
          const String::SubString CONTRACT_NUMBER("number");
          const String::SubString CONTRACT_TYPE("type");
          const String::SubString CONTRACT_SUBJECTTYPE("subject_type");
          const String::SubString CONTRACT_ACTIONTYPE("action_type");
          const String::SubString CONTRACT_ID("id");
          const String::SubString CONTRACT_ADOID("ado_id");
          const String::SubString CONTRACT_VATINCLUDED("vat_included");
          const String::SubString CONTRACT_PARENTCONTRACTID("parent_contract_id");
        }
      }

      namespace OpenX
      {
        const String::SubString AD_OX_CATS("ad_ox_cats");
        const String::SubString MATCHING_AD_ID("matching_ad_id");
      }

      namespace Yandex
      {
        const String::SubString ID("id");
        const String::SubString SETUSERDATA("setuserdata");
        const String::SubString BIDSET("bidset");
        const String::SubString BID("bid");
        const String::SubString BIDID("bidid");
        const String::SubString CUR("cur");
        const String::SubString UNITS("units");
        const String::SubString ADID("adid");
        const String::SubString PRICE("price");
        const String::SubString ADM("adm");
        const String::SubString VIEW_NOTICES("view_notices");
        const String::SubString NURL("nurl");
        const String::SubString BURL("burl");

        const String::SubString BANNER("banner");
        const String::SubString WIDTH("w");
        const String::SubString HEIGHT("h");
        const String::SubString SETSKIPTOKEN("setskiptoken");

        const String::SubString DSP_PARAMS("dsp_params");
        /*
          const String::SubString URL_PARAM3("url_param3");
          const String::SubString URL_PARAM4("url_param4");
        */
        const String::SubString URL_PARAM17("url_param17");
        const String::SubString URL_PARAM18("url_param18");

        const String::SubString URL_PARAM_ALL[] = {
          String::SubString("url_param1"),
          String::SubString("url_param2"),
          String::SubString("url_param3"),
          String::SubString("url_param4"),
          String::SubString("url_param5"),
          String::SubString("url_param6"),
          String::SubString("url_param7"),
          String::SubString("url_param8"),
          String::SubString("url_param9"),
          String::SubString("url_param10"),
          String::SubString("url_param11"),
          String::SubString("url_param12"),
          String::SubString("url_param13"),
          String::SubString("url_param14"),
          String::SubString("url_param15")
        };
      } // namespace Yandex
    } // namespace Response

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
      const AdServer::CampaignSvcs::CampaignManager::
        ExternalCreativeCategoryIdSeq& categories)
    {
      AdServer::Commons::JsonObject array(parent.add_array(seq_name));
      for(CORBA::ULong cat_i = 0; cat_i < categories.length(); ++cat_i)
      {
        int cat_id = 0;
        if(String::StringManip::str_to_int(
             String::SubString(categories[cat_i].in()), cat_id))
        {
          array.add_number(cat_id);
        }
      }
    }
  }

  OpenRtbBidRequestTask::OpenRtbBidRequestTask(
    Frontend* bid_frontend,
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::HttpResponseWriter_var response_writer,
    const Generics::Time& start_processing_time)
    /*throw(Invalid)*/
    : BidRequestTask(
        bid_frontend,
        std::move(request_holder),
        std::move(response_writer),
        start_processing_time),
      uri_(request_holder_->request().uri().str())
  {}

  /*
  void
  OpenRtbBidRequestTask::execute_impl_() noexcept
  {
    bool bad_request_val = false;

    if(bid_frontend_->process_openrtb_request_(
         bad_request_val,
         this,
         request_info_,
         bid_request_.c_str()))
    {
      //write_response_();
    }
    else
    {
      write_empty_response(bad_request_val ? 400 : 0);
    }
  }
  */

  bool
  OpenRtbBidRequestTask::read_request() noexcept
  {
    static const char* FUN = "OpenRtbBidRequestTask::read_request()";

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
          //throw BidRequestTask::Invalid("can make unzip");
        }

        res.resize(res.size() - zs.avail_out);

        ::inflateEnd(&zs);

        bid_request.swap(res);
      }

      /*
      std::cout << ">>>>>>>>>>>>>>>>>>>>" << std::endl <<
        bid_request << std::endl <<
        ">>>>>>>>>>>>>>>>>>>>" <<
        std::endl;
      */

      bid_frontend_->request_info_filler_->fill_by_openrtb_request(
        *request_params_,
        request_info_,
        keywords_,
        context_,
        bid_request.c_str());

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

  bool
  OpenRtbBidRequestTask::write_response(
    const AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult&
      campaign_match_result)
    noexcept
  {
    //static const char* FUN = "OpenRtbBidRequestTask::write_response()";

    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params =
      *request_params_;

    std::ostringstream response_ostr;
    bool first_is_native = context_.ad_slots.empty() ? false : (context_.ad_slots.begin()->native.in() != 0);

    if(request_params.common_info.request_type != AdServer::CampaignSvcs::AR_YANDEX || first_is_native)
    {
      // standard OpenRTB, OpenX, Yandex native
      fill_openrtb_response_(
        response_ostr,
        request_info(),
        request_params,
        context_,
        campaign_match_result,
        request_params.common_info.request_type == AdServer::CampaignSvcs::AR_YANDEX);
    }
    else
    {
      fill_yandex_response_(
        response_ostr,
        request_info(),
        request_params,
        context_,
        campaign_match_result);
    }

    // write response
    const std::string bid_response = response_ostr.str();

    if(!bid_response.empty())
    {
      FCGI::HttpResponse_var response(new FCGI::HttpResponse());
      response->set_content_type(Response::Type::JSON);
      response->add_header(
        Response::Header::OPENRTB_VERSION,
        Response::Header::OPENRTB_VERSION_VALUE);

      FCGI::OutputStream& output = response->get_output_stream();
      std::string bid_response = response_ostr.str();
      output.write(bid_response.data(), bid_response.size());

      write_response_(200, response);

      return true;
    }

    return false;
  }

  void
  OpenRtbBidRequestTask::write_empty_response(unsigned int code)
    noexcept
  {
    FCGI::HttpResponse_var response(new FCGI::HttpResponse());

    if(code < 300)
    {
      // no-bid is No content by OpenRTB 2.0 spec
      response->set_content_type(Response::Type::JSON);
      response->add_header(
        Response::Header::OPENRTB_VERSION,
        Response::Header::OPENRTB_VERSION_VALUE);
      write_response_(204, response);
    }
    else
    {
      write_response_(code, response);
    }

    //return !bad_request ? 204 /* HTTP_NO_CONTENT */ : 400 /* HTTP_BAD_REQUEST */;
  }

  void
  OpenRtbBidRequestTask::clear() noexcept
  {
    BidRequestTask::clear();
    uri_.clear();
    context_ = JsonProcessingContext();
  }

  void
  OpenRtbBidRequestTask::print_request(std::ostream& /*out*/) const noexcept
  {
    //out << bid_request_;
  }

  void
  OpenRtbBidRequestTask::add_response_notice_(
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
  OpenRtbBidRequestTask::fill_ext0_nroa_(
    AdServer::Commons::JsonObject& nroa_obj,
    const RequestInfo& request_info,
    const AdServer::CampaignSvcs::CampaignManager::AdSlotResult& ad_slot_result)
    noexcept
  {
    nroa_obj.add_escaped_string_if_non_empty(
      Response::OpenRtb::NROA_ERID, String::SubString(ad_slot_result.erid));

    if(ad_slot_result.contracts.length() > 0)
    {
      AdServer::Commons::JsonObject contracts_array(nroa_obj.add_array(Response::OpenRtb::NROA_CONTRACTS));

      for(CORBA::ULong contract_i = 0; contract_i < ad_slot_result.contracts.length(); ++contract_i)
      {
        const AdServer::CampaignSvcs::CampaignManager::ExtContractInfo& contract = ad_slot_result.contracts[contract_i];
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
  OpenRtbBidRequestTask::fill_buzsape_nroa_contract_(
    AdServer::Commons::JsonObject& contract_obj,
    const AdServer::CampaignSvcs::CampaignManager::ExtContractInfo& ext_contract_info)
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
  OpenRtbBidRequestTask::fill_buzsape_nroa_(
    AdServer::Commons::JsonObject& nroa_obj,
    const RequestInfo& request_info,
    const AdServer::CampaignSvcs::CampaignManager::AdSlotResult& ad_slot_result)
    noexcept
  {
    nroa_obj.add_escaped_string_if_non_empty(
      Response::OpenRtb::NROA_ERID, String::SubString(ad_slot_result.erid));
    nroa_obj.add_number(
      Response::OpenRtb::BuzSapeNroa::HASNROAMARKUP, 1);

    if(ad_slot_result.contracts.length() > 0)
    {
      const AdServer::CampaignSvcs::CampaignManager::ExtContractInfo& initial_contract = ad_slot_result.contracts[0];
      
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
      if(ad_slot_result.contracts.length() > 1)
      {
        AdServer::Commons::JsonObject parent_contracts_array(nroa_obj.add_array(Response::OpenRtb::BuzSapeNroa::PARENTCONTRACTS));
        for(CORBA::ULong contract_i = 1; contract_i < ad_slot_result.contracts.length(); ++contract_i)
        {
          AdServer::Commons::JsonObject contract_obj(parent_contracts_array.add_object());
          fill_buzsape_nroa_contract_(contract_obj, ad_slot_result.contracts[contract_i]);
        }
      }
    }
  }

  void
  OpenRtbBidRequestTask::fill_openrtb_response_(
    std::ostream& response_ostr,
    const RequestInfo& request_info,
    const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    const JsonProcessingContext& context,
    const AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult& campaign_match_result,
    bool fill_yandex_attributes)
    noexcept
  {
    static const char* FUN = "OpenRtbBidRequestTask::fill_openrtb_response_()";

    try
    {
      AdServer::Commons::JsonFormatter root_json(response_ostr);
      root_json.add_escaped_string(Response::OpenRtb::ID, context.request_id);
      if(fill_yandex_attributes)
      {
        root_json.add_number(Response::Yandex::UNITS, 2);
      }

      {
        char bid_id[20];
        size_t len = String::StringManip::int_to_str(
          request_params.common_info.random, bid_id, sizeof(bid_id));
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

        if(campaign_match_result.ad_slots.length() != context.ad_slots.size())
        {
          Stream::Error ostr;
          ostr << FUN << ": Error on writing open rtb response(assert): "
            "campaign_match_result.ad_slots.length() != context.ad_slots.size()";
          bid_frontend_->logger()->log(ostr.str(), Logging::Logger::EMERGENCY, Aspect::BIDDING_FRONTEND);

          assert(false);
        }

        JsonAdSlotProcessingContextList::const_iterator slot_it =
          context.ad_slots.begin();

        for(CORBA::ULong ad_slot_i = 0;
            ad_slot_i < campaign_match_result.ad_slots.length();
            ++ad_slot_i, ++slot_it)
        {
          const AdServer::CampaignSvcs::CampaignManager::
            AdSlotResult& ad_slot_result = campaign_match_result.ad_slots[ad_slot_i];

          if(pub_currency_code.empty())
          {
            pub_currency_code = ad_slot_result.pub_currency_code;
            String::AsciiStringManip::to_upper(pub_currency_code);
          }

          if(ad_slot_result.selected_creatives.length() > 0)
          {
            // campaigns selected
            CampaignSvcs::RevenueDecimal sum_pub_ecpm = CampaignSvcs::RevenueDecimal::ZERO;
            for(CORBA::ULong creative_i = 0;
                creative_i < ad_slot_result.selected_creatives.length();
                ++creative_i)
            {
              sum_pub_ecpm += CorbaAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
                ad_slot_result.selected_creatives[creative_i].pub_ecpm);
            }

            bid_frontend_->limit_max_cpm_(
              sum_pub_ecpm, request_params.publisher_account_ids);

            // result price in RUB/1000, ecpm is in 0.01/1000
            CampaignSvcs::RevenueDecimal openrtb_price = CampaignSvcs::RevenueDecimal::div(
              sum_pub_ecpm,
              CampaignSvcs::RevenueDecimal(false, 100, 0));

//            std::string escaped_impid = String::StringManip::json_escape(slot_it->id);
            std::string escaped_creative_url;
            std::string escaped_creative_body;

            if(ad_slot_result.creative_url[0])
            {
              escaped_creative_url = String::StringManip::json_escape(
                String::SubString(ad_slot_result.creative_url));
            }

            if(!slot_it->native && ad_slot_result.creative_body[0])
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

            bid_object.add_escaped_string(Response::OpenRtb::IMPID, slot_it->id);
            bid_object.add_number(Response::OpenRtb::PRICE, openrtb_price);

            if (bid_frontend_->request_info_filler_->fill_adid(request_info))
            {
              bid_object.add_as_string(Response::OpenRtb::ADID, ad_slot_result.selected_creatives[0].creative_id);
            }

            bid_object.add_as_string(
              Response::OpenRtb::CRID,
              ad_slot_result.selected_creatives[0].creative_version_id);

            {
              AdServer::Commons::JsonObject adomain_obj(bid_object.add_array(Response::OpenRtb::ADOMAIN));

              // standard OpenRTB, Allyes, OpenX
              for(CORBA::ULong creative_i = 0;
                  creative_i < ad_slot_result.selected_creatives.length();
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
                  std::ostringstream native_response_ostr;

                  {
                    AdServer::Commons::JsonFormatter native_json(native_response_ostr);
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
                    String::SubString(native_response_ostr.str()));

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
                if(request_params.ad_instantiate_type == AdServer::CampaignSvcs::AIT_VIDEO_URL)
                {
                  video_url_in_ext = true;
                  notice_enabled = true;
                }
                else if(request_params.ad_instantiate_type ==
                    AdServer::CampaignSvcs::AIT_VIDEO_URL_IN_BODY ||
                  request_params.ad_instantiate_type ==
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
                else if(ad_slot_result.notice_url[0])
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
                ad_slot_result.tag_size.in());

              if(banner_by_size_it != slot_it->size_banner.end())
              {
                const JsonAdSlotProcessingContext::BannerFormat& use_banner_format =
                  *(banner_by_size_it->second.banner_format);
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
                ad_slot_result.tag_size.in());

              const bool fill_overlay_ext = (!slot_it->banners.empty() && (banner_by_size_it != slot_it->size_banner.end()));
              const bool fill_nroa = (ad_slot_result.erid[0] || ad_slot_result.contracts.length() > 0);

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

                  if(ad_slot_result.ext_tokens.length() > 0)
                  {
                    for(CORBA::ULong token_i = 0;
                      token_i < ad_slot_result.ext_tokens.length(); ++token_i)
                    {
                      if(ad_slot_result.ext_tokens[token_i].name[0])
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
                    *(banner_by_size_it->second.banner);
                  const JsonAdSlotProcessingContext::BannerFormat& use_banner_format =
                    *(banner_by_size_it->second.banner_format);

                  bool add_ext_width_height = (
                    ad_slot_result.selected_creatives.length() == 1 &&
                    use_banner_format.ext_type == "20");

                  if(add_ext_width_height ||
                    request_params.common_info.request_type == AdServer::CampaignSvcs::AR_OPENX ||
                    ad_slot_result.erid[0] ||
                    ad_slot_result.contracts.length() > 0)
                  {
                    if(add_ext_width_height)
                    {
                      ext_obj.add_as_string(Response::OpenRtb::WIDTH, ad_slot_result.overlay_width);
                      ext_obj.add_as_string(Response::OpenRtb::HEIGHT, ad_slot_result.overlay_height);
                    }

                    if(request_params.common_info.request_type == AdServer::CampaignSvcs::AR_OPENX)
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
                    if(ad_slot_result.erid[0])
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

            if(ad_slot_result.iurl[0])
            {
              bid_object.add_escaped_string(
                Response::OpenRtb::IURL,
                String::SubString(ad_slot_result.iurl));
            }

            bid_object.add_as_string(
              Response::OpenRtb::CID,
              ad_slot_result.selected_creatives[0].campaign_group_id);
          } // if(ad_slot_result.selected_creatives.length() > 0)
        } // for(CORBA::ULong ad_slot_i = 0, ...
      }
      root_json.add_string(Response::OpenRtb::CUR, pub_currency_code);
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
    catch(const CORBA::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Error on writing open rtb response: '" << ex << "'";
      bid_frontend_->logger()->log(ostr.str(), Logging::Logger::EMERGENCY, Aspect::BIDDING_FRONTEND);
    }
  }

  void
  OpenRtbBidRequestTask::fill_yandex_response_(
    std::ostream& response_ostr,
    const RequestInfo& request_info,
    const AdServer::CampaignSvcs::CampaignManager::
      RequestParams& request_params,
    const JsonProcessingContext& context,
    const AdServer::CampaignSvcs::CampaignManager::
      RequestCreativeResult& campaign_match_result)
    noexcept
  {
    static const char* FUN = "OpenRtbBidRequestTask::fill_yandex_response_()";

    try
    {
      std::string escaped_request_id =
        String::StringManip::json_escape(context.request_id);

      AdServer::Commons::JsonFormatter root_json(response_ostr);
      root_json.add(Response::Yandex::ID, YandexIdFormatter(escaped_request_id));
      root_json.add_number(Response::Yandex::UNITS, 2);

      {
        char bid_id[20];
        size_t len = String::StringManip::int_to_str(
          request_params.common_info.random, bid_id, sizeof(bid_id));
        root_json.add_string(Response::OpenRtb::BIDID, String::SubString(bid_id, len));
      }

      bool some_campaign_selected = false;

      {
        for(CORBA::ULong ad_slot_i = 0;
          ad_slot_i < campaign_match_result.ad_slots.length();
          ++ad_slot_i)
        {
          some_campaign_selected |= (
            campaign_match_result.ad_slots[ad_slot_i].selected_creatives.length() > 0);
        }
      }

      {
        AdServer::Commons::UserId user_id = CorbaAlgs::unpack_user_id(
          request_params.common_info.user_id);

        if(!user_id.is_null())
        {
          root_json.add_string(
            Response::Yandex::SETUSERDATA,
            bid_frontend_->common_module_->user_id_controller()->ssp_uuid_string(
              bid_frontend_->common_module_->user_id_controller()->ssp_uuid(
                user_id, request_info.source_id)));
        }
        else if(some_campaign_selected && request_params.common_info.external_user_id[0] == 0)
        {
          std::string tmp_ssp_user_id = bid_frontend_->common_module_->user_id_controller()->ssp_uuid_string(
            bid_frontend_->common_module_->user_id_controller()->ssp_uuid(
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

        if(campaign_match_result.ad_slots.length() != context.ad_slots.size())
        {
          Stream::Error ostr;
          ostr << FUN << ": Error on writing open rtb response(assert): "
            "campaign_match_result.ad_slots.length() != context.ad_slots.size()";
          bid_frontend_->logger()->log(ostr.str(), Logging::Logger::EMERGENCY, Aspect::BIDDING_FRONTEND);

          assert(false);
        }

        JsonAdSlotProcessingContextList::const_iterator slot_it =
          context.ad_slots.begin();

        for(CORBA::ULong ad_slot_i = 0;
            ad_slot_i < campaign_match_result.ad_slots.length();
            ++ad_slot_i, ++slot_it)
        {
          const AdServer::CampaignSvcs::CampaignManager::
            AdSlotResult& ad_slot_result = campaign_match_result.ad_slots[ad_slot_i];

          if(pub_currency_code.empty())
          {
            pub_currency_code = ad_slot_result.pub_currency_code;
            String::AsciiStringManip::to_upper(pub_currency_code);
          }

          if(ad_slot_result.selected_creatives.length() > 0)
          {
            // campaigns selected
            CampaignSvcs::RevenueDecimal sum_pub_ecpm = CampaignSvcs::RevenueDecimal::ZERO;
            for(CORBA::ULong creative_i = 0;
                creative_i < ad_slot_result.selected_creatives.length();
                ++creative_i)
            {
              sum_pub_ecpm += CorbaAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
                ad_slot_result.selected_creatives[creative_i].pub_ecpm);
            }

            bid_frontend_->limit_max_cpm_(
              sum_pub_ecpm, request_params.publisher_account_ids);

            // result price in RUB/1000000, ecpm is in 0.01/1000
            CampaignSvcs::RevenueDecimal openrtb_price = CampaignSvcs::RevenueDecimal::mul(
              sum_pub_ecpm,
              CampaignSvcs::RevenueDecimal(false, 10, 0),
              Generics::DMR_ROUND);

            std::string escaped_creative_url;
            std::string escaped_click_params;
            if(ad_slot_result.creative_url[0])
            {
              escaped_creative_url = String::StringManip::json_escape(
                String::SubString(ad_slot_result.creative_url));
            }

            if(ad_slot_result.click_params[0])
            {
              std::string base64_encoded_click_params;
              String::SubString click_params(ad_slot_result.click_params);
              String::StringManip::base64mod_encode(
                base64_encoded_click_params,
                click_params.data(),
                click_params.length());
              escaped_click_params = String::StringManip::json_escape(
                base64_encoded_click_params);
            }

            AdServer::Commons::JsonObject bid_object(bidarray.add_object());

            bid_object.add(Response::Yandex::ID,
              YandexIdFormatter(slot_it->id));
            bid_object.add_number(Response::Yandex::PRICE,
              openrtb_price.integer<unsigned long>());
            // Always add adid for yandex (don't check source_id)
            bid_object.add_number(Response::Yandex::ADID,
              ad_slot_result.selected_creatives[0].creative_id);

            // fill view_notices, nurl, dsp_params
            if(ad_slot_result.track_pixel_urls.length() > 0)
            {
              AdServer::Commons::JsonObject track_pixel_urls(
                bid_object.add_array(Response::Yandex::VIEW_NOTICES));

              for(CORBA::ULong i = 0;
                i < ad_slot_result.track_pixel_urls.length(); ++i)
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
            else if(ad_slot_result.notice_url[0])
            {
              add_response_notice_(
                bid_object,
                String::SubString(ad_slot_result.notice_url),
                notice_instantiate_type);
            }

            {
              AdServer::Commons::JsonObject banner(bid_object.add_object(Response::Yandex::BANNER));

              auto banner_by_size_it = slot_it->size_banner.find(
                ad_slot_result.tag_size.in());

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

              if(ad_slot_result.yandex_track_params[0])
              {
                std::string escaped_track_params;
                String::StringManip::base64mod_encode(
                  escaped_track_params,
                  ad_slot_result.yandex_track_params,
                  ::strlen(ad_slot_result.yandex_track_params));
                dsp_params.add_escaped_string(
                  Response::Yandex::URL_PARAM17,
                  escaped_track_params);
                dsp_params.add_escaped_string(
                  Response::Yandex::URL_PARAM18,
                  escaped_track_params);
              }
            }

            // fill adm, token, properties
            for(CORBA::ULong token_i = 0; token_i < ad_slot_result.tokens.length(); ++token_i)
            {
              std::string escaped_name = String::StringManip::json_escape(
                String::SubString(ad_slot_result.tokens[token_i].name));

              bid_object.add_escaped_string(escaped_name,
                String::SubString(ad_slot_result.tokens[token_i].value));
            }
            //} // 
          } // if(ad_slot_result.selected_creatives.length() > 0)
        } // for(CORBA::ULong ad_slot_i = 0, ...
      } // close bidset oject and array
      root_json.add_string(Response::Yandex::CUR, pub_currency_code);
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
  OpenRtbBidRequestTask::fill_native_response_(
    AdServer::Commons::JsonObject* root_json,
    const JsonAdSlotProcessingContext::Native& native_context,
    const AdServer::CampaignSvcs::CampaignManager::AdSlotResult& ad_slot_result,
    bool need_escape,
    bool add_root_native,
    SourceTraits::NativeAdsInstantiateType instantiate_type)
  {
    static const char* FUN = "OpenRtbBidRequestTask::fill_native_response_()";

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
        if(ad_slot_result.selected_creatives.length() == 0)
        {
          Stream::Error ostr;
          ostr << FUN << ": Error on writing open rtb response(assert): "
            "ad_slot_result.selected_creatives.length() == 0";
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
        if(ad_slot_result.track_html_body[0])
        {
          native_obj.add_opt_escaped_string(
            Response::OpenRtb::NATIVE_JS_TRACKER,
            String::SubString(ad_slot_result.track_html_body),
            need_escape);
        }

        if(ad_slot_result.track_pixel_urls.length() > 0)
        {
          if(instantiate_type == SourceTraits::NAIT_NATIVE_AS_ELEMENT_1_2 ||
            instantiate_type == SourceTraits::NAIT_ADM_1_2 ||
            instantiate_type == SourceTraits::NAIT_ADM_NATIVE_1_2)
          {
            AdServer::Commons::JsonObject event_trackers_obj(
              native_obj.add_array(Response::OpenRtb::NATIVE_EVENT_TRACKERS));

            for(CORBA::ULong i = 0;
              i < ad_slot_result.track_pixel_urls.length(); ++i)
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

            for(CORBA::ULong i = 0;
              i < ad_slot_result.track_pixel_urls.length(); ++i)
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
      if(native_context.data_assets.size() != ad_slot_result.native_data_tokens.length())
      {
        Stream::Error ostr;
        ostr << FUN << ": Error on writing open rtb response(assert): "
          "native_context.data_assets.size() != ad_slot_result.native_data_tokens.length()";
        throw Exception(ostr);
      }

      size_t data_i = 0;
      for (auto data_it = native_context.data_assets.begin();
        data_it != native_context.data_assets.end(); ++data_it, ++data_i)
      {
        const AdServer::CampaignSvcs::CampaignManager::TokenInfo& token =
          ad_slot_result.native_data_tokens[data_i];
        if(token.value[0])
        {
          AdServer::Commons::JsonObject asset(
            assets_obj.add_object());
          asset.add_number(
            Response::OpenRtb::NATIVE_ASSET_ID, data_it->id);
          bool is_title =
            data_it->data_type == JsonAdSlotProcessingContext::Native::NDTE_TITLE;

          size_t token_len = ::strlen(token.value);
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
      if(ad_slot_result.native_image_tokens.length() > 0)
      {
        size_t image_i = 0;
        for (auto image_it = native_context.image_assets.begin();
           image_it != native_context.image_assets.end(); ++image_it, ++image_i)
        {
          const AdServer::CampaignSvcs::CampaignManager::TokenImageInfo& token =
            ad_slot_result.native_image_tokens[image_i];
          if(token.value[0])
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
      if(ad_slot_result.creative_body[0])
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
    catch(const CORBA::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Error on writing open rtb response: '" << ex << "'";
      bid_frontend_->logger()->log(ostr.str(), Logging::Logger::EMERGENCY, Aspect::BIDDING_FRONTEND);

      throw;
    }
  }

}
}
