/*
 * @file CorbaConfig.cpp
 * Description: Wrapper for loading CorbaConfig from xsd structs
 *
 * */

#include <sstream>

#include "CorbaConfig.hpp"

typedef std::list<std::string> CertificateSeq;

namespace
{
  void parse_certificate_seq(
    const char* certificates,
    CertificateSeq& certificate_seq)
    /*throw(eh::Exception)*/
  {
    /* parse certificate sequence */
    std::string certificate_seq_s = certificates;
    std::string::size_type begin_word = 0;
    std::string::size_type end_word;

    while ((end_word = certificate_seq_s.find(';', begin_word)) !=
      std::string::npos)
    {
      certificate_seq.push_back(
        std::string(certificate_seq_s.begin() + begin_word,
                    certificate_seq_s.begin() + end_word));
        begin_word = end_word + 1;
    }

    certificate_seq.push_back(
      std::string(certificate_seq_s.begin() + begin_word,
                  certificate_seq_s.end()));
  }
}


namespace Config
{
  /** CorbaConfigReader */
  void CorbaConfigReader::read_config(
    const xsd::AdServer::Configuration::CorbaConfigType&
      xml_corba_config,
    CORBACommons::CorbaConfig& corba_config)
    /*throw(CorbaConfigReader::Exception)*/
  {
    typedef
      xsd::AdServer::Configuration::CorbaConfigType::Endpoint_sequence
      EndpointSeq;

    const EndpointSeq& endpoints = xml_corba_config.Endpoint();

    corba_config.thread_pool = xml_corba_config.threading_pool();

    for(EndpointSeq::const_iterator it = endpoints.begin();
        it != endpoints.end(); ++it)
    {
      CORBACommons::EndpointConfig endpoint_config;
      read_endpoint(*it, endpoint_config);
      corba_config.endpoints.push_back(endpoint_config);
    }
  }

  void CorbaConfigReader::read_endpoint(
    const xsd::AdServer::Configuration::EndpointConfigType&
      xml_endpoint_config,
    CORBACommons::EndpointConfig& endpoint_config)
    /*throw(Exception)*/
  {
    {
      if(!xml_endpoint_config.host().present())
      {
        // default: use canonical host name
        char canonical_host_name[MAXHOSTNAMELEN + 1];
        ::gethostname(canonical_host_name, MAXHOSTNAMELEN);

        endpoint_config.host = canonical_host_name;
      }
      else
      {
        endpoint_config.host = xml_endpoint_config.host().get();
      }

      if(xml_endpoint_config.ior_names().present())
      {
        endpoint_config.ior_names = xml_endpoint_config.ior_names().get();
      }
      else
      {
        endpoint_config.ior_names = endpoint_config.host;
      }

      endpoint_config.port = xml_endpoint_config.port();
    }

    typedef xsd::AdServer::Configuration::EndpointConfigType::Object_sequence
      ObjectSeq;

    const ObjectSeq& corba_objects = xml_endpoint_config.Object();

    for(ObjectSeq::const_iterator it = corba_objects.begin();
        it != corba_objects.end(); ++it)
    {
      endpoint_config.objects[it->servant()].insert(it->name());
    }

    if(xml_endpoint_config.Secure().present())
    {
      read_secure_params(
        xml_endpoint_config.Secure().get(),
        endpoint_config.secure_connection_config);
    }
  }

  void CorbaConfigReader::read_secure_params(
    const xsd::AdServer::Configuration::SecureParamsType&
      xml_secure_params,
    CORBACommons::SecureConnectionConfig& secure_connection_config)
    /*throw(CorbaConfigReader::Exception)*/
  {
    const std::string& key_file = xml_secure_params.key();
    const std::string& pass_phrase = xml_secure_params.key_word();
    const std::string& certificate = xml_secure_params.certificate();
    const std::string& certificate_authority =
      xml_secure_params.certificate_authority();

    try
    {
      secure_connection_config.parse(key_file.c_str(), pass_phrase.c_str(),
        certificate.c_str(), certificate_authority.c_str());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "CorbaConfigReader: " << ex.what();
      throw CorbaConfigReader::Exception(ostr);
    }
  }

  void
  CorbaConfigReader::read_corba_ref(
    const xsd::AdServer::Configuration::CorbaObjectRefType&
      xml_corba_object_ref,
    CORBACommons::CorbaObjectRef& corba_object_ref,
    std::string& object_name,
    std::string& service_index)
    /*throw(CorbaConfigReader::Exception)*/
  {
    if (xml_corba_object_ref.name().present())
    {
      object_name = *xml_corba_object_ref.name();
    }

    if (xml_corba_object_ref.service_index().present())
    {
      service_index = *xml_corba_object_ref.service_index();
    }
    read_corba_ref(xml_corba_object_ref, corba_object_ref);
  }

  void
  CorbaConfigReader::read_multi_corba_ref(
    const xsd::AdServer::Configuration::MultiCorbaObjectRefType&
      xml_corba_object_refs,
    CORBACommons::CorbaObjectRefList& corba_object_refs)
    /*throw(CorbaConfigReader::Exception)*/
  {
    for(xsd::AdServer::Configuration::MultiCorbaObjectRefType::Ref_sequence::const_iterator
          it = xml_corba_object_refs.Ref().begin();
        it != xml_corba_object_refs.Ref().end(); ++it)
    {
      CORBACommons::CorbaObjectRef ref;
      read_corba_ref(*it, ref);
      corba_object_refs.push_back(ref);
    }
  }

  CORBACommons::CorbaObjectRefList
  CorbaConfigReader::read_multi_corba_ref(
    const xsd::AdServer::Configuration::MultiCorbaObjectRefType&
      xml_corba_object_refs)
    /*throw(CorbaConfigReader::Exception)*/
  {
    CORBACommons::CorbaObjectRefList res;
    read_multi_corba_ref(xml_corba_object_refs, res);
    return res;
  }

  void
  CorbaConfigReader::read_indexed_multi_corba_ref(
    const xsd::AdServer::Configuration::MultiCorbaObjectRefType&
      xml_corba_object_refs,
    CORBACommons::CorbaObjectRefList& corba_object_refs,
    ServiceIndexRefHashMap& service_index_ref)
    /*throw(CorbaConfigReader::Exception)*/
  {
    unsigned i = 0;
    for (xsd::AdServer::Configuration::
      MultiCorbaObjectRefType::Ref_sequence::const_iterator
      cit = xml_corba_object_refs.Ref().begin();
      cit != xml_corba_object_refs.Ref().end(); ++cit, ++i)
    {
      CORBACommons::CorbaObjectRef ref;
      std::string ignore_obj_name, service_index;
      read_corba_ref(*cit, ref, ignore_obj_name, service_index);
      corba_object_refs.push_back(ref);
      if (!service_index.empty())
      {
        service_index_ref.insert(ServiceIndexRefHashMap::value_type(service_index, i));
      }
    }
  }
}
