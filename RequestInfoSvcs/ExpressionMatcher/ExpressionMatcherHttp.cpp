#include "ExpressionMatcherHttp.hpp"

#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <Commons/Coro/SetAwaitable.hpp>
#include <Commons/FastJsonParser.hpp>
#include <Commons/JsonFormatter.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserNavigationProfile.hpp>

namespace AdServer::RequestInfoSvcs
{
  namespace
  {
    using HttpServer = AdServer::Commons::HttpServer::HttpServer;
    using Profile = Generics::ConstSmartMemBuf_var;

    class InvalidRequest final: public std::runtime_error
    {
    public:
      using std::runtime_error::runtime_error;
    };

    struct UserNavigationHttpRequest
    {
      std::vector<std::string> user_ids;
      std::optional<std::uint32_t> date;
      bool user_ids_array = false;
    };

    class UserIdsProcessor final: public AdServer::Commons::FastJsonParser<>::ValueProcessor
    {
    public:
      void array_started(std::string_view, void* context) const override
      {
        request_(context).user_ids_array = true;
      }

      void process_string(std::string_view value, std::string_view, void* context) const override
      {
        if (!request_(context).user_ids_array)
        {
          throw InvalidRequest("user_ids must be an array");
        }

        request_(context).user_ids.emplace_back(value);
      }

      void process_string(std::string&& value, std::string_view, void* context) const override
      {
        if (!request_(context).user_ids_array)
        {
          throw InvalidRequest("user_ids must be an array");
        }

        request_(context).user_ids.emplace_back(std::move(value));
      }

    private:
      static UserNavigationHttpRequest& request_(void* context) noexcept
      {
        return *static_cast<UserNavigationHttpRequest*>(context);
      }
    };

    std::uint32_t
    parse_date(std::string_view value)
    {
      const Generics::Time date(std::string(value), "%Y-%m-%d");
      if (date.tv_sec < 0 ||
        static_cast<std::uint64_t>(date.tv_sec) > std::numeric_limits<std::uint32_t>::max())
      {
        throw InvalidRequest("date is out of range");
      }

      return static_cast<std::uint32_t>(date.tv_sec);
    }

    class DateProcessor final: public AdServer::Commons::FastJsonParser<>::ValueProcessor
    {
    public:
      void process_string(std::string_view value, std::string_view, void* context) const override
      {
        request_(context).date = parse_date(value);
      }

      void process_string(std::string&& value, std::string_view path, void* context) const override
      {
        process_string(std::string_view(value), path, context);
      }

      void process_integer(std::int64_t value, std::string_view, void* context) const override
      {
        if (value < 0 ||
          static_cast<std::uint64_t>(value) > std::numeric_limits<std::uint32_t>::max())
        {
          throw InvalidRequest("date is out of range");
        }

        request_(context).date = static_cast<std::uint32_t>(value);
      }

    private:
      static UserNavigationHttpRequest& request_(void* context) noexcept
      {
        return *static_cast<UserNavigationHttpRequest*>(context);
      }
    };

    AdServer::Commons::FastJsonParser<>
    make_request_parser()
    {
      AdServer::Commons::FastJsonParser<>::ProcessorSet processors;
      processors.add_processor("user_ids", std::make_shared<UserIdsProcessor>());
      processors.add_processor("date", std::make_shared<DateProcessor>());
      return AdServer::Commons::FastJsonParser<>(std::move(processors));
    }

    const AdServer::Commons::FastJsonParser<>&
    request_parser()
    {
      static const AdServer::Commons::FastJsonParser<> parser = make_request_parser();
      return parser;
    }

    UserNavigationHttpRequest
    parse_request(std::string_view body)
    {
      UserNavigationHttpRequest request;
      request_parser().parse(body, &request);
      if (request.user_ids.empty())
      {
        throw InvalidRequest("user_ids must not be empty");
      }

      return request;
    }

    struct ProfileResult
    {
      Profile profile;
    };

    AdServer::Commons::StartableAwaitable<void>
    co_get_profile(
      ExpressionMatcherImpl* expression_matcher,
      AdServer::Commons::UserId user_id,
      std::optional<std::uint32_t> date,
      ProfileResult& result)
    {
      result.profile = co_await expression_matcher->co_get_user_navigation_profile(
        std::move(user_id),
        date);
    }

    AdServer::Commons::StartableAwaitable<std::vector<ProfileResult>>
    co_get_profiles(
      ExpressionMatcherImpl* expression_matcher,
      std::vector<AdServer::Commons::UserId> user_ids,
      std::optional<std::uint32_t> date)
    {
      std::vector<ProfileResult> results(user_ids.size());
      std::vector<AdServer::Commons::StartableAwaitable<void>> operations;
      operations.reserve(user_ids.size());
      for (std::size_t i = 0; i < user_ids.size(); ++i)
      {
        operations.emplace_back(co_get_profile(
          expression_matcher,
          std::move(user_ids[i]),
          date,
          results[i]));
      }

      co_await AdServer::Commons::SetAwaitable(std::move(operations));
      co_return results;
    }

    HttpServer::Response
    error_response(unsigned int status, std::string_view message)
    {
      std::string body;
      {
        AdServer::Commons::JsonFormatter json(body);
        json.add_escaped_string("error", message);
      }
      body += '\n';
      return {status, "application/json", std::move(body)};
    }

    std::string
    format_response(
      const std::vector<std::string>& user_ids,
      const std::vector<ProfileResult>& results)
    {
      std::string body;
      {
        AdServer::Commons::JsonFormatter root(body);
        AdServer::Commons::JsonObject profiles(root.add_array("profiles"));
        for (std::size_t i = 0; i < results.size(); ++i)
        {
          AdServer::Commons::JsonObject profile_json(profiles.add_object());
          profile_json.add_escaped_string("user_id", user_ids[i]);
          profile_json.add_boolean("found", results[i].profile.in() != nullptr);
          AdServer::Commons::JsonObject navigations(profile_json.add_array("navigations"));
          if (results[i].profile.in())
          {
            const UserNavigationProfileReader profile(
              results[i].profile->membuf().data(),
              results[i].profile->membuf().size());
            for (const auto navigation : profile.navigations())
            {
              AdServer::Commons::JsonObject navigation_json(navigations.add_object());
              navigation_json.add_escaped_string(
                "date",
                Generics::Time(navigation.date()).get_gm_time().format("%F"));
              navigation_json.add_escaped_string("url", navigation.url());
              navigation_json.add_number("count", navigation.count());
            }
          }
        }
      }
      body += '\n';
      return body;
    }
  }

  AdServer::Commons::HttpServer::HttpServer::Handler
  make_expression_matcher_stats_http_handler(ExpressionMatcherImpl* expression_matcher)
  {
    ExpressionMatcherImpl_var expression_matcher_holder(
      ReferenceCounting::add_ref(expression_matcher));
    return [expression_matcher_holder = std::move(expression_matcher_holder)](
      const HttpServer::Request& request)
    {
      if (request.method != "GET")
      {
        return error_response(405, "only GET is supported");
      }

      const auto sizes = expression_matcher_holder->profile_sizes();
      std::string body;
      {
        AdServer::Commons::JsonFormatter json(body);
        json.add_number("user_inventory_profiles", sizes.user_inventory);
        json.add_number("user_navigation_profiles", sizes.user_navigation);
        json.add_number("user_trigger_match_profiles", sizes.user_trigger_match);
        json.add_number(
          "temporary_user_trigger_match_profiles",
          sizes.temporary_user_trigger_match);
        json.add_number("request_trigger_match_profiles", sizes.request_trigger_match);
        json.add_number("household_colo_reach_profiles", sizes.household_colo_reach);
      }
      body += '\n';

      return HttpServer::Response{200, "application/json", std::move(body)};
    };
  }

  AdServer::Commons::HttpServer::HttpServer::Handler
  make_user_navigation_profile_http_handler(ExpressionMatcherImpl* expression_matcher)
  {
    ExpressionMatcherImpl_var expression_matcher_holder(
      ReferenceCounting::add_ref(expression_matcher));
    return [expression_matcher_holder = std::move(expression_matcher_holder)](
      const HttpServer::Request& request)
    {
      if (request.method != "POST")
      {
        return error_response(405, "only POST is supported");
      }

      UserNavigationHttpRequest parsed_request;
      std::vector<AdServer::Commons::UserId> user_ids;
      try
      {
        parsed_request = parse_request(request.body);
        user_ids.reserve(parsed_request.user_ids.size());
        for (const auto& user_id : parsed_request.user_ids)
        {
          user_ids.emplace_back(user_id);
        }
      }
      catch (const std::exception& ex)
      {
        return error_response(400, ex.what());
      }

      try
      {
        const auto results = AdServer::Commons::sync_wait(co_get_profiles(
          expression_matcher_holder.in(),
          std::move(user_ids),
          parsed_request.date));
        return HttpServer::Response{
          200,
          "application/json",
          format_response(parsed_request.user_ids, results)
        };
      }
      catch (const ExpressionMatcherImpl::NotReady& ex)
      {
        return error_response(503, ex.what());
      }
      catch (const std::exception& ex)
      {
        return error_response(500, ex.what());
      }
    };
  }
}
