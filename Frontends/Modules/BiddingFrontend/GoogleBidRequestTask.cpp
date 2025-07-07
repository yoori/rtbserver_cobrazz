#include "GoogleBidRequestTask.hpp"

namespace AdServer
{
namespace Bidding
{
  namespace
  {
    namespace Aspect
    {
      const char BIDDING_FRONTEND[] = "BiddingFrontend";
    }

    namespace Response
    {
      namespace Type
      {
        const String::SubString OCTET_STREAM("application/octet-stream");
      }

      // https://storage.googleapis.com/adx-rtb-dictionaries/buyer-declarable-creative-attributes.txt
      namespace Google
      {
        const ::google::protobuf::int32 CREATIVE_ATTR[] =
        {
          8, // CookieTargeting: IsCookieTargeted
          9, // UserInterestTargeting: IsUserInterestTargeted
        };

        // RichMediaCapabilityType: RichMediaCapabilitySSL
        const ::google::protobuf::int32 CREATIVE_SECURE = 47;

        // Expanding attributes
        //   ExpandingDirection: ExpandingNone
        const ::google::protobuf::int32 CREATIVE_EXPAND_NONE = 12;
        //   ExpandingDirection: ExpandingUp
        const ::google::protobuf::int32 CREATIVE_EXPAND_UP = 13;
        //   ExpandingDirection: ExpandingDown
        const ::google::protobuf::int32 CREATIVE_EXPAND_DOWN = 14;
        //   ExpandingDirection: ExpandingLeft
        const ::google::protobuf::int32 CREATIVE_EXPAND_LEFT = 15;
        //   ExpandingDirection: ExpandingRight
        const ::google::protobuf::int32 CREATIVE_EXPAND_RIGHT = 16;
        //   ExpandingDirection: ExpandingUpLeft
        const ::google::protobuf::int32 CREATIVE_EXPAND_UP_LEFT = 17;
        //   ExpandingDirection: ExpandingUpRight
        const ::google::protobuf::int32 CREATIVE_EXPAND_UP_RIGHT = 18;
        //   ExpandingDirection: ExpandingDownLeft
        const ::google::protobuf::int32 CREATIVE_EXPAND_DOWN_LEFT = 19;
        //   ExpandingDirection: ExpandingDownRight
        const ::google::protobuf::int32 CREATIVE_EXPAND_DOWN_RIGHT = 20;
        //   ExpandingDirection: ExpandingUpOrDown
        const ::google::protobuf::int32 CREATIVE_EXPAND_UP_OR_DOWN = 25;
        //   ExpandingDirection: ExpandingLeftOrRight
        const ::google::protobuf::int32 CREATIVE_EXPAND_LEFT_OR_RIGHT = 26;
        //   ExpandingDirection: ExpandingAnyDiagonal
        const ::google::protobuf::int32 CREATIVE_EXPAND_ANY_DIAGONAL = 27;

        const ::google::protobuf::int32 CREATIVE_EXPAND_MAP[] =
        {
          // expanding = 0
          CREATIVE_EXPAND_NONE,
          // expanding = 1 - CREATIVE_EXPANDING_LEFT
          CREATIVE_EXPAND_LEFT,
          // expanding = 2 - CREATIVE_EXPANDING_RIGHT
          CREATIVE_EXPAND_RIGHT,
          // expanding = 3 - CREATIVE_EXPANDING_RIGHT | CREATIVE_EXPANDING_LEFT
          CREATIVE_EXPAND_LEFT_OR_RIGHT,
          // expanding = 4 - CREATIVE_EXPANDING_UP
          CREATIVE_EXPAND_UP,
          // expanding = 5 - CREATIVE_EXPANDING_UP | CREATIVE_EXPANDING_LEFT
          CREATIVE_EXPAND_UP_LEFT,
          // expanding = 6 - CREATIVE_EXPANDING_UP | CREATIVE_EXPANDING_RIGHT
          CREATIVE_EXPAND_UP_RIGHT,
          // expanding = 7 - CREATIVE_EXPANDING_UP | CREATIVE_EXPANDING_RIGHT | CREATIVE_EXPANDING_LEFT
          -1,
          // expanding = 8 - CREATIVE_EXPANDING_DOWN
          CREATIVE_EXPAND_DOWN,
          // expanding = 9 - CREATIVE_EXPANDING_DOWN | CREATIVE_EXPANDING_LEFT
          CREATIVE_EXPAND_DOWN_LEFT,
          // expanding = 10 - CREATIVE_EXPANDING_DOWN | CREATIVE_EXPANDING_RIGHT
          CREATIVE_EXPAND_DOWN_RIGHT,
          // expanding = 11 - CREATIVE_EXPANDING_DOWN | CREATIVE_EXPANDING_RIGHT | CREATIVE_EXPANDING_LEFT
          -1,
          // expanding = 12 - CREATIVE_EXPANDING_DOWN | CREATIVE_EXPANDING_TOP
          CREATIVE_EXPAND_UP_OR_DOWN,
          // expanding = 13 - CREATIVE_EXPANDING_DOWN | CREATIVE_EXPANDING_TOP | CREATIVE_EXPANDING_LEFT
          -1,
          // expanding = 14 - CREATIVE_EXPANDING_DOWN | CREATIVE_EXPANDING_TOP | CREATIVE_EXPANDING_RIGHT
          -1,
          // expanding = 15 - ALL
          CREATIVE_EXPAND_ANY_DIAGONAL,
        };
      }
    }

    template <typename T, size_t Size, typename Function>
    void for_range(
      const T(&range) [Size],
      Function fn)
    {
      std::for_each (range, range + Size, fn);
    }

    template <typename AddRepeatedFn>
    void categories_to_repeated(
      const AdServer::CampaignSvcs::CampaignManager::
        ExternalCreativeCategoryIdSeq& categories,
      AddRepeatedFn fn)
    {
      for(CORBA::ULong cat_i = 0; cat_i < categories.length(); ++cat_i)
      {
        int32_t cat_id = 0;
        if(String::StringManip::str_to_int(
             String::SubString(categories[cat_i].in()),
             cat_id))
        {
          fn(cat_id);
        }
      }
    } 

    // Google add expanding attributes util
    void
    fill_google_expanding_attributes(
      Google::BidResponse_Ad* ad,
      const CORBA::Octet& expanding)
    {
      if (expanding > 15)
      {
        // Wrong expanding value from CM,
        // May be we need assert or error log here
        // No, we just see empty expanding
        return;
      }
      if (expanding == 0)
      {
        ad->add_attribute(
          Response::Google::CREATIVE_EXPAND_MAP[expanding]);
      }
      else
      {
        for (short i = 1; i <= expanding; ++i)
        {
          ::google::protobuf::int32 current =
            Response::Google::CREATIVE_EXPAND_MAP[i];
          
          if ((i & expanding) == i && current != -1)
          {
            ad->add_attribute(current);
          }
        }
      }
    }
  }

  GoogleBidRequestTask::GoogleBidRequestTask(
    Frontend* bid_frontend,
    FrontendCommons::HttpRequestHolder_var request_holder,
    FrontendCommons::HttpResponseWriter_var response_writer,
    const Generics::Time& start_processing_time)
    /*throw(Invalid)*/
    : BidRequestTask(
        bid_frontend,
        std::move(request_holder),
        std::move(response_writer),
        start_processing_time)
  {}

  bool
  GoogleBidRequestTask::read_request() noexcept
  {
    static const char* FUN = "GoogleBidRequestTask::GoogleBidRequestTask()";

    try
    {
      Stream::BinaryStreamReader request_reader(
        &request_holder_->request().get_input_stream());

      bid_request_.ParseFromIstream(&request_reader);

      /*
      if(bid_frontend_->logger()->log_level() >= Logging::Logger::TRACE)
      {
        Generics::Time end_process_time = Generics::Time::get_time_of_day();
        Stream::Error ostr;
        ostr << FUN << ": Google task initialized after = " <<
          (end_process_time - start_processing_time);
        bid_frontend_->logger()->log(
          ostr.str(),
          Logging::Logger::TRACE,
          Aspect::BIDDING_FRONTEND);
      }
      */

      bid_frontend_->request_info_filler_->fill_by_google_request(
        *request_params_,
        request_info_,
        keywords_,
        ad_slots_context_,
        bid_request_);

      if(bid_request_.has_is_ping() && bid_request_.is_ping())
      {
        return false;
      }

      return true;
    }
    catch(const FrontendCommons::HTTPExceptions::InvalidParamException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": bad request, " << ex.what();
      bid_frontend_->logger()->log(
        ostr.str(),
        Logging::Logger::ERROR,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-7601");
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception: " << ex.what();
      bid_frontend_->logger()->log(
        ostr.str(),
        Logging::Logger::ERROR,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-7601");
    }

    return false;
  }

  bool
  GoogleBidRequestTask::write_response(
    const AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult&
      campaign_match_result)
    noexcept
  {
    try
    {
      AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params =
        *request_params_;

      Google::BidResponse bid_response;

      assert(
        campaign_match_result.ad_slots.length() == ad_slots_context_.size());

      for(CORBA::ULong ad_slot_i = 0;
          ad_slot_i < campaign_match_result.ad_slots.length();
          ++ad_slot_i)
      {
        const AdServer::CampaignSvcs::CampaignManager::
          AdSlotResult& ad_slot_result = campaign_match_result.ad_slots[ad_slot_i];

        const Google::BidRequest_AdSlot& adslot = bid_request_.adslot(ad_slot_i);

        if(ad_slot_result.selected_creatives.length() > 0)
        {
          // campaigns selected
          CampaignSvcs::RevenueDecimal sum_pub_ecpm = CampaignSvcs::RevenueDecimal::ZERO;

          Google::BidResponse_Ad* ad = bid_response.add_ad();
          
          for(CORBA::ULong creative_i = 0;
              creative_i < ad_slot_result.selected_creatives.length();
              ++creative_i)
          {
            sum_pub_ecpm += CorbaAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
              ad_slot_result.selected_creatives[creative_i].pub_ecpm);

            ad->add_click_through_url(
              ad_slot_result.selected_creatives[creative_i].destination_url.in());
          }

          categories_to_repeated(
            ad_slot_result.external_content_categories,
            std::bind1st(
              std::mem_fun(&Google::BidResponse_Ad::add_category), ad));

          bid_frontend_->limit_max_cpm_(sum_pub_ecpm, request_params.publisher_account_ids);

          CampaignSvcs::ExtRevenueDecimal google_price = CampaignSvcs::ExtRevenueDecimal::mul(
            CampaignSvcs::ExtRevenueDecimal(sum_pub_ecpm.str()),
            CampaignSvcs::ExtRevenueDecimal(false, 10000, 0),
            Generics::DMR_ROUND);

          int64_t max_cpm_micros(google_price.integer<int64_t>());
          
          Google::BidResponse_Ad_AdSlot* r_adslot = ad->add_adslot();
          const GoogleAdSlotContext& ad_slot_context = ad_slots_context_[ad_slot_i];
          
          if (ad_slot_context.direct_deal_id &&
              max_cpm_micros >= ad_slot_context.fixed_cpm_micros)
          {
            r_adslot->set_max_cpm_micros(ad_slot_context.fixed_cpm_micros);
            r_adslot->set_deal_id(ad_slot_context.direct_deal_id);
          }
          else
          {
            r_adslot->set_max_cpm_micros(max_cpm_micros);
          }

          r_adslot->set_id(adslot.id());
          
          if(ad_slot_context.width && ad_slot_context.height)
          {
            ad->set_width(ad_slot_context.width);
            ad->set_height(ad_slot_context.height);
          }

          // choose billing_id
          int64_t billing_id = 0;
          for(auto publisher_account_it = request_info_.publisher_account_ids.begin();
              publisher_account_it != request_info_.publisher_account_ids.end();
              ++publisher_account_it)
          {
            auto account_it = bid_frontend_->account_traits_.find(*publisher_account_it);
            if(account_it != bid_frontend_->account_traits_.end())
            {
              if(bid_request_.has_video())
              {
                billing_id = account_it->second->video_billing_id;
              }
              else
              {
                billing_id = account_it->second->display_billing_id;
              }

              break;
            }
          }

          if(billing_id != 0 &&
             ad_slot_context.billing_ids.find(billing_id) != ad_slot_context.billing_ids.end())
          {
            r_adslot->set_billing_id(billing_id);
          }
          else if(!ad_slot_context.billing_ids.empty())
          {
            r_adslot->set_billing_id(*ad_slot_context.billing_ids.begin());            
          }

          // Fill attributes
          for_range(
            Response::Google::CREATIVE_ATTR,
            std::bind1st(
              std::mem_fun(&Google::BidResponse_Ad::add_attribute), ad));

          // Fill external attributes
          categories_to_repeated(
            ad_slot_result.external_visual_categories,
            std::bind1st(
              std::mem_fun(&Google::BidResponse_Ad::add_attribute), ad));
         
          {
            // buyer_creative_id
            const AdServer::CampaignSvcs::CampaignManager::
              CreativeSelectResult& creative = ad_slot_result.selected_creatives[0];

            std::ostringstream creative_version_ostr;

            creative_version_ostr << creative.creative_version_id.in() << "-" <<
              creative.creative_size << "S";

            ad->set_buyer_creative_id(creative_version_ostr.str());

            // Expanding attributes
            fill_google_expanding_attributes(
              ad,
              creative.expanding);

            // Secure attribute
            if (creative.https_safe_flag)
            {
              ad->add_attribute(Response::Google::CREATIVE_SECURE);
            }
          }

          // Video
          if(request_params.ad_instantiate_type == AdServer::CampaignSvcs::AIT_VIDEO_URL &&
             ad_slot_result.creative_url[0])
          {
            ad->set_video_url(ad_slot_result.creative_url);
          }
          // Banner
          else
          {
            ad->set_html_snippet(ad_slot_result.creative_body.in());
          }
        }
      }

      // write response
      FrontendCommons::HttpResponse_var response = bid_frontend_->create_response();
      response->set_content_type(Response::Type::OCTET_STREAM);
      Stream::BinaryStreamWriter response_writer(&response->get_output_stream());
      bid_response.set_processing_time_ms(
        (Generics::Time::get_time_of_day() - start_processing_time()).microseconds() / 1000);
      bid_response.SerializeToOstream(&response_writer);
      response_writer.flush();
      write_response_(200, response);

      return true;
    }
    catch(const eh::Exception&)
    {}

    return false;
  }

  void
  GoogleBidRequestTask::write_empty_response(unsigned int code)
    noexcept
  {
    FrontendCommons::HttpResponse_var response = bid_frontend_->create_response();
    if(code < 300)
    {
      response->set_content_type(Response::Type::OCTET_STREAM);

      Stream::BinaryStreamWriter response_writer(&response->get_output_stream());
      Google::BidResponse empty_bid_response;
      empty_bid_response.set_processing_time_ms(
        (Generics::Time::get_time_of_day() - start_processing_time_).microseconds() / 1000);
      empty_bid_response.SerializeToOstream(&response_writer);
      response_writer.flush();

      write_response_(200, response);
    }
    else
    {
      write_response_(code, response);
    }
  }

  void
  GoogleBidRequestTask::print_request(std::ostream& /*out*/) const noexcept
  {
    //out << bid_request__.DebugString();
  }

  void
  GoogleBidRequestTask::clear() noexcept
  {
    BidRequestTask::clear();
    ad_slots_context_.clear();
  }
}
}
