#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM
  {
    // class ExpressionChannel
    ExpressionChannel::ExpressionChannel (
      DB::IConn& connection,
      int _account_id,
      const std::string& _name) :
      PQ::Channel(connection)
    {
      name              = _name;
      account           = _account_id;
      type              = "E"; //expression
      channel_namespace = "A";
      status            = "A";
      flags             = 3;
      qa_status         = "A";
    }

    ExpressionChannel::ExpressionChannel (
      DB::IConn& connection) :
      PQ::Channel(connection)
    {
      type              = "E"; //expression
      channel_namespace = "A";
      status            = "A";
      flags             = 3;
      qa_status         = "A";
    }

    ExpressionChannel::ExpressionChannel (
      DB::IConn& connection,
      const ORMInt::value_type& id) :
      PQ::Channel(connection, id)
    { }

    // class CategoryChannel
    CategoryChannel::CategoryChannel (
      DB::IConn& connection,
      int _account_id,
      const std::string& _name) :
      PQ::Channel(connection)
    {
      name              = _name;
      account           = _account_id;
      type              = "C"; //category
      visibility        = "PRI";
      channel_namespace = "C";
      status            = "A";
      flags             = 2;
      qa_status         = "A";
      country_code.null();
    }

    CategoryChannel::CategoryChannel (
      DB::IConn& connection) :
      PQ::Channel(connection)
    {
      type              = "C"; //category
      channel_namespace = "C";
      visibility        = "PRI";
      status            = "A";
      flags             = 2;
      qa_status         = "A";
      country_code.null();
    }

    CategoryChannel::CategoryChannel (
      DB::IConn& connection,
      const ORMInt::value_type& id) :
      PQ::Channel(connection, id)
    { }

    // class TriggerChannel
       
    TriggerChannel::TriggerChannel (
      DB::IConn& connection) :
      PQ::Channeltrigger(connection),
      channel_(connection)
    { }

    TriggerChannel::TriggerChannel (
      DB::IConn& connection,
      const ORMInt::value_type& id) :
      PQ::Channeltrigger(connection, id),
      channel_(connection)
    { }

    TriggerChannel::TriggerChannel(
      const TriggerChannel& from) :
      PQ::Channeltrigger(from),
      channel_(from.channel_)
    { }
    
    TriggerChannel&
    TriggerChannel::operator=(
      const TriggerChannel& from)
    {
      PQ::Channeltrigger::operator=(from);
      channel_ = from.channel_;
      return *this;
    }

    bool
    TriggerChannel::select()
    {
      return
        PQ::Channeltrigger::select() &&
        channel_.select(channel_id.value());
    }

    bool TriggerChannel::select (const ORMInt::value_type& id)
    {
      return PQ::Channeltrigger::select(id) &&
        channel_.select(channel_id.value());
    }
    
    bool
    TriggerChannel::update(
      bool set_defaults)
    {
      bool result = PQ::Channeltrigger::update(set_defaults);
      
      channel_.triggers_version.set_now();

      return result && channel_.update(false);
    }

    bool
    TriggerChannel::insert (bool set_defaults)
    {

      bool result = PQ::Channeltrigger::insert(set_defaults);

      channel_.select(channel_id.value());
      channel_.triggers_version.set_now();
     
      return
        result && channel_.update(false);
    }

    bool
    TriggerChannel::del ()
    {
      bool result = PQ::Channeltrigger::del();
      
      channel_.triggers_version.set_now();

      return result && channel_.update(false);
    }
    
    bool
    TriggerChannel::del (const ORMInt::value_type& id)
    {
      bool result = PQ::Channeltrigger::del(id);
     
      channel_.triggers_version.set_now();

      return  result && channel_.update(false);
    }

    bool
    TriggerChannel::delet ()
    {
      select();
     
      channel_.triggers_version.set_now();
     
      return PQ::Channeltrigger::delet() && channel_.update(false);
    }
    
    bool
    TriggerChannel::delet (
      const ORMInt::value_type& id)
    {
      select(id);
     
      channel_.triggers_version.set_now();

      return PQ::Channeltrigger::delet(id) && channel_.update(false);
    }

    // class BehavioralChannel

    BehavioralChannel::BehavioralChannel (
      DB::IConn& connection) :
      PQ::Channel(connection),
      params(connection)
    {
      type              = "B";
      channel_namespace = "A";
      flags             = 1;
    }

    BehavioralChannel::BehavioralChannel (
      DB::IConn& connection,
      const ORMInt::value_type& id) :
      PQ::Channel(connection, id),
      params(connection)
    {
      type              = "B";
      channel_namespace = "A";
      flags             = 1;
    }

    BehavioralChannel::BehavioralChannel (
      const BehavioralChannel& from) :
      PQ::Channel(from),
      params(from.params)
    { }
    
    BehavioralChannel&
    BehavioralChannel::operator= (const BehavioralChannel& from)
    { 
      PQ::Channel::operator=(from); 
      params = from.params;
      return *this;
    }

    std::string BehavioralChannel::id_with_suffix ()
    {
      return ::strof(id()) +
        params.trigger_type.value();
    }

    bool BehavioralChannel::select ()
    {
      return PQ::Channel::select() &&
        params.select_channel(id());
    }

    bool BehavioralChannel::select (const ORMInt::value_type& id)
    {
      return PQ::Channel::select(id) &&
        params.select_channel(id);
    }

    bool BehavioralChannel::update (bool set_defaults)
    {
      return PQ::Channel::update(set_defaults) ||
        params.update_channel(id(), set_defaults);
    }
    
    bool BehavioralChannel::update (
      const ORMInt::value_type& id,
      bool set_defaults)
    {
      return PQ::Channel::update(id, set_defaults) ||
        params.update_channel(id, set_defaults);
    }
    
    bool BehavioralChannel::insert (bool set_defaults)
    {
      return PQ::Channel::insert(set_defaults) &&
        params.insert_channel(id(), set_defaults);
    }

    bool BehavioralChannel::delet    ()
    {
      return  params.delet_channel(id()) &&
        PQ::Channel::delet();
    }

    bool BehavioralChannel::del    ()
    {
      return params.delet_channel(id()) &&
        PQ::Channel::del();
    }

    bool BehavioralChannel::delet  (const ORMInt::value_type& id)      
    {
      return params.delet_channel(id) &&
        PQ::Channel::delet(id);
    }

    bool BehavioralChannel::del  (const ORMInt::value_type& id)      
    {
      return params.del_channel(id) &&
        PQ::Channel::del(id);
    }

    bool BehavioralChannel::has_name (const ORMString::value_type& name)
    {
      if(PQ::Channel::has_name(name))
      {
        return params.has_channel(id());
      }
      return false;
    }

    bool BehavioralChannel::select_name (const ORMString::value_type& name)
    {
      return PQ::Channel::select_name(name) &&
        params.select_channel(id());
    }

    bool BehavioralChannel::insert_name (
      const std::string& name,
      bool set_defaults)
    {
      return PQ::Channel::insert_name(name, set_defaults ) &&
        params.insert_channel(id(), set_defaults);
    }

    // class DiscoverChannel
    DiscoverChannel::DiscoverChannel (
      DB::IConn& connection):
      BehavioralChannel(connection)
    { }
      
    DiscoverChannel::DiscoverChannel (
      DB::IConn& connection,
      const ORMInt::value_type& id) :
      BehavioralChannel(connection, id)
    { }

    bool
    DiscoverChannel::delet()
    {
      delet_stats_();
      return BehavioralChannel::delet();
    }

    bool
    DiscoverChannel::delet(
      const ORMInt::value_type& id)
    {
      id_ = id;
      delet_stats_();
      return BehavioralChannel::delet(id);      
    }

    void DiscoverChannel::delet_stats_()
    {
      try
      {
        DB::Query  query(
          conn.get_query(
            "DELETE FROM DISCOVERCHANNELSTATE "
            "WHERE CHANNEL_ID = :i1"));
        query  << id_;
        query.update(); // can return 0 - if no rows
        conn.commit();
      }
      catch(const eh::Exception& exc)
      {
        Logger::thlog().log(exc.what(), Logging::Logger::ERROR);
      }
    }

    // class SearchChannel
    SearchChannel::SearchChannel (
      DB::IConn& connection,
      int _account_id,
      const std::string& _name,
      int _minimum_visits,
      int _time_from,
      int _time_to)
      : BehavioralChannel(connection)
    {
      name             = _name;
      account          = _account_id;
      status           = "A";
      qa_status        = "A";
      
      params.minimum_visits   = _minimum_visits;
      params.time_from        = _time_from;
      params.time_to          = _time_to;
      params.trigger_type     = "S"; //search trigger type 
    }

    SearchChannel::SearchChannel (
      DB::IConn& connection)
      : BehavioralChannel(connection)
    {
      status           = "A";
      qa_status        = "A";

      params.minimum_visits   = 0;
      params.time_from        = 0;
      params.time_to          = DEFAULT_TIME_TO;
      params.trigger_type     = "S"; //search trigger type
    }

    // class PageChannel

    PageChannel::PageChannel (
      DB::IConn& connection,
      int _account_id,
      const std::string& _name,
      int _minimum_visits,
      int _time_from,
      int _time_to)
      : BehavioralChannel(connection)
    {
      name             = _name;
      account          = _account_id;
      status           = "A";
      qa_status        = "A";
      
      params.minimum_visits   = _minimum_visits;
      params.time_from        = _time_from;
      params.time_to          = _time_to;
      params.trigger_type     = "P"; //page trigger type
    }
    
    PageChannel::PageChannel (
      DB::IConn& connection)
      : BehavioralChannel(connection)
    {
      status           = "A";
      qa_status        = "A";
      
      params.minimum_visits   = 0;
      params.time_from        = 0;
      params.time_to          = DEFAULT_TIME_TO;
      params.trigger_type     = "P"; //page trigger type
    }

    PageChannel::PageChannel (
      DB::IConn& connection,
      const ORMInt::value_type& id)
      : BehavioralChannel(connection, id)
    { }


    // RatedTagPricing implementation
    RatedTagPricing::RatedTagPricing(DB::IConn& connection):
      PQ::TagPricing(connection),
      rate(connection)
    {}


    RatedTagPricing::RatedTagPricing(const RatedTagPricing& from):
      PQ::TagPricing(from),
      rate(from.rate)
    {}

    RatedTagPricing&
    RatedTagPricing::operator=(const RatedTagPricing& from)
    {
      PQ::TagPricing::operator=(from);
      rate = from.rate;
      return *this;
    }

    RatedTagPricing::RatedTagPricing(
      DB::IConn& connection,
      const ORMInt::value_type& id):
      PQ::TagPricing(connection, id),
      rate(connection)
    {}

    bool RatedTagPricing::select()
    {
      bool return_value = TagPricing::select();
      if (return_value && !site_rate.is_null())
      {
        return_value = rate.select(site_rate.value());
      }
      return return_value;
    }

    bool RatedTagPricing::update(bool set_defaults)
    {
      return
        PQ::TagPricing::update(set_defaults) &&
          rate.update(set_defaults);
    }

    bool RatedTagPricing::insert(bool set_defaults)
    {
      if (!PQ::TagPricing::insert(set_defaults))
      { return false; }
      rate.tag_pricing = id_;
      if (!rate.insert(set_defaults))
      { return false; }
      site_rate = rate.id();
      return TagPricing::update(false);
    }

    bool RatedTagPricing::delet()
    {
      site_rate.null();
      return
        PQ::TagPricing::update(false) &&
          rate.delet() && TagPricing::delet();
    }

    void RatedTagPricing::log_in(Logger& logger, unsigned long severity)
    {
      PQ::TagPricing::log_in(logger, severity);
      rate.log_in(logger, severity);
    }


    // RatedColocation implementation
    RatedColocation::RatedColocation(
      DB::IConn& connection):
      Colocation(connection),
      rate(connection)
    {}


    RatedColocation::RatedColocation(
      const RatedColocation& from):
      Colocation(from),
      rate(from.rate)
    {}

    RatedColocation&
    RatedColocation::operator=(
      const RatedColocation& from)
    {
      PQ::Colocation::operator=(from);
      rate = from.rate;
      return *this;
    }

    RatedColocation::RatedColocation(
      DB::IConn& connection,
      const ORMInt::value_type& id):
      Colocation(connection, id),
      rate(connection)
    {}

    bool
    RatedColocation::select()
    {
      return PQ::Colocation::select() &&
        (colo_rate.is_null() || rate.select(colo_rate.value()));
    }

    bool
    RatedColocation::update(bool set_defaults)
    {
      return PQ::Colocation::update(set_defaults) &&
        rate.update(set_defaults);
    }

    bool
    RatedColocation::insert(bool set_defaults)
    {
      return PQ::Colocation::insert(set_defaults);
    }

    bool
    RatedColocation::insert_rate(bool set_defaults)
    {
      rate.colo = id_;
      if (!rate.insert(set_defaults))
      {
        return false;
      }
      colo_rate = rate.id();
      return PQ::Colocation::update(false);
    }

    bool
    RatedColocation::delet()
    {
      if (!colo_rate.is_null())
      {
        colo_rate.null();
        return PQ::Colocation::update(false) &&
          rate.delet() && PQ::Colocation::delet();
      }
      else
      {
        return PQ::Colocation::delet() && rate.delet();
      }
    }

    // RatedCampaignCreativeGroup implementation
    RatedCampaignCreativeGroup::RatedCampaignCreativeGroup(DB::IConn& connection):
      PQ::CampaignCreativeGroup(connection),
      rate(connection)
    {}


    RatedCampaignCreativeGroup::RatedCampaignCreativeGroup(
      const RatedCampaignCreativeGroup& from):
      PQ::CampaignCreativeGroup(from),
      rate(from.rate)
    {}

    RatedCampaignCreativeGroup&
      RatedCampaignCreativeGroup::operator=(
        const RatedCampaignCreativeGroup& from)
    {
      PQ::CampaignCreativeGroup::operator=(from);
      rate = from.rate;
      return *this;
    }

    RatedCampaignCreativeGroup::RatedCampaignCreativeGroup(
      DB::IConn& connection,
      const ORMInt::value_type& id):
      CampaignCreativeGroup(connection, id),
      rate(connection)
    {}

    bool RatedCampaignCreativeGroup::select()
    {
      bool return_value = PQ::CampaignCreativeGroup::select();
      if (return_value && !ccg_rate.is_null())
      {
        return_value = rate.select(ccg_rate.value());
      }
      return return_value;
    }

    bool RatedCampaignCreativeGroup::update(bool set_defaults)
    {
      return
        PQ::CampaignCreativeGroup::update(set_defaults) &&
          rate.update(set_defaults);
    }

    bool RatedCampaignCreativeGroup::insert(bool set_defaults)
    {
      if (!PQ::CampaignCreativeGroup::insert(set_defaults))
      { return false; }
      rate.ccg = id_;
      if (!rate.insert(set_defaults))
      { return false; }
      ccg_rate = rate.id();
      return PQ::CampaignCreativeGroup::update(false);
    }

    bool RatedCampaignCreativeGroup::delet()
    {
      ccg_rate.null();
      return
        PQ::CampaignCreativeGroup::update(false) &&
          rate.delet() && PQ::CampaignCreativeGroup::delet();
    }

    void RatedCampaignCreativeGroup::log_in(Logger& logger, unsigned long severity)
    {
      PQ::CampaignCreativeGroup::log_in(logger, severity);
      rate.log_in(logger, severity);
    }
  }
 
}
