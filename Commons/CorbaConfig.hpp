/**
 * @file CorbaConfig.hpp
 * $Id: CorbaConfig.hpp,v 1.4 2007-10-09 10:50:21 jurij_kuznecov Exp $
 *
 * Description: Wrapper for loading CorbaConfig from xsd structs
 */
#ifndef _COMMONS_CORBACONFIG_HPP_
#define _COMMONS_CORBACONFIG_HPP_

#include <list>
#include <Generics/GnuHashTable.hpp>
#include <eh/Exception.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

#include <xsd/AdServerCommons/AdServerCommons.hpp>

namespace CORBACommons
{
  typedef std::list<CorbaObjectRef> CorbaObjectRefList;
}

namespace Config
{
  class CorbaConfigReader
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    typedef Generics::GnuHashTable<Generics::StringHashAdapter, unsigned>
      ServiceIndexRefHashMap;

    static void read_config(
      const xsd::AdServer::Configuration::CorbaConfigType&
        xml_corba_config,
      CORBACommons::CorbaConfig& corba_config) /*throw(Exception)*/;

    static inline void
    read_corba_ref(
      const xsd::AdServer::Configuration::CorbaObjectRefType&
        xml_corba_object_ref,
      CORBACommons::CorbaObjectRef& corba_object_ref)
      /*throw(Exception)*/;

    static void
    read_corba_ref(
      const xsd::AdServer::Configuration::CorbaObjectRefType&
        xml_corba_object_ref,
      CORBACommons::CorbaObjectRef& corba_object_ref,
      std::string& object_name,
      std::string& service_index)
      /*throw(Exception)*/;

    static void
    read_multi_corba_ref(
      const xsd::AdServer::Configuration::MultiCorbaObjectRefType&
        xml_corba_object_ref,
      CORBACommons::CorbaObjectRefList& corba_object_refs)
      /*throw(Exception)*/;

    static CORBACommons::CorbaObjectRefList
    read_multi_corba_ref(
      const xsd::AdServer::Configuration::MultiCorbaObjectRefType&
        xml_corba_object_ref)
      /*throw(Exception)*/;

    static void
    read_indexed_multi_corba_ref(
      const xsd::AdServer::Configuration::MultiCorbaObjectRefType&
        xml_corba_object_ref,
      CORBACommons::CorbaObjectRefList& corba_object_refs,
      ServiceIndexRefHashMap& service_index_ref)
      /*throw(Exception)*/;

  protected:
    static void read_endpoint(
      const xsd::AdServer::Configuration::EndpointConfigType&
        xml_endpoint_config,
      CORBACommons::EndpointConfig& endpoint_config)
      /*throw(Exception)*/;

    static void read_secure_params(
      const xsd::AdServer::Configuration::SecureParamsType&
        secure_params,
      CORBACommons::SecureConnectionConfig& secure_connection_config)
      /*throw(Exception)*/;

  };


  inline void
  CorbaConfigReader::read_corba_ref(
    const xsd::AdServer::Configuration::CorbaObjectRefType&
    xml_corba_object_ref,
    CORBACommons::CorbaObjectRef& corba_object_ref)
    /*throw(Exception)*/
  {
    corba_object_ref.object_ref = xml_corba_object_ref.ref();
    corba_object_ref.type = CORBACommons::CorbaObjectRef::CT_NON_SECURE;

    if (xml_corba_object_ref.Secure().present())
    {
      corba_object_ref.type = CORBACommons::CorbaObjectRef::CT_SECURE;
      read_secure_params(
        xml_corba_object_ref.Secure().get(),
        corba_object_ref.secure_connection_config);
    }
  }

}

#endif /*_COMMONS_CORBACONFIG_HPP*/

