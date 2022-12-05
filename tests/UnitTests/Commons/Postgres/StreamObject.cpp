#include"PostgresTest.hpp"
#include<Generics/Time.hpp>
#include<Generics/Rand.hpp>
#include<Commons/Postgres/Declarations.hpp>
#include<Commons/Postgres/SqlStream.hpp>

namespace
{
  class TestStreamObject: public AdServer::Commons::Postgres::Object
  {
  public:
    virtual const char* getSQLTypeName() const /*throw(eh::Exception)*/
    {
      return "test_obj_type";
    }

    virtual void writeSQL(AdServer::Commons::Postgres::SqlStream& stream)
      /*throw(eh::Exception, AdServer::Commons::Postgres::SqlException)*/;

    virtual void readSQL(AdServer::Commons::Postgres::SqlStream& stream)
      /*throw(eh::Exception, AdServer::Commons::Postgres::SqlException)*/;

    void init_random_values_(int seed) noexcept;

  protected:
    virtual ~TestStreamObject() noexcept {};
  private:
    typedef Generics::SimpleDecimal<uint64_t, 8, 2> TestDecimal;

    Generics::Time action_date_;
    Generics::Time action_time_;
    Generics::Time action_timestamp_;
    unsigned int ccg_id_;
    long action_id_;
    TestDecimal action_revenue_;
    std::string name_;
    std::string value_;
  };

  typedef ReferenceCounting::SmartPtr<TestStreamObject> TestStreamObject_var;

  void TestStreamObject::writeSQL(AdServer::Commons::Postgres::SqlStream& stream)
    /*throw(eh::Exception, AdServer::Commons::Postgres::SqlException)*/
  {
    stream.set_date(action_date_);
    stream.set_time(action_time_);
    stream.set_timestamp(action_timestamp_);
    stream.set_value(ccg_id_);
    stream.set_value(action_id_);
    stream.set_decimal(action_revenue_);
    stream.set_string(name_);
    if (value_.empty())
    {
      stream.set_null();
    }
    else
    {
      stream.set_string(name_);
    }
  }

  void TestStreamObject::readSQL(AdServer::Commons::Postgres::SqlStream&)
    /*throw(eh::Exception, AdServer::Commons::Postgres::SqlException)*/
  {
    throw AdServer::Commons::Postgres::SqlException("Not supported");
  }

  void TestStreamObject::init_random_values_(int value) noexcept
  {
    action_date_ = Generics::Time(Generics::safe_rand());
    action_date_ -= action_date_.tv_sec % 86400;
    action_time_ = Generics::Time(Generics::safe_rand() % 86400);
    action_timestamp_ = Generics::Time(
      Generics::safe_rand(), Generics::safe_rand() % 1000000);
    ccg_id_ = value;
    action_id_ = Generics::safe_rand();
    action_revenue_ = TestDecimal(
      Generics::safe_rand() % 2,
      Generics::safe_rand() % 100000,
      Generics::safe_rand() % 100);
    std::ostringstream ostr;
    ostr << "name__" << Generics::safe_rand();
    name_  = ostr.str();
    if(value % 2 == 0)
    {
      ostr << "\"\\value";
      value_ = ostr.str();
    }
  }

}

namespace AdServer
{
  namespace UnitTests
  {
    void PostgresTest::init_objs_(Commons::Postgres::ObjectVector& objs)
      /*throw(eh::Exception)*/
    {
      for(auto i = 0; i < 10; ++i)
      {
        TestStreamObject_var obj = new TestStreamObject();
        obj->init_random_values_(i);
        objs.push_back(obj);
      }
    }
  }
}
