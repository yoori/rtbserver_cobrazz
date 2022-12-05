#ifndef ORACLE_CONNECTIONDESCRIPTION_HPP
#define ORACLE_CONNECTIONDESCRIPTION_HPP

#include <string>
#include <Generics/Time.hpp>

namespace AdServer
{
namespace Commons
{
  namespace Oracle
  {
    struct ConnectionDescription
    {
      ConnectionDescription();

      ConnectionDescription(
        const char* user_name_val,
        const char* password_val,
        const char* db_val,
        const char* schema_val = "",
        const Generics::Time* timeout = 0);

      std::string user_name;
      std::string password;
      std::string db;
      // isn't required, will be used default user schema if not defined
      std::string schema;
      Generics::Time statement_timeout;
    };
  }
}
}

namespace AdServer
{
namespace Commons
{
  namespace Oracle
  {
    inline
    ConnectionDescription::ConnectionDescription()
    {}

    inline
    ConnectionDescription::ConnectionDescription(
      const char* user_name_val,
      const char* password_val,
      const char* db_val,
      const char* schema_val,
      const Generics::Time* statement_timeout_val)
      : user_name(user_name_val),
        password(password_val),
        db(db_val),
        schema(schema_val),
        statement_timeout(
          statement_timeout_val ? *statement_timeout_val : Generics::Time::ZERO)
    {}
  }
}
}

#endif /*ORACLE_CONNECTIONDESCRIPTION_HPP*/
