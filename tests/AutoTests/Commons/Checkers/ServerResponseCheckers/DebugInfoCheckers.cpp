
#include "DebugInfoCheckers.hpp"
#include <tests/AutoTests/Commons/Sequence.hpp>

namespace AutoTest
{
  // class ChannelsCheck
  ChannelsCheck::~ChannelsCheck() noexcept
  {}
  
  bool
  ChannelsCheck::check(bool throw_error)
    /*throw(eh::Exception)*/
  {
    if (exp_channels_.empty())
    {
      return true;
    }
    std::list<std::string> exp_channels;
    String::StringManip::SplitComma tokenizer(exp_channels_);
    String::SubString token;
    while (tokenizer.get_token(token))
    {
      String::StringManip::trim(token);
      try
      {
        exp_channels.push_back(test_->fetch_string(token.str()));
      }
      catch (const BaseUnit::InvalidArgument&)
      {
        exp_channels.push_back(token.str());
      }
    }
   
    return
      sequence_checker(
        exp_channels,
        got_channels_,
        check_type_).check(throw_error);
  }

  // class ChannelSequenceChecker
  ChannelSequenceChecker::ChannelSequenceChecker(
    BaseUnit* test,
    const AdClient& client,
    const NSLookupRequest& request,
    const std::string& expected,
    Member member,
    SequenceCheckerEnum check_type) :
    test_(test),
    client_(client),
    request_(request),
    expected_(expected),
    member_(member),
    check_type_(check_type)
  {}

  ChannelSequenceChecker::~ChannelSequenceChecker() noexcept
  {}

  bool
  ChannelSequenceChecker::check(bool throw_error)
    /*throw(eh::Exception)*/
  {
     client_.process_request(request_);

     return
       ChannelsCheck(
         test_,
         expected_.c_str(),
         client_.debug_info.*(member_),
         check_type_).check(throw_error);
  }

  // class MatchChecker
  MatchChecker::MatchChecker(
    BaseUnit* test,
    const NSLookupRequest& request,
    const std::string& expected,
    SequenceCheckerEnum check_type) 
    : ChannelSequenceChecker(
        test,
        AdClient::create_user(test),
        request,
        expected,
        &DebugInfo::DebugInfo::trigger_channels,
        check_type)
  {}

  // class HistoryChecker
  HistoryChecker::HistoryChecker(
    BaseUnit* test,
    const AdClient& client,
    const NSLookupRequest& request,
    const std::string& expected,
    SequenceCheckerEnum check_type) 
    : ChannelSequenceChecker(
        test,
        client,
        request,
        expected,
        &DebugInfo::DebugInfo::history_channels,
        check_type)
  {}
 
  // class SelectedCreativesCCID
  SelectedCreativesCCID::SelectedCreativesCCID(
    AdClient& client)
    : SelectedCreativesSimpleSlice(
        client.debug_info.selected_creatives,
        &DebugInfo::SelectedCreative::ccid)
  {}

  // class SelectedCreativesActualCPC
  SelectedCreativesActualCPC::SelectedCreativesActualCPC(
    AdClient& client)
    : SelectedCreativesMoneySlice(
        client.debug_info.selected_creatives,
        &DebugInfo::SelectedCreative::actual_cpc)
  {}

  // class SelectedCreativesImpRevenue
  SelectedCreativesImpRevenue::SelectedCreativesImpRevenue(
    AdClient& client)
    : SelectedCreativesMoneySlice(
        client.debug_info.selected_creatives,
        &DebugInfo::SelectedCreative::imp_revenue_value)
  {}

  // class SelectedCreativesChecker
  SelectedCreativesChecker::SelectedCreativesChecker(
    BaseUnit* test,
    const AdClient& client,
    const NSLookupRequest& request,
    const char* expected_ccids) :
    client_(client),
    request_(request)
  {
    if (expected_ccids)
    {
      test->fetch_objects(
        std::inserter(
          expected_ccids_,
          expected_ccids_.begin()),
        expected_ccids);
    }
  }

  SelectedCreativesChecker::~SelectedCreativesChecker()
    noexcept
  {}
    
  bool
  SelectedCreativesChecker::check(
    bool throw_error)
    /*throw(eh::Exception)*/
  {
    client_.process_request(request_);

    bool check =
      AutoTest::equal_seq(
        expected_ccids_,
        SelectedCreativesCCID(client_));
    
    if (!check && throw_error)
    {
      Stream::Error err;
      err << AutoTest::seq_to_str(expected_ccids_) << " !=  " <<
        AutoTest::seq_to_str(SelectedCreativesCCID(client_));
      throw AutoTest::CheckFailed(err);
    }

    return check;
  }

  
  // class SpecialEffectsChecker
  const char* SpecialEffectsChecker::S_E_STR[SpecialEffectsChecker::S_E_SIZE] =
  {
    "NO ADV",
    "NO TRACK",
    "ADV",
    "TRACK"
  };
      
  SpecialEffectsChecker::~SpecialEffectsChecker() noexcept
  {}

  bool
  SpecialEffectsChecker::check(
    bool throw_error) /*throw(eh::Exception)*/
  {
    bool result = SelectedCreativeChecker::check(throw_error);

    unsigned long got = 0;

    for (DebugInfo::StringListValue::const_iterator it = 
           client_.debug_info.special_channels_effects.begin();
         it != client_.debug_info.special_channels_effects.end(); ++it)
    {
      for (size_t i = 0; i < S_E_SIZE; ++i)
      {
        if (*it == S_E_STR[i])
        {
          got |= (1 << i);
        }
      }
    }
   
    if ( (got & special_effects_) != special_effects_ )
    {
      if (throw_error)
      {
        Stream::Error error;
        error << "'Special effects': expected [ " <<
          effects_to_str(special_effects_) << " ], but got [ " <<
          effects_to_str(got) << " ]";
        throw AutoTest::CheckFailed(error);
      }
      return false;
    }
    return result;
  }

  std::string
  SpecialEffectsChecker::effects_to_str(
    unsigned long special_effects)
  {
    std::string effects;
    for (size_t i = 0; i < S_E_SIZE; ++i)
    {
      if (special_effects & (1 << i))
      {
        effects += effects.empty()?"": ",";
        effects += S_E_STR[i];
      }
    }
    return effects;
  }
};
