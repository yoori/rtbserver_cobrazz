#include "PostClickActionValue.hpp"

#include <memory>
#include <regex>

#include <Commons/FastJsonParser.hpp>
#include <Commons/ExternalMetrics.hpp>

namespace AdServer::RequestInfoSvcs
{
  std::string_view
  post_click_action_type(std::string_view action_name) noexcept
  {
    return action_name.substr(0, action_name.find(':'));
  }

  namespace
  {
    using Parser = AdServer::Commons::FastJsonParser<>;

    enum class Field : unsigned int
    {
      BOUNCED = 1,
      SESSION_TIME = 2,
      PAGE_VIEWS = 4,
      NEW_USER = 8,
      CONNECTION_TYPE = 16,
      SOURCE_DATE = 32,
      REPORTING_COMPARABLE = 64,
      ROBOT = 128,
      ROBOT_AVAILABLE = 256,
      VISITS = 512,
      GOALS = 1024
    };

    constexpr unsigned int ALL_FIELDS = 15;
    constexpr unsigned int EXTERNAL_FIELDS = 48;
    constexpr unsigned int ROBOT_FIELDS = 384;

    struct ParseContext
    {
      PostClickActionValue value;
      std::uint64_t visits = 0;
      std::uint64_t goals = 0;
      unsigned int fields = 0;
    };

    class FieldProcessor final: public Parser::ValueProcessor
    {
    public:
      explicit FieldProcessor(Field field, bool legacy_connection = false) noexcept
        : field_(field), legacy_connection_(legacy_connection)
      {}

      void
      process_integer(int64_t value, std::string_view, void* context) const override
      {
        if (field_ == Field::BOUNCED || field_ == Field::NEW_USER ||
            field_ == Field::REPORTING_COMPARABLE || field_ == Field::ROBOT ||
            field_ == Field::ROBOT_AVAILABLE)
        {
          if (value != 0 && value != 1)
          {
            throw Parser::UnexpectedType("Post-click boolean metric must be 0 or 1");
          }
        }
        else if (value < 0)
        {
          throw Parser::UnexpectedType("Post-click metric must be non-negative");
        }

        set_value_(static_cast<std::uint64_t>(value), context);
      }

      void
      process_bool(bool value, std::string_view, void* context) const override
      {
        if (field_ != Field::BOUNCED && field_ != Field::NEW_USER &&
            field_ != Field::REPORTING_COMPARABLE && field_ != Field::ROBOT &&
            field_ != Field::ROBOT_AVAILABLE)
        {
          throw Parser::UnexpectedType("Post-click numeric metric must be an integer");
        }

        set_value_(value ? 1 : 0, context);
      }

    private:
      void
      set_value_(std::uint64_t value, void* context) const
      {
        auto& parse_context = *static_cast<ParseContext*>(context);
        const auto field_mask = static_cast<unsigned int>(field_);
        if (parse_context.fields & field_mask)
        {
          throw Parser::ParseError("Duplicate post-click metric");
        }

        parse_context.fields |= field_mask;

        switch (field_)
        {
          case Field::BOUNCED:
            parse_context.value.landing_bounced = value != 0;
            break;
          case Field::SESSION_TIME:
            parse_context.value.landing_session_time = value;
            break;
          case Field::PAGE_VIEWS:
            parse_context.value.landing_page_views = value;
            break;
          case Field::NEW_USER:
            parse_context.value.landing_is_new_user = value != 0;
            break;
          case Field::ROBOT:
            parse_context.value.landing_is_robot = value != 0;
            break;
          case Field::ROBOT_AVAILABLE:
            parse_context.value.landing_robot_available = value != 0;
            break;
          case Field::CONNECTION_TYPE:
            if (legacy_connection_)
            {
              value = value ? Commons::YANDEX_METRIKA : Commons::INTERNAL_METRICS;
            }

            if (value > Commons::MAX_METRICS_CONNECTION_TYPE)
            {
              throw Parser::ParseError("Invalid external metrics connection type");
            }

            parse_context.value.external_metrics_connection_type_id = value;
            break;
          case Field::REPORTING_COMPARABLE:
            parse_context.value.reporting_comparable = value != 0;
            break;
          case Field::SOURCE_DATE:
            break;
          case Field::VISITS:
            parse_context.visits = value;
            break;
          case Field::GOALS:
            parse_context.goals = value;
            break;
        }
      }

      Field field_;
      bool legacy_connection_;
    };

    class EventDateProcessor final: public Parser::ValueProcessor
    {
    public:
      void
      process_string(std::string_view value, std::string_view, void* context) const override
      {
        auto& parse_context = *static_cast<ParseContext*>(context);
        const auto field_mask = static_cast<unsigned int>(Field::SOURCE_DATE);
        if (parse_context.fields & field_mask)
        {
          throw Parser::ParseError("Duplicate source date");
        }

        parse_context.value.source_date = Generics::Time(
          String::SubString(value.data(), value.size()),
          "%Y-%m-%d",
          true);
        parse_context.fields |= field_mask;
      }
    };

    Parser
    make_parser()
    {
      Parser::ProcessorSet processors;
      processors.add_processor(
        "landing_bounced",
        std::make_shared<FieldProcessor>(Field::BOUNCED));
      processors.add_processor(
        "landing_session_time",
        std::make_shared<FieldProcessor>(Field::SESSION_TIME));
      processors.add_processor(
        "landing_page_views",
        std::make_shared<FieldProcessor>(Field::PAGE_VIEWS));
      processors.add_processor(
        "landing_is_new_user",
        std::make_shared<FieldProcessor>(Field::NEW_USER));
      processors.add_processor(
        "landing_is_robot",
        std::make_shared<FieldProcessor>(Field::ROBOT));
      processors.add_processor(
        "landing_robot_available",
        std::make_shared<FieldProcessor>(Field::ROBOT_AVAILABLE));
      processors.add_processor(
        "external_metrics_connection_type_id",
        std::make_shared<FieldProcessor>(Field::CONNECTION_TYPE));
      processors.add_processor(
        "source_date",
        std::make_shared<EventDateProcessor>());
      processors.add_processor(
        "reporting_comparable",
        std::make_shared<FieldProcessor>(Field::REPORTING_COMPARABLE));
      processors.add_processor("visits", std::make_shared<FieldProcessor>(Field::VISITS));
      processors.add_processor("goals", std::make_shared<FieldProcessor>(Field::GOALS));
      // Pending profiles and queued logs may still contain the pre-normalization names.
      processors.add_processor("external_metrics_connection_id",
        std::make_shared<FieldProcessor>(Field::CONNECTION_TYPE, true));
      processors.add_processor("yandex_ref_id",
        std::make_shared<FieldProcessor>(Field::CONNECTION_TYPE, true));
      processors.add_processor("yandex_event_date", std::make_shared<EventDateProcessor>());
      processors.add_processor("yandex_reporting_comparable",
        std::make_shared<FieldProcessor>(Field::REPORTING_COMPARABLE));
      return Parser(std::move(processors));
    }
  }

  PostClickActionValue
  parse_post_click_action_value(std::string_view value)
  {
    static const Parser parser = make_parser();

    ParseContext context;
    parser.parse(value, &context);
    if (context.fields != ALL_FIELDS)
    {
      if ((context.fields & ALL_FIELDS) != ALL_FIELDS)
      {
        throw Parser::ParseError("Post-click action value does not contain all landing metrics");
      }

      if ((context.fields & EXTERNAL_FIELDS) != 0 &&
          (context.fields & EXTERNAL_FIELDS) != EXTERNAL_FIELDS)
      {
        throw Parser::ParseError("Post-click action value contains incomplete external metadata");
      }

      if ((context.fields & ROBOT_FIELDS) != 0 &&
          (context.fields & ROBOT_FIELDS) != ROBOT_FIELDS)
      {
        throw Parser::ParseError("Post-click action contains incomplete robot metadata");
      }
    }

    return context.value;
  }

  bool is_post_click_conversion(std::string_view name) noexcept
  {
    return name.substr(0, 3) == "ym_";
  }

  PostClickConversionValue
  parse_post_click_conversion(std::string_view name, std::string_view value)
  {
    static const Parser parser = make_parser();
    static const std::regex name_pattern(
      "ym_[1-9][0-9]*_[1-9][0-9]*:([vg]):([0-9]{4}-[0-9]{2}-[0-9]{2}):[0-9]+(:[0-9]+)?");
    std::match_results<std::string_view::const_iterator> match;
    if (!std::regex_match(name.begin(), name.end(), match, name_pattern) ||
        (match[1] == "g") != match[3].matched)
    {
      throw Parser::ParseError("Invalid post-click conversion event name");
    }

    ParseContext context;
    parser.parse(value, &context);
    constexpr unsigned int REQUIRED = 16 | 32 | 512 | 1024;
    if (context.fields != REQUIRED ||
        context.value.external_metrics_connection_type_id != Commons::YANDEX_METRIKA ||
        context.visits > 1 || context.goals > 1 || context.visits + context.goals != 1 ||
        (match[1] == "v") != (context.visits == 1))
    {
      throw Parser::ParseError("Invalid post-click conversion metrics");
    }

    if (context.value.source_date.get_gm_time().format("%Y-%m-%d") != match[2].str())
    {
      throw Parser::ParseError("Post-click conversion event date mismatch");
    }

    return {context.visits, context.goals, context.value.external_metrics_connection_type_id,
      context.value.source_date};
  }
}
