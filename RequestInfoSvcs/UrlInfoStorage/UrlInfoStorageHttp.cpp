#include "UrlInfoStorageHttp.hpp"

#include <stdexcept>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/util/json_util.h>

namespace AdServer::RequestInfoSvcs
{
  namespace
  {
    using HttpServer = Commons::HttpServer::HttpServer;
    using Value = google::protobuf::Value;

    std::string to_json(const Value& value)
    {
      std::string result;
      const auto status = google::protobuf::util::MessageToJsonString(value, &result);
      if (!status.ok())
      {
        throw std::runtime_error(status.ToString());
      }

      return result;
    }

    Value error_value(unsigned int status, const std::string& message)
    {
      Value result;
      auto& fields = *result.mutable_struct_value()->mutable_fields();
      fields["error"].set_string_value(message);
      fields["status"].set_number_value(status);
      return result;
    }

    const std::string& string_field(const Value& value, const std::string& name)
    {
      const auto& fields = value.struct_value().fields();
      const auto field = fields.find(name);
      if (field == fields.end() || field->second.kind_case() != Value::kStringValue)
      {
        throw std::invalid_argument(name + " must be a string");
      }

      return field->second.string_value();
    }

    Commons::StartableAwaitable<Value>
    execute(UrlInfoStorage& storage, const Value& request, bool write, unsigned int& status)
    {
      try
      {
        if (request.kind_case() != Value::kStructValue)
        {
          throw std::invalid_argument("request must be an object");
        }

        for (const auto& field : request.struct_value().fields())
        {
          if (field.first != "url" && !(write && field.first == "text"))
          {
            throw std::invalid_argument("unknown field: " + field.first);
          }
        }

        const auto& url = string_field(request, "url");
        Value result;
        auto& fields = *result.mutable_struct_value()->mutable_fields();
        if (write)
        {
          co_await storage.set_url_profile(url, string_field(request, "text"));
        }
        else
        {
          fields["text"].set_string_value(co_await storage.get_url_profile(url));
        }

        co_return result;
      }
      catch (const std::invalid_argument& error)
      {
        status = 400;
        co_return error_value(status, error.what());
      }
      catch (const std::exception& error)
      {
        status = 500;
        co_return error_value(status, error.what());
      }
    }

    Commons::StartableAwaitable<HttpServer::Response>
    handle_request(UrlInfoStorage_var storage, HttpServer::Request request, bool write)
    {
      if (request.method != "POST")
      {
        co_return HttpServer::Response{405, "application/json",
          to_json(error_value(405, "POST required"))};
      }

      Value input;
      if (!google::protobuf::util::JsonStringToMessage(request.body, &input).ok())
      {
        co_return HttpServer::Response{400, "application/json",
          to_json(error_value(400, "invalid JSON"))};
      }

      unsigned int status = 200;
      Value output;
      if (input.kind_case() == Value::kListValue)
      {
        auto* values = output.mutable_list_value()->mutable_values();
        for (const auto& item : input.list_value().values())
        {
          unsigned int item_status = 200;
          *values->Add() = co_await execute(*storage, item, write, item_status);
        }
      }
      else
      {
        output = co_await execute(*storage, input, write, status);
      }

      co_return HttpServer::Response{status, "application/json", to_json(output)};
    }
  }

  void add_url_info_storage_http_handlers(HttpServer& server, UrlInfoStorage* storage)
  {
    UrlInfoStorage_var holder = ReferenceCounting::add_ref(storage);
    server.add_handler("/get_url_profile", [holder](HttpServer::Request request)
    {
      return handle_request(holder, std::move(request), false);
    });
    server.add_handler("/set_url_profile", [holder](HttpServer::Request request)
    {
      return handle_request(holder, std::move(request), true);
    });
  }
}
