#ifndef __AUTOTESTS_COMMONS_CHECKER_DEBUGINFOCHECKERS_HPP
#define __AUTOTESTS_COMMONS_CHECKERS_DEBUGINFOCHECKERS_HPP

#include <tests/AutoTests/Commons/Checkers/Checker.hpp>
#include <tests/AutoTests/Commons/Checkers/CommonCheckers.hpp>
#include <tests/AutoTests/Commons/AdClient.hpp>
#include <tests/AutoTests/Commons/Utils.hpp>
#include <tests/AutoTests/Commons/DebugInfo.hpp>

namespace AutoTest
{
  typedef SliceSeq<
    DebugInfo::SelectedCreativesList,
    const DebugInfo::SimpleValue> SelectedCreativesSimpleSlice;

  typedef SliceSeq<
    DebugInfo::SelectedCreativesList,
    const Money> SelectedCreativesMoneySlice;

  class SelectedCreativesCCID:
    public SelectedCreativesSimpleSlice
  {
  public:
    SelectedCreativesCCID(AdClient& client);
  };

  class SelectedCreativesActualCPC:
    public SelectedCreativesMoneySlice
  {
  public:
    SelectedCreativesActualCPC (AdClient& client);
  };

  class SelectedCreativesImpRevenue:
    public SelectedCreativesMoneySlice
  {
  public:
    SelectedCreativesImpRevenue(AdClient& client);
  };

  /**
   * @class DebugInfoChecker
   * @brief Checks that debug-info records has expected values
   *
   * Send NSLookupRequest to adserver and checks indicated record
   * of debug-info header of response for expcted value.
   */
  template<typename ValueType>
  class DebugInfoChecker : public AutoTest::Checker
  {
    typedef ValueType DebugInfo::DebugInfo::* MemberType;

  public:

    /**
     * @brief Constructor
     *
     * @param client AdClient object for requesting adserver
     * @param request request object with prepared params that will be sent to adserver
     * @param expected_value expected value for debug-info record
     * @param member debug-info record for check
     * @param member_name symbol name of debug-info record (will be used in logging)
     */
    template<typename ExpectedValueType>
    DebugInfoChecker(const AdClient& client,
                     const NSLookupRequest& request,
                     const ExpectedValueType& expected_value,
                     MemberType member,
                     const char* member_name = "debug info value");

    /**
     * @brief Destructor.
     */
    virtual ~DebugInfoChecker() noexcept;

    bool
    check(
      bool throw_error = true)
      /*throw(CheckFailed, eh::Exception)*/;

    AdClient&
    client() noexcept;

  protected:
    AdClient client_;
    NSLookupRequest request_;
    std::string expected_value_;
    MemberType member_;
    const char* member_name_;
  };

  typedef DebugInfoChecker<const DebugInfo::SimpleValue> SimpleValueDebugInfoChecker;


  /**
   * @class SelectedCreativeChecker
   * @brief Checks ccid debug-info record
   *
   * Sends nslookup request and checks server response for expected creative
   */
  class SelectedCreativeChecker: public SimpleValueDebugInfoChecker
  {
    typedef SimpleValueDebugInfoChecker Base;
  public:

    /**
     * @brief Constructor
     *
     * @param client AdClient object for requesting adserver
     * @param request request object with prepared params that will be sent to adserver
     * @param expected_value expected cc_id
     */
    template<typename ExpectedValueType>
    SelectedCreativeChecker(
      const AdClient& client,
      const NSLookupRequest& request,
      const ExpectedValueType& expected_value);
  };

  /**
   * @class SelectedCreativesChecker
   * @brief Checks selected creatives
   *
   * Sends nslookup request and checks server response for expected creatives
   */
  class SelectedCreativesChecker: public AutoTest::Checker
  {
  public:

    /**
     * @brief Constructor
     *
     * @param test
     * @param client
     * @param adserver request
     * @param expected ccids
     */
    SelectedCreativesChecker(
      BaseUnit* test,
      const AdClient& client,
      const NSLookupRequest& request,
      const char* expected_ccids);

    /**
     * @brief Destructor.
     */
    virtual ~SelectedCreativesChecker() noexcept;
    
    /**
     * @brief Check.
     * @param throw on error flag.
     */
    bool
    check(bool throw_error = true)
      /*throw(eh::Exception)*/;
    
  private:
    AdClient client_; // client
    NSLookupRequest request_; // test request
    std::list<std::string> expected_ccids_; // expected ccids
  };

  /**
   * @class ChannelsCheck
   * @brief Check channels on client (user)
   */
  class ChannelsCheck : public AutoTest::Checker
  {
  public:
    /**
     * @brief Constructor.
     *
     * @param test.
     * @param got (DebugInfo) channels.
     * @param expected trigger channel names.
     * @param channels check type (entry, not entry, compare).
     */
    template <typename ChannelsList>
    ChannelsCheck(
      BaseUnit* test,
      const char* exp_channels,
      const ChannelsList& got_channels,
      SequenceCheckerEnum check_type = SCE_ENTRY);
    
    /**
     * @brief Destructor.
     */
    virtual ~ChannelsCheck() noexcept;
    
    /**
     * @brief Check.
     * @param throw on error flag.
     */
    bool check(bool throw_error = true)
      /*throw(eh::Exception)*/;
    
  private:
    BaseUnit* test_;                      // test
    std::string exp_channels_;            // expected channels names
    std::list<std::string> got_channels_; // got (DebugInfo) channels
    SequenceCheckerEnum check_type_;      // check type
  };

  /**
   * @class ChannelSequenceChecker
   * @brief Check channel sequence on client (user)
   */
  class ChannelSequenceChecker : public AutoTest::Checker
  {
  public:

    typedef DebugInfo::StringListValue DebugInfo::DebugInfo::* Member;

    /**
     * @brief Constructor.
     *
     * @param test.
     * @param testing request.
     * @param expected sequence.
     */
    ChannelSequenceChecker(
      BaseUnit* test,
      const AdClient& client,
      const NSLookupRequest& request,
      const std::string& expected,
      Member member,
      SequenceCheckerEnum check_type = SCE_ENTRY);

    /**
     * @brief Destructor.
     */
    virtual ~ChannelSequenceChecker() noexcept;

    /**
     * @brief Check.
     * @param throw on error flag.
     */
    bool
    check(bool throw_error = true)
      /*throw(eh::Exception)*/;

  protected:
    BaseUnit* test_;
    AdClient client_;
    NSLookupRequest request_;
    std::string expected_;
    Member member_;
    SequenceCheckerEnum check_type_;
  };


  /**
   * @class MatchChecker
   * @brief Check trigger channels matched on client (user)
   */
  class MatchChecker : public ChannelSequenceChecker
  {
  public:

    /**
     * @brief Constructor.
     *
     * @param test.
     * @param testing request.
     * @param expected trigger channel id.
     */
    MatchChecker(
      BaseUnit* test,
      const NSLookupRequest& request,
      const std::string& expected_channels,
      SequenceCheckerEnum check_type = SCE_ENTRY);
  };

  /**
   * @class HistoryChecker
   * @brief Check history channels matched on client (user)
   */
  class HistoryChecker : public ChannelSequenceChecker
  {
  public:

    /**
     * @brief Constructor.
     *
     * @param test.
     * @param testing request.
     * @param expected trigger channel id.
     */
    HistoryChecker(
      BaseUnit* test,
      const AdClient& client,
      const NSLookupRequest& request,
      const std::string& expected_channels,
      SequenceCheckerEnum check_type = SCE_ENTRY);
  };

    /**
   * @class SpecialEffectsChecker
   * @brief Check creative & channel special effects on client (user)
   */
  class SpecialEffectsChecker : public SelectedCreativeChecker
  {

    static const size_t S_E_SIZE = 4;
    static const char* S_E_STR[S_E_SIZE];
    
  public:
    enum SpecialEffects
    {
      SE_NO_ADV = 1,
      SE_NO_TRACK = 2,
      SE_ADV = 4,
      SE_TRACK = 8,
      SE_DEFAULT = SE_ADV | SE_TRACK
    };

    /**
     * @brief Constructor.
     *
     * @param client.
     * @param special effects flags.
     */
    template<typename ExpectedValueType>
    SpecialEffectsChecker(
      const AdClient& client,
      const NSLookupRequest& request,
      const ExpectedValueType& expected_value,
      unsigned long special_effects = 0);

    /**
     * @brief Destructor.
     */
    virtual ~SpecialEffectsChecker() noexcept;

    /**
     * @brief Check.
     * @param throw on error flag.
     * @return true - success check, false - fail.
     */
    virtual
    bool
    check(bool throw_error = true)
      /*throw(eh::Exception)*/;
   
  private:

    std::string
    effects_to_str(
      unsigned long special_effects);
    
    unsigned long special_effects_;
  };
} //namespace AutoTest

#include "DebugInfoCheckers.ipp"

#endif  // __AUTOTESTS_COMMONS_CHECKERS_DEBUGINFOCHECKERS_HPP
