
#ifndef _AUTOTEST__USERPROFILESEXCHANGECOMMON_
#define _AUTOTEST__USERPROFILESEXCHANGECOMMON_

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace UserProfilesExchange
  {

    // Alias for NSLookupRequest
    typedef AutoTest::NSLookupRequest  NSLookupRequest;

    /**
   * @class CheckWaitHistoryChannel
   * @brief History channel appearence checker
   *
   * This checker used for wait history channel appearance.
   * Also, it check response colo_id.
   */
    class CheckWaitHistoryChannel: public AutoTest::Checker
    {
      static const unsigned long SLEEP_TIME = 30; // 30 seconds between checks
    public:

      /**
       * @brief Constructor.
       *
       * @param client (user).
       * @param expected channel.
       * @param expected colo_id.
       * @param wait timeout.
       * @param checker request.
       */
      CheckWaitHistoryChannel (
          AutoTest::AdClient& client,
          unsigned long channel,
          const char* colo,
          unsigned long wait_time,
          const NSLookupRequest& request = NSLookupRequest());

      /**
       * @brief Constructor.
       *
       * @param client (user).
       * @param expected channels array.
       * @param channels array size.
       * @param expected colo_id.
       * @param wait timeout.
       * @param checker request.
       */
      CheckWaitHistoryChannel (
        AutoTest::AdClient& client,
        unsigned long channels[],
        size_t channel_size,
        const char* colo,
        unsigned long wait_time,
        const NSLookupRequest& request = NSLookupRequest());

     /**
      * @brief Destructor.
      */
      virtual ~CheckWaitHistoryChannel() noexcept;

      /**
      * @brief Check.
      * @param throw on error flag.
      */
      bool check(bool throw_error = true) /*throw(eh::Exception)*/;

    private:
      AutoTest::AdClient& client_;      // client(user)
      std::list<std::string> expected_; // expected channels list
      std::string colo_;                // expected colo
      unsigned long wait_time_;         // wait timeout
      std::string request_;             // checker request
    };
  }
}


#endif  // _AUTOTEST__USERPROFILESEXCHANGECOMMON_
