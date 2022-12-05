#include "ChannelStatusTest.hpp"

REFLECT_UNIT(ChannelStatusTest) (
  "GranularUpdate",
  AUTO_TEST_FAST | AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::SimpleChannelChecker BChannelChecker;
  typedef AutoTest::ExpressionChannelChecker EChannelChecker;
  typedef AutoTest::TriggerChecker TriggerChecker;
  typedef AutoTest::WaitChecker<TriggerChecker> WaitTriggerChecker;

  void update_channel_status( 	 
    DB::IConn& pq_conn) 	 
  {
    ORM::SerializeQueryManager::instance().execute( 	 
      pq_conn, 	 
      pq_conn.get_query( 	 
        "select displaystatus.update_channel_status_by_stats();")); 
  }

};

template<typename ChannelChecker>
void ChannelStatusTest::ChannelStatusChecker<ChannelChecker>::init_()
{}

template<>
void ChannelStatusTest::ChannelStatusChecker<EChannelChecker>::init_()
{
  AutoTest::ORM::update_display_status(
    test_, "CHANNEL", test_->fetch_int(prefix_ + "/CHANNEL_ID"));
}

template<typename ChannelChecker>
ChannelStatusTest::ChannelStatusChecker<ChannelChecker>::ChannelStatusChecker(
  BaseUnit* test,
  const std::string& prefix,
  const std::string& status,
  bool wait,
  bool update_stats) :
  test_(test),
  prefix_(prefix),
  status_(status),
  wait_(wait),
  update_stats_(update_stats)
{
  init_();
}

template<typename ChannelChecker>
ChannelStatusTest::ChannelStatusChecker<ChannelChecker>::~ChannelStatusChecker()
  noexcept
{}

template<typename ChannelChecker>
bool
ChannelStatusTest::ChannelStatusChecker<ChannelChecker>::request_check_(
  bool throw_error) /*throw(AutoTest::CheckFailed, eh::Exception)*/
{
  AutoTest::MatchChecker checker(
    test_,
    NSLookupRequest().
      referer_kw(test_->fetch_string(prefix_ + "/TRIGGERS")),
    test_->fetch_string(prefix_ + "/CHANNEL_ID") + "P",
    status_ != "A" && status_ != "W"?
      AutoTest::SCE_NOT_ENTRY: AutoTest::SCE_ENTRY);

  // The following situation may occur:
  // CampaignServer loaded new status for channel,
  // but ChannelServer had not loaded it yet,
  // so we must wait some time to get channels matched.
  return (wait_ ? AutoTest::wait_checker(checker).check(throw_error)
                : checker.check(throw_error));
}

template<>
bool
ChannelStatusTest::ChannelStatusChecker<EChannelChecker>::request_check_(
  bool throw_error) /*throw(AutoTest::CheckFailed, eh::Exception)*/
{

  AutoTest::SelectedCreativeChecker checker(
    AdClient::create_user(test_),
    NSLookupRequest().
      referer_kw(test_->fetch_string(prefix_ + "/TRIGGERS")).
      tid(test_->fetch_int(prefix_ + "/TAG_ID")),
    status_ == "A" ? test_->fetch_string(prefix_ + "/CC_ID") : "0");

  EChannelChecker targeting_channel_chekcer(
    test_,
    test_->fetch_int(prefix_ + "/TARGETING_CHANNEL_ID"),
    EChannelChecker::Expected().status("A"));

  return (wait_ ? AutoTest::wait_checker(checker).check(throw_error)
                : checker.check(throw_error))
          && targeting_channel_chekcer.check(throw_error);
}

template<typename ChannelChecker>
bool
ChannelStatusTest::ChannelStatusChecker<ChannelChecker>::check(bool throw_error)
  /*throw(AutoTest::CheckFailed, eh::Exception)*/
{
  unsigned long channel_id = test_->fetch_int(prefix_ + "/CHANNEL_ID");

  if (update_stats_)
  {
    DB::Conn pq_conn(test_->open_pq());
    update_channel_status(pq_conn);
    AutoTest::ORM::update_display_status(
      test_, "CHANNEL", channel_id);
  }

 
  ChannelChecker checker(
    test_,
    channel_id,
    typename ChannelChecker::Expected().
      channel_id(channel_id).
      status(status_ == "D" ? AutoTest::ANY : status_),
    status_ == "D" ? AutoTest::AEC_NOT_EXISTS : AutoTest::AEC_EXISTS );

  return (wait_ ? AutoTest::wait_checker(checker).check(throw_error)
                : checker.check(throw_error))
          && request_check_(throw_error);
}

ChannelStatusTest::ReachChannelThresholdChecker::ReachChannelThresholdChecker(
  DB::IConn& connection,
  unsigned long channel_id,
  int threshold_value) :
  conn_(connection),
  stats_(Stats::Key().channel_id(channel_id)),
  threshold_value_(threshold_value)
{}

ChannelStatusTest::ReachChannelThresholdChecker::~ReachChannelThresholdChecker()
  noexcept
{}

bool
ChannelStatusTest::ReachChannelThresholdChecker::check(bool throw_error)
  /*throw(AutoTest::CheckFailed, eh::Exception)*/
{

  stats_.select(conn_);
  AutoTest::Logger::thlog().stream(AutoTest::Logger::INFO, "ReachChannelThresholdChecker") << "total user: " <<
    stats_.total_users();
  if (stats_.total_users() > threshold_value_)
  { return true; }

  if (throw_error)
  {
    Stream::Error ostr;
    ostr << "Channel '" << stats_.key().channel_id()
      << "' hasn't reached the threshold value: "
      << "got unique users number (" << stats_.total_users()
      << ") less than threshold value (" << threshold_value_ << ").";
    throw AutoTest::CheckFailed(ostr);
  }
  return false;
}

void
ChannelStatusTest::set_up()
{
  add_descr_phrase("Setup");
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_ALL, STE_CAMPAIGN_SERVER)),
    "CampaignServer must set in the XML configuration file");
}

void
ChannelStatusTest::pre_condition()
{
  add_descr_phrase("Sync channel stats between Postgre and ORA");

  update_channel_status(pq_conn_);
}

bool
ChannelStatusTest::run()
{
  {
    std::string description("Check loading of simple channels");
    add_descr_phrase(description);

    String::StringManip::SplitComma tokenizer(fetch_string("SimpleChannelsStaticCases"));
    String::SubString token;
    while (tokenizer.get_token(token))
    {
      String::StringManip::trim(token);
      NOSTOP_FAIL_CONTEXT(
        ChannelStatusChecker<>(
          this,
          token.str(),
          fetch_string(token.str() + "/STATUS"),
          false).check(),
        description + ". Checking expected status for '" +
        token.str() + "' channel");
    }
  }

  {
    std::string description("Check loading of expression channels");
    add_descr_phrase(description);

    String::StringManip::SplitComma tokenizer(
      fetch_string("ExpressionChannelsStaticCases"));
    String::SubString token;
    while (tokenizer.get_token(token))
    {
      String::StringManip::trim(token);
      NOSTOP_FAIL_CONTEXT(
        ChannelStatusChecker<EChannelChecker>(
          this,
          token.str(),
          fetch_string(token.str() + "/STATUS"),
          true).check(),
        description + ". Checking expected status for '" +
          token.str() + "' channel");
    }
  }

  return true;
}

void
ChannelStatusTest::tear_down()
{
  add_descr_phrase("Tear down");

  int channels[] = {
    static_cast<int>(fetch_int("B-CT-W/CHANNEL_ID")),
    static_cast<int>(fetch_int("B-CT-A/CHANNEL_ID"))
  };

  for (size_t i = 0; i < sizeof(channels)/sizeof(*channels); ++i)
  {
    NOSTOP_FAIL_CONTEXT(
      DB::Query query(
        pq_conn_.query(
          "DELETE FROM CHANNELINVENTORY"
          " WHERE CHANNEL_ID = $1"));
      query.set(channels[i]);
      query.update();
      pq_conn_.commit(););
  }

  update_channel_status(pq_conn_);
}

void
ChannelStatusTest::channel_threshold_feature()
{
  std::string description("Channel threshold feature.");
  add_descr_phrase(description);

  unsigned long tag = fetch_int("Tag");

  const std::string channel_A_name = "B-CT-A";
  const std::string channel_W_name = "B-CT-W";

  ORM::ORMRestorer<ORM::PQ::Channel>* channel_A =
    create<ORM::PQ::Channel>(fetch_int(channel_A_name + "/CHANNEL_ID"));

  ORM::ORMRestorer<ORM::PQ::Channel>* channel_W =
    create<ORM::PQ::Channel>(fetch_int(channel_W_name + "/CHANNEL_ID"));

  ORM::ORMRestorer<ORM::PQ::CampaignCreativeGroup>* ccg_A =
    create<ORM::PQ::CampaignCreativeGroup>(fetch_int(channel_A_name + "/CCG_ID"));

  // Initial checks
  FAIL_CONTEXT(AutoTest::and_checker(
    ChannelStatusChecker<>(this, channel_W_name, "W", true, true),
    AutoTest::CampaignChecker(this, fetch_int(channel_W_name + "/CCG_ID"),
      AutoTest::CampaignChecker::Expected().status("I"))).and_if(
    AutoTest::SelectedCreativeChecker(AdClient::create_user(this),
      NSLookupRequest().referer_kw(fetch_string(channel_W_name + "/TRIGGERS")).
        tid(tag),
      "0")).check(),
    description + " Initial");

  FAIL_CONTEXT(AutoTest::and_checker(
    ChannelStatusChecker<>(this, channel_A_name, "A", true, true),
    AutoTest::SelectedCreativeChecker(AdClient::create_user(this),
      NSLookupRequest().referer_kw(fetch_string(channel_A_name + "/TRIGGERS")).
        tid(tag),
      fetch_string(channel_A_name + "/CC_ID"))).check(),
    description + " Initial");

  const std::string expression_W_and_A = "E-WandA";
  const std::string expression_W_or_A = "E-WorA";
  const std::string expression_W_andnot_A = "E-WandnotA";
  const std::string expression_A_or_I = "E-AorI";

  // update_channels in previous checkers have updated expression channels too
  // so we haven't call it in other checkers
  FAIL_CONTEXT(
    ChannelStatusChecker<EChannelChecker>(this,
      expression_W_and_A,
      fetch_string(expression_W_and_A + "/STATUS"),
      true).check(),
    description + " Initial");

  FAIL_CONTEXT(
    ChannelStatusChecker<EChannelChecker>(this,
      expression_W_or_A,
      fetch_string(expression_W_or_A + "/STATUS"),
      true).check(),
    description + " Initial");

  FAIL_CONTEXT(
    ChannelStatusChecker<EChannelChecker>(this,
      expression_W_andnot_A,
      fetch_string(expression_W_andnot_A + "/STATUS"),
      true).check(),
    description + " Initial");

  FAIL_CONTEXT(
    ChannelStatusChecker<EChannelChecker>(this,
      expression_A_or_I,
      fetch_string(expression_A_or_I + "/STATUS"),
      true).check(),
    description + " Initial");

  // Changes in DB
  channel_A->country_code = "GB";
  channel_A->language = "en";
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      channel_A->update()),
    "updating channel.country_code");

    FAIL_CONTEXT(AutoTest::predicate_checker(
      ccg_A->set_display_status(AutoTest::ORM::DS_NOT_LIVE_BY_CHANNEL_TARGET)),
    "updating ccg.display_status");

  // For GB high_channel_threshold = 20
  // Five requests have been processed in initial check
  NSLookupRequest request;
  request.referer_kw = fetch_string(channel_W_name + "/TRIGGERS");
  request.tid = tag;
  for (size_t i = 0; i < 15; ++i)
  {
    AdClient client(AdClient::create_user(this));
    client.process_request(request);

    FAIL_CONTEXT(
      AutoTest::entry_checker(
        strof(channel_W->id()) + "P",
        client.debug_info.trigger_channels).check(),
      "server must match expected channels");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.debug_info.selected_creatives.empty()),
      "server must return empty ccid");
  }

  ADD_WAIT_CHECKER(
    description + " Channel below threshold",
    AutoTest::and_checker(
      ChannelStatusChecker<>(this, channel_A_name, "W", true, true),
      AutoTest::CampaignChecker(this, fetch_int(channel_A_name + "/CCG_ID"),
         AutoTest::CampaignChecker::Expected().status("I").channels("NULL"))).and_if(
      AutoTest::SelectedCreativeChecker(AdClient::create_user(this),
        NSLookupRequest().
          referer_kw(fetch_string(channel_A_name + "/TRIGGERS")).
          tid(tag),
        "0")));

  ADD_WAIT_CHECKER(description + " Channel above threshold",
    AutoTest::and_checker(
       ReachChannelThresholdChecker(pq_conn_, channel_W->id(), 20),
      ChannelStatusChecker<>(this, channel_W_name, "A", true, true)).and_if(
        AutoTest::CampaignChecker(this, fetch_int(channel_W_name + "/CCG_ID"),
          AutoTest::CampaignChecker::Expected().status("A"))).and_if(
      AutoTest::SelectedCreativeChecker(AdClient::create_user(this),
        NSLookupRequest().
          referer_kw(fetch_string(channel_W_name + "/TRIGGERS")).
          tid(tag), 
        fetch_string(channel_W_name + "/CC_ID"))));

  // Status changed also for some expression channels
  add_checker(description + "expression status changed to 'A'",
    ChannelStatusChecker<EChannelChecker>(this,
      expression_W_and_A, "A", true));

  add_checker(description + "expression status without changes",
    ChannelStatusChecker<EChannelChecker>(this,
      expression_W_or_A, "A", true));

  add_checker(description + "expression status changed to 'A'",
    ChannelStatusChecker<EChannelChecker>(this,
      expression_W_andnot_A, "A", true));

  add_checker(description + "expression status changed to 'I'",
    ChannelStatusChecker<EChannelChecker>(this,
      expression_A_or_I, "I", true));

}

void
ChannelStatusTest::change_channel_status()
{
  std::string description("Change channel status.");
  add_descr_phrase(description);

  const std::string channel_A_name("B-A-var");
  const std::string channel_I_name("B-I-var");
  const std::string channel_D_name("B-D-var");
  const std::string channel_E_name("B-E-var");

  ORM::ORMRestorer<ORM::PQ::Channel>* channel_A =
    create<ORM::PQ::Channel>(fetch_int(channel_A_name + "/CHANNEL_ID"));

  ORM::ORMRestorer<ORM::PQ::Channel>* channel_I =
    create<ORM::PQ::Channel>(fetch_int(channel_I_name + "/CHANNEL_ID"));

  ORM::ORMRestorer<ORM::PQ::Channel>* channel_D =
    create<ORM::PQ::Channel>(fetch_int(channel_D_name + "/CHANNEL_ID"));

  ORM::ORMRestorer<ORM::PQ::Channel>* channel_E =
    create<ORM::PQ::Channel>(fetch_int(channel_E_name + "/CHANNEL_ID"));

  // initial check
  FAIL_CONTEXT(AutoTest::and_checker(
    BChannelChecker(this, channel_A->id(),
      BChannelChecker::Expected().
        channel_id(channel_A->id()).status("A")),
    BChannelChecker(this, channel_I->id(),
      BChannelChecker::Expected().
        channel_id(channel_I->id()).status("I"))).and_if(
    BChannelChecker(this, channel_D->id(),
      BChannelChecker::Expected().
        channel_id(channel_D->id()), AutoTest::AEC_NOT_EXISTS)).and_if(
    BChannelChecker(this, channel_E->id(),
      BChannelChecker::Expected().
        channel_id(channel_E->id()).status("A"))).check(),
    description + " Initial");

  // Changes in DB
  channel_A->status = "I";
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      channel_A->set_display_status(ORM::DS_INACTIVE)),
    "updating channel.status");

  channel_I->status = "A";
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      channel_I->set_display_status(ORM::DS_LIVE)),
    "updating channel.status");

  channel_D->status = "A";
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      channel_D->set_display_status(ORM::DS_LIVE)),
    "updating channel.status");

  channel_E->status = "D";
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      channel_E->set_display_status(ORM::DS_DELETED)),
    "updating channel.status");

  // Checks
  add_checker(description + "Deactivating channel",
    ChannelStatusChecker<>(this, channel_A_name, "I", true));

  add_checker(description + "Activating channel",
    ChannelStatusChecker<>(this, channel_I_name, "A", true));

  add_checker(description + "Activating channel",
    ChannelStatusChecker<>(this, channel_D_name, "A", true));

  add_checker(description + "Deleting channel",
    ChannelStatusChecker<>(this, channel_E_name, "D", true));

  // Status changed also for some expression channels
  add_checker(description + "Deactivating channel with expression 'A'",
    ChannelStatusChecker<EChannelChecker>(this, "E-A", "I", true));

  add_checker(description + "Activating channel with expression 'I'",
    ChannelStatusChecker<EChannelChecker>(this, "E-I", "A", true));

  add_checker(description + "Activating channel with expression 'D'",
    ChannelStatusChecker<EChannelChecker>(this, "E-D", "A", true));

  add_checker(description + "Activating channel with expression 'A&I'",
    ChannelStatusChecker<EChannelChecker>(this, "E-AandI", "A", true));

  add_checker(description + "Deactivating channel with expression 'A|D'",
    ChannelStatusChecker<EChannelChecker>(this, "E-AorD", "I", true));

  add_checker(description + "Activating channel with expression 'A^D'",
    ChannelStatusChecker<EChannelChecker>(this, "E-AandnotD", "A", true));

  add_checker(description + "Activating channel with expression 'A^I'",
    ChannelStatusChecker<EChannelChecker>(this, "E-AandnotI", "A", true));

  add_checker(description + "Deactivating channel with expression '(A|D)&A'",
    ChannelStatusChecker<EChannelChecker>(this, "E-{AorD}andA", "I", true));

  add_checker(description + "Channel with expression '(A^I)andA' without changes",
    ChannelStatusChecker<EChannelChecker>(this, "E-{AandnotI}andA", "I", true));

  add_checker(description + "Deactivating channel with expression '(A&I)|A'",
    ChannelStatusChecker<EChannelChecker>(this, "E-{AandI}orA", "I", true));

  add_checker(description + "Channel with expression '(A^I)|A' without changes",
    ChannelStatusChecker<EChannelChecker>(this, "E-{AandnotI}orA", "A", true));

  add_checker(description + "Deactivating channel with expression '(A|I)^A'",
    ChannelStatusChecker<EChannelChecker>(this, "E-{AorI}andnotA", "I", true));

  add_checker(description + "Activating channel with expression '(A&D)^A'",
    ChannelStatusChecker<EChannelChecker>(this, "E-{AandD}andnotA", "A", true));

  add_checker(description + "Deactivating channel with expression 'eA|I'",
    ChannelStatusChecker<EChannelChecker>(this, "E-eAorI", "I", true));

  add_checker(description + "Activating channel with expression 'eA&eI'",
    ChannelStatusChecker<EChannelChecker>(this, "E-eAandeI", "A", true));
}

void
ChannelStatusTest::post_condition()
{
  AutoTest::Logger::thlog().stream(AutoTest::Logger::INFO, "ChannelStatusTest") << "Timeout:" << BaseUnit::timeout();
  change_channel_status();
  channel_threshold_feature();
  AutoTest::Logger::thlog().stream(AutoTest::Logger::INFO, "ChannelStatusTest") << "Timeout:" << BaseUnit::timeout();
}
