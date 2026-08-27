#include <String/AsciiStringManip.hpp>
#include <Commons/ErrorHandler.hpp>

#include "FrontendInterface.hpp"

namespace FrontendCommons
{
  namespace
  {
    const String::AsciiStringManip::CharCategory NONTOKEN("\x01-\x31()<>@,;:\\\"/[]?={} \t", true);

    const String::AsciiStringManip::Caseless FORMURL("application/x-www-form-urlencoded");

    RequestTask
    write_response_task(RequestTask request_task, FCGI::BaseHttpResponseWriter_var response_writer)
      noexcept
    {
      try
      {
        auto result = co_await std::move(request_task);
        if (!result.already_written && response_writer)
        {
          FCGI::HttpResponse_var response = result.response;
          if (!response)
          {
            response = new FCGI::HttpResponse();
          }

          response_writer->write(result.status, response);
        }
      }
      catch(...)
      {
        try
        {
          if (response_writer)
          {
            FCGI::HttpResponse_var response(new FCGI::HttpResponse());
            response_writer->write(500, response);
          }
        }
        catch(...)
        {}
      }

      co_return RequestResult::written();
    }
  }

  // FrontendInterface::Configuration

  FrontendInterface::Configuration::Configuration(const char* config_path) :
    config_path_(config_path)
  { }

  void
  FrontendInterface::Configuration::read()
    /*throw(InvalidConfiguration)*/
  {
    Config::ErrorHandler error_handler;

    try
    {
      config_ = xsd::AdServer::Configuration::FeConfiguration(config_path_.c_str(), error_handler);

      if (error_handler.has_errors())
      {
        std::string error_string;
        throw InvalidConfiguration(error_handler.text(error_string));
      }
    }
    catch (const xml_schema::parsing& e)
    {
      Stream::Error err;
      err << "Can't parse config file '" << config_path_ << "'." << ": ";
      if (error_handler.has_errors())
      {
        std::string error_string;
        err << error_handler.text(error_string);
      }
      throw InvalidConfiguration(err);
    }
  }

  // FrontendInterface
  bool
  FrontendInterface::parse_args_(FCGI::HttpRequestHolder_var request_holder)
    /*throw(eh::Exception)*/
  {
    FCGI::HttpRequest& request = request_holder->request();

    HTTP::ParamList params;

    try
    {
      // read parameters
      if (!request.args().empty())
      {
        FCGI::HttpRequest::parse_params(request.args(), params);
      }

      if (request.method() == FCGI::HttpRequest::RM_POST)
      {
        bool params_in_body = false;

        for (auto header_it = request.headers().begin();
          header_it != request.headers().end(); ++header_it)
        {
          if (header_it->name == String::AsciiStringManip::Caseless("content-type") &&
            FORMURL.start(header_it->value) &&
            NONTOKEN(header_it->value[FORMURL.str.length()]))
          {
            params_in_body = true;
            break;
          }
        }

        if (params_in_body)
        {
          FCGI::HttpRequest::parse_params(request.body(), params);
        }
      }

      request.set_params(std::move(params));
    }
    catch(const String::StringManip::InvalidFormatException&)
    {
      return false;
    }

    return true;
  }

  void
  FrontendInterface::handle_request_noparams(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::BaseHttpResponseWriter_var response_writer)
    /*throw(eh::Exception)*/
  {
    if (parse_args_(request_holder))
    {
      handle_request(std::move(request_holder), std::move(response_writer));
    }
    else
    {
      FCGI::HttpResponse_var response(new FCGI::HttpResponse(1));
      response_writer->write(400, response);
    }
  }

  RequestTask
  CoroFrontendInterface::co_handle_request_noparams(FCGI::HttpRequestHolder_var request_holder)
    noexcept
  {
    if (parse_args_(request_holder))
    {
      auto result = co_await co_handle_request(std::move(request_holder));
      co_return std::move(result);
    }

    FCGI::HttpResponse_var response(new FCGI::HttpResponse());
    co_return RequestResult{400, response};
  }

  void
  CoroFrontendInterface::handle_request(
    FCGI::HttpRequestHolder_var request,
    FCGI::BaseHttpResponseWriter_var response_writer)
    noexcept
  {
    write_response_task(
      co_handle_request(std::move(request)),
      std::move(response_writer)).start_detached({});
  }

  void
  CoroFrontendInterface::handle_request_noparams(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::BaseHttpResponseWriter_var response_writer)
    noexcept
  {
    if (parse_args_(request_holder))
    {
      write_response_task(
        co_handle_request(std::move(request_holder)),
        std::move(response_writer)).start_detached({});
    }
    else
    {
      FCGI::HttpResponse_var response(new FCGI::HttpResponse());
      response_writer->write(400, response);
    }
  }

}
