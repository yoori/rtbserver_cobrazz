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
    std::size_t number_rows = 0;
    while (result->next())
    {
      const auto cc_id = result->get_number<Types::CcId>(1);
      const auto creative_category_id = result->get_number<Types::CreativeCategoryId>(1);
      cc_id_to_categories[cc_id].emplace_back(creative_category_id);
      number_rows += 1;
    }

    std::size_t number_categories_reached_1 = 0;
    std::size_t number_categories_reached_3 = 0;
    std::size_t number_categories_reached_5 = 0;
    std::size_t number_categories_reached_10 = 0;
    std::size_t number_categories_reached_50 = 0;
    std::size_t number_categories_reached_100 = 0;
    std::size_t number_categories_reached_200 = 0;
    std::size_t number_categories_reached_300 = 0;
    std::size_t number_categories_reached_400 = 0;
    for (const auto& [cc_id, categories] : cc_id_to_categories)
    {
      const std::size_t size = categories.size();
      number_categories_reached_1 += 1;
      if (size >= 3)
      {
        number_categories_reached_3 += 1;
      }
      if (size >= 5)
      {
        number_categories_reached_5 += 1;
      }
      if (size >= 10)
      {
        number_categories_reached_10 += 1;
      }
      if (size >= 50)
      {
        number_categories_reached_50 += 1;
      }
      if (size >= 100)
      {
        number_categories_reached_100 += 1;
      }
      if (size >= 200)
      {
        number_categories_reached_200 += 1;
      }
      if (size >= 300)
      {
        number_categories_reached_300 += 1;
      }
      if (size >= 400)
      {
        number_categories_reached_400 += 1;
      }
    }

    {
      std::ostringstream stream;
      stream << FNS
             << "Loading is stopped. Count rows = "
             << number_rows
             << ", number different cc_id : "
             << cc_id_to_categories.size()
             << '\n'
             << "Categories reached 1 per cc_id : "
             << number_categories_reached_1
             << '\n'
             << "Categories reached 3 per cc_id : "
             << number_categories_reached_3
             << '\n'
             << "Categories reached 5 per cc_id : "
             << number_categories_reached_5
             << '\n'
             << "Categories reached 10 per cc_id : "
             << number_categories_reached_10
             << '\n'
             << "Categories reached 50 per cc_id : "
             << number_categories_reached_50
             << '\n'
             << "Categories reached 100 per cc_id : "
             << number_categories_reached_100
             << '\n'
             << "Categories reached 200 per cc_id : "
             << number_categories_reached_200
             << '\n'
             << "Categories reached 300 per cc_id : "
             << number_categories_reached_300
             << '\n'
             << "Categories reached 400 per cc_id : "
             << number_categories_reached_400
             << '\n';
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