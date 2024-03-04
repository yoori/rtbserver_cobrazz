#ifndef _CREATIVETEXTGENERATOR_HPP_
#define _CREATIVETEXTGENERATOR_HPP_

#include "CampaignConfig.hpp"

namespace AdServer
{
  namespace CampaignSvcs
  {
    namespace CreativeTokens
    {
      const std::string ADV_CLICK_URL("CRCLICK");

      const std::string CLICKURL("CLICK");
      const std::string CLICKF("CLICKF");
      const std::string CLICK0("CLICK0");
      const std::string CLICKF0("CLICKF0");

      const std::string PRECLICKURL("PRECLICK");
      const std::string PRECLICKF("PRECLICKF");
      const std::string PRECLICK0("PRECLICK0");
      const std::string PRECLICKF0("PRECLICKF0");

      const std::string CCID("CCID");
      const std::string WIDTH("WIDTH");
      const std::string HEIGHT("HEIGHT");
      const std::string TAGWIDTH("TAGWIDTH");
      const std::string TAGHEIGHT("TAGHEIGHT");
      const std::string TAGSIZE("TAGSIZE");
      const std::string TAGID("TAGID");
      const std::string ADIMAGE_SERVER("ADIMAGE-SERVER");
      const std::string CRVSERVER("CRVSERVER");
      const std::string AD_SERVER("ADSERVER");
      const std::string REFERER("REFERER");
      const std::string REFERER_DOMAIN("DOMAIN");
      const std::string REFERER_DOMAIN_HASH("DOMAIN_HASH");
      const std::string UID("UID");
      const std::string UNSIGNEDUID("UNSIGNEDUID");
      const std::string USER_STATUS("USERSTATUS");
      const std::string ORIGLINK("ORIGLINK");
      const std::string REFERER_KW_MATCH("REFERER-KW-MATCH");
      const std::string CONTEXT_KW_MATCH("CONTEXT-KW-MATCH");
      const std::string SEARCH_TR_MATCH("SEARCH-TR-MATCH");
      const std::string REFERER_KW("REFERER-KW");
      const std::string CONTEXT_KW("CONTEXT-KW");
      const std::string SEARCH_TR("SEARCH-TR");
      const std::string TRACKPIXEL("TRACKPIXEL");
      const std::string TRACKHTMLURL("TRACKHTMLURL");
      const std::string ACTIONPIXEL("ACTIONPIXEL");
      const std::string RANDOM("RANDOM");
      const std::string PP("PP");
      const std::string EXTDATA("EXTDATA");
      const std::string VIDEOH("VIDEOH");
      const std::string VIDEOW("VIDEOW");
      const std::string TAG_TRACK_PIXEL("PUBL_TAG_TRACK_PIXEL");
      const std::string ADV_TRACK_PIXEL("CRADVTRACKPIXEL");
      const std::string KEYWORD("KEYWORD");
      const std::string REQUEST_TOKEN("SECTOK");
      const std::string COLOCATION("COLOID");
      const std::string TEST_REQUEST("TESTREQUEST");
      const std::string COHORT("COHORT");
      const std::string PUBPIXELS("PUBPIXELS");
      const std::string PUB_PIXELS_OPTIN("PUBPIXELSOPTIN");
      const std::string PUB_PIXELS_OPTOUT("PUBPIXELSOPTOUT");
      const char USER_BIND[] = "USERBIND";

      const std::string APP_FORMAT("APP_FORMAT");
      const std::string CREATIVE_SIZE("SIZE");
      const std::string TEMPLATE_FORMAT("TEMPLATE");
      const std::string ADIMAGE_PATH("ADIMAGE-PATH");
      const std::string ADIMAGE("ADIMAGE");
      const std::string CRVBASE("CRVBASE");

      const std::string PASSBACK_URL("PASSBACK_URL");
      const std::string PASSBACK_TYPE("PASSBACK_TYPE");
      const std::string PASSBACK_PIXEL("PASSBACK_PIXEL");
      const std::string PASSBACK_CODE("PASSBACK_CODE");

      const std::string DESTURL("DESTURL");
      const std::string DISPLAY_URL("DISPLAY_URL");

      const std::string OVERLAY_WIDTH("OVERLAY_WIDTH");
      const std::string OVERLAY_HEIGHT("OVERLAY_HEIGHT");

      const std::string URL("URL");

      const std::string CREATIVES_JSON("CREATIVES_JSON");

      const std::string CGID("CGID");
      const std::string CID("CID");
      const std::string VIDEO_DURATION("MP4_DURATION");
      const std::string VIDEO_SKIP_OFFSET("SKIP_TIME_DURATION");

      const std::string PUBPRECLICK("PUBPRECLICK");
      const std::string ADVERTISER_ID("ADVID");
      const std::string SITE_ID("SITEID");
      const std::string PUBLISHER_ID("PUBID");

      const std::string REQUEST_ID("REQUESTID");
      const std::string GREQUESTID("GREQUESTID");

      const std::string HTTPS_SAFE("HTTPS_SAFE");
      const std::string PUB_POSITION_BOTTOM("PUB_POSITION_BOTTOM");

      const std::string ETID("ETID");
      const std::string DNS_ENCODED_UIDS("DNSUIDS");

      // REQ-3977
      const std::string IP("IP");
      const std::string UA("UA");
      const std::string EXT_TRACK_PARAMS("EXT_TRACK_PARAMS");

      const std::string APPLICATION_ID("APPLICATION_ID");
      const std::string ADVERTISING_ID("ADVERTISING_ID");
      const std::string IDFA("IDFA");
      const std::string TNS_COUNTER_DEVICE_TYPE("TNS_COUNTER_DEVICE_TYPE");

      const std::string SOURCE_ID("SOURCEID");
      const std::string EXTERNAL_USER_ID("EXTERNALID");

      const std::string ERID("ERID");
    }

    class CreativeTextGenerator
    {
    public:
      static void
      init_creative_tokens(
        const CreativeInstantiateRule& rule,
        const CreativeInstantiateArgs& creative_instantiate_args,
        const TokenProcessorMap& token_processors,
        const TokenValueMap& request_args,
        const OptionTokenValueMap& creative_args,
        TokenValueMap& result_creative_args)
        /*throw(eh::Exception)*/;
    };
  }
}

#endif
