
#ifndef _AUTOTESTS_COMMONS_TANXRESPONSECHECKER_HPP
#define _AUTOTESTS_COMMONS_TANXRESPONSECHECKER_HPP

#include "ProtoBufResponse.hpp"
#include <Frontends/Modules/BiddingFrontend/tanx-bidding.pb.h>

namespace AutoTest
{

  class TanxResponseChecker;
  
  namespace ProtoBuf
  {
    template <>
    struct BaseTraits<TanxResponseChecker>
    {
      typedef ::Tanx::BidResponse Response;
      typedef ::Tanx::BidResponse_Ads Ad;

      /**
       * @class Expected
       * @brief Tanx response expected.
       */
      class Expected
      {
        typedef ProtoBuf::ExpectedUtils<Ad> Utils;
        
      public:
        
        /**
         * @brief Default constructor.
         */
        Expected();
        
        /**
         * @brief Copy constructor.
         */
        Expected(
          TanxResponseChecker& checker,
          const Expected& other);
        
        /**
         * @brief Add creative_id to expected.
         *
         * @param creative_id.
         */
        Expected&
        creative_id(
          const std::string& val);
        
        /**
         * @brief Add destination_url to expected.
         *
         * @param destination_url.
         */
        Expected&
        destination_url(
          const std::string& val);
        
        /**
         * @brief Add ad_bid_count_idx to expected.
         *
         * @param ad_bid_count_idx.
         */
        Expected&
        ad_bid_count_idx(
          unsigned long val);
        
        /**
         * @brief Add category to expected.
         * @param val TanX category id.
         */
        Expected&
        category(unsigned long val);
        
        /**
         * @brief Add creative type to expected.
         * @param val TanX creative type id.
         */
        Expected&
        creative_type(unsigned long val);
        
        /**
         * @brief Add creative type to expected.
         * @param val TanX creative type id.
         */
        Expected&
        max_cpm_price(Money val);

        /**
         * @brief Add category exist checker to expected.
         * @param exist or not.
         */
        Expected&
        category_checked();
        
        /**
         * @brief Add creative type exist checker to expected.
         * @param exist or not.
         */
        Expected&
        creative_type_checked();
        
        ExpValue<std::string> creative_id_;
        ExpValue< std::list<std::string> > destination_url_;
        ExpValue<unsigned long> ad_bid_count_idx_;
        ExpValue< std::list<unsigned long> > category_;
        ExpValue< std::list<unsigned long> > creative_type_;
        ExpValue<AutoTest::Money> max_cpm_price_;
      };
    };
  }
  
  /**
   * @class TanxResponseChecker
   * @brief Tanx response checker.
   */
  class TanxResponseChecker:
    public ProtoBuf::AdChecker<TanxResponseChecker>
  {

    typedef ProtoBuf::AdChecker<TanxResponseChecker> Base;
    
  public:

    /**
     * @brief Constructor.
     *
     * @param client (user).
     * @param response expectation.
     * @param checked response creative.
     */
    TanxResponseChecker(
      const AdClient& client,
      const Expected& expected,
      size_t creative_num = 0);

    /**
     * @brief Destructor.
     */
    virtual ~TanxResponseChecker() noexcept;

    /**
     * @brief Get protobuf Ad-message
     * @return A Message contains the advertising
     */
    const Ad& ad() const;

    /**
     * @brief Get response Ad count
     * @return advertising size
     */
    virtual
    size_type ad_size() const;
  };
}

#endif  // _AUTOTESTS_COMMONS_TANXRESPONSECHECKER_HPP
