#include <arpa/inet.h>
#include <sstream>

#include <Generics/Network.hpp>
#include <Stream/MemoryStream.hpp>
#include <String/StringManip.hpp>

#include "ZmqConfig.hpp"

namespace Config
{
  namespace
  {
    std::string
    decode_key(const std::string& encoded_key) /*throw(eh::Exception)*/
    {
      std::string decoded_key;
      String::StringManip::base64mod_decode(
        decoded_key,
        encoded_key,
        true);
      return decoded_key;
    }
  }

  int
  ZmqConfigReader::get_socket_type(const std::string& str_type)
    /*throw(Exception)*/
  {
    if (str_type == "PULL")
    {
      return ZMQ_PULL;
    }
    else if (str_type == "PUSH")
    {
      return ZMQ_PUSH;
    }
    else if (str_type == "PUB")
    {
      return ZMQ_PUB;
    }
    else if (str_type == "SUB")
    {
      return ZMQ_SUB;
    }

    throw Exception("invalid zmq socket type: " + str_type);
  }

  void
  ZmqConfigReader::set_socket_params(
    const xsd::AdServer::Configuration::ZmqSocketType& socket_config,
    zmq::socket_t& socket)
    /*throw(zmq::error_t)*/
  {
#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(3, 0, 0)
    const int MAX_SOCKET_BUF_SIZE = 65536;
#else
    const uint64_t MAX_SOCKET_BUF_SIZE = 65536;
#endif
    socket.setsockopt(ZMQ_RCVBUF, &MAX_SOCKET_BUF_SIZE, sizeof(MAX_SOCKET_BUF_SIZE));
    socket.setsockopt(ZMQ_SNDBUF, &MAX_SOCKET_BUF_SIZE, sizeof(MAX_SOCKET_BUF_SIZE));

#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 0, 0)
    const int TCP_KEEPALIVE = 1;
    socket.setsockopt(ZMQ_TCP_KEEPALIVE, &TCP_KEEPALIVE, sizeof(TCP_KEEPALIVE));
#endif

    if (socket_config.hwm().present())
    {
#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(3, 0, 0)
      const int hwm = *socket_config.hwm();
      socket.setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));
      socket.setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
#else
      const uint64_t hwm = *socket_config.hwm();
      socket.setsockopt(ZMQ_HWM, &hwm, sizeof(hwm));
#endif
    }

    if(socket_config.linger().present())
    {
      const int linger = *socket_config.linger();
      socket.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
    }
    else
    {
      // by default no linger period
      // pending messages shall be discarded immediately when the socket is closed
      const int linger = 0;
      socket.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
    }

    if (socket_config.reconnect_interval().present())
    {
      const int reconnect_interval = *socket_config.reconnect_interval();
      socket.setsockopt(ZMQ_RECONNECT_IVL,
        &reconnect_interval, sizeof(reconnect_interval));
    }

#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 0, 0)
    if (socket_config.ServerSecurity().present())
    {
      const int as_server = 1;
      socket.setsockopt(ZMQ_CURVE_SERVER, &as_server, sizeof(as_server));
      const std::string server_secret =
        decode_key(socket_config.ServerSecurity().get().server_secret());
      socket.setsockopt(ZMQ_CURVE_SECRETKEY, server_secret.c_str(), server_secret.length());
    }

    if (socket_config.ClientSecurity().present())
    {
      const std::string server_public =
        decode_key(socket_config.ClientSecurity().get().server_public());
      const std::string client_secret =
        decode_key(socket_config.ClientSecurity().get().client_secret());
      const std::string client_public =
        decode_key(socket_config.ClientSecurity().get().client_public());
      socket.setsockopt(ZMQ_CURVE_SERVERKEY, server_public.c_str(), server_public.length());
      socket.setsockopt(ZMQ_CURVE_SECRETKEY, client_secret.c_str(), client_secret.length());
      socket.setsockopt(ZMQ_CURVE_PUBLICKEY, client_public.c_str(), client_public.length());
    }
#endif
  }

  std::string
  ZmqConfigReader::get_address(
    const xsd::AdServer::Configuration::ZmqAddressType& address_config)
    /*throw(eh::Exception)*/
  {
    std::string address = address_config.domain();

    if(address.empty())
    {
      address = "*";
    }

    if (address != "*")
    {
      char buf[4096];
      hostent addresses;

      try
      {
        Generics::Network::Resolver::get_host_by_name(
          address.c_str(),
          addresses,
          buf,
          sizeof(buf));
      }
      catch(const Generics::Network::Resolver::Exception& e)
      {
        Stream::Error err;
        err << "can't resolve hostname '" << address << "': " << e.what();
        throw Exception(err);
      }

      if (addresses.h_length > 0)
      {
        in_addr addr;
        addr.s_addr = *reinterpret_cast<in_addr_t*>(addresses.h_addr_list[0]);
        address = inet_ntoa(addr);
      }
      else
      {
        Stream::Error err;
        err << "can't get address of hostname '" << address << '\'';
        throw Exception(err);
      }
    }

    std::ostringstream oss;
    oss << "tcp://" << address << ":" << address_config.port();
    return oss.str();
  }
}
