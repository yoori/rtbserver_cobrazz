#include <String/AsciiStringManip.hpp>
#include <Commons/ErrorHandler.hpp>

#include "FrontendInterface.hpp"

namespace FrontendCommons
{
  namespace
  {
    const String::AsciiStringManip::CharCategory NONTOKEN(
      "\x01-\x31()<>@,;:\\\"/[]?={} \t", true);

    const String::AsciiStringManip::Caseless FORMURL(
      "application/x-www-form-urlencoded");
  }

  // FrontendInterface::Configuration
    
  FrontendInterface::Configuration::Configuration(
    const char* config_path) :
    config_path_(config_path)
  { }
  
  void
  FrontendInterface::Configuration::read()
    /*throw(InvalidConfiguration)*/
  {
    Config::ErrorHandler error_handler;
    
    try
    {
      config_ =
        xsd::AdServer::Configuration::FeConfiguration(
          config_path_.c_str(), error_handler);
    
      if(error_handler.has_errors())
      {
        std::string error_string;
        throw InvalidConfiguration(error_handler.text(error_string));
      }
    }
    catch (const xml_schema::parsing& e)
    {
      Stream::Error err;
      err << "Can't parse config file '"
          << config_path_ << "'."
          << ": ";
      if(error_handler.has_errors())
      {
        std::string error_string;
        err << error_handler.text(error_string);
      }
      throw InvalidConfiguration(err);
    }
  }

  FrontendInterface::FrontendInterface(
    FrontendCommons::HttpResponseFactory* response_factory)
    : response_factory_(ReferenceCounting::add_ref(response_factory))
  {
  }

  // FrontendInterface
  bool
  FrontendInterface::parse_args_(
    FrontendCommons::HttpRequestHolder_var request_holder,
    FrontendCommons::HttpResponseWriter_var response_writer)
    /*throw(eh::Exception)*/
  {
    FrontendCommons::HttpRequest& request = request_holder->request();

    HTTP::ParamList params;

    try
    {
      // read parameters
      if(!request.args().empty())
      {
        FrontendCommons::HttpRequest::parse_params(request.args(), params);
      }

      if(request.method() == FrontendCommons::HttpRequest::RM_POST)
      {
        bool params_in_body = false;

        for(auto header_it = request.headers().begin();
          header_it != request.headers().end(); ++header_it)
        {
          if(header_it->name == String::AsciiStringManip::Caseless("content-type") &&
            FORMURL.start(header_it->value) &&
            NONTOKEN(header_it->value[FORMURL.str.length()]))
          {
            params_in_body = true;
            break;
          }
        }

        if(params_in_body)
        {
          FrontendCommons::HttpRequest::parse_params(request.body(), params);
        }
      }

      request.set_params(std::move(params));
    }
    catch(const String::StringManip::InvalidFormatException&)
    {
      FrontendCommons::HttpResponse_var response = create_response();
      response_writer->write(400, response);
      return false;
    }

    return true;
  }

  void
  FrontendInterface::handle_request_noparams(
    FrontendCommons::HttpRequestHolder_var request_holder,
    FrontendCommons::HttpResponseWriter_var response_writer)
    /*throw(eh::Exception)*/
  {
    if(parse_args_(request_holder, response_writer))
    {
      handle_request(std::move(request_holder), std::move(response_writer));
    }
  }

  FrontendCommons::HttpResponse_var
  FrontendInterface::create_response()
  {
    return response_factory_->create();
  }
}

