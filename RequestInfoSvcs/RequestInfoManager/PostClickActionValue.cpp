#include "PostClickActionValue.hpp"

#include <memory>

#include <Commons/FastJsonParser.hpp>

namespace AdServer::RequestInfoSvcs
{
  namespace
  {
    using Parser = AdServer::Commons::FastJsonParser<>;

    enum class Field : unsigned int
    {
      BOUNCED = 1,
      SESSION_TIME = 2,
      PAGE_VIEWS = 4,
      NEW_USER = 8,
      YANDEX_REF_ID = 16,
      YANDEX_EVENT_DATE = 32,
      YANDEX_REPORTING_COMPARABLE = 64
    };

    constexpr unsigned int ALL_FIELDS = 15;
    constexpr unsigned int YANDEX_FIELDS = 112;

    struct ParseContext
    {
      PostClickActionValue value;
      unsigned int fields = 0;
    };

    class FieldProcessor final: public Parser::ValueProcessor
    {
    public:
      explicit FieldProcessor(Field field) noexcept
        : field_(field)
      {}

      void
      process_integer(int64_t value, std::string_view, void* context) const override
      {
        if (field_ == Field::BOUNCED || field_ == Field::NEW_USER ||
            field_ == Field::YANDEX_REPORTING_COMPARABLE)
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
            field_ != Field::YANDEX_REPORTING_COMPARABLE)
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
          case Field::YANDEX_REF_ID:
            parse_context.value.yandex_ref_id = value;
            break;
          case Field::YANDEX_REPORTING_COMPARABLE:
            parse_context.value.yandex_reporting_comparable = value != 0;
            break;
          case Field::YANDEX_EVENT_DATE:
            break;
        }
      }

      Field field_;
    };

    class EventDateProcessor final: public Parser::ValueProcessor
    {
    public:
      void
      process_string(std::string_view value, std::string_view, void* context) const override
      {
        auto& parse_context = *static_cast<ParseContext*>(context);
        const auto field_mask = static_cast<unsigned int>(Field::YANDEX_EVENT_DATE);
        if (parse_context.fields & field_mask)
        {
          throw Parser::ParseError("Duplicate Yandex event date");
        }

        parse_context.value.yandex_event_date = Generics::Time(
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
        "yandex_ref_id",
        std::make_shared<FieldProcessor>(Field::YANDEX_REF_ID));
      processors.add_processor(
        "yandex_event_date",
        std::make_shared<EventDateProcessor>());
      processors.add_processor(
        "yandex_reporting_comparable",
        std::make_shared<FieldProcessor>(Field::YANDEX_REPORTING_COMPARABLE));
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

      if ((context.fields & YANDEX_FIELDS) != 0 &&
          (context.fields & YANDEX_FIELDS) != YANDEX_FIELDS)
      {
        throw Parser::ParseError("Post-click action value contains incomplete Yandex metadata");
      }
    }

    return context.value;
  }
}
