#ifndef AUTOTESTS_COMMONS_CHECKERS_DEBUGINFOCHECKERS_IPP
#define AUTOTESTS_COMMONS_CHECKERS_DEBUGINFOCHECKERS_IPP

namespace AutoTest
{
  // ChannelsCheck
  template <typename ChannelsList>
  ChannelsCheck::ChannelsCheck(
    BaseUnit* test,
    const char* exp_channels,
    const ChannelsList& got_channels,
    SequenceCheckerEnum check_type) :
    test_(test),
    exp_channels_(exp_channels? exp_channels: std::string()),
    got_channels_(got_channels.begin(), got_channels.end()),
    check_type_(check_type)
  {}

  // DebugInfoChecker
  template<typename ValueType>
  template<typename ExpectedValueType>
  DebugInfoChecker<ValueType>::DebugInfoChecker(
    const AdClient& client,
    const NSLookupRequest& request,
    const ExpectedValueType& expected_value,
    MemberType member,
    const char* member_name):
      client_(client),
      request_(request),
      member_(member)
  {
    expected_value_ = strof(expected_value);
    member_name_ = member_name;
  }

  template<typename ValueType>
  DebugInfoChecker<ValueType>::~DebugInfoChecker() noexcept
  {}

  template<typename ValueType>
  bool 
  DebugInfoChecker<ValueType>::check(bool throw_error)
    /*throw(CheckFailed, eh::Exception)*/
  {
    client_.process_request(request_);

    if (throw_error &&
      !equal(expected_value_, client_.debug_info.*member_))
    {
      Stream::Error ostr;
      ostr << member_name_ << " "
           << expected_value_ << " != " << client_.debug_info.*member_
           << " (expected != got)";
      throw AutoTest::CheckFailed(ostr);
    }
    return true;
  }

  template<typename ValueType>
  AdClient&
  DebugInfoChecker<ValueType>::client() noexcept
  {
    return client_;
  }

  template<typename ExpectedValueType>
  SelectedCreativeChecker::SelectedCreativeChecker(
    const AdClient& client,
    const NSLookupRequest& request,
    const ExpectedValueType& expected_value)
    : Base(client, request,  expected_value,
           &DebugInfo::DebugInfo::ccid, "selected creative")
  {}

  // SpecialEffectsChecker
  template<typename ExpectedValueType>
  SpecialEffectsChecker::SpecialEffectsChecker(
    const AdClient& client,
    const NSLookupRequest& request,
    const ExpectedValueType& expected_value,
    unsigned long special_effects)
    : SelectedCreativeChecker(client, request, expected_value),
      special_effects_(
        special_effects?
          special_effects: static_cast<unsigned long>(SE_DEFAULT))
  {}
}

#endif /*AUTOTESTS_COMMONS_CHECKERS_DEBUGINFOCHECKERS_IPP*/
