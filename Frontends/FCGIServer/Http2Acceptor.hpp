#pragma once

#include <array>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <boost/asio.hpp>

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/ActiveObject.hpp>

#include <String/SubString.hpp>
#include <Logger/Logger.hpp>

#include <Commons/BoostAsioContextRunActiveObject.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/HttpResponse.hpp>

namespace AdServer::Frontends
{
  class Http2Response;

  class Http2Acceptor final:
    public Generics::RefCountableSimpleActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    Http2Acceptor(
      Logging::Logger* logger,
      FrontendCommons::FrontendInterface* frontend,
      const String::SubString& bind_address,
      unsigned long port,
      unsigned long threads,
      unsigned long max_concurrent_streams,
      unsigned long read_buffer_size,
      unsigned long max_request_size);

    Http2Acceptor(
      Logging::Logger* logger,
      FrontendCommons::FrontendInterface* frontend,
      const String::SubString& unix_socket_path,
      unsigned long threads,
      unsigned long max_concurrent_streams,
      unsigned long read_buffer_size,
      unsigned long max_request_size);

  protected:
    ~Http2Acceptor() noexcept override;

    void activate_object_() override;
    void deactivate_object_() override;
    void wait_object_() override;

  private:
    class Http2ResponseWriter;
    class Connection;
    using Connection_var = std::shared_ptr<Connection>;

    void create_accept_stub_();
    void handle_accept_(
      const Connection_var& accepted_connection,
      const boost::system::error_code& error);
    void erase_connection_(Connection* connection) noexcept;

    Logging::Logger* logger_i_() noexcept;
    FrontendCommons::FrontendInterface* frontend_i_() noexcept;

  private:
    const Logging::Logger_var logger_;
    const FrontendCommons::Frontend_var frontend_;
    const std::string bind_address_;
    const unsigned long port_;
    const unsigned long threads_;
    const unsigned long max_concurrent_streams_;
    const unsigned long read_buffer_size_;
    const unsigned long max_request_size_;
    const bool use_unix_socket_;

    std::shared_ptr<boost::asio::io_service> io_service_;
    std::unique_ptr<boost::asio::io_service::work> io_work_;
    std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
    std::unique_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      io_runner_;

    std::mutex connections_lock_;
    std::unordered_map<Connection*, Connection_var> connections_;
  };
}
