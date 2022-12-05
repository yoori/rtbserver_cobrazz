
#include <tests/AutoTests/Commons/Common.hpp>
#include <Commons/ErrorHandler.hpp>

// class GlobalConfig

const GlobalConfig::ServiceList GlobalConfig::EMPTY_;

GlobalConfig::Service::Service(
  const std::string& address_,
  bool remote_) :
  address(address_),
  remote(remote_)
{}

GlobalConfig::GlobalConfig(
  const char* config_path) :
  params_()
{
  initialize(config_path);
}

const GlobalConfig::Service&
GlobalConfig::get_service(
  ClusterTypeEnum cluster,
  ServiceTypeEnum service,
  unsigned long index) const
  /*throw(eh::Exception)*/
{
  ClusterDictionary::const_iterator cit =
    cluster_services_.find(cluster);
  if (cit != cluster_services_.end())
  {
    ServiceDictionary::const_iterator sit =
      cit->second.find(service);
    if ( sit != cit->second.end() && sit->second.size() > index)
    {
      return sit->second[index];
    }
  }

  std::string cluster_str = cluster == CTE_ALL? "any":
    cluster == CTE_ALL_REMOTE? "any remote":
      ClusterEnum(static_cast<ClusterEnum::value>(cluster));

  Stream::Error error;
  error << "XsdParams::get_service(). " <<
    "Error: Service '" <<
    ServiceEnum(static_cast<ServiceEnum::value>(service)) <<
    "#" << index << "' absent in '" <<
    cluster_str << "' cluster";
  throw ConfigError(error);
}

const GlobalConfig::ServiceList&
GlobalConfig::get_services(
    ClusterTypeEnum cluster,
    ServiceTypeEnum service) const
    /*throw(eh::Exception)*/
{
   ClusterDictionary::const_iterator cit =
     cluster_services_.find(cluster);
   if (cit != cluster_services_.end())
   {
     ServiceDictionary::const_iterator sit =
       cit->second.find(service);
     if ( sit != cit->second.end())
     {
       return sit->second;
     }
   }

   return EMPTY_;
}


bool
GlobalConfig::check_service(
  ClusterTypeEnum cluster,
  ServiceTypeEnum service,
  unsigned long index) const
{
  ClusterDictionary::const_iterator cit =
    cluster_services_.find(cluster);
  if (cit == cluster_services_.end())
  {
    return false;
  }
  ServiceDictionary::const_iterator sit =
    cit->second.find(service);
  if (sit == cit->second.end() || sit->second.size() <= index )
  {
    return false;
  }
  return true;
}


void
GlobalConfig::initialize(const char* config_path)
{

  params_ = Config::load_helper<ParamsPtr>(
    xsd::AdServer::Configuration::GeneralParams,
    config_path,
    xml_schema::flags::dont_initialize);

  ClusterIterator cit_begin = params_->Cluster().begin();
  ClusterIterator cit_end = params_->Cluster().end();

  for (ClusterIterator cit = cit_begin;
       cit != cit_end; ++cit)
  {
    ServiceIterator sit_begin = cit->Service().begin();
    ServiceIterator sit_end = cit->Service().end();
    for (ServiceIterator sit = sit_begin;
         sit != sit_end; ++sit)
    {
      ClusterTypeEnum c =
        static_cast<ClusterTypeEnum>(
          (ClusterEnum::value)cit->name());
      ServiceTypeEnum s =
        static_cast<ServiceTypeEnum>(
          (ServiceEnum::value)sit->name());

      cluster_services_[c][s].
        push_back(
          Service(
            sit->address(),
            c != CTE_CENTRAL));

      if (c == CTE_REMOTE1 || c == CTE_REMOTE2)
      {
        (cluster_services_[CTE_ALL_REMOTE])[s].
          push_back(
            Service(
              sit->address(), true));
      }

      (cluster_services_[CTE_ALL])[s].
        push_back(
          Service(
            sit->address(),
            c != CTE_CENTRAL));
    }
  }
}

Params
GlobalConfig::get_params() const noexcept
{
  return *params_;
}



// class XsdParams
XsdParams::XsdParams(
  const GlobalConfig& config,
  Locals locals)
  noexcept
  : config_(config),
    locals_(locals)
{
}

Params XsdParams::get_global_params() const noexcept
{
  return config_.get_params();
}

Locals XsdParams::get_local_params() const noexcept
{
  return locals_;
}

const
GlobalConfig& XsdParams::get_config() const
  noexcept
{
  return config_;
}
