#include "CalculateParamsFilter.hpp"

namespace
{
  typedef const String::AsciiStringManip::Char2Category<',', '|'>
    ListSepType;
}


namespace AdServer
{
  namespace CampaignSvcs
  {

    // CalculateParamsFiller
    
    const unsigned long CalculateParamsFiller::BF_TIMESTAMP = 900;
    const unsigned long CalculateParamsFiller::BF_LINK = 901;
    
    CalculateParamsFiller::CalculateParamsFiller()
    {
      processors_.resize(1000, CalcParamFiller_var());
      
      processors_[CTR::BF_PUBLISHER_ID] = new CalcParamFillerIntImpl<uint32_t>(
        &CTRGenerator::CalculateParams::publisher_id);
      processors_[CTR::BF_SITE_ID] = new CalcParamFillerIntImpl<uint32_t>(
        &CTRGenerator::CalculateParams::site_id);
      processors_[CTR::BF_TAG_ID] = new CalcParamFillerIntImpl<uint32_t>(
        &CTRGenerator::CalculateParams::tag_id);
      processors_[CTR::BF_ETAG_ID] = new CalcParamFillerStringImpl(
        &CTRGenerator::CalculateParams::etag);
      processors_[CTR::BF_DOMAIN] = new CalcParamFillerStringImpl(
        &CTRGenerator::CalculateParams::domain);
      processors_[CTR::BF_URL] = new CalcParamFillerIntImpl<uint32_t>(
        &CTRGenerator::CalculateParams::referer_hash);
      processors_[CTR::BF_ADVERTISER_ID] = new CalcParamFillerIntImpl<uint32_t>(
        &CTRGenerator::CalculateParams::advertiser_id);
      processors_[CTR::BF_CAMPAIGN_ID] = new CalcParamFillerIntImpl<uint32_t>(
        &CTRGenerator::CalculateParams::campaign_id);
      processors_[CTR::BF_CCG_ID] = new CalcParamFillerIntImpl<uint32_t>(
        &CTRGenerator::CalculateParams::ccg_id);

      processors_[CTR::BF_SIZE_TYPE_ID] = new CalcParamFillerIntImpl<uint32_t>(
        &CTRGenerator::CalculateParams::size_type_id);
      processors_[CTR::BF_SIZE_ID] = new CalcParamFillerIntImpl<uint32_t>(
        &CTRGenerator::CalculateParams::size_id);
      processors_[CTR::BF_HOUR] = new CalcParamFillerIntImpl<unsigned char>(
        &CTRGenerator::CalculateParams::hour);
      processors_[CTR::BF_WEEK_DAY] = new CalcParamFillerIntImpl<unsigned char>(
        &CTRGenerator::CalculateParams::wd);
      processors_[CTR::BF_ISP_ID] = new CalcParamFillerIntImpl<uint32_t>(
        &CTRGenerator::CalculateParams::isp_id);
      processors_[CTR::BF_COLO_ID] = new CalcParamFillerIntImpl<uint32_t>(
        &CTRGenerator::CalculateParams::colo_id);
      processors_[CTR::BF_DEVICE_CHANNEL_ID] = new CalcParamFillerIntImpl<uint32_t>(
        &CTRGenerator::CalculateParams::device_id);
      processors_[CTR::BF_CREATIVE_ID] = new CalcParamFillerIntImpl<uint32_t>(
        &CTRGenerator::CalculateParams::creative_id);
      processors_[CTR::BF_CC_ID] = new CalcParamFillerIntImpl<uint32_t>(
        &CTRGenerator::CalculateParams::cc_id);
      processors_[CTR::BF_CAMPAIGN_FREQ_ID] = new CampaignFreqFiller();
      
      processors_[CTR::BF_HISTORY_CHANNELS] = new CalcParamFillerIntListImpl(
        &CTRGenerator::CalculateParams::channels);
      processors_[CTR::BF_GEO_CHANNELS] = new CalcParamFillerIntListImpl(
        &CTRGenerator::CalculateParams::geo_channels);
      processors_[CTR::BF_CONTENT_CATEGORIES] = new CalcParamFillerIntListImpl(
        &CTRGenerator::CalculateParams::content_categories);
      processors_[CTR::BF_VISUAL_CATEGORIES] = new CalcParamFillerIntListImpl(
        &CTRGenerator::CalculateParams::visual_categories);

      processors_[BF_TIMESTAMP] = new CalcParamFillerTimestampImpl();
      processors_[BF_LINK] = new LinkFillerImpl();
    }

    void
    CalculateParamsFiller::set_value(
      CTRGenerator::CalculateParams& calc_params,
      unsigned long basic_feature,
      const String::SubString& str)
    {
      processors_[basic_feature]->set_value(calc_params, str);
    }

    // CampaignFreqFiller
    
    CalculateParamsFiller::CampaignFreqFiller::CampaignFreqFiller()
    { }
      
    void
    CalculateParamsFiller::CampaignFreqFiller::set_value(
      CTRGenerator::CalculateParams& calc_params,
      const String::SubString& str)
    {
      if(!String::StringManip::str_to_int(str, calc_params.campaign_freq))
      {
        Stream::Error ostr;
        ostr << "invalid feature value '" << str << "'(expected integer)";
        throw CTRGenerator::InvalidConfig(ostr);
      }
        
      calc_params.campaign_freq_log = Generics::BitAlgs::highest_bit_32(
        calc_params.campaign_freq + 1);
    }

    // CalcParamFillerStringImpl
    
    CalculateParamsFiller::CalcParamFillerStringImpl::CalcParamFillerStringImpl(
        std::string CTRGenerator::CalculateParams::* field)
        : field_(field)
    { }
      
    void
    CalculateParamsFiller::CalcParamFillerStringImpl::set_value(
      CTRGenerator::CalculateParams& calc_params,
      const String::SubString& str)
    {
      calc_params.*field_ = str.str();
    }

    // CalcParamFillerIntListImpl
    
    CalculateParamsFiller::CalcParamFillerIntListImpl::CalcParamFillerIntListImpl(
      CTRGenerator::IdSet CTRGenerator::CalculateParams::* field)
      : field_(field)
    { }
      
    void
    CalculateParamsFiller::CalcParamFillerIntListImpl::set_value(
      CTRGenerator::CalculateParams& calc_params,
      const String::SubString& str)
    {
      String::StringManip::Splitter<ListSepType> tokenizer(str);
      String::SubString token;
      while(tokenizer.get_token(token))
      {
        uint32_t val;
        if(!String::StringManip::str_to_int(token, val))
        {
          Stream::Error ostr;
          ostr << "invalid list feature value '" << str << "'(expected integer)";
          throw CTRGenerator::InvalidConfig(ostr);
        }
        (calc_params.*field_).insert(val);
      }
    }

    // CalcParamFillerTimestampImpl
    CalculateParamsFiller::CalcParamFillerTimestampImpl::CalcParamFillerTimestampImpl()
    { }
      
    void
    CalculateParamsFiller::CalcParamFillerTimestampImpl::set_value(
      CTRGenerator::CalculateParams& calc_params,
      const String::SubString& str)
    {
      // fill hour, wd by timestamp (%F %T)
      Generics::Time ts(str, "%Y-%m-%d %H:%M:%S");
      calc_params.wd = ((ts / Generics::Time::ONE_DAY.tv_sec).tv_sec + 3) % 7;
      calc_params.hour = ts.get_gm_time().tm_hour;
    }

    // LinkFillerImpl
    CalculateParamsFiller::LinkFillerImpl::LinkFillerImpl()
    { }

    void
    CalculateParamsFiller::LinkFillerImpl::set_value(
      CTRGenerator::CalculateParams& calc_params,
      const String::SubString& str)
    {
      calc_params.referer_hash = Generics::CRC::quick(0, str.data(), str.size());
    }
    
  }
}
