#include <iostream>
#include <string>
#include <sstream>

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{

  ClientRequest::ClientRequest()
    /*throw(ClientRequest::Exception, eh::Exception)*/
    : timeout_(
        AutoTest::GlobalSettings::instance().frontend_timeout()),
      bytes_out_(0),
      bytes_in_(0),
      status_(0),
      decoder_(nullptr)
  {
  }

  ClientRequest::ClientRequest(const ClientRequest& src)
    /*throw(Exception, eh::Exception)*/
    : ReferenceCounting::Interface(),
      ReferenceCounting::AtomicImpl(),
      request_headers_(src.request_headers_),
      response_headers_(src.response_headers_),
      request_data_(src.request_data_),
      response_data_(src.response_data_),
      timeout_(src.timeout_),
      bytes_out_(0),
      bytes_in_(0),
      status_(src.status_),
      decoder_(nullptr)
  {
    address(src.address());
  }

  ClientRequest::~ClientRequest() noexcept
  {
  }

  void
  ClientRequest::address(const HTTP::HTTPAddress& addr)
    /*throw(Exception, eh::Exception)*/
  {
    address_ = addr;

    http_connection_ = HTTP_Connection_var(
                                           new HTTP::HTTP_Connection(addr, 0));
  }

  bool
  ClientRequest::send(
    HTTP::HTTP_Connection::HTTP_Method method,
    bool need_response,
    Generics::Time& time)
    /*throw(Exception, eh::Exception)*/
  {
    if(http_connection_.get() == 0)
    {
      throw Exception("ClientRequest::send: http_connection_ == 0");
    }

    response_data_.erase();
    response_headers_ = request_headers_;
    bytes_out_ = 0;
    bytes_in_ = 0;

    HTTP::HTTP_Connection::HttpBody* body = 0;

    try
    {
      size_t data_len = request_data_.length();
      if (data_len)
      {
        body = new HTTP::HTTP_Connection::HttpBody(data_len);
        memcpy(body->wr_ptr(), request_data_.data(), data_len);
        body->wr_ptr(data_len);
      }

      finish_time_= Generics::Time::ZERO;
      start_time_ = Generics::Time::get_time_of_day();

      Generics::Timer timer;
      timer.start();

      http_connection_->connect(&timeout_);

      // socket options

      struct linger linger_info = {1, 0};
      if(
        http_connection_->stream().set_option(
          SOL_SOCKET,
          SO_LINGER,
          &linger_info,
          sizeof (linger_info)) == -1)
      {
        throw Exception("ClientRequest::send: unable to set socket "
                        "option (SO_LINGER)");
      }

      try
      {
        status_ = http_connection_->process_request(
          method,
          HTTP::ParamList(),
          response_headers_,
          body,
          need_response,
          &timeout_,
          &timeout_,
          &bytes_out_,
          &bytes_in_);
      }
      catch(const HTTP::HTTP_Connection::StatusException& exc)
      {
        status_ = exc.status;
        throw;
      }
      timer.stop();
      time = timer.elapsed_time();

      finish_time_ = start_time_ + time;

      if (need_response)
      {
        HTTP::HTTP_Connection::HttpBody* body_ptr = body;

        while (body_ptr)
        {
          response_data_.append(body_ptr->base(), body_ptr->size());
          body_ptr = body_ptr->cont();
        }
      }
      else
      {
        response_headers_.clear();
      }

    }
    catch (const HTTP::HTTP_Connection::Exception& e)
    {
      if (finish_time_ == Generics::Time::ZERO)
      {
        finish_time_ = Generics::Time::get_time_of_day();
      }

      time = finish_time_ - start_time_;

      if (body)
      {
        body->release();
        body = NULL;
      }

      response_headers_.clear();

      Stream::Error ostr;
      ostr << "ClientRequest::send: HTTP_Connection::Exception exception caught "
        "while requesting " << address_.url() << ". :" <<
        std::endl << e.what();

      throw Exception(ostr);
    }

    if (body)
    {
      body->release();
      body = NULL;
    }

    return true;
  }

  void
  ClientRequest::print(
    std::ostream& out,
    Decoder::Types data_type) const
  {
    const std::string& data =
      data_type == Decoder::T_REQUEST? request_data_: response_data_;

    if (decoder_ && !data.empty())
    {
      out << std::endl << std::endl;
      decoder_->decode(out, data_type, data);
      out << std::endl;
    }
    else
    {
      out << std::endl << "        ";
      out << data;
      out << std::endl << std::endl;
    }
  }

} //namespace AutoTest

