#include <iostream>
#include <sstream>
#include <string.h>
#include <assert.h>

#include <tests/AutoTests/Commons/Common.hpp>

#define SPACE  " \t\v\f\r\n"

#define CONCAT(msg, ...)                                \
  (CommaConcat(), msg, ##__VA_ARGS__).get().c_str()


namespace AutoTest
{
  namespace DebugInfo
  {
    namespace
    {
      class CommaConcat
      {
      public:
        template<typename T>
        CommaConcat &
        operator , (const T &v)
        {
          ss_ << v;
          return *this;
        }

        std::string
        get() const
        {
          return ss_.str();
        }

      private:
        std::stringstream ss_;
      };
    }


    static
    void
    skip_whitespace(const char *&pos)
    {
      pos += strspn(pos, SPACE);
    }


    static
    const char *
    backward_first_whitespace(const char *pos, const char *start)
    {
      do
        --pos;
      while (pos >= start && isspace(*pos));

      return ++pos;
    }


    static
    bool
    parse_parenthesized(const char *&pos)
    {
      pos += strcspn(pos, "()");
      switch (*pos)
      {
      case '(':
          return parse_parenthesized(++pos);

      case ')':
        skip_whitespace(++pos);
        return true;

      default:                // '\0'
        return false;
      }
    }

    inline
    std::ostream&
    Value::indent(std::ostream& out, int level)
    {
      while(level-- > 0) out << ' ' << ' ';
      return out;
    }

    void
    SimpleValue::clear()
    {
      value_.clear();
    }

    bool
    SimpleValue::parse(const char *&pos, const char *stoplist)
    {
      const char *start = pos;
      pos += strcspn(pos, stoplist);

      value_.assign(start, backward_first_whitespace(pos, start));

      return true;
    }

    std::ostream&
    SimpleValue::dump(std::ostream& out, int level) const
    {
      return indent(out, level) << value_;
    }


    bool
    SimpleValue::empty() const
    {
      return value_.empty();
    }

    bool
    ParenthesesValue::parse(const char *&pos, const char *stoplist)
    {
      const char *start = pos;
      while (*pos)
      {
        pos += strcspn(pos, CONCAT("(", stoplist));

        if (*pos != '(')
          break;

        if (! parse_parenthesized(++pos))
          return false;
      }

      while (*pos == ')') ++pos;

      value_.assign(start, backward_first_whitespace(pos, start));

      return true;
    }

    // class TimeValue
    bool
    TimeValue::parse(const char *&pos, const char *stoplist)
    {
      const char *start = pos;
      const char *stop  = pos;
      while (*pos)
      {
        pos += strcspn(pos, CONCAT(":;", stoplist));

        if (start == stop)
        {
          if (*pos == ';' || (*pos == ':' && isspace(*(pos+1))))
            stop = pos;
        }

        if (*pos == ';') break;

        ++pos;
      }

      value_.assign(start, backward_first_whitespace(stop, start));

      return true;
    }

    // class StringListValue
    void
    StringListValue::clear()
    {
      values_.clear();
    }

    std::ostream&
    StringListValue::dump (std::ostream& out, int /*level*/) const
    {
      if (empty()) return out;
      const_iterator last = --values_.end();
      for (const_iterator i = values_.begin();  i != last; ++i)
      {
        out << *i << ' ' << ',' << ' ';
      }
      return out << *last;
    }

    bool
    StringListValue::empty() const
    {
      return values_.empty ();
    }

    bool
    StringListValue::has(const std::string& value) const
    {
      for (const_iterator i = values_.begin();  i != values_.end(); ++i)
      {
        if (*i == value)
          return true;
      }
      return false;
    }

    bool
    StringListValue::parse(const char *&pos, const char *stoplist)
    {
      SimpleValue value;
      while (value.parse(pos, CONCAT(",", stoplist)))
      {
        if (! (*pos != ',' && values_.empty() && value.value().empty()))
          values_.push_back(value.value());

        if (*pos != ',')
          break;

        skip_whitespace(++pos);
      }

      return true;
    }


    void
    StructValue::clear()
    {
      ValueMap map;
      construct(map);

      ValueMap::iterator it = map.begin(),
        end = map.end();
      for (; it != end; ++it)
        it->second->clear();
    }

    bool
    StructValue::parse(const char *&pos, const char *stoplist)
    {
      ValueMap map;
      construct(map);

      SimpleValue key;
      while (key.parse(pos, "=") && *pos == '=')
      {
        skip_whitespace(++pos);
        ValueMap::iterator it = map.find(key.value());
        if (it != map.end())
        {
          Value *v = it->second;
          if (! v->parse(pos, CONCAT(";", stoplist)))
            return false;
        }
        else
        {
          /*
            Treat unknown pairs as parenthesized expressions.
            This will eat any expression with balanced
            parenthesis (including StructValue and ValueList).
          */
          ParenthesesValue pv;
          if (! pv.parse(pos, CONCAT(";", stoplist)))
            return false;
        }

        if (*pos != ';')
          break;

        skip_whitespace(++pos);
      }

      return true;
    }

    std::ostream&
    StructValue::dump (std::ostream& out, int level) const
    {
      ValueMap map;
      /*
        Const cast is ugly, but safe here: we do not modify anything.
        And having second set of construct() instead to build a map
        with const pointers would be an overkill.  Instead, it would
        be better to re-implement ValueMap as a static table of member
        offsets.
      */
      const_cast<StructValue *>(this)->construct(map);

      out << '(' << std::endl;
      for (ValueMap::const_iterator i = map.begin(); i != map.end(); ++i)
      {
        indent(out, level+1) << i->first << ' ' << '=' << ' ';
        i->second->dump(out) << ';' << ' ' << std::endl;
      }
      return indent(out, level) << ')';
    }

    bool
    StructValue::empty () const
    {
      return false;
    }

    template<class T>
    void
    ValuesList<T>::clear()
    {
      values_.clear();
    }


    template<class T>
    bool
    ValuesList<T>::parse(const char *&pos, const char */*stoplist*/)
    {
      while (*pos == '(')
      {
        skip_whitespace(++pos);
        values_.push_back(T());
        T &v = values_.back();
        if (! (v.parse(pos, ")") && *pos == ')'))
        {
          values_.pop_back();
          return false;
        }

        skip_whitespace(++pos);
      }

      return true;
    }

    template<class T>
    std::ostream &
    ValuesList<T>::dump(std::ostream& out, int level) const
    {
      if (empty()) return out;
      const_iterator last = --values_.end();
      for (const_iterator i = values_.begin();  i != last; ++i)
      {
        i->dump(out, level+1) << ',';
      }
      return last->dump(out, level+1);
    }

    void TriggersList::clear()
    {
      triggers_.clear();
      values_.clear();
    }

    const SimpleValue&
      TriggersList::operator[] (const std::string& trigger) const
      /*throw(TriggerNotFound)*/
    {
      const_iterator it = values_.find(trigger);
      if (it == values_.end())
      {
        Stream::Error err;
        err << "Trigger '" << trigger << "' not found";
        throw TriggerNotFound(err);
      }
      return it->second;
    }

    bool TriggersList::parse (const char *&pos, const char *stoplist)
    {
      while (*pos == '(')
      {
        skip_whitespace(++pos);
        const char *start = pos;
        std::string value;
        while(true)
        {

          if (isspace(*pos))
          {
            skip_whitespace(++pos);
            if (*pos == ':')
            {
              value.assign(start, backward_first_whitespace(pos, start));
              break;
            }
          }
          if (*pos == ')') return false;
          ++pos;
        }
        skip_whitespace(++pos);
        if (*pos != ':') return false;
        while (*pos == ':') pos++;
        skip_whitespace(++pos);
        values_[value].parse(pos, CONCAT(stoplist, ")"));
        // skip channel_trigger_id parsing
        while (*pos == ':') pos++;
        while (*pos != ')') pos++;
        triggers_.push_back(
          std::string(
            start,
            backward_first_whitespace(pos, start)));
        skip_whitespace(++pos);
      }

      return true;
    }

    std::ostream&
    TriggersList::dump (std::ostream& out, int level) const
    {
      const_iterator it = values_.begin();
      for (;it != values_.end(); ++it)
      {
        indent(out, level) << it->first << std::endl;
        it->second.dump(out, level+1);
      }
      return out;
    }

    const SelectedCreative&
    SelectedCreativesList::operator[](const std::string& ccid) const
    {
      static const SelectedCreative empty_ret;
      if (empty())
        return empty_ret;
      for (const_iterator i = begin();  i != end(); ++i)
      {
        if (i->ccid == ccid)
        {
          return *i;
        }
      }
      return empty_ret;
    }

    SelectedCreativesList::const_iterator
    SelectedCreativesList::find(const std::string& ccid) const
    {
      if (empty()) return end();
      for (const_iterator i = begin();  i != end(); ++i)
      {
        if (i->ccid == ccid)
        {
          return i;
        }
      }
      return end();
    }

    bool
    SelectedCreative::parse(const char *&pos, const char *stoplist)
    {
      bool ret = StructValue::parse(pos, stoplist);
      actual_cpc = click_revenue.value();
      imp_revenue_value = imp_revenue.value();
      ecpm_bid_value = ecpm_bid.value();
      ecpm_value = ecpm.value();
      return ret;
    }

    void
    SelectedCreative::construct(ValueMap &map)
    {
      map["ccid"]                 = &ccid;
      map["cmp_id"]               = &cmp_id;
      map["ecpm"]                 = &ecpm;
      map["ecpm_bid"]             = &ecpm_bid;
      map["triggered_expression"] = &triggered_expression;
      map["click_url"]            = &click_url;
      map["html_url"]             = &html_url;
      map["action_adv_url"]       = &action_adv_url;
      map["revenue"]              = &revenue;
      map["imp_revenue"]          = &imp_revenue;
      map["click_revenue"]        = &click_revenue;
      map["action_revenue"]       = &action_revenue;
      map["margin_rule_id"]       = &margin_rule_id;
      map["request_id"]           = &request_id;
    }

    void
    DebugInfo::fetch_exterior_()
    {
      click_url.clear();
      html_url.clear();
      action_adv_url.clear();

      SelectedCreativesList::const_iterator it = selected_creatives.begin();
      SelectedCreativesList::const_iterator end = selected_creatives.end();

      for (; it != end; ++it)
      {
        if (it->ccid == ccid.value() && it->cmp_id == cmpid.value())
        {
          click_url = it->click_url;
          html_url = it->html_url;
          action_adv_url = it->action_adv_url;
          triggered_expression = it->triggered_expression;
          creative_request_id = it->request_id;
          break;
        }
      }
    }

    bool
    DebugInfo::parse(const char *&pos, const char *stoplist)
    {
      StructValue::clear();
      bool result = StructValue::parse(pos, stoplist);

      //Exterior data:
      fetch_exterior_();

      cpm_threshold_value = cpm_threshold.value();

      return result;
    }

    void
    DebugInfo::construct(ValueMap &map)
    {
      map["action_revenue"]              = &action_revenue;
      map["last_request_time"]           = &last_request_time;
      map["request_id"]                  = &request_id;
      map["location"]                    = &location;
      map["session_start"]               = &session_start;
      map["fraud_request"]               = &fraud_request;
      map["ccid"]                        = &ccid;
      map["click_revenue"]               = &click_revenue;
      map["cmpid"]                       = &cmpid;
      map["colo_id"]                     = &colo_id;
      map["context_words"]               = &context_words;
      map["country"]                     = &country;
      map["creative_size_id"]            = &creative_size;
      map["day_ccg_appear"]              = &day_ccg_appear;
      map["history_channels"]            = &history_channels;
      map["imp_revenue"]                 = &imp_revenue;
      map["ip"]                          = &ip;
      map["merge_uid"]                   = &merge_uid;
      map["optout"]                      = &optout;
      map["referer"]                     = &referer;
      map["revenue"]                     = &revenue;
      map["search-phrase"]               = &search_phrase;
      map["geo_channels"]                = &geo_channels;
      map["device_channels"]             = &device_channels;
      map["selected_creatives"]          = &selected_creatives;
      map["site_id"]                     = &site_id;
      map["site_rate_id"]                = &site_rate_id;
      map["special_channels_effects"]    = &special_channels_effects;
      map["tag_id"]                      = &tag_id;
      map["test_request"]                = &test_request;
      map["tid"]                         = &tid;
      map["cpm_threshold"]               = &cpm_threshold;
      map["total_ccg_appear"]            = &total_ccg_appear;
      map["trigger_channels"]            = &trigger_channels;
      map["triggered_expression"]        = &triggered_expression;
      map["triggers"]                    = &triggers;
      map["uid"]                         = &uid;

      map["trigger_match_time"]          = &trigger_match_time;
      map["request_fill_time"]           = &request_fill_time;
      map["history_match_time"]          = &history_match_time;
      map["creative_selection_time"]     = &creative_selection_time;

      map["ccids"]                       = &ccids;
      map["trigger_channels_count"]      = &trigger_channels_count;
      map["history_channels_count"]      = &history_channels_count;

      map["track_pixel_url"]             = &track_pixel_url;
      map["click_url"]                   = &click_url;
      map["html_url"]                    = &html_url;
      map["action_adv_url"]              = &action_adv_url;
      map["creative_request_id"]         = &creative_request_id;
    }
  }
}
