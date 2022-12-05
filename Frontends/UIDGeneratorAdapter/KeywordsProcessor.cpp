#include <iostream>
#include <Generics/SimpleDecimal.hpp>
#include <String/StringManip.hpp>
#include <String/UTF8Case.hpp>

#include <Commons/DecimalUtils.hpp>

#include "KeywordsProcessor.hpp"

namespace AdServer
{
namespace Frontends
{
  namespace
  {
    typedef Generics::SimpleDecimal<uint64_t, 18, 8> ParseDecimal;

    // DM_SPP
    const String::SubString AGE_PROB_17("age_prob_17"); // floor(1)
    const String::SubString AGE_PROB_18_24("age_prob_18_24"); // floor(1)
    const String::SubString AGE_PROB_25_34("age_prob_25_34"); // floor(1)
    const String::SubString AGE_PROB_35_44("age_prob_35_44"); // floor(1)
    const String::SubString AGE_PROB_45_54("age_prob_45_54"); // floor(1)
    const String::SubString AGE_PROB_55_63("age_prob_55_63"); // floor(1)
    const String::SubString AGE_PROB_64("age_prob_64"); // floor(1)
    const String::SubString GENDER_PROB("gender_prob"); // floor(1)
    const String::SubString APPROX_AUTO_IND("approx_auto_ind"); // floor(1)

    // DM_SPP (coordinates)
    const String::SubString HOME_LAT_3("home_top_1_latitude"); // floor(3)
    const String::SubString HOME_LAT_2("home_top_1_latitude"); // floor(2)
    const String::SubString HOME_LAT_1("home_top_1_latitude"); // floor(1)
    const String::SubString HOME_LONG_3("home_top_1_longitude"); // floor(3)
    const String::SubString HOME_LONG_2("home_top_1_longitude"); // floor(2)
    const String::SubString HOME_LONG_1("home_top_1_longitude"); // floor(1)
    const String::SubString JOB_LAT_3("job_top_1_latitude"); // floor(3)
    const String::SubString JOB_LAT_2("job_top_1_latitude"); // floor(2)
    const String::SubString JOB_LAT_1("job_top_1_latitude"); // floor(1)
    const String::SubString JOB_LONG_3("job_top_1_longitude"); // floor(3)
    const String::SubString JOB_LONG_2("job_top_1_longitude"); // floor(2)
    const String::SubString JOB_LONG_1("job_top_1_longitude"); // floor(1)

    // AGG_SHOPS
    const String::SubString PURCHASE_TYPE("purchase_type"); // sep(,) -> norm()
    const String::SubString PURCHASES_COUNT("purchases_count"); // log(2)
    const String::SubString SHARE_PURCHASES("share_purchases"); // log(10)
    const String::SubString AVERAGE_BILL("average_bill"); // log(10)

    // AGG_OWNERS
    const String::SubString CAR_MODEL("car_model"); // norm()
    const String::SubString PLAN_BUY_CAR_MODEL("plan_buy_car_model"); // norm()
    const String::SubString AVG_BILL_FOREIGN_NET_SHOPS("avg_bill_foreign_net_shops"); // log(10)
    const String::SubString AVG_BILL_RUS_NET_SHOPS("avg_bill_rus_net_shops"); // log(10)

    // AGG_MCC_SEGMENTATION
    const String::SubString MCC_CATEGORY("mcc_category"); // norm()
    const String::SubString MCC_CODE("mcc_code"); // norm()
    const String::SubString MCC_RETAIL_CNT("mcc_retail_cnt"); // log(2)

    // AGG_INTERNET_ACTIVITY
    const String::SubString ANY_SOC_NETWORKS_TRAFFIC_MB("any_soc_networks_traffic_mb"); // log(10)
    const String::SubString VK_TRAFFIC_MB("vk_traffic_mb"); // log(10)
    const String::SubString OK_TRAFFIC_MB("ok_traffic_mb"); // log(10)
    const String::SubString FACEBOOK_TRAFFIC_MB("facebook_traffic_mb"); // log(10)
    const String::SubString INSTAGRAM_TRAFFIC_MB("instagram_traffic_mb"); // log(10)
    const String::SubString GAME_APPS_TRAFFIC_MB("game_apps_traffic_mb"); // log(10)
    const String::SubString GAMES_TITLES("games_titles"); // sep(,) -> norm()
    const String::SubString MOBILE_GAME_TRAFFIC_MB("mobile_game_traffic_mb"); // log(10)
    const String::SubString WHATSAPP_TRAFFIC_MB("whatsapp_traffic_mb"); // log(10)
    const String::SubString VIBER_TRAFFIC_MB("viber_traffic_mb"); // log(10)
    const String::SubString TELEGRAM_TRAFFIC_MB("telegram_traffic_mb"); // log(10)
    const String::SubString SKYPE_TRAFFIC_MB("skype_traffic_mb"); // log(10)
    const String::SubString LINE_TRAFFIC_MB("line_traffic_mb"); // log(10)
    const String::SubString ANY_MESSENGER_TRAFFIC_MB("any_messenger_traffic_mb"); // log(10)
    const String::SubString FACEBOOK_VOICE_TRAFFIC_MB("facebook_voice_traffic_mb"); // log(10)
    const String::SubString VIBER_VOICE_TRAFFIC_MB("viber_voice_traffic_mb"); // log(10)
    const String::SubString WHATSAPP_VOICE_TRAFFIC_MB("whatsapp_voice_traffic_mb"); // log(10)
    const String::SubString SKYPE_VOICE_TRAFFIC_MB("skype_voice_traffic_mb"); // log(10)

    // AGG_INTERESTS_DAILY
    // AGG_HOUSEHOLD
    const String::SubString AVG_M_HOUSEHOLD_INCOME("avg_m_household_income"); // log(10)
    const String::SubString FAMILY_STATUS("family_status"); // sep(,) -> norm()

    // AGG_BANKS_LOANS
    const String::SubString NUM_OF_DEB_CARDS("num_of_deb_cards"); // log(2)
    const String::SubString NUM_OF_CRD_CARDS("num_of_crd_cards"); // log(2)
    const String::SubString CREDIT_BANK_NAME("credit_bank_name"); // sep(,)
    const String::SubString DEBIT_BANK_NAME("debit_bank_name"); // sep(,)
    const String::SubString DEBIT_REFILL_SUM("debit_refill_sum"); // sep(,) -> sum() -> log(10)
    const String::SubString ALL_DEBIT_REFILL_SUM("all_debit_refill_sum"); // log(10), step 100000
    const String::SubString DEBIT_DOWN_SUM("debit_down_sum"); // sep(,) -> sum() -> log(10)
    const String::SubString DEBIT_DOWN_COUNT("debit_down_count"); // sep(,) -> sum() -> log(2)
    const String::SubString ALL_DEBIT_DOWN_SUM("all_debit_down_sum"); // log(10)
    const String::SubString ALL_DEBIT_DOWN_COUNT("all_debit_down_count"); // log(2)
    const String::SubString SUM_PAST_DUE_AMOUNT("sum_past_due_amount"); // log(10)
    const String::SubString AVG_MONTHLY_PAY("avg_monthly_pay"); // sep(,) -> sum() -> log(10)
    const String::SubString ALL_AVG_MONTHLY_PAY("all_avg_monthly_pay"); // log(10)
    const String::SubString COUNT_APPROVED_BANKS("count_approved_banks"); // log(2)
    const String::SubString APPROVED_CREDIT_BANK("approved_credit_bank"); // sep(,) -> norm()
    const String::SubString ACTIVE_CREDIT_BANK("active_credit_bank"); // sep(,) -> norm()
    const String::SubString SUM_TOP_5_REFILL("sum_top_5_refill"); // step 1000 -> log(2)
    const String::SubString SUM_TOP_5_DOWN("sum_top_5_down"); // step 100 -> log(2)
    const String::SubString SUM_TRANSFER("sum_transfer"); // step 100 -> log(2)
    const String::SubString SUM_CASH_ADVANCE("sum_cash_advance"); // step 1000 -> log(2)
    const String::SubString SUM_RETAIL("sum_retail"); // step 100 -> log(2)
    const String::SubString SUM_TRANSFER_YOURSELF("sum_transfer_yourself"); // step 100 -> log(2)
    const String::SubString INCOME_DEBIT_CARD("income_debit_card"); // step 100 -> log(2)

    // AGG_TRIPS
    const String::SubString NUM_RUS_TRIPS_M("num_rus_trips_m"); // log(2)
    const String::SubString NUM_RUS_TRIPS_6M("num_rus_trips_6m"); // log(2)
    const String::SubString NUM_ABROAD_TRIPS_M("num_abroad_trips_m"); // log(2)
    const String::SubString NUM_ABROAD_TRIPS_6M("num_abroad_trips_6m"); // log(2)
    const String::SubString NUM_SUBWAY_USE("num_subway_use"); // log(2)
    const String::SubString COUNTRIES_TRAVEL_CNT("countries_travel_cnt");
    const String::SubString REGIONS_TRAVEL_CNT("regions_travel_cnt");

    // AGG_SGMT_AUTO_M
    const String::SubString APPROX_AUTO_PROB("approx_auto_prob"); // floor(1)
    const String::SubString HJ_DISTANCE_KM("hj_distance_km"); // log(10)
    const String::SubString HJ_DIFFERENCE_KEY("hj_difference_key"); // norm()
    const String::SubString BS_WEEKDAYS_CNT("bs_weekdays_cnt"); // log(2)
    const String::SubString BS_WEEKEND_CNT("bs_weekend_cnt"); // log(2)
    const String::SubString BS_CNT("bs_cnt"); // log(2)
    const String::SubString METRO_BS_WEEKDAYS_CNT("metro_bs_weekdays_cnt"); // log(2)
    const String::SubString METRO_BS_WEEKEND_CNT("metro_bs_weekend_cnt"); // log(2)
    const String::SubString METRO_BS_CNT("metro_bs_cnt"); // log(2)
    const String::SubString VOICE_BS_WEEKDAYS_CNT("voice_bs_weekend_cnt"); // log(2)
    const String::SubString VOICE_BS_WEEKEND_CNT("voice_bs_weekdays_cnt"); // log(2)
    const String::SubString VOICE_BS_CNT("voice_bs_cnt"); // log(2)
    const String::SubString AUTO_SITE_CNT("auto_site_cnt"); // log(2)
    const String::SubString AZS_PAYMENT_CNT("azs_payment_cnt");  // log(2)
    const String::SubString AZS_CARD_IND("azs_card_ind"); // norm ()
    const String::SubString PARKING_PAYMENT_CNT("parking_payment_cnt"); // log(2)
    const String::SubString AUTODEALER_OFFER_CNT("autodealer_offer_cnt"); // log(2)
    const String::SubString CAR_CHANGE_CNT("car_change_cnt"); // log(2)
    const String::SubString INSURANCE_CNT("insurance_cnt"); // log(2)
    const String::SubString AUTOCOURSES_CNT("autocourses_cnt"); // log(2)
    const String::SubString TRANSPONDER_CNT("transponder_cnt"); // log(2)
    const String::SubString FINES_CNT("fines_cnt"); // log(2)

  };

  // KeywordsProcessor::NumberPreNormProcessor
  class KeywordsProcessor::NumberPreNormProcessor:
    public KeywordsProcessor::ParamProcessor
  {
  public:
    NumberPreNormProcessor(ParamProcessor* next_processor = 0)
      noexcept;

    virtual void
    process(
      KeywordArray& keywords,
      const String::SubString& value)
      noexcept;

  private:
    ParamProcessor_var next_processor_;
  };

  // KeywordsProcessor::FloatFloorParamProcessor
  class KeywordsProcessor::FloatFloorParamProcessor:
    public KeywordsProcessor::ParamProcessor
  {
  public:
    FloatFloorParamProcessor(
      unsigned long floor_digits,
      ParamProcessor* next_processor = 0)
      noexcept;

    virtual void
    process(
      KeywordArray& keywords,
      const String::SubString& value)
      noexcept;

  private:
    const unsigned long floor_digits_;
    ParamProcessor_var next_processor_;
  };

  // KeywordsProcessor::Log10ParamProcessor
  class KeywordsProcessor::Log10ParamProcessor:
    public KeywordsProcessor::ParamProcessor
  {
  public:
    Log10ParamProcessor(
      unsigned long floor_digits,
      ParamProcessor* next_processor = 0)
      noexcept;

    virtual void
    process(
      KeywordArray& keywords,
      const String::SubString& value)
      noexcept;

  private:
    const unsigned long floor_digits_;
    ParamProcessor_var next_processor_;
  };

  // KeywordsProcessor::NumericStepParamProcessor
  class KeywordsProcessor::NumericStepParamProcessor:
    public KeywordsProcessor::ParamProcessor
  {
  public:
    NumericStepParamProcessor(
      unsigned long floor_digits,
      const ParseDecimal& step,
      ParamProcessor* next_processor = 0)
      noexcept;

    virtual void
    process(
      KeywordArray& keywords,
      const String::SubString& value)
      noexcept;

  private:
    const unsigned long floor_digits_;
    const ParseDecimal& step_;
    ParamProcessor_var next_processor_;
  };

  // KeywordsProcessor::Log2ParamProcessor
  class KeywordsProcessor::Log2ParamProcessor:
    public KeywordsProcessor::ParamProcessor
  {
  public:
    Log2ParamProcessor(
      unsigned long floor_digits,
      ParamProcessor* next_processor = 0)
      noexcept;

    virtual void
    process(
      KeywordArray& keywords,
      const String::SubString& value)
      noexcept;

  private:
    const unsigned long floor_digits_;
    ParamProcessor_var next_processor_;
  };

  // KeywordsProcessor::SepParamProcessor
  template<typename SepCategory>
  class KeywordsProcessor::SepParamProcessor:
    public KeywordsProcessor::ParamProcessor
  {
  public:
    SepParamProcessor(
      ParamProcessor* tok_processor,
      char new_separator = '\n')
      noexcept;

    virtual void
    process(
      KeywordArray& keywords,
      const String::SubString& value)
      noexcept;

  private:
    const unsigned long new_separator_;
    ParamProcessor_var tok_processor_;
  };

  // KeywordsProcessor::SepParamProcessor
  template<typename SepCategory>
  class KeywordsProcessor::SepSumParamProcessor:
    public KeywordsProcessor::ParamProcessor
  {
  public:
    SepSumParamProcessor(ParamProcessor* next_processor)
      noexcept;

    virtual void
    process(
      KeywordArray& keywords,
      const String::SubString& value)
      noexcept;

  private:
    ParamProcessor_var next_processor_;
  };

  // KeywordsProcessor::NormParamProcessor
  class KeywordsProcessor::NormParamProcessor:
    public KeywordsProcessor::ParamProcessor
  {
  public:
    NormParamProcessor()
      noexcept;

    virtual void
    process(
      KeywordArray& keywords,
      const String::SubString& value)
      noexcept;
  };

  // KeywordsProcessor::AddPrefixParamProcessor
  class KeywordsProcessor::AddPrefixParamProcessor:
    public KeywordsProcessor::ParamProcessor
  {
  public:
    AddPrefixParamProcessor(
      const String::SubString& prefix,
      ParamProcessor* next_processor = 0)
      noexcept;

    virtual void
    process(
      KeywordArray& keywords,
      const String::SubString& value)
      noexcept;

  protected:
    virtual
    ~AddPrefixParamProcessor() noexcept
    {}

  private:
    const std::string prefix_;
    ParamProcessor_var next_processor_;
  };

  // KeywordsProcessor::CompositeParamProcessor
  class KeywordsProcessor::CompositeParamProcessor:
    public KeywordsProcessor::ParamProcessor
  {
  public:
    CompositeParamProcessor(
      ParamProcessor* next_processor1,
      ParamProcessor* next_processor2 = 0)
      noexcept;

    virtual void
    process(
      KeywordArray& keywords,
      const String::SubString& value)
      noexcept;

  protected:
    virtual
    ~CompositeParamProcessor() noexcept
    {}

  private:
    std::vector<ParamProcessor_var> next_processors_;
  };

  // KeywordsProcessor::FloatFloorParamProcessor impl
  KeywordsProcessor::FloatFloorParamProcessor::FloatFloorParamProcessor(
    unsigned long floor_digits,
    ParamProcessor* next_processor)
    noexcept
    : floor_digits_(floor_digits),
      next_processor_(ReferenceCounting::add_ref(next_processor))
  {}

  void
  KeywordsProcessor::FloatFloorParamProcessor::process(
    KeywordArray& keywords,
    const String::SubString& value)
    noexcept
  {
    String::SubString trimed_value = value;
    String::StringManip::trim(trimed_value);

    std::string res;

    try
    {
      ParseDecimal val = AdServer::Commons::extract_decimal<ParseDecimal>(
        trimed_value);
      val.floor(floor_digits_);
      res = val.str();
    }
    catch(const eh::Exception& ex)
    {
      res = trimed_value.str();

      Stream::Error ostr;
      ostr  << "On '" << value << "': " << ex.what() << std::endl;
      std::cerr << ostr.str() << std::endl;
    }

    if(next_processor_)
    {
      next_processor_->process(keywords, res);
    }
    else
    {
      keywords.push_back(new AdServer::Commons::StringHolder(res));
    }
  }

  // KeywordsProcessor::Log10ParamProcessor
  KeywordsProcessor::Log10ParamProcessor::Log10ParamProcessor(
    unsigned long floor_digits,
    ParamProcessor* next_processor)
    noexcept
    : floor_digits_(floor_digits),
      next_processor_(ReferenceCounting::add_ref(next_processor))
  {}

  void
  KeywordsProcessor::Log10ParamProcessor::process(
    KeywordArray& keywords,
    const String::SubString& value)
    noexcept
  {
    String::SubString trimed_value = value;
    String::StringManip::trim(trimed_value);

    std::string res;

    try
    {
      ParseDecimal val = AdServer::Commons::extract_decimal<ParseDecimal>(
        trimed_value);
      if(val <= ParseDecimal::ZERO)
      {
        res = "-1";
      }
      else
      {
        double val_floating = val.floating<double>();
        if(val_floating <= 1.00001)
        {
          res = "0";
        }
        else
        {
          double float_res = std::log10(val_floating);
          val = ParseDecimal(float_res);
          val.floor(floor_digits_);
          res = val.str();
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      (void)ex;
      res = trimed_value.str();

#     ifdef OUT_INVALID_VALUES_
      Stream::Error ostr;
      ostr << "On '" << value << "': " << ex.what() << std::endl;
      std::cerr << ostr.str() << std::endl;
#     endif
    }

    if(next_processor_)
    {
      next_processor_->process(keywords, res);
    }
    else
    {
      keywords.push_back(new AdServer::Commons::StringHolder(res));
    }
  }

  // KeywordsProcessor::Log2ParamProcessor
  KeywordsProcessor::Log2ParamProcessor::Log2ParamProcessor(
    unsigned long floor_digits,
    ParamProcessor* next_processor)
    noexcept
    : floor_digits_(floor_digits),
      next_processor_(ReferenceCounting::add_ref(next_processor))
  {}

  void
  KeywordsProcessor::Log2ParamProcessor::process(
    KeywordArray& keywords,
    const String::SubString& value)
    noexcept
  {
    String::SubString trimed_value = value;
    String::StringManip::trim(trimed_value);

    std::string res;

    try
    {
      ParseDecimal val = AdServer::Commons::extract_decimal<ParseDecimal>(
        trimed_value);
      if(val <= ParseDecimal::ZERO)
      {
        res = "-1";
      }
      else
      {
        double val_floating = val.floating<double>();
        if(val_floating <= 1.00001)
        {
          res = "0";
        }
        else
        {
          double float_res = std::log2(val_floating);
          val = ParseDecimal(float_res);
          val.floor(floor_digits_);
          res = val.str();
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      (void)ex;
      res = trimed_value.str();

#     ifdef OUT_INVALID_VALUES_
      Stream::Error ostr;
      ostr << "On '" << value << "': " << ex.what() << std::endl;
      std::cerr << ostr.str() << std::endl;
#     endif
    }

    if(next_processor_)
    {
      next_processor_->process(keywords, res);
    }
    else
    {
      keywords.push_back(new AdServer::Commons::StringHolder(res));
    }
  }

  // KeywordsProcessor::NumericStepParamProcessor
  KeywordsProcessor::NumericStepParamProcessor::NumericStepParamProcessor(
    unsigned long floor_digits,
    const ParseDecimal& step,
    ParamProcessor* next_processor)
    noexcept
    : floor_digits_(floor_digits),
      step_(step),
      next_processor_(ReferenceCounting::add_ref(next_processor))
  {}

  void
  KeywordsProcessor::NumericStepParamProcessor::process(
    KeywordArray& keywords,
    const String::SubString& value)
    noexcept
  {
    String::SubString trimed_value = value;
    String::StringManip::trim(trimed_value);

    std::string res;

    try
    {
      ParseDecimal val = AdServer::Commons::extract_decimal<ParseDecimal>(
        trimed_value);
      val = ParseDecimal::div(val, step_);

      val.floor(floor_digits_);
      res = val.str();
    }
    catch(const eh::Exception& ex)
    {
      (void)ex;
      res = trimed_value.str();

#     ifdef OUT_INVALID_VALUES_
      Stream::Error ostr;
      ostr << "On '" << value << "': " << ex.what() << std::endl;
      std::cerr << ostr.str() << std::endl;
#     endif
    }

    if(next_processor_)
    {
      next_processor_->process(keywords, res);
    }
    else
    {
      keywords.push_back(new AdServer::Commons::StringHolder(res));
    }
  }

  // KeywordsProcessor::NumberPreNormProcessor
  KeywordsProcessor::NumberPreNormProcessor::NumberPreNormProcessor(
    ParamProcessor* next_processor)
    noexcept
    : next_processor_(ReferenceCounting::add_ref(next_processor))
  {}

  void
  KeywordsProcessor::NumberPreNormProcessor::process(
    KeywordArray& keywords,
    const String::SubString& value)
    noexcept
  {
    std::string res;
    std::string res2;

    String::StringManip::replace(
      value,
      res,
      String::SubString("-"),
      String::SubString("minus"));
    
    String::StringManip::replace(
      res,
      res2,
      String::SubString("."),
      String::SubString("dot"));

    if(next_processor_)
    {
      next_processor_->process(keywords, res2);
    }
    else
    {
      keywords.push_back(new AdServer::Commons::StringHolder(res2));
    }
  }

  // KeywordsProcessor::NormParamProcessor
  KeywordsProcessor::NormParamProcessor::NormParamProcessor()
    noexcept
  {}

  void
  KeywordsProcessor::NormParamProcessor::process(
    KeywordArray& keywords,
    const String::SubString& value)
    noexcept
  {
    std::string simplified_res;

    {
      String::SubString trimed_value = value;
      String::StringManip::trim(trimed_value);
      String::case_change<String::Simplify>(trimed_value, simplified_res);
    }

    std::string space_packed_res;

    {
      String::StringManip::Splitter<
        String::AsciiStringManip::SepSpace> tokenizer(simplified_res);
      String::SubString cur;
      while(tokenizer.get_token(cur))
      {
        if(!space_packed_res.empty())
        {
          space_packed_res += ' ';
        }

        cur.append_to(space_packed_res);
      }
    }

    {
      String::SubString trimed_value = space_packed_res;
      String::StringManip::trim(trimed_value);
      space_packed_res = trimed_value.str();

      std::replace(
        space_packed_res.begin(),
        space_packed_res.end(),
        ' ',
        'x');

    }

    keywords.push_back(new AdServer::Commons::StringHolder(space_packed_res));
  }

  // KeywordsProcessor::SepParamProcessor
  template<typename SepCategory>
  KeywordsProcessor::SepParamProcessor<SepCategory>::SepParamProcessor(
    ParamProcessor* tok_processor,
    char new_separator)
    noexcept
    : new_separator_(new_separator),
      tok_processor_(ReferenceCounting::add_ref(tok_processor))
  {}

  template<typename SepCategory>
  void
  KeywordsProcessor::SepParamProcessor<SepCategory>::process(
    KeywordArray& keywords,
    const String::SubString& value)
    noexcept
  {
    String::StringManip::Splitter<SepCategory> tokenizer(value);

    String::SubString cur;
    while(tokenizer.get_token(cur))
    {
      tok_processor_->process(keywords, cur);
    }
  }

  // KeywordsProcessor::SepSumParamProcessor
  template<typename SepCategory>
  KeywordsProcessor::SepSumParamProcessor<SepCategory>::SepSumParamProcessor(
    ParamProcessor* next_processor)
    noexcept
    : next_processor_(ReferenceCounting::add_ref(next_processor))
  {}

  template<typename SepCategory>
  void
  KeywordsProcessor::SepSumParamProcessor<SepCategory>::process(
    KeywordArray& keywords,
    const String::SubString& value)
    noexcept
  {
    String::StringManip::Splitter<SepCategory> tokenizer(value);

    ParseDecimal sum = ParseDecimal::ZERO;
    String::SubString cur;
    while(tokenizer.get_token(cur))
    {
      try
      {
        String::SubString trimed_value = cur;
        String::StringManip::trim(trimed_value);
        ParseDecimal val = AdServer::Commons::extract_decimal<ParseDecimal>(
          trimed_value);
        sum += val;
      }
      catch(const ParseDecimal::Overflow&)
      {
        sum = ParseDecimal::MAXIMUM;
      }
      catch(const eh::Exception&)
      {}
    }

    next_processor_->process(keywords, sum.str());
  }

  // KeywordsProcessor::AddPrefixParamProcessor
  KeywordsProcessor::AddPrefixParamProcessor::AddPrefixParamProcessor(
    const String::SubString& prefix,
    ParamProcessor* next_processor)
    noexcept
    : prefix_(prefix.str()),
      next_processor_(ReferenceCounting::add_ref(next_processor))
  {}

  void
  KeywordsProcessor::AddPrefixParamProcessor::process(
    KeywordArray& keywords,
    const String::SubString& value)
    noexcept
  {
    KeywordArray next_keywords;
    next_processor_->process(next_keywords, value);
    for(auto it = next_keywords.begin(); it != next_keywords.end(); ++it)
    {
      keywords.push_back(new Commons::StringHolder(prefix_ + (*it)->str()));
    }
  }

  // KeywordsProcessor::CompositeParamProcessor
  KeywordsProcessor::CompositeParamProcessor::CompositeParamProcessor(
    ParamProcessor* next_processor1,
    ParamProcessor* next_processor2)
    noexcept
  {
    if(next_processor1)
    {
      next_processors_.push_back(ReferenceCounting::add_ref(next_processor1));
    }

    if(next_processor2)
    {
      next_processors_.push_back(ReferenceCounting::add_ref(next_processor2));
    }
  }

  void
  KeywordsProcessor::CompositeParamProcessor::process(
    KeywordArray& keywords,
    const String::SubString& value)
    noexcept
  {
    for(auto proc_it = next_processors_.begin(); proc_it != next_processors_.end(); ++proc_it)
    {
      (*proc_it)->process(keywords, value);
    }
  }

  // KeywordsProcessor impl
  KeywordsProcessor::KeywordsProcessor() noexcept
  {
    // DM_SPP
    add_processor_(
      AGE_PROB_17,
      new FloatFloorParamProcessor(1, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      AGE_PROB_18_24,
      new FloatFloorParamProcessor(1, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      AGE_PROB_25_34,
      new FloatFloorParamProcessor(1, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      AGE_PROB_35_44,
      new FloatFloorParamProcessor(1, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      AGE_PROB_45_54,
      new FloatFloorParamProcessor(1, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      AGE_PROB_55_63,
      new FloatFloorParamProcessor(1, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      AGE_PROB_64,
      new FloatFloorParamProcessor(1, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      GENDER_PROB,
      new FloatFloorParamProcessor(1, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      APPROX_AUTO_IND,
      new FloatFloorParamProcessor(1, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      HOME_LAT_3,
      new FloatFloorParamProcessor(3, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      HOME_LAT_2,
      new FloatFloorParamProcessor(2, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      HOME_LAT_1,
      new FloatFloorParamProcessor(1, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      HOME_LONG_3,
      new FloatFloorParamProcessor(3, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      HOME_LONG_2,
      new FloatFloorParamProcessor(2, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      HOME_LONG_1,
      new FloatFloorParamProcessor(1, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      JOB_LAT_3,
      new FloatFloorParamProcessor(3, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      JOB_LAT_2,
      new FloatFloorParamProcessor(2, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      JOB_LAT_1,
      new FloatFloorParamProcessor(1, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      JOB_LONG_3,
      new FloatFloorParamProcessor(3, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      JOB_LONG_2,
      new FloatFloorParamProcessor(2, ParamProcessor_var(new NumberPreNormProcessor()))); 
    add_processor_(
      JOB_LONG_1,
      new FloatFloorParamProcessor(1, ParamProcessor_var(new NumberPreNormProcessor())));

    // AGG_SHOPS
    add_processor_(
      PURCHASE_TYPE,
      new SepParamProcessor<>(ParamProcessor_var(new NormParamProcessor())));
    add_processor_(
      PURCHASES_COUNT,
      new Log2ParamProcessor(
        0,
        ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      SHARE_PURCHASES,
      new Log10ParamProcessor(
        0,
        ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      AVERAGE_BILL,
      new Log10ParamProcessor(
        0,
        ParamProcessor_var(new NumberPreNormProcessor())));

    // AGG_OWNERS
    add_processor_(CAR_MODEL, new NormParamProcessor());
    add_processor_(PLAN_BUY_CAR_MODEL, new NormParamProcessor());
    add_processor_(
      AVG_BILL_FOREIGN_NET_SHOPS,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      AVG_BILL_RUS_NET_SHOPS,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));

    // AGG_MCC_SEGMENTATION
    add_processor_(MCC_CATEGORY, new NormParamProcessor());
    add_processor_(MCC_CODE, new NormParamProcessor());
    add_processor_(
      MCC_RETAIL_CNT,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));

    // AGG_INTERNET_ACTIVITY
    add_processor_(
      ANY_SOC_NETWORKS_TRAFFIC_MB,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      VK_TRAFFIC_MB,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      OK_TRAFFIC_MB,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      FACEBOOK_TRAFFIC_MB,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      INSTAGRAM_TRAFFIC_MB,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      GAME_APPS_TRAFFIC_MB,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      GAMES_TITLES,
      new SepParamProcessor<>(ParamProcessor_var(new NormParamProcessor())));
    add_processor_(
      MOBILE_GAME_TRAFFIC_MB,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      WHATSAPP_TRAFFIC_MB,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      VIBER_TRAFFIC_MB,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      TELEGRAM_TRAFFIC_MB,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      SKYPE_TRAFFIC_MB,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      LINE_TRAFFIC_MB,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      ANY_MESSENGER_TRAFFIC_MB,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      FACEBOOK_VOICE_TRAFFIC_MB,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      VIBER_VOICE_TRAFFIC_MB,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      WHATSAPP_VOICE_TRAFFIC_MB,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      SKYPE_VOICE_TRAFFIC_MB,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));

    // AGG_HOUSEHOLD
    add_processor_(
      AVG_M_HOUSEHOLD_INCOME, 
      new Log10ParamProcessor(
        0, 
        ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      FAMILY_STATUS,
      new SepParamProcessor<>(ParamProcessor_var(new NormParamProcessor())));

    // AGG_BANKS_LOANS
    add_processor_(
      NUM_OF_DEB_CARDS,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      NUM_OF_CRD_CARDS,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      CREDIT_BANK_NAME,
      new SepParamProcessor<>(ParamProcessor_var(new NormParamProcessor())));
    add_processor_(
      DEBIT_BANK_NAME,
      new SepParamProcessor<>(ParamProcessor_var(new NormParamProcessor())));
    add_processor_(
      DEBIT_REFILL_SUM, 
      new SepSumParamProcessor<>(ParamProcessor_var(new Log10ParamProcessor(0))));
    add_processor_(
      ALL_DEBIT_REFILL_SUM,
      new CompositeParamProcessor(
        ParamProcessor_var(
          new Log10ParamProcessor(
            0,
            ParamProcessor_var(new NumberPreNormProcessor()))),
        ParamProcessor_var(
          new AddPrefixParamProcessor(
            String::SubString("st"),
            ParamProcessor_var(
              new NumericStepParamProcessor(
                0,
                ParseDecimal("100000"),
                ParamProcessor_var(new NumberPreNormProcessor())))))
        )
      );
    add_processor_(
      DEBIT_DOWN_SUM,
      new SepSumParamProcessor<>(ParamProcessor_var(new Log10ParamProcessor(0))));
    add_processor_(
      DEBIT_DOWN_COUNT,
      new SepSumParamProcessor<>(ParamProcessor_var(new Log2ParamProcessor(0))));
    add_processor_(
      ALL_DEBIT_DOWN_SUM,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      ALL_DEBIT_DOWN_COUNT,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      SUM_PAST_DUE_AMOUNT,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      AVG_MONTHLY_PAY,
      new SepSumParamProcessor<>(ParamProcessor_var(new Log10ParamProcessor(0))));
    add_processor_(
      ALL_AVG_MONTHLY_PAY,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      COUNT_APPROVED_BANKS,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      APPROVED_CREDIT_BANK,
      new SepParamProcessor<>(ParamProcessor_var(new NormParamProcessor())));
    add_processor_(
      ACTIVE_CREDIT_BANK,
      new SepParamProcessor<>(ParamProcessor_var(new NormParamProcessor())));
    add_processor_(
      SUM_TOP_5_REFILL,
      new NumericStepParamProcessor(
        0,
        ParseDecimal("1000"),
        ParamProcessor_var(
          new Log2ParamProcessor(
            0,
            ParamProcessor_var(new NumberPreNormProcessor())))));
    add_processor_(
      SUM_TOP_5_DOWN,
      new NumericStepParamProcessor(
        0,
        ParseDecimal("100"),
        ParamProcessor_var(
          new Log2ParamProcessor(
            0,
            ParamProcessor_var(new NumberPreNormProcessor())))));
    add_processor_(
      SUM_TRANSFER,
      new NumericStepParamProcessor(
        0,
        ParseDecimal("100"),
        ParamProcessor_var(
          new Log2ParamProcessor(
            0,
            ParamProcessor_var(new NumberPreNormProcessor())))));
    add_processor_(
      SUM_CASH_ADVANCE,
      new NumericStepParamProcessor(
        0,
        ParseDecimal("1000"),
        ParamProcessor_var(
          new Log2ParamProcessor(
            0,
            ParamProcessor_var(new NumberPreNormProcessor())))));
    add_processor_(
      SUM_TRANSFER_YOURSELF,
      new NumericStepParamProcessor(
        0,
        ParseDecimal("100"),
        ParamProcessor_var(
          new Log2ParamProcessor(
            0,
            ParamProcessor_var(new NumberPreNormProcessor())))));
    /*
    add_processor_(
      SUM_RETAIL,
      new NumericStepParamProcessor(
        0,
        ParseDecimal("100"),
        ParamProcessor_var(
          new Log2ParamProcessor(
            0,
            ParamProcessor_var(new NumberPreNormProcessor())))));
    add_processor_(
      INCOME_DEBIT_CARD,
      new NumericStepParamProcessor(
        0,
        ParseDecimal("100"),
        ParamProcessor_var(
          new Log2ParamProcessor(
            0,
            ParamProcessor_var(new NumberPreNormProcessor())))));
    */
    add_processor_(
      SUM_RETAIL,
      new NormParamProcessor());
    add_processor_(
      INCOME_DEBIT_CARD,
      new NormParamProcessor());

    // AGG_TRIPS
    add_processor_(
      NUM_RUS_TRIPS_M,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      NUM_RUS_TRIPS_6M,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      NUM_ABROAD_TRIPS_M,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      NUM_ABROAD_TRIPS_6M,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      NUM_SUBWAY_USE,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      COUNTRIES_TRAVEL_CNT,
      new SepParamProcessor<>(ParamProcessor_var(new NormParamProcessor())));
    add_processor_(
      REGIONS_TRAVEL_CNT,
      new SepParamProcessor<>(ParamProcessor_var(new NormParamProcessor())));

    // AGG_SGMT_AUTO_M
    add_processor_(
      APPROX_AUTO_PROB,
      new FloatFloorParamProcessor(1, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      HJ_DISTANCE_KM,
      new Log10ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      HJ_DIFFERENCE_KEY,
      new NormParamProcessor());
    add_processor_(
      BS_WEEKDAYS_CNT,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      BS_WEEKEND_CNT,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      BS_CNT,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      METRO_BS_WEEKDAYS_CNT,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      METRO_BS_WEEKEND_CNT,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      METRO_BS_CNT,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      VOICE_BS_WEEKDAYS_CNT,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      VOICE_BS_WEEKEND_CNT,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      VOICE_BS_CNT,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      AUTO_SITE_CNT,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      AZS_PAYMENT_CNT,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      AZS_CARD_IND,
      new NormParamProcessor());
    add_processor_(
      PARKING_PAYMENT_CNT,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      AUTODEALER_OFFER_CNT,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      CAR_CHANGE_CNT,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      INSURANCE_CNT,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      AUTOCOURSES_CNT,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      TRANSPONDER_CNT,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
    add_processor_(
      FINES_CNT,
      new Log2ParamProcessor(0, ParamProcessor_var(new NumberPreNormProcessor())));
  }

  void
  KeywordsProcessor::process(
    KeywordArray& keywords,
    const ru::madnet::enrichment::protocol::DmpRequest& dmp_request)
    noexcept
  {
    //std::string orig_keywords;

    if(dmp_request.has_data())
    {
      for(int i = 0; i < dmp_request.data().buckets_size(); ++i)
      {
        //orig_keywords += dmp_request.data().buckets(i).name();
        process_(keywords, dmp_request.data().buckets(i).name());
      }

      for(int i = 0; i < dmp_request.data().obuckets_size(); ++i)
      {
        //orig_keywords += dmp_request.data().obuckets(i).name();
        process_(keywords, dmp_request.data().obuckets(i).name());
      }
    }
  }

  void
  KeywordsProcessor::add_processor_(
    const String::SubString& name,
    const ParamProcessor_var& processor)
    noexcept
  {
    param_processors_[name].push_back(processor);
  }

  void
  KeywordsProcessor::process_(
    KeywordArray& keywords,
    const String::SubString& name_and_value)
    const noexcept
  {
    // div to name and value
    String::SubString::SizeType pos = name_and_value.find(':');
    String::SubString name;
    String::SubString value;

    if(pos == String::SubString::NPOS)
    {
      name = name_and_value;
    }
    else
    {
      name = name_and_value.substr(0, pos);
      if(pos < name_and_value.size())
      {
        value = name_and_value.substr(pos + 1);
      }
    }

    ParamProcessorMap::const_iterator param_it = param_processors_.find(name);

    KeywordArray sub_keywords;

    if(param_it != param_processors_.end())
    {
      for(auto processor_it = param_it->second.begin();
        processor_it != param_it->second.end();
        ++processor_it)
      {
        (*processor_it)->process(sub_keywords, value);
      }
    }
    else
    {
      sub_keywords.push_back(new AdServer::Commons::StringHolder(value.str()));
    }

    for(auto keyword_it = sub_keywords.begin(); keyword_it != sub_keywords.end(); ++keyword_it)
    {
      std::string res = "poadbb";
      res += name.str();
      res += 'x';
      res += (*keyword_it)->str();

      std::string simplified_res;
      String::case_change<String::Simplify>(res, simplified_res);
      std::replace(
        simplified_res.begin(),
        simplified_res.end(),
        ' ',
        'x');

      keywords.push_back(new AdServer::Commons::StringHolder(simplified_res));
    }
  }
}
}
