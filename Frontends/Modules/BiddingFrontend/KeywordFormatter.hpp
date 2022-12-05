#ifndef BIDDINGFRONTEND_KEYWORDFORMATTER_HPP_
#define BIDDINGFRONTEND_KEYWORDFORMATTER_HPP_

#include <string>
#include <list>

#include <String/SubString.hpp>
#include <String/AsciiStringManip.hpp>
#include <String/StringManip.hpp>

namespace AdServer
{
namespace Bidding
{
  namespace MatchKeywords
  {
    const std::string PLAYBACKMETHOD("playbackmethod");
    const std::string PLACEMENT("placement");
    const std::string AGE("age");
    const std::string YOB("yob");
    const std::string IP("ip");
    const std::string REQ("req");
    const std::string RTBREQ("rtbreq");
    const std::string VT("vt");
    const std::string DCL("dcl");
    const std::string NO_ID("noid");
    const std::string NATIVE_PLACEMENT("nativeplacement");

    const std::string FULL_NOREF("poadnoref");
    const std::string FULL_IDFA("rtbidfa");
    const std::string FULL_IDFA_KNOWN("rtbidfaknown");
    const std::string FULL_REQ("rtbreq");
    const std::string FULL_NO_ID("rtbnoid");
    const std::string FULL_COPPA("rtbcoppa");
    const std::string VIEWABILITY("viewability");
  };

  class KeywordFormatter
  {
  public:
    // short_rtb_name == source_id
    KeywordFormatter(const std::string& source_id)
      : keywords_non_empty_(false),
        short_rtb_name_(source_id)
    {
      keywords_osrt_.reserve(1024);
    }

    void
    assign_to(std::string& kw) const
    {
      if(kw.empty())
      {
        kw = keywords_osrt_;
      }
      else
      {
        kw += '\n';
        kw += keywords_osrt_;
      }
    }

    template <typename ValueType>
    void
    add_cat(const ValueType& cat, bool open_rtb = false)
    {
      if (open_rtb)
      {
        add_(String::SubString(), String::SubString(), cat);
      }

      if (!open_rtb || !short_rtb_name_.empty())
      {
        add_(String::SubString(), short_rtb_name_, cat);
      }
    }

    void
    add_cat(const std::string& cat, bool open_rtb = false)
    {
      // Skip empty category values
      if(cat.empty())
      {
        return;
      }

      if (open_rtb)
      {
        add_(String::SubString(), String::SubString(), cat);
      }

      if (!open_rtb || !short_rtb_name_.empty())
      {
        add_(String::SubString(), short_rtb_name_, cat);
      }
    }

    void
    add_cat_list(const std::list<std::string>& cat_list, bool open_rtb = false)
    {
      for(std::list<std::string>::const_iterator l_iter = cat_list.begin();
          l_iter != cat_list.end(); ++l_iter)
      {
        add_cat(*l_iter, open_rtb);
      }
    }

    void
    add_gender(const std::string& gender)
    {
      // "unknown" / "other" gender is skipped
      std::string norm_gender_holder = gender;
      String::AsciiStringManip::to_lower(norm_gender_holder);

      if(norm_gender_holder == "m")
      {
        norm_gender_holder = "male";
      }
      else if(norm_gender_holder == "f")
      {
        norm_gender_holder = "female";
      }

      if(norm_gender_holder == "male" || norm_gender_holder == "female")
      {
        add_(String::SubString(), String::SubString(),  norm_gender_holder);
        if(!short_rtb_name_.empty())
        {
          add_(String::SubString(), short_rtb_name_, norm_gender_holder);
        }
      }
    }

    template <typename ValueType>
    void
    add_yob(const ValueType& yob)
    {
      add_(MatchKeywords::YOB, String::SubString(), yob);
      if(!short_rtb_name_.empty())
      {
        add_(MatchKeywords::YOB, short_rtb_name_, yob);
      }
    }

    template <typename ValueType>
    void
    add_age(const ValueType& age)
    {
      add_(MatchKeywords::AGE, String::SubString(), age);
      if(!short_rtb_name_.empty())
      {
        add_(MatchKeywords::AGE, short_rtb_name_, age);
      }
    }

    void
    add_ip(const String::SubString& addr)
    {
      String::StringManip::Splitter<String::AsciiStringManip::SepPeriod> tokenizer(addr);
      String::SubString token1;
      String::SubString token2;
      String::SubString token3;
      String::SubString token4;
      if(tokenizer.get_token(token1) &&
         tokenizer.get_token(token2) &&
         tokenizer.get_token(token3) &&
         tokenizer.get_token(token4))
      {
        std::string res_keyword_ip3;
        res_keyword_ip3 += token1.str();
        res_keyword_ip3 += 'x';
        res_keyword_ip3 += token2.str();
        res_keyword_ip3 += 'x';
        res_keyword_ip3 += token3.str();

        std::string res_keyword_ip4(res_keyword_ip3);
        res_keyword_ip4 += 'x';
        res_keyword_ip4 += token4.str();

        add_(MatchKeywords::IP, String::SubString(), res_keyword_ip3);
        add_(MatchKeywords::IP, String::SubString(), res_keyword_ip4);
        if(!short_rtb_name_.empty())
        {
          add_(MatchKeywords::IP, short_rtb_name_, res_keyword_ip3);
          add_(MatchKeywords::IP, short_rtb_name_, res_keyword_ip4);
        }
      }
    }

    template <typename ValueType>
    void
    add_dict_keyword(
      const String::SubString& dict_name,
      const ValueType& keyword,
      bool add_rtb_prefix = true)
    {
      if (add_rtb_prefix)
      {
        add_(dict_name, short_rtb_name_, keyword);
      }
      else
      {
        add_(dict_name, String::SubString(), keyword);
      }
    }

    template <typename ValueType>
    void
    add_rtb_keyword(
      const String::SubString& dict_name,
      const ValueType& keyword)
    {
      if(!short_rtb_name_.empty())
      {
        add_(dict_name, short_rtb_name_, keyword, true);
      }

      add_(dict_name, String::SubString(), keyword, true);
    }

    void
    add_keyword(const std::string& kw)
    {
      add_(String::SubString(), String::SubString(), kw, false);
    }

    template <typename ValueStringListType>
    void
    add_keyword_list(const ValueStringListType& kwl)
    {
      for(typename ValueStringListType::const_iterator l_iter = kwl.begin();
          l_iter != kwl.end(); ++l_iter)
      {
        if(!l_iter->empty())
        {
          add_keyword(*l_iter);
        }
      }
    }

  protected:
    void
    add_(
      const String::SubString& param_name,
      const String::SubString& short_rtb_name,
      unsigned long value,
      bool add_rtb_prefix = true)
    {
      char value_str[40];
      size_t value_str_size = String::StringManip::int_to_str(
        value, value_str, sizeof(value_str));

      add_(
        param_name,
        short_rtb_name,
        String::SubString(value_str, value_str_size),
        add_rtb_prefix);
    }

    void
    add_(
      const String::SubString& param_name,
      const String::SubString& short_rtb_name,
      const String::SubString& value,
      bool add_rtb_prefix = true)
    {
      if(param_name.empty() && value.empty())
      {
        return;
      }

      if(keywords_non_empty_)
      {
        keywords_osrt_ += CUSTOM_KEYWORD_SEPARATOR;
      }

      if(add_rtb_prefix)
      {
        keywords_osrt_ += RTB_PREFIX;
      }

      short_rtb_name.append_to(keywords_osrt_);
      param_name.append_to(keywords_osrt_);
      value.append_to(keywords_osrt_);

      keywords_non_empty_ = true;
    }

    std::string keywords_osrt_;
    bool keywords_non_empty_;
    const std::string short_rtb_name_;
    const static std::string CUSTOM_KEYWORD_SEPARATOR;
    const static std::string RTB_PREFIX;
  };
}
}

#endif /*BIDDINGFRONTEND_KEYWORDFORMATTER_HPP_*/
