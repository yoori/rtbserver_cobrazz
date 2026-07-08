
#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstring>

#include <Commons/FastJsonParser.hpp>

#include "OpenRTBResponseChecker.hpp"

namespace AutoTest
{
  namespace
  {
    using FastJsonParser = AdServer::Commons::FastJsonParser<>;
    using ValueProcessor = FastJsonParser::ValueProcessor;

    struct ParseContext
    {
      std::string id;
      std::string currency;
      OpenRTBResponse::Bids bids;
      OpenRTBResponse::Bid* bid = 0;
    };

    std::string
    to_string(std::string_view value)
    {
      return std::string(value.data(), value.size());
    }

    template<typename T>
    void
    parse_integer(std::string_view value, T& result)
    {
      T parsed = T();
      const auto parse_result = std::from_chars(
        value.data(),
        value.data() + value.size(),
        parsed);
      if(parse_result.ec == std::errc() &&
        parse_result.ptr == value.data() + value.size())
      {
        result = parsed;
      }
    }

    template<typename T>
    void
    assign_integer(int64_t value, T& result)
    {
      result = static_cast<T>(value);
    }

    class StartBidProcessor final: public ValueProcessor
    {
    public:
      void
      array_started(std::string_view, void*) const override
      {}

      void
      object_started(std::string_view, void* context) const override
      {
        auto* parse_context = static_cast<ParseContext*>(context);
        parse_context->bids.push_back(OpenRTBResponse::Bid());
        parse_context->bid = &parse_context->bids.back();
      }
    };

    class ResponseStringProcessor final: public ValueProcessor
    {
    public:
      explicit
      ResponseStringProcessor(std::string ParseContext::* field)
        : field_(field)
      {}

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        static_cast<ParseContext*>(context)->*field_ = to_string(value);
      }

      void
      process_string(std::string&& value, std::string_view, void* context)
        const override
      {
        static_cast<ParseContext*>(context)->*field_ = std::move(value);
      }

    private:
      std::string ParseContext::* field_;
    };

    class BidStringProcessor final: public ValueProcessor
    {
    public:
      explicit
      BidStringProcessor(std::string OpenRTBResponse::Bid::* field)
        : field_(field)
      {}

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        auto* bid = static_cast<ParseContext*>(context)->bid;
        if(bid)
        {
          bid->*field_ = to_string(value);
        }
      }

      void
      process_string(std::string&& value, std::string_view, void* context)
        const override
      {
        auto* bid = static_cast<ParseContext*>(context)->bid;
        if(bid)
        {
          bid->*field_ = std::move(value);
        }
      }

    private:
      std::string OpenRTBResponse::Bid::* field_;
    };

    template<typename T>
    class BidIntegerProcessor final: public ValueProcessor
    {
    public:
      explicit
      BidIntegerProcessor(T OpenRTBResponse::Bid::* field)
        : field_(field)
      {}

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        auto* bid = static_cast<ParseContext*>(context)->bid;
        if(bid)
        {
          assign_integer(value, bid->*field_);
        }
      }

      void
      process_number(std::string_view value, bool, std::string_view, void* context)
        const override
      {
        auto* bid = static_cast<ParseContext*>(context)->bid;
        if(bid)
        {
          parse_integer(value, bid->*field_);
        }
      }

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        auto* bid = static_cast<ParseContext*>(context)->bid;
        if(bid)
        {
          parse_integer(value, bid->*field_);
        }
      }

      void
      process_string(std::string&& value, std::string_view path, void* context)
        const override
      {
        process_string(std::string_view(value), path, context);
      }

    private:
      T OpenRTBResponse::Bid::* field_;
    };

    class BidMoneyProcessor final: public ValueProcessor
    {
    public:
      explicit
      BidMoneyProcessor(Money OpenRTBResponse::Bid::* field)
        : field_(field)
      {}

      void
      process_float(double value, std::string_view, void* context) const override
      {
        auto* bid = static_cast<ParseContext*>(context)->bid;
        if(bid)
        {
          bid->*field_ = value;
        }
      }

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        auto* bid = static_cast<ParseContext*>(context)->bid;
        if(bid)
        {
          bid->*field_ = value;
        }
      }

      void
      process_number(std::string_view value, bool, std::string_view, void* context)
        const override
      {
        auto* bid = static_cast<ParseContext*>(context)->bid;
        if(bid)
        {
          double parsed = 0;
          Stream::Parser strm(String::SubString(value.data(), value.size()));
          strm >> parsed;
          bid->*field_ = parsed;
        }
      }

    private:
      Money OpenRTBResponse::Bid::* field_;
    };

    class BidStringListProcessor final: public ValueProcessor
    {
    public:
      explicit
      BidStringListProcessor(std::list<std::string> OpenRTBResponse::Bid::* field)
        : field_(field)
      {}

      void
      array_started(std::string_view, void*) const override
      {}

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        auto* bid = static_cast<ParseContext*>(context)->bid;
        if(bid)
        {
          (bid->*field_).push_back(to_string(value));
        }
      }

      void
      process_string(std::string&& value, std::string_view, void* context)
        const override
      {
        auto* bid = static_cast<ParseContext*>(context)->bid;
        if(bid)
        {
          (bid->*field_).push_back(std::move(value));
        }
      }

    private:
      std::list<std::string> OpenRTBResponse::Bid::* field_;
    };

    class BidOptionalStringListProcessor final: public ValueProcessor
    {
    public:
      explicit
      BidOptionalStringListProcessor(
        ExpValue<std::list<std::string> > OpenRTBResponse::Bid::* field)
        : field_(field)
      {}

      void
      array_started(std::string_view, void* context) const override
      {
        auto* bid = static_cast<ParseContext*>(context)->bid;
        if(bid)
        {
          (bid->*field_).is_set(true);
        }
      }

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        auto* bid = static_cast<ParseContext*>(context)->bid;
        if(bid)
        {
          (bid->*field_)->push_back(to_string(value));
        }
      }

      void
      process_string(std::string&& value, std::string_view, void* context)
        const override
      {
        auto* bid = static_cast<ParseContext*>(context)->bid;
        if(bid)
        {
          (bid->*field_)->push_back(std::move(value));
        }
      }

    private:
      ExpValue<std::list<std::string> > OpenRTBResponse::Bid::* field_;
    };

    template<typename T>
    class BidOptionalIntegerListProcessor final: public ValueProcessor
    {
    public:
      explicit
      BidOptionalIntegerListProcessor(
        ExpValue<std::list<T> > OpenRTBResponse::Bid::* field)
        : field_(field)
      {}

      void
      array_started(std::string_view, void* context) const override
      {
        auto* bid = static_cast<ParseContext*>(context)->bid;
        if(bid)
        {
          (bid->*field_).is_set(true);
        }
      }

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        auto* bid = static_cast<ParseContext*>(context)->bid;
        if(bid)
        {
          (bid->*field_)->push_back(static_cast<T>(value));
        }
      }

      void
      process_number(std::string_view value, bool, std::string_view, void* context)
        const override
      {
        auto* bid = static_cast<ParseContext*>(context)->bid;
        if(bid)
        {
          T result = T();
          parse_integer(value, result);
          (bid->*field_)->push_back(result);
        }
      }

    private:
      ExpValue<std::list<T> > OpenRTBResponse::Bid::* field_;
    };

    class BidOptionalIntegerProcessor final: public ValueProcessor
    {
    public:
      explicit
      BidOptionalIntegerProcessor(ExpValue<unsigned long> OpenRTBResponse::Bid::* field)
        : field_(field)
      {}

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        auto* bid = static_cast<ParseContext*>(context)->bid;
        if(bid)
        {
          bid->*field_ = static_cast<unsigned long>(value);
        }
      }

      void
      process_number(std::string_view value, bool, std::string_view, void* context)
        const override
      {
        auto* bid = static_cast<ParseContext*>(context)->bid;
        if(bid)
        {
          unsigned long result = 0;
          parse_integer(value, result);
          bid->*field_ = result;
        }
      }

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        auto* bid = static_cast<ParseContext*>(context)->bid;
        if(bid)
        {
          unsigned long result = 0;
          parse_integer(value, result);
          bid->*field_ = result;
        }
      }

      void
      process_string(std::string&& value, std::string_view path, void* context)
        const override
      {
        process_string(std::string_view(value), path, context);
      }

    private:
      ExpValue<unsigned long> OpenRTBResponse::Bid::* field_;
    };

    std::string
    normalize_json_spacing(std::string_view value)
    {
      std::string result;
      result.reserve(value.size() + 8);

      bool in_string = false;
      bool escaped = false;
      for(char ch : value)
      {
        result += ch;
        if(escaped)
        {
          escaped = false;
        }
        else if(in_string && ch == '\\')
        {
          escaped = true;
        }
        else if(ch == '"')
        {
          in_string = !in_string;
        }
        else if(!in_string && (ch == ':' || ch == ','))
        {
          result += ' ';
        }
      }

      return result;
    }

    bool
    extract_json_object_value(
      std::string_view json,
      std::string_view name,
      std::size_t& pos,
      std::string& value)
    {
      const std::string pattern = "\"" + std::string(name) + "\"";
      while((pos = json.find(pattern, pos)) != std::string_view::npos)
      {
        std::size_t value_pos = pos + pattern.size();
        while(value_pos < json.size() && std::isspace(
          static_cast<unsigned char>(json[value_pos])))
        {
          ++value_pos;
        }
        if(value_pos == json.size() || json[value_pos] != ':')
        {
          pos = value_pos;
          continue;
        }
        ++value_pos;
        while(value_pos < json.size() && std::isspace(
          static_cast<unsigned char>(json[value_pos])))
        {
          ++value_pos;
        }
        if(value_pos == json.size() || json[value_pos] != '{')
        {
          pos = value_pos;
          continue;
        }

        bool in_string = false;
        bool escaped = false;
        int depth = 0;
        for(std::size_t end_pos = value_pos; end_pos < json.size(); ++end_pos)
        {
          const char ch = json[end_pos];
          if(escaped)
          {
            escaped = false;
          }
          else if(in_string && ch == '\\')
          {
            escaped = true;
          }
          else if(ch == '"')
          {
            in_string = !in_string;
          }
          else if(!in_string && ch == '{')
          {
            ++depth;
          }
          else if(!in_string && ch == '}')
          {
            --depth;
            if(depth == 0)
            {
              value = normalize_json_spacing(
                json.substr(value_pos, end_pos - value_pos + 1));
              pos = end_pos + 1;
              return true;
            }
          }
        }

        return false;
      }

      return false;
    }

    void
    fill_raw_ext_objects(std::string_view body, OpenRTBResponse::Bids& bids)
    {
      std::size_t matching_ad_id_pos = 0;
      std::size_t nurl_pos = 0;
      for(auto& bid : bids)
      {
        extract_json_object_value(
          body,
          "matching_ad_id",
          matching_ad_id_pos,
          bid.matching_ad_id);
        extract_json_object_value(body, "nurl", nurl_pos, bid.nurl);
      }
    }

    FastJsonParser&
    openrtb_response_parser()
    {
      static FastJsonParser* parser = [] {
        FastJsonParser::ProcessorSet processors;

        processors.add_processor(
          "id",
          std::make_shared<ResponseStringProcessor>(&ParseContext::id));
        processors.add_processor(
          "cur",
          std::make_shared<ResponseStringProcessor>(&ParseContext::currency));
        processors.add_processor(
          "seatbid.bid",
          std::make_shared<StartBidProcessor>());

        processors.add_processor(
          "seatbid.bid.id",
          std::make_shared<BidStringProcessor>(&OpenRTBResponse::Bid::id));
        processors.add_processor(
          "seatbid.bid.impid",
          std::make_shared<BidStringProcessor>(&OpenRTBResponse::Bid::impid));
        processors.add_processor(
          "seatbid.bid.price",
          std::make_shared<BidMoneyProcessor>(&OpenRTBResponse::Bid::price));
        processors.add_processor(
          "seatbid.bid.adid",
          std::make_shared<BidIntegerProcessor<unsigned long> >(
            &OpenRTBResponse::Bid::adid),
          true);
        processors.add_processor(
          "seatbid.bid.crid",
          std::make_shared<BidStringProcessor>(&OpenRTBResponse::Bid::crid));
        processors.add_processor(
          "seatbid.bid.adomain",
          std::make_shared<BidStringListProcessor>(
            &OpenRTBResponse::Bid::adomain));
        processors.add_processor(
          "seatbid.bid.adm",
          std::make_shared<BidStringProcessor>(&OpenRTBResponse::Bid::adm));
        processors.add_processor(
          "seatbid.bid.nurl",
          std::make_shared<BidStringProcessor>(&OpenRTBResponse::Bid::nurl));
        processors.add_processor(
          "seatbid.bid.cid",
          std::make_shared<BidIntegerProcessor<unsigned long> >(
            &OpenRTBResponse::Bid::cid),
          true);
        processors.add_processor(
          "seatbid.bid.attr",
          std::make_shared<BidOptionalIntegerListProcessor<unsigned long> >(
            &OpenRTBResponse::Bid::attr));
        processors.add_processor(
          "seatbid.bid.fmt",
          std::make_shared<BidOptionalIntegerProcessor>(
            &OpenRTBResponse::Bid::fmt),
          true);
        processors.add_processor(
          "seatbid.bid.cat",
          std::make_shared<BidOptionalStringListProcessor>(
            &OpenRTBResponse::Bid::cat));
        processors.add_processor(
          "seatbid.bid.ext.ad_ox_cats",
          std::make_shared<BidOptionalIntegerListProcessor<unsigned long> >(
            &OpenRTBResponse::Bid::ad_ox_cats));

        return new FastJsonParser(std::move(processors));
      }();

      return *parser;
    }

    template<typename Exp, typename Got>
    bool check_bid_value(
      const char* name,
      const Exp& exp,
      const Got& got,
      Stream::Error& err)
    {
      if (exp.is_set() && !equal(*exp, got))
      {
        err << std::endl << "    " <<  name << " (" << *exp <<
          " != " << got << ")";
        return false;
      }
      return true;
    }

    template<typename Exp, typename Got>
    bool match_bid_value(
      const char* name,
      const Exp& exp,
      const Got& got,
      Stream::Error& err)
    {
      if (exp.is_set() && !equal(got, ComparableRegExp(*exp)))
      {
        err << std::endl << "    " <<  name << " (" << got <<
          " doesn't match " << *exp << ")";
        return false;
      }
      return true;
    }

    template<typename ExpSeq, typename GotSeq>
    bool check_bid_seq(
      const char* name,
      const ExpSeq& exp,
      const GotSeq& got,
      Stream::Error& err)
    {
      if (exp.is_set() && !equal_seq(*exp, got))
      {
        err << std::endl << "    " << name << " (" << seq_to_str(*exp) <<
          " != " << seq_to_str(got) << ")";
        return false;
      }
      return true;
    }

    template<typename T>
    bool check_bid_value(
      const char* name,
      const ExpValue<T>& exp,
      const ExpValue<T>& got,
      Stream::Error& err)
    {
      if (exp.is_set() && !equal(*exp, *got))
      {
        err << std::endl << "    " <<  name << " (" << *exp <<
          " != " << *got << ")";
        return false;
      }
      return true;
    }

    template<typename T>
    bool check_bid_seq(
      const char* name,
      const ExpValue<T>& exp,
      const ExpValue<T>& got,
      Stream::Error& err)
    {
      if (exp.is_set() && !equal_seq(*exp, *got))
      {
        err << std::endl << "    " << name << " (" << seq_to_str(*exp) <<
          " != " << seq_to_str(*got) << ")";
        return false;
      }
      return true;
    }

    template<typename T>
    bool check_existance(
      const char* name,
      const ExpValue<bool>& exp,
      const ExpValue<T>& got,
      Stream::Error& err)
    {
      if (exp.is_set() && (*exp ^ got.is_set()))
      {
        err << std::endl << "    " << name << (*exp? " not exist": " exist");
        return false;
      }
      return true;
    }
  }

  // OpenRTBResponse

  OpenRTBResponse::OpenRTBResponse(
    const std::string& body)
    : status_(JSON_PARSE_OK)
  {
    try
    {
      ParseContext context;
      openrtb_response_parser().parse(body, &context);
      fill_raw_ext_objects(body, context.bids);
      id_ = std::move(context.id);
      currency_ = std::move(context.currency);
      bids_ = std::move(context.bids);
    }
    catch(const eh::Exception&)
    {
      status_ = JSON_PARSE_ERROR;
    }
  }

  OpenRTBResponse::~OpenRTBResponse()
  { }

  // OpenRTBCid
  OpenRTBCid::OpenRTBCid(AdClient& client) :
    OpenRTBResponse(client.req_response_data()),
    OpenRTBIntSlice(
      OpenRTBResponse::bids(),
      &OpenRTBResponse::Bid::cid)
  { }

  OpenRTBCid::~OpenRTBCid()
  { }

  // OpenRTBResponseChecker
  OpenRTBResponseChecker::Expected::Expected()
  { }

  OpenRTBResponseChecker::OpenRTBResponseChecker(
    const AdClient& client,
    const Expected& expected) :
    response_(client.req_response_data())
  {
    expected_.push_back(expected);
  }

  OpenRTBResponseChecker::OpenRTBResponseChecker(
    const AdClient& client,
    const ExpectedList& expected) :
    response_(client.req_response_data()),
    expected_(expected)
  { }

  void
  OpenRTBResponseChecker::add_expected(
    const Expected& expected)
  {
    expected_.push_back(expected);
  }

  const OpenRTBResponse::Bid& OpenRTBResponseChecker::bids(size_t index) const
  {
    return *std::next(response_.bids().begin(), index);
  }

  OpenRTBResponseChecker::~OpenRTBResponseChecker() noexcept
  { }


  bool
  OpenRTBResponseChecker::check(
    bool throw_error)
    /*throw(eh::Exception)*/
  {
    if (response_.bids().size() != expected_.size())
    {
      if (throw_error)
      {
        Stream::Error err;
        err << "Unexpected bid size" << " (" <<
          expected_.size() << "(expected) != " << response_.bids().size() << "(got))";
        throw CheckFailed(err);
      }
      return false;
    }
    ExpectedList::const_iterator
      e_it = expected_.begin(),
      e_end = expected_.end();
    OpenRTBResponse::Bids::const_iterator
      g_it = response_.bids().begin(),
      g_end = response_.bids().end();
    unsigned long seq_idx = 0;
    bool result = true;
    Stream::Error err;
    err << "Unexpected bid[" << seq_idx << "]:";
    for (; e_it != e_end && g_it != g_end; ++e_it, ++g_it, ++seq_idx)
    {
      result&=check_bid_value("id", e_it->id_, g_it->id, err);
      result&=check_bid_value("impid", e_it->impid_, g_it->impid, err);
      result&=check_bid_value("price", e_it->price_, g_it->price, err);
      result&=check_bid_value("adid", e_it->adid_, g_it->adid, err);
      result&=check_bid_value("crid", e_it->crid_, g_it->crid, err);
      result&=check_bid_seq("adomain", e_it->adomain_, g_it->adomain, err);
      result&=match_bid_value("adm", e_it->adm_, g_it->adm, err);
      result&=match_bid_value("nurl", e_it->nurl_, g_it->nurl, err);
      result&=check_bid_value("cid", e_it->cid_, g_it->cid, err);
      result&=check_bid_seq("attr", e_it->attr_, g_it->attr, err);
      result&=check_bid_seq("ad_ox_cats", e_it->ad_ox_cats_, g_it->ad_ox_cats, err);
      result&=check_bid_seq("cat", e_it->cat_, g_it->cat, err);
      result&=check_bid_value("fmt", e_it->fmt_, g_it->fmt, err);
      result&=check_bid_value("matching_ad_id", e_it->matching_ad_id_, g_it->matching_ad_id, err);
      result&=check_existance("fmt", e_it->fmt_exist_, g_it->fmt, err);
      result&=check_existance("cat", e_it->cat_exist_, g_it->cat, err);
      result&=check_existance("attr", e_it->attr_exist_, g_it->attr, err);
      result&=check_existance("ad_ox_cats", e_it->ad_ox_cats_exist_, g_it->ad_ox_cats, err);
      if (throw_error && !result)
      {
        throw CheckFailed(err);
      }
    }
    return result;
  }
}
