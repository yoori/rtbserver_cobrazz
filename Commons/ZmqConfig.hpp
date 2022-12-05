#ifndef COMMONS_ZMQCONFIG_H_
#define COMMONS_ZMQCONFIG_H_

#include <string>

#include <eh/Exception.hpp>

#include <xsd/AdServerCommons/AdServerCommons.hpp>
#include <Commons/zmq.hpp>

namespace Config
{
  class ZmqConfigReader
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  public:
    static
    int
    get_socket_type(const std::string& str_type)
      /*throw(Exception)*/;

    static
    void
    set_socket_params(
      const xsd::AdServer::Configuration::ZmqSocketType& socket_config,
      zmq::socket_t& socket)
      /*throw(zmq::error_t)*/;

    static
    std::string
    get_address(const xsd::AdServer::Configuration::ZmqAddressType& address_config)
      /*throw(eh::Exception)*/;
  };
}

#endif /* COMMONS_ZMQCONFIG_H_ */
