#ifndef CAMPAIGNSERVER_MODIFYCONFIGSOURCE_HPP
#define CAMPAIGNSERVER_MODIFYCONFIGSOURCE_HPP

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Logger/Logger.hpp>

#include <Commons/Postgres/ConnectionPool.hpp>

namespace AdServer
{
namespace CampaignSvcs
{
  struct ModifyConfig: public virtual ReferenceCounting::AtomicImpl
  {
  public:
    struct CountryDef
    {
      unsigned long cpc_random_imps;
      unsigned long cpa_random_imps;
    };

    typedef std::map<std::string, CountryDef>
      CountryMap;

    CountryMap countries;

  protected:
    virtual
    ~ModifyConfig() noexcept
    {}
  };

  typedef ReferenceCounting::QualPtr<ModifyConfig>
    ModifyConfig_var;

  typedef ReferenceCounting::ConstPtr<ModifyConfig>
    CModifyConfig_var;

  class ModifyConfigSource: public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    virtual ModifyConfig_var
    update() noexcept = 0;

  protected:
    virtual ~ModifyConfigSource() noexcept
    {}
  };

  typedef ReferenceCounting::QualPtr<ModifyConfigSource>
    ModifyConfigSource_var;

  typedef ReferenceCounting::FixedPtr<ModifyConfigSource>
    FModifyConfigSource_var;

  class ModifyConfigDBSource: public ModifyConfigSource
  {
  public:
    ModifyConfigDBSource(
      Logging::Logger* logger,
      Commons::Postgres::ConnectionPool* pg_pool)
      noexcept;

    ModifyConfig_var
    update() noexcept;

  protected:
    virtual
    ~ModifyConfigDBSource() noexcept
    {}

    void
    query_countries_(
      Commons::Postgres::Connection* conn,
      ModifyConfig& config)
      /*throw(Exception)*/;

  private:
    Logging::Logger_var logger_;
    AdServer::Commons::Postgres::ConnectionPool_var pg_pool_;
  };
}
}

#endif /*CAMPAIGNSERVER_MODIFYCONFIGSOURCE_HPP*/
