
#include "ZeroCollectorTest.hpp"

REFLECT_UNIT(ZeroCollectorTest) (
  "Statistics",
  AUTO_TEST_SLOW);

namespace
{
  struct TestChannel
  {
    char type; // Channel.channel_type
    char status; // Channel.status
    char qa_status; // Channel.qa_status
    char ns;        // Channel.namespace
    unsigned int display_status; // Channel.display_status_id
    char expected; // Expected adserver_status
    bool collected;
  };

  // Country RU (channel threshold enabled)
  // use to emulate channels with 'W' status
  const TestChannel CHANNELS[] =
  {
    // Behavioral channels
    {'B', 'I', 'A', 'A', 4, 'I', false},
    {'B', 'D', 'D', 'A', 5, 'D', false},
    {'B', 'A', 'A', 'A', 8, 'W', true},
    {'B', 'A', 'A', 'A', 1, 'A', true},
    // Keyword channels
    {'K', 'A', 'A', 'K', 4, 'I', false},
    {'K', 'A', 'A', 'K', 1, 'A', true},
    // Expression channels
    {'E', 'I', 'A', 'A', 7, 'I', false},
    {'E', 'A', 'A', 'A', 8, 'W', true},
    {'E', 'A', 'A', 'A', 1, 'A', true},
    // Discover channels
    {'D', 'A', 'A', 'D', 1, 'A', false},
    // Targeting channels
    {'T', 'A', 'A', 'T', 1, 'A', true},
    {'T', 'E', 'A', 'T', 1, 'A', true},
    {'T', 'I', 'A', 'T', 7, 'I', false}
  };

  typedef AutoTest::ExpressionChannelChecker ExpressionChannelChecker;

    /**
   * @class ZeroCIChecker
   * @brief Check ChannelInventory zero collector.
   */
  class ZeroCIChecker : public AutoTest::Checker
  {

    typedef AutoTest::ORM::PQ::ChannelInventory ChannelInventory;
    typedef AutoTest::DBC::IConn Conn;
    typedef AutoTest::ORM::PQ::Channel Channel;
    
  public:
    /**
     * @brief Constructor.
     *
     * @param connection.
     * @param channel.
     * @param adserver status.
     * @param collected sign.
     */
    ZeroCIChecker(
      Conn& conn,
      const Channel& channel,
      char adserver_status,
      bool collected)
      : conn_(conn),
        channel_(channel),
        adserver_status_(adserver_status),
        collected_(collected)
    { }

    /**
     * @brief Destructor.
     */
    virtual ~ZeroCIChecker() noexcept
    { }

    /**
     * @brief Check.
     * @param throw on error flag.
     */
    bool check(bool throw_on_error = true) /*throw(eh::Exception)*/
    {
      Generics::ExtendedTime today = AutoTest::Time().get_gm_time().get_date();

      ChannelInventory ci(conn_);
      bool result = collected_?
        ci.select(today, channel_.id(), 1) &&
        ci.total_user_count.value() == 0:
        !ci.select(today, channel_.id(), 1);
      if (!result)
      {
        if (throw_on_error)
        {
          Stream::Error ostr;
          ostr << "ChannelInventory.Channel (id = " << channel_.id() <<
            ", type = " << channel_.type.value() <<
            ", adserver_status = " << adserver_status_ << ")";
          if (collected_)
            ostr << " not found";
          else
              ostr << " is unexpeced";
          throw AutoTest::CheckFailed(ostr);
        }
        return false;
      }
      return true;
    }

  private:
    Conn& conn_;
    const Channel& channel_;
    char adserver_status_;
    bool collected_;
  };

}

void
ZeroCollectorTest::set_up()
{
  add_descr_phrase("Setup.");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_ALL, STE_CAMPAIGN_SERVER)),
    "Need CampaignServer config");

  active_channels.push_back(fetch_int("Channel/GN"));
  wait_channels.push_back(fetch_int("Channel/RU"));
  
  std::list<int>::iterator active_channel = active_channels.begin();
  std::list<int>::iterator wait_channel = wait_channels.begin();
  
  // Create channels
  for (size_t i = 0; i < countof(CHANNELS); ++i)
  {
    Channel c(pq_conn_);
    if (CHANNELS[i].type != 'T')
    {
      c.account = fetch_int("Account");
    }
    c.name = fetch_string("Prefix") + strof(i+1);
    c.type = std::string(1, CHANNELS[i].type);
    c.status =
      CHANNELS[i].status == 'E'? "A":
        std::string(1, CHANNELS[i].status);
    c.qa_status = std::string(1, CHANNELS[i].qa_status);
    c.display_status_id = CHANNELS[i].display_status;
    if (CHANNELS[i].type != 'T')
    {
      c.country_code =
        CHANNELS[i].expected == 'W'?
          "RU": "GN";
      c.language = 
        CHANNELS[i].expected == 'W'?
          "ru": "gn";
    }

    if (CHANNELS[i].type == 'K')
    {
      c.trigger_type = "S";
    }

    if (CHANNELS[i].type == 'B' ||
        CHANNELS[i].type == 'D')
    {
      c.triggers_status = "A";
      c.distinct_url_triggers_count = 1;
    }

    c.channel_namespace =
       std::string(1, CHANNELS[i].ns);
    c.visibility =
      CHANNELS[i].type == 'K' || CHANNELS[i].type == 'D' ||
        CHANNELS[i].expected == 'W'? "PRI": "PUB";
    if (CHANNELS[i].type == 'E' || CHANNELS[i].type == 'T')
    {
      c.language.null();
      c.expression =
        CHANNELS[i].expected == 'W'?
          strof(*(wait_channel++)):
             strof(*(active_channel++));
    }
    if (CHANNELS[i].type == 'D')
    {
       c.behav_params_list_id = fetch_int("List");
       c.discover_query = "ZeroCollectorTest";
       c.discover_annotation = "ZeroCollectorTest";
    }
    c.flags = 0;
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        c.insert()),
      "Cann't insert channel");

    if (CHANNELS[i].status == 'E')
    {
      Rate rate(pq_conn_);
      rate.channel_id = c.id();
      rate.cpm = 10;
      rate.rate_type = "CPM";
      rate.currency_id = fetch_int("Currency");
      rate.effective_date = AutoTest::Time();

      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          rate.insert()),
        "Cann't insert channel rate");

      c.visibility = "CMP";
      c.channel_rate_id = rate.channel_rate_id();
      c.status = "E";

      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          c.update()),
        "Cann't update channel");
    }
    
    channels.push_back(c);

    if (CHANNELS[i].type == 'B' || CHANNELS[i].type == 'D')
    {
      ChannelTrigger ch_trigger(pq_conn_);
      ch_trigger.channel_id = c.id();
      ch_trigger.channel_type = CHANNELS[i].type == 'B' ? "A" : "D";
      ch_trigger.original_trigger = fetch_string("KEYWORD");
      ch_trigger.qa_status = "A";
      ch_trigger.trigger_id = fetch_int("TriggerID");
      ch_trigger.trigger_type = "P";
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          ch_trigger.insert()),
        "linking trigger with channel");
      triggers.push_back(ch_trigger);
    }
    
    if (CHANNELS[i].status == 'A')
    {
      active_channels.push_back(c.id());
    }
    if (CHANNELS[i].status == 'W')
    {
      wait_channels.push_back(c.id());
    }
    c.select();
    c.log();
  }


  // Check channels loading
  ChannelList::iterator it = channels.begin();
  const TestChannel* channel = CHANNELS;
  for (int index = 1;
       it != channels.end(); ++it, ++channel, ++index)
  {
    try
    {
      if (channel->expected == 'D' ||
        (channel->expected == 'I' && channel->type == 'K'))
      {
        FAIL_CONTEXT(
          AutoTest::wait_checker(
            ExpressionChannelChecker(
              this,
              it->id(),
              ExpressionChannelChecker::Expected(),
              AutoTest::AEC_NOT_EXISTS)).check(),
          "Check channel#" + strof(index));
      }
      else
      {
        FAIL_CONTEXT(
          AutoTest::wait_checker(
            ExpressionChannelChecker(
              this,
              it->id(),
              ExpressionChannelChecker::Expected().
                status(std::string(1, channel->expected)))).check(),
          "Check channel#" + strof(index));
      }
    }
    catch (const eh::Exception&)
    {
      it->select();
      it->log();
      throw;
    }
  }

}

bool
ZeroCollectorTest::run()
{
  add_descr_phrase("Check zero collector.");

  // Check channels, which should collected
  ChannelList::const_iterator it = channels.begin();
  for (const TestChannel* channel = CHANNELS;
       it != channels.end(); ++it, ++channel)
  {
    if (channel->collected)
    {
      FAIL_CONTEXT(
        AutoTest::wait_checker(
          ZeroCIChecker(pq_conn_, *it,
            channel->expected,
            channel->collected)).check());
    }
  }

  // Check channels, which shouldn't collected
  it = channels.begin();
  for (const TestChannel* channel = CHANNELS;
       it != channels.end(); ++it, ++channel)
  {
    if (!channel->collected)
    {
      FAIL_CONTEXT(
        ZeroCIChecker(pq_conn_, *it,
          channel->expected,
          channel->collected).check());
    }
  }
  
  return true;
}

void
ZeroCollectorTest::tear_down()
{
  add_descr_phrase("Tear-down.");

  for (TriggerList::iterator it = triggers.begin();
       it != triggers.end(); ++it)
  {
    NOSTOP_FAIL_CONTEXT(it->delet());
  }
  
  for (ChannelList::iterator it = channels.begin();
       it != channels.end(); ++it)
  {
    {
      AutoTest::DBC::Query query(
        pq_conn_.query(
          "DELETE FROM DISCOVERCHANNELSTATE "
          "WHERE CHANNEL_ID = :1"));
      
      query.set(it->id());
      query.update();
    }
    {
      AutoTest::DBC::Query query(
        pq_conn_.query(
          "DELETE FROM CHANNELINVENTORY "
          "WHERE CHANNEL_ID = :1"));

      query.set(it->id());
      query.update();
    }
    pq_conn_.commit();
    if (!it->channel_rate_id.is_null())
    {
      int rate_id =
        it->channel_rate_id.value();
      it->status = "A";
      it->channel_rate_id.null();
      it->visibility = "PUB";
      it->update(false);
      NOSTOP_FAIL_CONTEXT(Rate(pq_conn_).delet(rate_id));
    }
    pq_conn_.commit();
    if (it->type.value() == "A")
    {
      NOSTOP_FAIL_CONTEXT(it->delet());
    }
    else
    {
      NOSTOP_FAIL_CONTEXT(it->del());
    }
  }
}
