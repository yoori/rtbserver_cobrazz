#include <errno.h>
#include <arpa/inet.h>
#include <string>
#include <algorithm>

#include <String/StringManip.hpp>
#include <String/UTF8Handler.hpp>
#include <String/UTF8Case.hpp>
#include <String/UnicodeSymbol.hpp>
#include <Language/BLogic/NormalizeTrigger.hpp>

#include "RequestMatchers.hpp"

namespace FrontendCommons
{
  namespace
  {
    typedef String::AsciiStringManip::Char1Category<'.'>
      DotCharCategory;

    const unsigned long FULL_PLATFORM_MAX_SIZE = 1024;
    const unsigned long BROWSER_MAX_SIZE = 1024;
    const char APPLICATION_PLATFORM_DETECTOR_NAME[] = "applications";
  }

  /* UrlMatcher implementation */
  UrlMatcher::UrlMatcher() noexcept
    : root_matcher_(new HostnameMatcher())
  {}

  bool
  UrlMatcher::match(
    unsigned long& search_engine_id,
    std::string& match_result,
    const HTTP::HTTPAddress& url,
    const Language::Segmentor::SegmentorInterface* segmentor) const
    /*throw(eh::Exception)*/
  {
    std::string addr_path_and_query;
    url.get_view(
      HTTP::HTTPAddress::VW_PATH |
      HTTP::HTTPAddress::VW_QUERY |
      HTTP::HTTPAddress::VW_FRAGMENT,
      addr_path_and_query);

    return match_hostname_(
      search_engine_id,
      match_result,
      root_matcher_,
      url.host(),
      addr_path_and_query,
      segmentor);
  }

  bool
  UrlMatcher::match(
    unsigned long& search_engine_id,
    std::string& match_result,
    const String::SubString& url,
    const Language::Segmentor::SegmentorInterface* segmentor) const
    /*throw(eh::Exception)*/
  {
    return match(
      search_engine_id,
      match_result,
      HTTP::BrowserAddress(url),
      segmentor);
  }

  bool
  UrlMatcher::match_hostname_(
    unsigned long& search_engine_id,
    std::string& match_result,
    const HostnameMatcher* hostname_matcher,
    const String::SubString& hostname,
    const String::SubString& path_and_query,
    const Language::Segmentor::SegmentorInterface* segmentor)
    /*throw(Exception)*/
  {
    if(!hostname.empty())
    {
      String::SubString::SizeType hostname_pos = hostname.rfind('.');

      String::SubString hostname_part(
        hostname_pos == String::SubString::NPOS ?
        hostname : hostname.substr(hostname_pos + 1));

      String::SubString hostname_prefix(
        hostname_pos == String::SubString::NPOS ?
        String::SubString("") : hostname.substr(0, hostname_pos));

      HostnameMatcher::HostnameMatchMap::const_iterator hm_it =
        hostname_matcher->hostname_matchers.find(hostname_part);

      if(hm_it != hostname_matcher->hostname_matchers.end())
      {
        if(match_hostname_(
             search_engine_id,
             match_result,
             hm_it->second,
             hostname_prefix,
             path_and_query,
             segmentor))
        {
          return true;
        }
      }

      hm_it = hostname_matcher->hostname_matchers.find("");

      if(hm_it != hostname_matcher->hostname_matchers.end())
      {
        if(match_hostname_(
             search_engine_id,
             match_result,
             hm_it->second,
             hostname_prefix,
             path_and_query,
             segmentor))
        {
          return true;
        }
      }
    }

    return match_list_(
      search_engine_id,
      match_result,
      hostname_matcher->regexps,
      path_and_query,
      segmentor);
  }

  bool
  UrlMatcher::match_list_(
    unsigned long& search_engine_id,
    std::string& match_result,
    const UrlMatchElementList& match_elements,
    const String::SubString& path_and_query,
    const Language::Segmentor::SegmentorInterface* segmentor)
    /*throw(Exception)*/
  {
    static const char* FUN = "UrlMatcher::match_list_()";

    for(UrlMatchElementList::const_iterator it = match_elements.begin();
        it != match_elements.end(); ++it)
    {
      try
      {
        String::RegEx::Result sub_strs;

        if((*it)->regexp.search(sub_strs, path_and_query) && (sub_strs.size() > 1))
        {
          try
          {
            try
            {
              search_engine_id = (*it)->search_engine_id;
              sub_strs[1].assign_to(match_result);
              for (unsigned int i = 0; i < (*it)->decoding_depth; ++i)
              {
                String::StringManip::mime_url_decode(match_result);
              }
            }
            catch(const eh::Exception&)
            {
              continue;
            }

            // should decode here specific encoding if parsed string isn't UTF-8
            if(String::UTF8Handler::is_correct_utf8_string(
                match_result.c_str()) != 0)
            {
              if(!(*it)->charset.empty())
              {
                std::string buf;
                // need make encoding
                String::International::Convertion
                  conv("UTF-8", (*it)->charset.c_str());
                conv.encode(match_result.c_str(), match_result.size(), buf);
                buf.swap(match_result);
              }
              else
              {
                continue;
              }
            }
            // should decode special case
            switch((*it)->special_way)
            {
              case UrlMatchElement::SC_JS:
                {
                  std::string buf;
                  String::StringManip::js_unicode_decode(
                    match_result, buf, false, (*it)->special);
                  buf.swap(match_result);
                }
              break;
              case UrlMatchElement::SC_HTML:
                {
                  std::string buf;
                  if(html_unicode_decode(match_result, buf))
                  {
                    buf.swap(match_result);
                  }
                }
              break;
              default:
              break;
            }
            try
            {
              std::string buf;
              Language::Trigger::normalize_phrase(match_result, buf, segmentor);
              if(String::UTF8Handler::is_correct_utf8_string(buf.c_str()) == 0)
              {
                match_result.swap(buf);
                return true;
              }
              else
              {
                match_result.clear();
              }
            }
            catch(...)
            {
              match_result.clear();
            }
          }
          catch(const String::International::Convertion::Exception&)
          {}
        }
      }
      catch(const String::RegEx::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught String::RegEx::Exception: " << e.what();
        throw Exception(ostr);
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught eh::Exception: " << e.what();
        throw Exception(ostr);
      }
    }

    return false;
  }

  bool UrlMatcher::html_unicode_decode(
    const String::SubString& in,
    std::string& buf)
    /*throw(eh::Exception)*/
  {
    static const char* html_seq = "&#";
    bool ret_value = false;
    String::SubString::SizeType pos, prev_pos = 0;
    do
    {
      pos  = in.find(html_seq, prev_pos, 2);
      if(pos != String::SubString::NPOS)
      {
        if(!ret_value)
        {
          buf.clear();
          buf.reserve(in.size());
          ret_value = true;
        }
        if(prev_pos != pos)
        {
          buf.append(in.begin() + prev_pos, pos - prev_pos);
        }
        pos += 2;
        int base = 10;
        if(in.at(pos) == 'x')//16-based
        {
          base = 16;
          pos++;
        }
        char* last;
        long long int code = strtoll(in.begin() + pos, &last, base);
        if(!errno && *last == ';')//ignore erros and strange sequence
        {
          String::UnicodeSymbol symbol(code);
          buf.append(symbol.c_str(), symbol.length());
          pos = last - in.begin() + 1;
        }
        prev_pos = pos;
      }
      else
      {
        if(ret_value && prev_pos != in.size())
        {
          buf.append(in.begin() + prev_pos, in.size() - prev_pos);
        }
        break;
      }
    } while(pos < in.size());
    return ret_value;
  }

  /** UrlMatcher::read_config */
  void UrlMatcher::add_rule(
    unsigned long search_engine_id,
    const char* host_postfix_val,
    const char* regexp_val,
    unsigned long dec_depth,
    const char* enc,
    const char* post_enc)
    /*throw(Exception)*/
  {
    static const char* FUN = "UrlMatcher::add_rule()";

    try
    {
      std::string host_postfix(host_postfix_val);

      std::string::size_type prev_pos = std::string::npos;

      HostnameMatcher_var target_matcher = root_matcher_;
        
      do
      {
        std::string::size_type pos =
          host_postfix.rfind('.', prev_pos);

        std::string hostname_part =
          pos == std::string::npos ?
          host_postfix.substr(0, prev_pos + 1) :
          host_postfix.substr(pos + 1, prev_pos - pos);

        if(hostname_part == "*")
        {
          hostname_part.clear();
        }

        HostnameMatcher_var& matcher = target_matcher->hostname_matchers[hostname_part];
        if(!matcher.in())
        {
          matcher = new HostnameMatcher();
        }

        target_matcher = matcher;
        prev_pos = (pos == std::string::npos ? pos : pos - 1);
      }
      while(prev_pos != std::string::npos);

      UrlMatchElement_var match_element(new UrlMatchElement(
        search_engine_id,
        regexp_val,
        dec_depth,
        enc,
        post_enc));

      target_matcher->regexps.push_back(match_element);
    }
    catch(const String::RegEx::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": In processing regexp '" << regexp_val <<
        "' String::RegEx::Exception caught: " << e.what();
      throw Exception(ostr);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": In match element eh::Exception caught: " << e.what();
      throw Exception(ostr);
    }
  }

  /**
   *  PlatformMatcher implementation
   */
  PlatformMatcher::OnMatch::OnMatch(unsigned long max_priority) noexcept
    : max_priority_(max_priority), matched_element_(0)
  {}

  bool
  PlatformMatcher::OnMatch::operator() (
    const PlatformMatcher::MultiStringMatcher::MatchDetails& details)
    /*throw(String::RegEx::Exception)*/
  {
    if (matched_element_ && details.tag->priority <= matched_element_->priority)
    {
      return false;
    }

    if (details.tag->match_regexp.get() &&
      !details.tag->match_regexp->match(details.search_in_string))
    {
      return false;
    }

    matched_element_ = details.tag;
    // if max priority found then send terminate matching signal
    return (details.tag->priority == max_priority_);
  }

  const PlatformMatcher::MatchElement*
  PlatformMatcher::OnMatch::get_matched_element() const noexcept
  {
    return matched_element_;
  }

  PlatformMatcher::CategoryMatcher::CategoryMatcher() noexcept
  {
    max_priority_ = std::numeric_limits<unsigned long>::min();
  }

  const PlatformMatcher::MatchElement*
  PlatformMatcher::CategoryMatcher::match(
    const String::SubString& user_agent) const
    /*throw(eh::Exception, String::RegEx::Exception)*/
  {
    OnMatch on_match(max_priority_);
    matcher_.match(user_agent, on_match);

    for (auto it = empty_marker_elements_.begin();
         it != empty_marker_elements_.end(); ++it)
    {
      if (on_match({user_agent, &*it}))
      {
        break;
      }
    }

    return on_match.get_matched_element();
  }

  void
  PlatformMatcher::CategoryMatcher::add_rule(
    unsigned long platform_id,
    const String::SubString& name,
    const char* full_name,
    const char* marker,
    const char* match_regexp,
    const char* output_regexp,
    unsigned long priority)
    /*throw(String::RegEx::Exception)*/
  {
    if (marker[0] == 0)
    {
      empty_marker_elements_.emplace_back(
        platform_id,
        name,
        full_name,
        marker,
        match_regexp,
        output_regexp,
        priority);
    }
    else
    {
      elements_.emplace_back(
        platform_id,
        name,
        full_name,
        marker,
        match_regexp,
        output_regexp,
        priority);

      matcher_.add_pattern(marker, &elements_.back());
    }

    if (priority > max_priority_)
    {
      max_priority_ = priority;
    }
  }

  PlatformMatcher::PlatformMatcher() /*throw(eh::Exception)*/
    : os_matchers_(new CategoryMatcher()),
      application_platform_id_(0)
  {}

  bool
  PlatformMatcher::match(
    PlatformIdSet* platform_ids,
    std::string& platform,
    std::string& full_platform,
    const String::SubString& user_agent,
    bool application) const
    /*throw(eh::Exception)*/
  {
    return match(
      platform_ids,
      0, // platform_names
      platform,
      full_platform,
      user_agent,
      application);
  }

  bool
  PlatformMatcher::match(
    PlatformIdSet* platform_ids,
    PlatformNameSet* platform_names,
    std::string& platform,
    std::string& full_platform,
    const String::SubString& user_agent,
    bool application) const
    /*throw(eh::Exception)*/
  {
    std::string low_user_agent(user_agent.str());
    String::AsciiStringManip::to_lower(low_user_agent);

    bool res = match_(
      application ? 0 : platform_ids,
      platform_names,
      &platform,
      &full_platform,
      *os_matchers_,
      low_user_agent);

    if(platform_ids)
    {
      if(application)
      {
        if(application_platform_id_)
        {
          platform_ids->insert(application_platform_id_);

          if(platform_names)
          {
            platform_names->insert("application");
          }
        }
      }
      else
      {
        for (CategoryMatcherMap::const_iterator it = matchers_.begin();
             it != matchers_.end(); ++it)
        {
          res |= match_(
            platform_ids,
            platform_names,
            0,
            0,
            *(it->second),
            low_user_agent);
        }
      }
    }

    return res;
  }

  bool
  PlatformMatcher::match_(
    PlatformIdSet* platform_ids,
    PlatformNameSet* platform_names,
    std::string* platform,
    std::string* full_platform,
    const CategoryMatcher& matchers,
    const String::SubString& user_agent) const
    /*throw(eh::Exception)*/
  {
    const MatchElement* element = matchers.match(user_agent);
    
    if (element)
    {
      if (platform_ids)
      {
        platform_ids->insert(element->platform_id);
      }

      if(platform_names)
      {
        platform_names->insert(element->name);
      }

      if (platform)
      {
        *platform = element->name;
      }

      if (full_platform)
      {
        *full_platform = element->full_name;
        String::RegEx::Result output_sub_strs;

        if (element->output_regexp.get() &&
            element->output_regexp->search(output_sub_strs, user_agent) &&
            !output_sub_strs.empty())
        {
          for (String::RegEx::Result::iterator sit = ++output_sub_strs.begin();
               sit != output_sub_strs.end(); ++sit)
          {
            if(!sit->empty())
            {
              *full_platform += " ";
              std::string t(sit->str());
              std::replace(t.begin(), t.end(), '_', '.');
              *full_platform += t;
            }
          }
        }

        if(full_platform->size() > FULL_PLATFORM_MAX_SIZE)
        {
          full_platform->resize(FULL_PLATFORM_MAX_SIZE);
        }
      }
      
      return true;
    }

    return false;
  }

  void PlatformMatcher::add_rule(
    unsigned long platform_id,
    const String::SubString& name,
    const char* type,
    const char* full_name,
    const char* marker,
    const char* match_regexp,
    const char* output_regexp,
    unsigned long priority)
    /*throw(String::RegEx::Exception)*/
  {
    std::string norm_name = name.str();
    String::AsciiStringManip::to_lower(norm_name);

    std::string low_marker(marker);
    String::AsciiStringManip::to_lower(low_marker);

    if(strcmp(type, "APPLICATION") == 0)
    {
      application_platform_id_ = platform_id;
    }
    else
    {
      std::string low_marker(marker);
      String::AsciiStringManip::to_lower(low_marker);

      if(type[0] == 0)
      {
        os_matchers_->add_rule(
          platform_id,
          norm_name,
          full_name,
          low_marker.c_str(),
          match_regexp,
          output_regexp,
          priority);
      }
      else
      {
        CategoryMatcherMap::const_iterator ci = matchers_.find(type);

        if (ci == matchers_.end())
        {
          ci = matchers_.insert(CategoryMatcherMap::value_type(
            type, CategoryMatcher_var(new CategoryMatcher()))).first;
        }

        ci->second->add_rule(
          platform_id,
          norm_name,
          full_name,
          low_marker.c_str(),
          match_regexp,
          output_regexp,
          priority);
      }
    }
  }

  /**
   *  WebBrowserMatcher implementation
   */
  WebBrowserMatcher::OnMatch::OnMatch(
    const String::SubString& user_agent,
    unsigned long max_priority)
    noexcept
    : user_agent_(user_agent),
      max_priority_(max_priority), 
      matched_element_(0)
  {}

  bool
  WebBrowserMatcher::OnMatch::operator() (
    const MultiStringMatcher::MatchDetails& details)
    /*throw(String::RegEx::Exception)*/
  {
    if (details.tag->priority == max_priority_)
    {
      if (match_(details.tag))
      {
        matched_element_ = details.tag;
        // if max priority found then send terminate matching signal
        return true;
      }
    }
    else
    {
      candidates_.insert(std::make_pair(details.tag->priority, details.tag));
    }

    return false;
  }

  const WebBrowserMatcher::MatchElement*
  WebBrowserMatcher::OnMatch::get_matched_element() /*throw(String::RegEx::Exception)*/
  {
    if (!matched_element_)
    {
      for (MatchElements::const_reverse_iterator ci = candidates_.rbegin();
           ci != candidates_.rend(); ++ci)
      {
        if (match_(ci->second))
        {
          matched_element_ = ci->second;
          break;
        }
      }
    }

    return matched_element_;
  }

  const String::RegEx::Result&
  WebBrowserMatcher::OnMatch::get_sub_strs() const noexcept
  {
    return sub_strs_;
  }

  bool
  WebBrowserMatcher::OnMatch::match_(
    const MatchElement* element)
    /*throw(String::RegEx::Exception)*/
  {
    return (!element->regexp.get() ||
            element->regexp->search(sub_strs_, user_agent_) ||
            !element->regexp_required);
  }


  WebBrowserMatcher::WebBrowserMatcher() noexcept
    : max_priority_(std::numeric_limits<unsigned long>::min()),
      matcher_(false)
  {}

  bool
  WebBrowserMatcher::match(
    std::string& browser,
    const String::SubString& user_agent) const
    /*throw(eh::Exception)*/
  {
    OnMatch on_match(user_agent, max_priority_);
    matcher_.match(user_agent, on_match);

    for (auto it = empty_marker_elements_.begin();
         it != empty_marker_elements_.end(); ++it)
    {
      if (on_match({user_agent, &*it}))
      {
        break;
      }
    }

    const MatchElement* element = on_match.get_matched_element();

    if (element)
    {
      const String::RegEx::Result& sub_strs = on_match.get_sub_strs();
      browser = element->name;

      if (!sub_strs.empty())
      {
        for (String::RegEx::Result::const_iterator sit = ++sub_strs.begin();
            sit != sub_strs.end(); ++sit)
        {
          if (!sit->empty())
          {
            browser += " ";
            sit->append_to(browser);
          }
        }
      }

      if(browser.size() > FULL_PLATFORM_MAX_SIZE)
      {
        browser.resize(FULL_PLATFORM_MAX_SIZE);
      }

      return true;
    }

    return false;
  }

  void WebBrowserMatcher::add_rule(
    const String::SubString& name,
    const char* marker,
    const char* regexp,
    bool regexp_required,
    unsigned long priority)
    /*throw(String::RegEx::Exception)*/
  {
    if (marker[0] == 0)
    {
      empty_marker_elements_.emplace_back(name.str(), marker, regexp, regexp_required, priority);
    }
    else
    {
      elements_.emplace_back(name.str(), marker, regexp, regexp_required, priority);
      matcher_.add_pattern(marker, &elements_.back());
    }

    if (max_priority_ < priority)
    {
      max_priority_ = priority;
    }
  }

  // IPMatcher
  IPMatcher::CohortMaskHashAdapter::CohortMaskHashAdapter(
    const String::SubString& cohort_val,
    uint32_t ip_mask_val)
    noexcept
    : cohort(cohort_val),
      ip_mask(ip_mask_val)
  {
    Generics::Murmur64Hash hasher(hash_);
    Generics::hash_add(hasher, cohort.str());
    Generics::hash_add(hasher, ip_mask);
  }

  inline
  size_t
  IPMatcher::CohortMaskHashAdapter::hash() const noexcept
  {
    return hash_;
  }

  inline
  bool
  IPMatcher::CohortMaskHashAdapter::operator==(
    const CohortMaskHashAdapter& right) const
    noexcept
  {
    return cohort == right.cohort &&
      ip_mask == right.ip_mask;
  }

  struct IPMatcher::BitsMaskMatcherLess
  {
    bool
    operator()(const BitsMaskMatcher& left, uint32_t right)
      const
      noexcept
    {
      return left.bits_mask < right;
    }

    bool
    operator()(uint32_t left, const BitsMaskMatcher& right)
      const
      noexcept
    {
      return left < right.bits_mask;
    }
  };

  IPMatcher::IPMatcher() noexcept
  {}

  void
  IPMatcher::add_rule(
    const String::SubString& ip_masks,
    const String::SubString& cohorts,
    const MatchResult& match_result)
    /*throw(InvalidParameter)*/
  {
    static const char* FUN = "IPMatcher::add_rule()";

    typedef std::list<String::SubString> CohortList;

    CohortList cohort_list;

    if(!cohorts.empty())
    {
      String::StringManip::SplitNL tokenizer(cohorts);
      for (String::SubString cohort; tokenizer.get_token(cohort);)
      {
        const std::string& cohort_holder =
          *cohorts_holder_.insert(cohort.str()).first;

        cohort_list.push_back(cohort_holder);
      }
    }
    else
    {
      cohort_list.push_back(String::SubString());
    }

    String::StringManip::SplitNL tokenizer(ip_masks);
    for (String::SubString ip_mask; tokenizer.get_token(ip_mask);)
    {
      uint32_t bits_mask = 0;

      // parse ip_mask
      String::SubString::SizeType pos = ip_mask.find('/');
      if(pos == String::SubString::NPOS)
      {
        Stream::Error ostr;
        ostr << FUN << ": invalid ip mask '" << ip_mask << "'";
        throw InvalidParameter(ostr.str());
      }

      {
        uint32_t bits_in_mask;
        if(!String::StringManip::str_to_int(
             String::SubString(ip_mask.begin() + pos + 1, ip_mask.end()),
             bits_in_mask))
        {
          Stream::Error ostr;
          ostr << FUN << ": invalid mask bits value in mask '" << ip_mask << "'";
          throw InvalidParameter(ostr.str());
        }

        if(bits_in_mask > 0)
        {
          bits_mask = bits_in_mask < 32 ?
            (0xFFFFFFFF << (32 - bits_in_mask)) : 0xFFFFFFFF;
        }
      }

      bits_mask = htonl(bits_mask); // ip have network byte order

      uint32_t int_ip_mask = str_to_ip_(
        String::SubString(ip_mask.begin(), ip_mask.begin() + pos));

      for(CohortList::const_iterator cohort_it = cohort_list.begin();
          cohort_it != cohort_list.end(); ++cohort_it)
      {
        BitsMaskMatcherArray::iterator bits_matcher_it = std::lower_bound(
          bits_mask_matchers_.begin(),
          bits_mask_matchers_.end(),
          bits_mask,
          BitsMaskMatcherLess());

        BitsMaskMatcher* bits_mask_matcher;

        if(bits_matcher_it == bits_mask_matchers_.end() ||
           bits_matcher_it->bits_mask != bits_mask)
        {
          BitsMaskMatcherArray::iterator ins_it =
            bits_mask_matchers_.insert(bits_matcher_it, BitsMaskMatcher());
          ins_it->bits_mask = bits_mask;
          bits_mask_matcher = &*ins_it;
        }
        else
        {
          bits_mask_matcher = &*bits_matcher_it;
        }

        bits_mask_matcher->cohort_masks.insert(
          std::make_pair(
            CohortMaskHashAdapter(*cohort_it, int_ip_mask),
            match_result));
      }
    }
  }
  
  bool
  IPMatcher::match(
    MatchResult& result,
    const String::SubString& ip,
    const String::SubString& cohort) const
    /*throw(InvalidParameter)*/
  {
    uint32_t int_ip = str_to_ip_(ip);

    // split cohort by dot
    std::vector<String::SubString> cohorts;
    cohorts.reserve(cohort.size());
    String::StringManip::Splitter<
      String::AsciiStringManip::SepPeriod> splitter(cohort);
    String::SubString token;
    while(splitter.get_token(token))
    {
      cohorts.push_back(token);
    }

    if(!cohorts.empty())
    {
      for(BitsMaskMatcherArray::const_iterator
            bits_mask_matcher_it = bits_mask_matchers_.begin();
          bits_mask_matcher_it != bits_mask_matchers_.end();
          ++bits_mask_matcher_it)
      {
        uint32_t masked_ip = int_ip & bits_mask_matcher_it->bits_mask;

        for(std::vector<String::SubString>::const_iterator cohort_it =
              cohorts.begin();
            cohort_it != cohorts.end(); ++cohort_it)
        {
          CohortMaskMap::const_iterator cohort_mask_it =
            bits_mask_matcher_it->cohort_masks.find(
              CohortMaskHashAdapter(*cohort_it, masked_ip));
          if(cohort_mask_it != bits_mask_matcher_it->cohort_masks.end())
          {
            result = cohort_mask_it->second;
            return true;
          } 
        }
      }
    }

    for(BitsMaskMatcherArray::const_iterator
          bits_mask_matcher_it = bits_mask_matchers_.begin();
        bits_mask_matcher_it != bits_mask_matchers_.end();
        ++bits_mask_matcher_it)
    {
      uint32_t masked_ip = int_ip & bits_mask_matcher_it->bits_mask;

      CohortMaskMap::const_iterator cohort_mask_it =
        bits_mask_matcher_it->cohort_masks.find(
          CohortMaskHashAdapter(String::SubString(), masked_ip));
      if(cohort_mask_it != bits_mask_matcher_it->cohort_masks.end())
      {
        result = cohort_mask_it->second;
        return true;
      }
    }

    return false;
  }

  uint32_t
  IPMatcher::str_to_ip_(const String::SubString& ip)
    /*throw(InvalidParameter)*/
  {
    static const char* FUN = "IPMatcher::str_to_ip_()";

    in_addr addr;
    if(::inet_pton(AF_INET, ip.str().c_str(), &addr) <= 0)
    {
      Stream::Error ostr;
      ostr << FUN << ": invalid ip value '" <<
        ip << "'(errno = " << errno << ")";
      throw InvalidParameter(ostr.str());
    }

    return addr.s_addr;
  }

  // CountryFilter
  CountryFilter::CountryFilter() noexcept
  {}

  bool
  CountryFilter::enabled(
    const String::SubString& country_code) const
    noexcept
  {
    if(enabled_countries_.empty())
    {
      return true;
    }

    return enabled_countries_.find(country_code) != enabled_countries_.end();
  }

  void
  CountryFilter::enable_country(
    const String::SubString& country)
    noexcept
  {
    std::string country_str = country.str();
    String::AsciiStringManip::to_lower(country_str);

    {
      std::set<std::string>::const_iterator country_holder_it =
        country_holders_.insert(country_str).first;
      enabled_countries_.insert(*country_holder_it);
    }

    String::AsciiStringManip::to_upper(country_str);

    {
      std::set<std::string>::const_iterator country_holder_it =
        country_holders_.insert(country_str).first;
      enabled_countries_.insert(*country_holder_it);
    }
  }
} /* namespace FrontendCommons */

