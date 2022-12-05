
#ifndef _AUTOTESTS_COMMONS_OPENRTBRESPONSECHECKER_HPP
#define _AUTOTESTS_COMMONS_OPENRTBRESPONSECHECKER_HPP

#include <tests/AutoTests/Commons/Checkers/Checker.hpp>
#include <tests/AutoTests/Commons/Checkers/CompositeCheckers.hpp>
#include <tests/AutoTests/Commons/Checkers/ExpectedValue.hpp>
#include <tests/AutoTests/Commons/AdClient.hpp>
#include <Commons/Gason.hpp>

namespace AutoTest
{

  /**
   * @class OpenRTBResponse
   * @brief OpenRTB response.
   */
  class OpenRTBResponse
  {
  public:

    /**
     * @class Bid
     * @brief 'seatbid/bid' content.
     */
    struct Bid
    {
      // bid id
      std::string id;
      // impression id
      std::string impid;
      // max price for impression
      Money price;
      // ads id
      unsigned long adid;
      // ad domain array
      std::list <std::string> adomain;
      // ad instance
      std::string adm;
      // ccid
      unsigned long cid;
      // crid
      std::string crid;
      // Creative visual categories
      ExpValue< std::list<unsigned long> > attr;
      // Creative content categories (OpenX extension)
      ExpValue< std::list<unsigned long> > ad_ox_cats;
      // Creative categories
      ExpValue< std::list<std::string> >cat;
      // Creative format
      ExpValue< unsigned long > fmt;

      std::string matching_ad_id;
      std::string nurl;
    };

    typedef std::list<Bid> Bids;

    /**
     * @brief Constructor.
     *
     * @param ad client.
     */
    OpenRTBResponse(
      const std::string& body);

    /**
     * @brief Destructor.
     */
    virtual ~OpenRTBResponse();


    /**
     * @brief Get bids.
     *
     * @return bids.
     */
    const Bids& bids() const;

    /**
     * @brief Get response id.
     *
     * @return id.
     */
    const std::string& id() const;

    /**
     * @brief Get response currency.
     *
     * @return currency (ISO-4217).
     */
    const std::string& currency() const;

    /**
     * @brief Get parse status.
     *
     * @return JSON parse status (ISO-4217).
     */
    JsonParseStatus status() const;

  private:

    /**
     * @brief Parse json bid::ext structure.
     *
     * @param bid::ext iterator.
     */
    void
    parse_ext_(
      JsonIterator iterator,
      Bid& bid);

    /**
     * @brief Parse json bid structure.
     *
     * @param bid iterator.
     */
    void
    parse_bid_(
      JsonIterator iterator);

    /**
     * @brief Parse json seatbid structure.
     *
     * @param seatbid iterator.
     */
    void
    parse_seatbid_(
      JsonIterator iterator);

    /**
     * @brief Parse json content.
     *
     * @param json root value.
     */
    void
    parse_(
      const JsonValue& value);

    std::string id_;
    std::string currency_;
    JsonParseStatus status_;
    Bids bids_;
  };

  // Bid int slice sequence
  typedef SliceSeq<
    OpenRTBResponse::Bids,
    unsigned long> OpenRTBIntSlice;


  /**
   * @class OpenRTBCid
   * @brief Helper class for check bid.cid.
   */
  class OpenRTBCid :
    public OpenRTBResponse,
    public OpenRTBIntSlice
  {
  public:
    /**
     * @brief Constructor.
     *
     * @param ad client.
     */
    OpenRTBCid(AdClient& client);

    /**
     * @brief Destructor.
     */
    virtual ~OpenRTBCid();
  };

  /**
   * @class OpenRTBResponseChecker
   * @brief OpenRTB response checker.
   */
  class OpenRTBResponseChecker : public Checker
  {
  public:
    class Expected
    {
    public:
      /**
       * @brief Default constructor.
       */
      Expected();
      
      /**
       * @brief Set expected Bid.id.
       *
       * @param Bid.id
       */
      Expected&
      id(const std::string& val);

      /**
       * @brief Set expected Bid.impid.
       *
       * @param Bid.impid
       */
      Expected&
      impid(const std::string& val);

      /**
       * @brief Set expected Bid.price.
       *
       * @param Bid.price
       */
      Expected&
      price(Money val);

      /**
       * @brief Set expected Bid.adid.
       *
       * @param Bid.adid
       */
      Expected&
      adid(unsigned long val);

      /**
       * @brief Set expected bid.crid
       *
       * @param bid.crid
       */
      Expected& crid(const std::string& val);

      /**
       * @brief Add Bid.adomain to expected.
       *
       * @param Bid.adomain
       */
      Expected&
      adomain(const std::string& val);

      /**
       * @brief Set expected Bid.adm.
       *
       * @param Bid.adm
       */
      Expected&
      adm(const std::string& val);

      
      /**
       * @brief Set expected Bid.nurl.
       *
       * @param Bid.nurl
       */
      Expected&
      nurl(const std::string& val);

      /**
       * @brief Set expected Bid.cid.
       *
       * @param Bid.cid
       */
      Expected&
      cid(unsigned long val);

      /**
       * @brief Set expected Bid.attr.
       *
       * @param Bid.attr
       */
      Expected&
      attr(unsigned long val);

      /**
       * @brief Set expected Bid.ad_ox_cats
       *
       * @param val value that will be added to expected ad_ox_cats
       */
      Expected&
      ad_ox_cats(unsigned long val);

      /**
       * @brief Add Bid.cat to expected.
       *
       * @param Bid.cat
       */
      Expected&
      cat(const std::string& val);


      /**
       * @brief Set expected Bid.fmt.
       *
       * @param Bid.fmt
       */
      Expected&
      fmt(unsigned long val);

      /**
       * @brief Set expected bid.ext.matching_ad_id
       *
       * @param val value that will be added to expected matching_ad_id
       */
      Expected&
      matching_ad_id(const std::string& val);

      /**
       * @brief Set expected bid.fmt existance
       *
       * @param val true id bid.fmt exist expected, otherwise - false
       */
      Expected&
      fmt_exist(bool value);

      /**
       * @brief Set expected bid.cat existance
       *
       * @param val true id bid.cat exist expected, otherwise - false
       */
      Expected&
      cat_exist(bool value);
      
      /**
       * @brief Set expected bid.cat existance
       *
       * @param val true id bid.cat exist expected, otherwise - false
       */
      Expected&
      cat_checked();

      /**
       * @brief Set expected bid.attr existance
       *
       * @param val true id bid.attr exist expected, otherwise - false
       */
      Expected&
      attr_exist(bool value);
      
      /**
       * @brief Set expected bid.attr existance
       *
       * @param val true id bid.attr exist expected, otherwise - false
       */
      Expected&
      attr_checked();

      /**
       * @brief Set expected bid.ad_ox_cats existance
       *
       * @param val true id bid.ad_ox_cats exist expected, otherwise - false
       */
      Expected&
      ad_ox_cats_exist(bool value);
      
      /**
       * @brief Set expected bid.ad_ox_cats existance
       *
       * @param val true id bid.ad_ox_cats exist expected, otherwise - false
       */
      Expected&
      ad_ox_cats_checked();
      
    private:

      friend class OpenRTBResponseChecker;
      
      ExpValue<std::string> id_;
      ExpValue<std::string> impid_;
      ExpValue<Money> price_;
      ExpValue<unsigned long> adid_;
      ExpValue<std::string> crid_;
      ExpValue< std::list<std::string> > adomain_;
      ExpValue<std::string> adm_;
      ExpValue<std::string> nurl_;
      ExpValue<unsigned long> cid_;
      ExpValue< std::list<unsigned long> > attr_;
      ExpValue< std::list<unsigned long> > ad_ox_cats_;
      ExpValue< std::list<std::string> > cat_;
      ExpValue< unsigned long > fmt_;
      ExpValue<std::string> matching_ad_id_;

      ExpValue<bool> fmt_exist_;
      ExpValue<bool> cat_exist_;
      ExpValue<bool> attr_exist_;
      ExpValue<bool> ad_ox_cats_exist_;
    };

    typedef std::list<Expected> ExpectedList;

    /**
     * @brief Constructor.
     *
     * @param ad client.
     * @param first expected.
     */
    OpenRTBResponseChecker(
      const AdClient& client,
      const Expected& expected);

    /**
     * @brief Constructor.
     *
     * @param ad client.
     * @param expected list.
     */
    OpenRTBResponseChecker(
      const AdClient& client,
      const ExpectedList& expected);

    /**
     * @brief Add expected to list.
     *
     * @param expected.
     */
    void add_expected(
      const Expected& expected);

    /**
     * @brief Get response bid
     */
    const OpenRTBResponse::Bid& bids(size_t index = 0) const;

    /**
     * @brief Destructor.
     */
    virtual ~OpenRTBResponseChecker() noexcept;

    /**
     * @brief Check.
     * @param throw on error flag.
     */
    bool
    check(
      bool throw_error = true)
      /*throw(eh::Exception)*/;

  private:
    OpenRTBResponse response_;
    ExpectedList expected_;
  };

}

# include "OpenRTBResponseChecker.ipp"

#endif  // _AUTOTESTS_COMMONS_OPENRTBRESPONSECHECKER_HPP
