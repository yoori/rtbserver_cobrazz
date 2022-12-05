
#ifndef _AUTOTESTS_COMMONS_BAIDURESPONSECHECKER_HPP
#define _AUTOTESTS_COMMONS_BAIDURESPONSECHECKER_HPP

#include "ProtoBufResponse.hpp"
#include <Frontends/Modules/BiddingFrontend/baidu-bidding.pb.h>

namespace AutoTest
{
  class BaiduResponseChecker;

  namespace ProtoBuf
  {
    template <>
    struct BaseTraits<BaiduResponseChecker>
    {
      typedef ::Baidu::BidResponse Response;
      typedef ::Baidu::BidResponse_Ad Ad;
      
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
          BaiduResponseChecker& checker,
          const Expected& other);
        
        /**
         * @brief Add creative_id to expected.
         * @param creative_id.
         *
         * For static creative (dsp returns metadata, BES advertising assembled), creative_id
         * As a creative ID, uniquely identifies each metadata creative (currently only supports one idea).
         * For dynamic and creative, creative_id uniquely identifies the html snippet.
         *  BES requirements for dynamic creation
         * Italian materials and win notice url, etc. for review. Creative same same buyer's id 
         * Only be audited once, after the adoption of no audit.
         *   buyer should ensure that contain the same ideas and materials 
         * The html_snippet the same snippet id, to avoid duplication of audits.
         */
        Expected&
        creative_id(
          unsigned long val);
        
        /**
         * @brief Add target_url to expected.
         * @param target_url.
         *
         * Creative click url. When the response contains multiple ideas, creativity of each click
         * Url order should be consistent with the idea of order in the html snippet. BES will be the order of
         * Replace click url's.
         * If the order is not correct, click on the statistics will lead to bias.
         * Dynamic creative needs to be filled only
         */
        Expected&
        target_url(
          const std::string& val);
        
        /**
         * @brief Add category to expected.
         * @param val Baidu advertiser ID.
         *
         * Advertisers id. Only dynamic and creative needs to be filled
         * Dynamic creativity requires an html snippet of all the ads belong to the same advertisers.
         * Dynamic creative needs to be filled only
         */
        Expected&
        advertiser_id(unsigned long val);

        /**
         * @brief Add category to expected.
         * @param val Baidu category id.
         */
        Expected&
        category(int val);

        /**
         * @brief Add category exist checker to expected.
         * @param exist or not.
         */
        Expected&
        category_exist(bool exist);
        
        /**
         * @brief Add type to expected.
         * @param val Baidu type id.
         */
        Expected&
        type(int val);

        /**
         * @brief Add type exist checker to expected.
         * @param exist or not.
         */
        Expected&
        type_exist(bool exist);
     
      private:
        
        ExpValue<unsigned long> creative_id_;
        ExpValue< std::list<std::string> > target_url_;
        ExpValue<unsigned long> advertiser_id_;
        ExpValue<int> category_;
        ExpValue<int> type_;
        ExpValue<bool> category_exist_;
        ExpValue<bool> type_exist_;
      };
    };
  }

  /**
   * @class BaiduResponseChecker
   * @brief Baidu response checker.
   */
  class BaiduResponseChecker:
    public ProtoBuf::AdChecker<BaiduResponseChecker>
  {
    typedef ProtoBuf::AdChecker<BaiduResponseChecker> Base;

  public:

    /**
     * @brief Constructor.
     *
     * @param client (user).
     * @param response expectation.
     * @param checked response creative.
     */
    BaiduResponseChecker(
      const AdClient& client,
      const Expected& expected,
      size_t creative_num = 0);

    /**
     * @brief Destructor.
     */
    virtual ~BaiduResponseChecker() noexcept;

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
    


#endif  // _AUTOTESTS_COMMONS_BAIDURESPONSECHECKER_HPP
