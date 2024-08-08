// THIS
#include "CreativeProvider.hpp"

namespace PredictorSvcs::BidCostPredictor
{

namespace Aspect
{

inline constexpr char CREATIVE_PROVIDER[] = "CREATIVE_PROVIDER";

} // namespace Aspect

CreativeProviderDB::CreativeProviderDB(
  const std::string& host,
  const std::size_t port,
  const std::string& dbname,
  const std::string& user,
  const std::string& password,
  Logger* logger)
  : logger_(ReferenceCounting::add_ref(logger))
{
  std::ostringstream stream;
  stream << "host="
         << host
         << " port="
         << port
         << " dbname="
         << dbname
         << " user="
         << user
         << " password="
         << password;
  const auto environment_string = stream.str();

  {
    std::ostringstream stream;
    stream << FNS
           << "PostgreSQL environment string: ["
           << environment_string
           << "]";
    logger_->info(stream.str(), Aspect::CREATIVE_PROVIDER);
  }


  pg_env_ = new Environment(environment_string.c_str());
  pg_env_->activate_object();
  pg_pool_ = pg_env_->create_connection_pool();
}

CreativeProviderDB::~CreativeProviderDB()
{
  try
  {
    pg_env_->deactivate_object();
  }
  catch (...)
  {
  }

  try
  {
    pg_env_->wait_object();
  }
  catch (...)
  {
  }
}

void CreativeProviderDB::load(CcIdToCategories& cc_id_to_categories)
{
  using Statement = AdServer::Commons::Postgres::Statement;
  using Statement_var = AdServer::Commons::Postgres::Statement_var;

  try
  {
    {
      std::ostringstream stream;
      stream << FNS
             << "Start loading from postgrsql";
      logger_->info(stream.str(), Aspect::CREATIVE_PROVIDER);
    }

    const auto connection = pg_pool_->get_connection(true);

    Statement_var stmt = new Statement(
      "SELECT "
        "campaigncreative.cc_id, creativecategory_creative.creative_category_id "
      "FROM campaigncreative "
      "LEFT JOIN creativecategory_creative "
        "ON campaigncreative.creative_id=creativecategory_creative.creative_id "
      "WHERE creativecategory_creative.creative_category_id IS NOT NULL"
    );
    const auto result = connection->execute_statement(stmt);
    std::size_t size = 0;
    while (result->next())
    {
      const auto cc_id = result->get_number<unsigned long>(1);
      const auto creative_category_id = result->get_number<unsigned long>(1);
      cc_id_to_categories[cc_id].emplace_back(creative_category_id);
      size += 1;
    }

    {
      std::ostringstream stream;
      stream << FNS
             << "Loading is stopped. Count rows = "
             << size
             << ", hash table size = "
             << cc_id_to_categories.size();
      logger_->info(stream.str(), Aspect::CREATIVE_PROVIDER);
    }
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << exc.what();
    logger_->critical(stream.str(), Aspect::CREATIVE_PROVIDER);

    throw Exception(stream);
  }
}

} // namespace PredictorSvcs::BidCostPredictor