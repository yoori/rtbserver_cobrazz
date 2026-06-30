#pragma once

#include <memory_resource>
#include <string>
#include <string_view>
#include <type_traits>

#include <String/AsciiStringManip.hpp>
#include <String/StringManip.hpp>

namespace AdServer::Bidding
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
    const std::string DEAL_ID("dealid");

    const std::string FULL_NOREF("poadnoref");
    const std::string FULL_IDFA("rtbidfa");
    const std::string FULL_IDFA_KNOWN("rtbidfaknown");
    const std::string FULL_REQ("rtbreq");
    const std::string FULL_NO_ID("rtbnoid");
    const std::string FULL_COPPA("rtbcoppa");
    const std::string VIEWABILITY("viewability");
  };

  template<typename StringType = std::string>
  class BasicKeywordFormatter
  {
  public:
    // short_rtb_name == source_id
    explicit BasicKeywordFormatter(
      std::string_view source_id,
      std::pmr::memory_resource* resource = std::pmr::get_default_resource())
      : resource_(resource),
        keywords_osrt_(make_string_(std::string_view(), resource_)),
        keywords_non_empty_(false),
        short_rtb_name_(make_string_(source_id, resource_))
    {
      keywords_osrt_.reserve(1024);
    }

    template<std::size_t Size>
    explicit BasicKeywordFormatter(
      const char (&source_id)[Size],
      std::pmr::memory_resource* resource = std::pmr::get_default_resource())
      : BasicKeywordFormatter(std::string_view(source_id, Size - 1), resource)
    {}

    template<typename SourceIdType>
    explicit BasicKeywordFormatter(
      const SourceIdType& source_id,
      std::pmr::memory_resource* resource = std::pmr::get_default_resource())
      : BasicKeywordFormatter(
          std::string_view(source_id.data(), source_id.size()),
          resource)
    {}

    template<typename Traits, typename Allocator>
    void
    assign_to(std::basic_string<char, Traits, Allocator>& kw) const
    {
      if(empty())
      {
        return;
      }

      if(kw.empty())
      {
        kw.append(keywords_osrt_.data(), keywords_osrt_.size());
      }
      else
      {
        kw += '\n';
        kw.append(keywords_osrt_.data(), keywords_osrt_.size());
      }
    }

    bool
    empty() const noexcept
    {
      return !keywords_non_empty_;
    }

    void
    add_cat(std::string_view cat, bool open_rtb = false)
    {
      if(cat.empty())
      {
        return;
      }

      if (open_rtb)
      {
        add_(std::string_view(), std::string_view(), cat);
      }

      if (!open_rtb || !short_rtb_name_.empty())
      {
        add_(std::string_view(), short_rtb_name_, cat);
      }
    }

    void
    add_cat(const std::string& cat, bool open_rtb = false)
    {
      add_cat(std::string_view(cat.data(), cat.size()), open_rtb);
    }

    template <typename ValueType>
    void
    add_cat(const ValueType& cat, bool open_rtb = false)
    {
      add_cat(std::string_view(cat.data(), cat.size()), open_rtb);
    }

    template <typename CategoryStringContainerType>
    void
    add_cat_list(
      const CategoryStringContainerType& cat_list,
      bool open_rtb = false)
    {
      for(typename CategoryStringContainerType::const_iterator l_iter = cat_list.begin();
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
        add_(std::string_view(), std::string_view(), norm_gender_holder);
        if(!short_rtb_name_.empty())
        {
          add_(std::string_view(), short_rtb_name_, norm_gender_holder);
        }
      }
    }

    template <typename ValueType>
    void
    add_yob(const ValueType& yob)
    {
      add_(MatchKeywords::YOB, std::string_view(), yob);
      if(!short_rtb_name_.empty())
      {
        add_(MatchKeywords::YOB, short_rtb_name_, yob);
      }
    }

    template <typename ValueType>
    void
    add_age(const ValueType& age)
    {
      add_(MatchKeywords::AGE, std::string_view(), age);
      if(!short_rtb_name_.empty())
      {
        add_(MatchKeywords::AGE, short_rtb_name_, age);
      }
    }

    void
    add_ip(std::string_view addr_value)
    {
      const auto dot1 = addr_value.find('.');
      if(dot1 == std::string_view::npos)
      {
        return;
      }

      const auto dot2 = addr_value.find('.', dot1 + 1);
      if(dot2 == std::string_view::npos)
      {
        return;
      }

      const auto dot3 = addr_value.find('.', dot2 + 1);
      if(dot3 == std::string_view::npos)
      {
        return;
      }

      StringType res_keyword_ip3 = make_string_(std::string_view(), resource_);
      res_keyword_ip3.reserve(addr_value.size());
      res_keyword_ip3.append(addr_value.data(), dot1);
      res_keyword_ip3 += 'x';
      res_keyword_ip3.append(addr_value.data() + dot1 + 1, dot2 - dot1 - 1);
      res_keyword_ip3 += 'x';
      res_keyword_ip3.append(addr_value.data() + dot2 + 1, dot3 - dot2 - 1);

      StringType res_keyword_ip4(res_keyword_ip3);
      res_keyword_ip4 += 'x';
      res_keyword_ip4.append(
        addr_value.data() + dot3 + 1,
        addr_value.size() - dot3 - 1);

      add_(MatchKeywords::IP, std::string_view(), res_keyword_ip3);
      add_(MatchKeywords::IP, std::string_view(), res_keyword_ip4);
      if(!short_rtb_name_.empty())
      {
        add_(MatchKeywords::IP, short_rtb_name_, res_keyword_ip3);
        add_(MatchKeywords::IP, short_rtb_name_, res_keyword_ip4);
      }
    }

    template<typename ValueType>
    void
    add_ip(const ValueType& addr)
    {
      add_ip(std::string_view(addr.data(), addr.size()));
    }

    template <typename ValueType>
    void
    add_dict_keyword(
      std::string_view dict_name,
      const ValueType& keyword,
      bool add_rtb_prefix = true)
    {
      if (add_rtb_prefix)
      {
        add_(dict_name, short_rtb_name_, keyword);
      }
      else
      {
        add_(dict_name, std::string_view(), keyword);
      }
    }

    void
    add_dict_keyword(
      std::string_view dict_name,
      std::string_view keyword,
      bool add_rtb_prefix = true)
    {
      if (add_rtb_prefix)
      {
        add_(dict_name, short_rtb_name_, keyword);
      }
      else
      {
        add_(dict_name, std::string_view(), keyword);
      }
    }

    void
    add_dict_keyword_norm_spaces(
      std::string_view dict_name,
      std::string_view keyword,
      bool add_rtb_prefix = true)
    {
      if (add_rtb_prefix)
      {
        add_norm_spaces_(dict_name, short_rtb_name_, keyword);
      }
      else
      {
        add_norm_spaces_(dict_name, std::string_view(), keyword);
      }
    }

    template <typename ValueType>
    void
    add_rtb_keyword(
      std::string_view dict_name,
      const ValueType& keyword)
    {
      if(!short_rtb_name_.empty())
      {
        add_(dict_name, short_rtb_name_, keyword, true);
      }

      add_(dict_name, std::string_view(), keyword, true);
    }

    template<typename ValueType>
    void
    add_keyword(const ValueType& kw)
    {
      add_(
        std::string_view(),
        std::string_view(),
        std::string_view(kw.data(), kw.size()),
        false);
    }

    void
    add_full_rtb_keyword(std::string_view keyword)
    {
      if (keyword.empty())
      {
        return;
      }

      add_keyword(keyword);

      if (!short_rtb_name_.empty())
      {
        StringType source_keyword = make_string_(RTB_PREFIX, resource_);
        source_keyword.append(
          short_rtb_name_.data(),
          short_rtb_name_.size());
        if(keyword.compare(0, RTB_PREFIX.size(), RTB_PREFIX) == 0)
        {
          source_keyword.append(
            keyword.data() + RTB_PREFIX.size(),
            keyword.size() - RTB_PREFIX.size());
        }
        else
        {
          source_keyword.append(keyword.data(), keyword.size());
        }
        add_keyword(source_keyword);
      }
    }

    template <typename ValueStringContainerType>
    void
    add_keyword_list(const ValueStringContainerType& kwl)
    {
      for(typename ValueStringContainerType::const_iterator l_iter = kwl.begin();
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
      std::string_view param_name,
      std::string_view short_rtb_name,
      unsigned long value,
      bool add_rtb_prefix = true)
    {
      char value_str[40];
      size_t value_str_size = String::StringManip::int_to_str(
        value, value_str, sizeof(value_str));

      add_(
        param_name,
        short_rtb_name,
        std::string_view(value_str, value_str_size),
        add_rtb_prefix);
    }

    template<
      typename ValueType,
      typename = std::enable_if_t<!std::is_arithmetic_v<ValueType>>>
    void
    add_(
      std::string_view param_name,
      std::string_view short_rtb_name,
      const ValueType& value,
      bool add_rtb_prefix = true)
    {
      add_(
        param_name,
        short_rtb_name,
        std::string_view(value.data(), value.size()),
        add_rtb_prefix);
    }

    void
    add_(
      std::string_view param_name,
      std::string_view short_rtb_name,
      std::string_view value,
      bool add_rtb_prefix = true)
    {
      if(param_name.empty() && value.empty())
      {
        return;
      }

      if(keywords_non_empty_)
      {
        append_(keywords_osrt_, CUSTOM_KEYWORD_SEPARATOR);
      }

      if(add_rtb_prefix)
      {
        append_(keywords_osrt_, RTB_PREFIX);
      }

      append_(keywords_osrt_, short_rtb_name);
      append_(keywords_osrt_, param_name);
      append_(keywords_osrt_, value);

      keywords_non_empty_ = true;
    }

    void
    add_norm_spaces_(
      std::string_view param_name,
      std::string_view short_rtb_name,
      std::string_view value,
      bool add_rtb_prefix = true)
    {
      if(param_name.empty() && value.empty())
      {
        return;
      }

      if(keywords_non_empty_)
      {
        append_(keywords_osrt_, CUSTOM_KEYWORD_SEPARATOR);
      }

      if(add_rtb_prefix)
      {
        append_(keywords_osrt_, RTB_PREFIX);
      }

      append_(keywords_osrt_, short_rtb_name);
      append_(keywords_osrt_, param_name);
      for(std::string_view::const_iterator value_it = value.begin();
          value_it != value.end(); ++value_it)
      {
        const unsigned char ch = *value_it;
        if(ch >= 0x80)
        {
          keywords_osrt_ += *value_it;
        }
        else if((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z'))
        {
          keywords_osrt_ += *value_it;
        }
        else if(ch >= 'A' && ch <= 'Z')
        {
          keywords_osrt_ += static_cast<char>(ch - 'A' + 'a');
        }
        else
        {
          keywords_osrt_ += 'x';
        }
      }

      keywords_non_empty_ = true;
    }

    static StringType
    make_string_(
      std::string_view value,
      std::pmr::memory_resource* resource)
    {
      if constexpr (std::is_same_v<StringType, std::pmr::string>)
      {
        return StringType(value.data(), value.size(), resource);
      }
      else
      {
        return StringType(value.data(), value.size());
      }
    }

    static void
    append_(StringType& target, std::string_view value)
    {
      target.append(value.data(), value.size());
    }

    std::pmr::memory_resource* const resource_;
    StringType keywords_osrt_;
    bool keywords_non_empty_;
    const StringType short_rtb_name_;
    inline static constexpr std::string_view CUSTOM_KEYWORD_SEPARATOR = "\n";
    inline static constexpr std::string_view RTB_PREFIX = "rtb";
  };

  using KeywordFormatter = BasicKeywordFormatter<>;
  using PmrKeywordFormatter = BasicKeywordFormatter<std::pmr::string>;
}
