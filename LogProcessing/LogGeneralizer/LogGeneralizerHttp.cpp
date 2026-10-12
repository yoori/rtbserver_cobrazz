#include "LogGeneralizerHttp.hpp"

#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <type_traits>
#include <utility>

#include <Commons/JsonFormatter.hpp>

namespace AdServer::LogProcessing
{
  namespace
  {
    struct StatsJsonWriter
    {
      Commons::JsonFormatter& json;

      void operator()(std::size_t count) const noexcept;

      template<typename Value>
      void operator()(const Generics::Values::Key& key, const Value& value) const;
    };

    void StatsJsonWriter::operator()(std::size_t) const noexcept
    {}

    template<typename Value>
    void StatsJsonWriter::operator()(const Generics::Values::Key& key, const Value& value) const
    {
      std::string name;
      String::StringManip::json_escape_append(name, key.text());
      if constexpr (std::is_same_v<Value, Generics::Values::String>)
      {
        json.add_escaped_string(name, value);
      }
      else if constexpr (std::is_floating_point_v<Value>)
      {
        // JsonFormatter accepts preformatted numbers; keep fractions and locale-independent JSON.
        std::ostringstream number;
        number.imbue(std::locale::classic());
        if (std::isfinite(value))
        {
          number << std::setprecision(std::numeric_limits<Value>::max_digits10) << value;
        }
        else
        {
          number << "null";
        }
        json.add_number(name, number.str());
      }
      else
      {
        json.add_number(name, value);
      }
    }
  }

  Commons::HttpServer::HttpServer::Handler
  make_log_generalizer_stats_http_handler(Generics::Values* stats)
  {
    using HttpServer = Commons::HttpServer::HttpServer;
    ReferenceCounting::SmartPtr<Generics::Values> stats_holder(ReferenceCounting::add_ref(stats));
    return [stats_holder = std::move(stats_holder)](HttpServer::Request request)
    {
      if (request.method != "GET")
      {
        return Commons::HttpServer::make_ready_response(HttpServer::Response{
          405, "application/json", "{\"error\":\"only GET is supported\"}\n"});
      }

      std::string body;
      {
        Commons::JsonFormatter json(body);
        StatsJsonWriter writer{json};
        stats_holder->enumerate_all(writer);
      }
      body += '\n';
      return Commons::HttpServer::make_ready_response(
        HttpServer::Response{200, "application/json", std::move(body)});
    };
  }
}
