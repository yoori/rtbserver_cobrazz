#ifndef BIDCOSTPREDICTOR_CREATIVEPROVIDER_HPP
#define BIDCOSTPREDICTOR_CREATIVEPROVIDER_HPP

// STD
#include <vector>
#include <unordered_map>

// UNIXCOMMONS
#include <ReferenceCounting/SmartPtr.hpp>
#include <ReferenceCounting/Interface.hpp>
#include <Logger/Logger.hpp>

// THIS
#include <Commons/Postgres/Connection.hpp>
#include <Commons/Postgres/ConnectionPool.hpp>
#include <Commons/Postgres/Environment.hpp>

namespace PredictorSvcs::BidCostPredictor
{

class CreativeProvider : public virtual ReferenceCounting::Interface
{
public:
  using CcId = unsigned long;
  using CreativeCategoryId = unsigned long;
  using CreativeCategoryIds = std::vector<CreativeCategoryId>;
  using CcIdToCategories = std::unordered_map<CcId, CreativeCategoryIds>;

public:
  CreativeProvider() = default;

  virtual void load(CcIdToCategories& cc_id_to_categories) = 0;

protected:
  ~CreativeProvider() override = default;
};

using CreativeProvider_var = ReferenceCounting::SmartPtr<CreativeProvider>;

class CreativeProviderDB final :
  public CreativeProvider,
  public ReferenceCounting::AtomicImpl
{
private:
  using Environment = AdServer::Commons::Postgres::Environment;
  using Environment_var = AdServer::Commons::Postgres::Environment_var;
  using ConnectionPool = AdServer::Commons::Postgres::ConnectionPool;
  using ConnectionPool_var = AdServer::Commons::Postgres::ConnectionPool_var;
  using Connection = AdServer::Commons::Postgres::Connection;
  using Connection_var = AdServer::Commons::Postgres::Connection_var;

public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit CreativeProviderDB(
    const std::string& host,
    const std::size_t port,
    const std::string& dbname,
    const std::string& user,
    const std::string& password,
    Logger* logger);

  void load(CcIdToCategories& cc_id_to_categories) override;

protected:
  ~CreativeProviderDB() override;

private:
  const Logger_var logger_;

  Environment_var pg_env_;

  ConnectionPool_var pg_pool_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_CREATIVEPROVIDER_HPP