
#include <algorithm>
#include "OpenRTBResponseChecker.hpp"

namespace AutoTest
{
  namespace
  {
    // Utils
    void
    json_serialize(std::ostream& ostr, JsonNode* node);

    void
    json_serialize(std::ostream& ostr, const JsonValue& value)
    {
      if(value.getTag() == JSON_TAG_NUMBER)
      {
        ostr << value.toNumber();
      }
      else if(value.getTag() == JSON_TAG_STRING)
      {
        std::string escaped_str =
          String::StringManip::json_escape(String::SubString(value.toString()));
        ostr << "\"" << escaped_str << "\"";
      }
      else if(value.getTag() == JSON_TAG_BOOL)
      {
        ostr << (value.toBool() ? "true" : "false");
      }
      else if(value.getTag() == JSON_TAG_ARRAY)
      {
        ostr << "[";
        for(JsonIterator arr_it = begin(value); arr_it != end(value); ++arr_it)
        {
          if(arr_it != begin(value))
          {
            ostr << ", ";
          }
          json_serialize(ostr, arr_it->value);
        }
        ostr << "]";
      }
      else if(value.getTag() == JSON_TAG_OBJECT)
      {
        ostr << "{";
        for(JsonIterator arr_it = begin(value); arr_it != end(value); ++arr_it)
        {
          if(arr_it != begin(value))
          {
            ostr << ", ";
          }
          json_serialize(ostr, *arr_it);
        }
        ostr << "}";
      }
      else if(value.getTag() == JSON_TAG_NULL)
      {
        ostr << "null";
      }
    }

    void
    json_serialize(std::ostream& ostr, JsonNode* node)
    {
      ostr << "\"" << node->key << "\": ";
      json_serialize(ostr, node->value);
    }

    void
    process_json_object_as_string(
      JsonIterator iterator,
      const char* name,
      std::string& value)
    {
      if (iterator->value.getTag() == JSON_TAG_OBJECT &&
        strcmp(name,iterator->key) == 0)
      {
        std::ostringstream ostr;
        json_serialize(ostr, iterator->value);
        value = ostr.str();;
      }
    }

    void
    process_json_string(
      JsonIterator iterator,
      const char* name,
      std::string& value)
    {
      if (iterator->value.getTag() == JSON_TAG_STRING &&
        strcmp(name,iterator->key) == 0)
      {
        value = iterator->value.toString();
      }
    }


    template <typename T>
    void
    process_json_int(
      JsonIterator iterator,
      const char* name,
      T& value)
    {
      if (strcmp(name,iterator->key) == 0)
      {
        switch (iterator->value.getTag())
        {
          case JSON_TAG_STRING:
          {
            Stream::Parser strm(iterator->value.toString());
            strm >> value;
            break;
          }
          case JSON_TAG_NUMBER:
          {
            value = iterator->value.toNumber();
            break;
          }
          default: break;
        }
      }
    }

    void
    process_json_money(
      JsonIterator iterator,
      const char* name,
      Money& value)
    {
      if (iterator->value.getTag() == JSON_TAG_NUMBER &&
        strcmp(name,iterator->key) == 0)
      {
        value = iterator->value.toNumber();
      }
    }

    void
    string_to_opt_list(
      ExpValue< std::list<std::string> >* list,
      JsonIterator iterator)
    {
      if (iterator->value.getTag() == JSON_TAG_STRING)
      {
        (*list)->push_back(iterator->value.toString());
      }
    }

    void
    string_to_list(
      std::list<std::string>* list,
      JsonIterator iterator)
    {
      if (iterator->value.getTag() == JSON_TAG_STRING)
      {
        list->push_back(iterator->value.toString());
      }
    }

    template<typename T>
    void int_to_opt_list(
      ExpValue< std::list<T> >* list,
      JsonIterator iterator)
    {
      if (iterator->value.getTag() == JSON_TAG_NUMBER)
      {
        (*list)->push_back(iterator->value.toNumber());
      }
    }

    template<typename T>
    void int_to_list(
      std::list<T>* list,
      JsonIterator iterator)
    {
      if (iterator->value.getTag() == JSON_TAG_NUMBER)
      {
        list->push_back(iterator->value.toNumber());
      }
    }

    template<typename T>
    void
    set_array_exist(T)
    { }

    template<typename T>
    void
    set_array_exist(ExpValue<T>* array)
    {
      array->is_set(true);
    }

    template<typename Parser, typename T>
    void
    parse_array(
      JsonIterator iterator,
      const char* name,
      Parser parser,
      T value)
    {
      if (iterator->value.getTag() == JSON_TAG_ARRAY &&
        strcmp(name,iterator->key) == 0)
      {
        for(JsonIterator it = begin(iterator->value);
            it != end(iterator->value); ++it)
        {
          auto fp = std::bind(parser, value, it);
          fp();
        }
        set_array_exist(value);
      }
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

  void
  OpenRTBResponse::parse_ext_(
    JsonIterator iterator,
    Bid& bid)
  {
    if (iterator->value.getTag() == JSON_TAG_OBJECT &&
        strcmp(iterator->key, "ext") == 0)
    {
      for (JsonIterator it = begin(iterator->value);
              it != end(iterator->value); ++it)
      {
        parse_array(
          it,
          "ad_ox_cats",
          std::ptr_fun(
            &int_to_opt_list<unsigned long>),
          &bid.ad_ox_cats);
        process_json_object_as_string(it, "matching_ad_id", bid.matching_ad_id);
        process_json_object_as_string(it, "nurl", bid.nurl);
      }
    }
  }


  void
  OpenRTBResponse::parse_bid_(
    JsonIterator iterator)
  {
    if (iterator->value.getTag() == JSON_TAG_OBJECT)
    {
      Bid bid;
      for(JsonIterator it = begin(iterator->value);
          it != end(iterator->value); ++it)
      {
        process_json_string(it, "id", bid.id);
        process_json_string(it, "impid", bid.impid);
        process_json_money(it, "price", bid.price);
        process_json_int(it, "adid", bid.adid);
        process_json_string(it, "crid", bid.crid);
        parse_array(
          it,
          "adomain",
          std::ptr_fun(
            &string_to_list),
          &bid.adomain);
        process_json_string(it, "adm", bid.adm);
        process_json_string(it, "nurl", bid.nurl);
        process_json_int(it, "cid", bid.cid);
        parse_array(
          it,
          "attr",
          std::ptr_fun(
            &int_to_opt_list<unsigned long>),
          &bid.attr);
        process_json_int(it, "fmt", bid.fmt);
        parse_array(
          it,
          "cat",
          std::ptr_fun(
            &string_to_opt_list),
          &bid.cat);
        parse_ext_(it, bid);
      }
      bids_.push_back(bid);
    }
  }

  void
  OpenRTBResponse::parse_seatbid_(
    JsonIterator iterator)
  {
    if (iterator->value.getTag() == JSON_TAG_OBJECT)
    {
      for(JsonIterator it = begin(iterator->value);
          it != end(iterator->value); ++it)
      {
        parse_array(
          it,
          "bid",
          std::mem_fun(
            &OpenRTBResponse::parse_bid_),
          this);
      }
    }
  }

  void
  OpenRTBResponse::parse_(
    const JsonValue& value)
  {
    if (value.getTag() == JSON_TAG_OBJECT)
    {
      for(JsonIterator it = begin(value); it != end(value); ++it)
      {
        process_json_string(it, "id", id_);
        process_json_string(it, "cur", currency_);
        parse_array(
          it,
          "seatbid",
          std::mem_fun(
            &OpenRTBResponse::parse_seatbid_),
          this);
      }
    }
  }

  OpenRTBResponse::OpenRTBResponse(
    const std::string& body)
  {
    JsonValue root_value;
    JsonAllocator json_allocator;
    Generics::ArrayAutoPtr<char> body_holder(body.length() + 1);
    char* parse_end;
    strcpy(body_holder.get(), body.c_str());
    
    status_ = json_parse(
      body_holder.get(), &parse_end, &root_value, json_allocator);

    if (status_ == JSON_PARSE_OK)
    {
        parse_(root_value);
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

