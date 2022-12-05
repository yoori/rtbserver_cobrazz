#include <iostream>
#include <string>

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM
  {

    // class IORMValue
    IORMValue::~IORMValue()
    { }

    // class ORMString
    
    ORMString::ORMString()
    { }

    ORMString::ORMString(
      const std::string& value) :
      Base(value)
    { }

    ORMString::ORMString(
      const ORMString& from) :
      Base(from)
    { }

    // class ORMText
    ORMText::ORMText() :
      Base()
    {  }

    ORMText::ORMText(
      const std::string& value) :
      Base()
    {
      _value.set(value);
    }

    // ORMDate class

    ORMDate::ORMDate() :
      Base(
        Generics::Time::ZERO.get_gm_time())
    {
      _changed = false;
      _null = true;
    }
    
    ORMDate::ORMDate(
      const value_type& value) :
      Base(value)
    {
      _null = _value == Generics::Time::ZERO;
    }

    // ORMTimestamp class
    
    ORMTimestamp::ORMTimestamp() :
      Base(Generics::Time::ZERO)
    {
      _changed = false;
      _null = true;
    }
      
    ORMTimestamp::ORMTimestamp(
      const value_type& value) :
      Base(value)
    {
      _null = _value == Generics::Time::ZERO;
    }

 
    // ORMObjectMember struct
    IORMValue&
    ORMObjectMember::value(
      void* object) const
    {
      return *reinterpret_cast<IORMValue*>(
        reinterpret_cast<char*>(member) + (
          reinterpret_cast<const char*>(object) - null<const char>()));
    }

    // class Restorer

    Restorer::Restorer(
      EntityType entity_type) :
      entity_type_(entity_type)
    { }

    
    Restorer::~Restorer() noexcept
    { }
    
    void Restorer::restore()
    { }

    // class SerializeQueryManager_
    
    SerializeQueryManager_::SerializeQueryManager_()
    { }

    bool
    SerializeQueryManager_::execute(
      AutoTest::DBC::IConn& connection,
      AutoTest::DBC::IQuery& query)
    {
      QueryGuard guard(lock_);
      try
      {
        bool ret = query.update() > 0;
        connection.commit();
        return ret;
      }
      catch(const eh::Exception& exc)
      {
        Stream::Error error;
        error << "SerializeQueryManager_::execute(): " << exc.what();
        Logger::thlog().log(error.str(), Logging::Logger::ERROR);
        return false;
      }
      catch (...)
      {
        Logger::thlog().log(
          "SerializeQueryManager_::execute(): Unexpeced error.",
          Logging::Logger::ERROR);
        return false;
      }
    }

    bool
    SerializeQueryManager_::execute(
      AutoTest::DBC::IConn& connection,
      AutoTest::DBC::IQuery* query)
    {
      AutoTest::DBC::Query q(query);
      return execute(connection, q);
    }

    SerializeQueryManager_::SyncPolicy::Mutex&
    SerializeQueryManager_::lock()
    {
      return lock_;
    }
    
    static const char* display_statuses [] =
    {
      "Inactive",
      "Live",
      "Not Live",
      "Deleted",
      "Live - Linked Creatives Need Attention",
      "Live - Keywords Need Attention",
      "Not Live - Linked Creatives Need Attention",
      "Live - pending inactivation",
      "Not Live - Declined",
      "Live - Campaigns Need Attention",
      "No Active Campaigns",
      "Not Live - Campaigns Need Attention",
      "Not Live - Not Enough Unique Users",
      "Not Live - Overdraft",
      "Not Live - Date not in Range",
      "Not Live - No Available Budget",
      "Not Live - Keywords Need Attention",
      "Not Live - Channel Target Needs Attention",
      "Not Live - Pending Approval by User",
      "Not Live - Problem with Creative",
      "Not Live - Pending Approval by OIX",
      "Live - Linked Creatives and Keywords Need Attention",
      "Not Live - Linked Creatives and Keywords Need Attention",
      "Not Live - No billing contact",
      "Live - Pending Inactivation",
      "Not Live - Channels Needs Attention",
      "Live - Channels Need Attention",
      "Live - Triggers Need Attention",
      "Live - Channel Target Needs Attention"
    };
    
    int
    get_display_status_id(
      DB::IConn& conn,
      const char* object,
      DisplayStatus status)
    {
      int ret = 0;
      DB::Query query(conn.get_query("SELECT display_status_id "
        "FROM DisplayStatus ds "
        " JOIN ObjectType ot "
        " ON (ds.object_type_id = ot.object_type_id) "
        "WHERE upper(ot.name) = upper(:1) and ds.description = :2"));
      query.set(object);
      query.set(display_statuses[status]);
      DB::Result result(query.ask());
      if(result.next())
      {    
        result.get(ret);
        return ret;
      }
      Stream::Error error;
      error << "No such display status " << display_statuses[status] << " for " << object;
      Logger::thlog().log(error.str(), Logging::Logger::ERROR);
      throw NoStatusException(error.str());
      return ret;
    }

  }
}
