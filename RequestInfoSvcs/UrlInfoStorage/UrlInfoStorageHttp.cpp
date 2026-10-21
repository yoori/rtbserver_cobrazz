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
    execute(UrlInfoStorage& storage, const Value& request, int operation,
      unsigned int& status, bool description)
    {
      try
      {
        if (request.kind_case() != Value::kStructValue)
        {
          throw std::invalid_argument("request must be an object");
        }

        for (const auto& field : request.struct_value().fields())
        {
          if (field.first != "url" &&
            !((operation == 1 || operation == 2) && field.first == "text") &&
            !(operation == 2 && field.first == "expected") &&
            !(operation == 4 && (field.first == "version" || field.first == "job_id")))
          {
            throw std::invalid_argument("unknown field: " + field.first);
          }
        }

        const auto& url = string_field(request, "url");
        Value result;
        auto& fields = *result.mutable_struct_value()->mutable_fields();
        if (operation == 4)
        {
          const auto version = co_await storage.order_url_description_full_indexing(
            url, string_field(request, "version"), string_field(request, "job_id"));
          if (!version)
          {
            status = 409;
            co_return error_value(status, "VERSION_CONFLICT");
          }
          fields["updated"].set_bool_value(true);
          fields["version"].set_string_value(*version);
        }
        else if (operation == 2)
        {
          const auto& expected = string_field(request, "expected");
          const auto& text = string_field(request, "text");
          bool updated;
          if (description)
          {
            updated = co_await storage.compare_exchange_url_description(url, expected, text);
          }
          else
          {
            updated = co_await storage.compare_exchange_url_profile(url, expected, text);
          }
          fields["updated"].set_bool_value(updated);
        }
        else if (operation == 1)
        {
          if (description)
          {
            co_await storage.set_url_description(url, string_field(request, "text"));
          }
          else
          {
            co_await storage.set_url_profile(url, string_field(request, "text"));
          }
        }
        else
        {
          std::string text;
          if (description)
          {
            text = co_await storage.get_url_description(url);
          }
          else
          {
            text = co_await storage.get_url_profile(url);
          }
          fields["text"].set_string_value(text);
          if (operation == 3)
          {
            fields["version"].set_string_value(UrlInfoStorage::description_version(text));
          }
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
    handle_count(UrlInfoStorage_var storage, HttpServer::Request request, bool description)
    {
      if (request.method != "GET")
      {
        co_return HttpServer::Response{405, "application/json",
          to_json(error_value(405, "GET required"))};
      }

      try
      {
        std::uint64_t count;
        if (description)
        {
          count = co_await storage->get_url_description_count();
        }
        else
        {
          count = co_await storage->get_url_profile_count();
        }
        co_return HttpServer::Response{200, "application/json",
          "{\"count\":" + std::to_string(count) + "}"};
      }
      catch (const std::exception& error)
      {
        co_return HttpServer::Response{500, "application/json",
          to_json(error_value(500, error.what()))};
      }
    }

    Commons::StartableAwaitable<HttpServer::Response>
    handle_request(UrlInfoStorage_var storage, HttpServer::Request request, int operation,
      bool description = false)
    {
      if (request.method != "POST")
      {
        co_return HttpServer::Response{
          405,
          "application/json",
          to_json(error_value(405, "POST required"))
        };
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
          *values->Add() = co_await execute(*storage, item, operation, item_status, description);
        }
      }
      else
      {
        output = co_await execute(*storage, input, operation, status, description);
      }

      co_return HttpServer::Response{status, "application/json", to_json(output)};
    }
  }

  void add_url_info_storage_http_handlers(HttpServer& server, UrlInfoStorage* storage)
  {
    UrlInfoStorage_var holder = ReferenceCounting::add_ref(storage);
    server.add_handler("/get_url_profile_count", [holder](HttpServer::Request request)
    {
      return handle_count(holder, std::move(request), false);
    });
    server.add_handler("/get_url_description_count", [holder](HttpServer::Request request)
    {
      return handle_count(holder, std::move(request), true);
    });
    server.add_handler("/get_url_profile", [holder](HttpServer::Request request)
      {
        return handle_request(holder, std::move(request), false);
      });
    server.add_handler("/set_url_profile", [holder](HttpServer::Request request)
      {
        return handle_request(holder, std::move(request), true);
      });
    server.add_handler("/compare_exchange_url_profile", [holder](HttpServer::Request request)
      {
        return handle_request(holder, std::move(request), 2);
      });
    server.add_handler("/get_url_description", [holder](HttpServer::Request request)
      {
        return handle_request(holder, std::move(request), 0, true);
      });
    server.add_handler("/set_url_description", [holder](HttpServer::Request request)
      {
        return handle_request(holder, std::move(request), 1, true);
      });
    server.add_handler("/compare_exchange_url_description", [holder](HttpServer::Request request)
      {
        return handle_request(holder, std::move(request), 2, true);
      });
    server.add_handler("/get_url_description_versioned", [holder](HttpServer::Request request)
    {
      return handle_request(holder, std::move(request), 3, true);
    });
    server.add_handler("/order_url_description_full_indexing", [holder](HttpServer::Request request)
    {
      return handle_request(holder, std::move(request), 4, true);
    });
  }
}
