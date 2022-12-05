#include "ModifyConfigSource.hpp"

namespace Aspect
{
  const char MODIFY_CONFIG_DB_SOURCE[] = "ModifyConfigDBSource";
}

namespace AdServer
{
namespace CampaignSvcs
{
  ModifyConfigDBSource::ModifyConfigDBSource(
    Logging::Logger* logger,
    Commons::Postgres::ConnectionPool* pg_pool)
    noexcept
    : logger_(ReferenceCounting::add_ref(logger)),
      pg_pool_(ReferenceCounting::add_ref(pg_pool))
  {}

  ModifyConfig_var
  ModifyConfigDBSource::update() noexcept
  {
    static const char* FUN = "ModifyConfigDBSource::update()";

    try
    {
      Commons::Postgres::Connection_var conn;

      try
      {
        conn = pg_pool_->get_connection();
      }
      catch (const Commons::Postgres::NotActive&)
      {
        return ModifyConfig_var();
      }

      ModifyConfig_var res(new ModifyConfig());
      try
      {
        query_countries_(conn, *res);
      }
      catch(const Exception&)
      {
        pg_pool_->bad_connection(conn);
        throw;
      }
      return res;
    } 
    catch (const eh::Exception& ex)
    {
      logger_->sstream(
        Logging::Logger::ERROR,
        Aspect::MODIFY_CONFIG_DB_SOURCE,
        "ADS-DB-6000") << FUN <<
        ": can't receive stats from DB: " << ex.what();
    }

    return ModifyConfig_var();
  }

  void
  ModifyConfigDBSource::query_countries_(
    Commons::Postgres::Connection* conn,
    ModifyConfig& config)
    /*throw(Exception)*/
  {
    static const char* FUN = "ModifyConfigDBSource::query_countries_()";

    try
    {
      enum
      {
        POS_COUNTRY_CODE = 1,
        POS_CPC_RANDOM_IMPS,
        POS_CPA_RANDOM_IMPS
      };

      Commons::Postgres::Statement_var stmt =
        new Commons::Postgres::Statement(
          "SELECT "
            "lower(country_code), "
            "coalesce(cpc_random_imps, 2000), "
            "coalesce(cpa_random_imps, 10000) "
          "FROM Country LEFT JOIN CTRAlgorithm USING(country_code)");

      Commons::Postgres::ResultSet_var rs = conn->execute_statement(stmt);

      while(rs->next())
      {
        std::string country_code = rs->get_string(POS_COUNTRY_CODE);
        ModifyConfig::CountryDef country;
        country.cpc_random_imps = rs->get_number<unsigned long>(POS_CPC_RANDOM_IMPS);
        country.cpa_random_imps = rs->get_number<unsigned long>(POS_CPA_RANDOM_IMPS);
        config.countries.insert(std::make_pair(
          country_code, country));
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }
}
}
