#include "CampaignCreativeUpdate.hpp"

REFLECT_UNIT(CampaignCreativeUpdate) (
  "GranularUpdate",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::CreativeAdmin CreativeAdmin;
  typedef AutoTest::CreativeChecker CreativeChecker;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::WaitChecker<CreativeChecker> CreativeWaitChecker;
  typedef GlobalConfig::ServiceList::const_iterator ServiceIterator_const;

  /**
   * @class ClientCreativeChecker
   * @brief Check creative returned
   */
  class ClientCreativeChecker : public AutoTest::Checker
  {
  public:

    /**
     * @brief Constructor.
     *
     * @param client.
     * @param testing request.
     * @param expected ccid.
     */
    ClientCreativeChecker(
      const AdClient& client,
      const NSLookupRequest& request,
      unsigned long ccid) :
      client_(client),
      request_(request),
      ccid_(ccid)
    { }

    /**
     * @brief Destructor.
     */
    virtual ~ClientCreativeChecker() noexcept
    { }

    /**
     * @brief Check.
     * @param throw on error flag.
     */
    bool
    check(bool throw_error)
      /*throw(eh::Exception)*/
    {
      client_.process_request(request_);
          
      if (throw_error &&
        !AutoTest::equal(strof(ccid_), client_.debug_info.ccid))
      {
        Stream::Error ostr;
        ostr << "'CCID' " <<
          ccid_ <<
          " != " <<  client_.debug_info.ccid <<
          " (expected != got)" ;
        throw AutoTest::CheckFailed(ostr);
      }
      return true;
    }

  private:
    AdClient client_;
    NSLookupRequest request_;
    unsigned long ccid_;
  };

  std::string size_regexp(
    const std::string id,
    const std::string name)
  {
    return
      std::string(
        "^\\[" + id + ",'" + name +
          "',\\d+,\\d+,\\d+,\\d+\\]$");
  }
}

void CampaignCreativeUpdate::set_up()
{
  add_descr_phrase("Setup.");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_ALL, STE_CAMPAIGN_MANAGER)),
    "CampaignManager must set in the XML configuration file");
}

void CampaignCreativeUpdate::tear_down()
{
  add_descr_phrase("Tear down.");
}

bool 
CampaignCreativeUpdate::run()
{
  update_option_value_();
  add_creative_();
  remove_creative_();
  update_creative_();
  
  return true;
}
 

void CampaignCreativeUpdate::add_creative_()
{
  std::string description("Add new creative.");
  add_descr_phrase(description);
  ORM::ORMRestorer<ORM::PQ::Creative>* creative =
    create<ORM::PQ::Creative>();

  creative->name = fetch_string("ADDCREATIVE/Name");
  creative->account = fetch_int("ADDCREATIVE/Account");
  creative->size = fetch_int("ADDCREATIVE/Size");
  creative->template_id  = fetch_int("ADDCREATIVE/Template");
  creative->status = "A";
  creative->qa_status = "A";
  creative->flags = 0;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      creative->insert()),
    description + 
      " Must insert creative");

  ORM::ORMRestorer<ORM::PQ::CampaignCreative>* cc =
    create<ORM::PQ::CampaignCreative>();

  cc->ccg = fetch_int("ADDCREATIVE/CCG");
  cc->creative = creative->id();
  cc->status = "A";
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      cc->insert()),
    description +     
      " Must insert CC");

  add_checker(
    description,
    CreativeWaitChecker(
      CreativeChecker(
        this,
        cc->id(),
        CreativeAdmin::Expected().
          ccid(cc->id()).
          campaign_id(fetch_int("ADDCREATIVE/CCG")).
          creative_format(fetch_string("ADDCREATIVE/TemplateName")).
          sizes(
            size_regexp(
              fetch_string("ADDCREATIVE/Size"),
              fetch_string("ADDCREATIVE/SizeName"))))));
}

void CampaignCreativeUpdate::remove_creative_()
{
  std::string description("Remove creative.");
  add_descr_phrase(description);

  unsigned int ccid = fetch_int("REMOVECREATIVE/CC");

  FAIL_CONTEXT(
   AutoTest::wait_checker(
     CreativeChecker(
       this,
       ccid,
       CreativeChecker::Expected().
         ccid(ccid))).check(),
   description + "Initial check");

  ORM::ORMRestorer<ORM::PQ::CampaignCreative>* cc =
    create<ORM::PQ::CampaignCreative>(ccid);

  cc->status = "D";
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      cc->update()),
    description + 
     "must update CC");

  add_checker(
    description,
    CreativeWaitChecker(
      CreativeChecker(
        this,
        ccid,
        CreativeAdmin::Expected(),
        AutoTest::AEC_NOT_EXISTS)));
}

void CampaignCreativeUpdate::update_creative_()
{
  std::string description("Update creative.");
  add_descr_phrase(description);

  unsigned long cc_id =
    fetch_int("UPDATECREATIVE/CC");

  unsigned long creative_id =
    fetch_int("UPDATECREATIVE/Creative");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      CreativeChecker(
        this,
        cc_id,
        CreativeChecker::Expected().
          ccid(cc_id).
          creative_format(
            fetch_string("UPDATECREATIVE/TemplateName")).
          sizes(
            size_regexp(
              fetch_string("UPDATECREATIVE/Size"),
              fetch_string("UPDATECREATIVE/SizeName"))))).check(),
    description + " Initial state");

  ORM::ORMRestorer<ORM::PQ::Creative>* creative =
    create<ORM::PQ::Creative>(creative_id);

  creative->size = fetch_int("UPDATECREATIVE/NewSize");
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      creative->update()),
    description +  " Must update creative");

  ORM::ORMRestorer<ORM::PQ::Creative_tagsize>* cr_size_link =
    create(
      ORM::PQ::Creative_tagsize(
        pq_conn_,
        creative_id,
        fetch_int("UPDATECREATIVE/Size")));

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      cr_size_link->delet()),
    description +  " Creative tag size link delete");

  ORM::ORMRestorer<ORM::PQ::Template>* ctemplate =
    create<ORM::PQ::Template>(fetch_int("UPDATECREATIVE/Template"));

  ctemplate->name =  fetch_string("UPDATECREATIVE/NewTemplateName");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      ctemplate->update()),
    description + " Must update template");
    
  add_checker(
    description,
    CreativeWaitChecker(
      CreativeChecker(
        this,
        cc_id,
        CreativeAdmin::Expected().
          ccid(cc_id).
          creative_format(
            fetch_string("UPDATECREATIVE/NewTemplateName")).
          sizes(
            size_regexp(
              fetch_string("UPDATECREATIVE/NewSize"),
              fetch_string("UPDATECREATIVE/NewSizeName"))))));
}


void CampaignCreativeUpdate::update_option_value_()
{
  std::string description("Update option value.");
  add_descr_phrase(description);

  unsigned int ccid = fetch_int("UPDATEOPTION/CC");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      CreativeChecker(
        this,
        ccid,
        CreativeChecker::Expected().
        ccid(ccid))).check(),
     description + " Initial check (CC present)");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      CreativeChecker(
        this,
        ccid,
        CreativeChecker::Expected().
          categories(
            "[0-9, ]*" + fetch_string("UPDATEOPTION/Category") +
              "[0-9, ]*"),
        AutoTest::AEC_NOT_EXISTS)).check(),
    description + " Initial check (exclusion tags category absent)");

  NSLookupRequest request;
  request.tid = fetch_string("UPDATEOPTION/Tag");
  request.referer_kw = fetch_string("UPDATEOPTION/Keyword");

  AdClient user(AdClient::create_user(this));
  user.process_request(request);
  user.repeat_request();

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      strof(ccid),
      user.debug_info.ccid).check(),
    description + 
      " Check creative");

  ORM::ORMRestorer<ORM::PQ::CreativeOptionValue>* option =
    create(
      ORM::PQ::CreativeOptionValue(
        pq_conn_,
        fetch_int("UPDATEOPTION/Creative"),
        fetch_int("UPDATEOPTION/Option")));
  
  option->value = fetch_string("UPDATEOPTION/ExclusionTag");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      option->update()),
    description + 
      "Must update CreativeOptionValue");

  add_checker(
    description + " Admin check",
    CreativeWaitChecker(
      CreativeChecker(
        this,
        ccid,
        CreativeAdmin::Expected().
          ccid(ccid).
          categories(
            "[0-9, ]*"  +  fetch_string("UPDATEOPTION/Category") +
              "[0-9, ]*"))));

  add_checker(
    description + " Client check",
    ClientCreativeChecker(
      user,
      request,
      0));
}
