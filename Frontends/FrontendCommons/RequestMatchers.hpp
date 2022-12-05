/// @file FrontendCommons/RequestMatchers.hpp

#ifndef _FRONTENDCOMMONS_REQUESTMATCHERS_HPP_
#define _FRONTENDCOMMONS_REQUESTMATCHERS_HPP_

#include <string>
#include <list>
#include <vector>
#include <set>
#include <queue>
#include <map>
#include <memory>

#include <eh/Exception.hpp>
#include <Generics/GnuHashTable.hpp>
#include <String/RegEx.hpp>
#include <String/InterConvertion.hpp>
#include <HTTP/UrlAddress.hpp>
#include <Language/SegmentorCommons/SegmentorInterface.hpp>

#include "AhoCorasick.hpp"

namespace FrontendCommons
{
  /**
   * UrlMatcher class
   */
  class UrlMatcher: public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  public:
    UrlMatcher() noexcept;

    bool
    match(
      unsigned long& search_engine_id,
      std::string& match_result,
      const HTTP::HTTPAddress& url,
      const Language::Segmentor::SegmentorInterface* segmentor) const
      /*throw(eh::Exception)*/;

    bool
    match(
      unsigned long& search_engine_id,
      std::string& match_result,
      const String::SubString& url,
      const Language::Segmentor::SegmentorInterface* segmentor) const
      /*throw(eh::Exception)*/;

    void add_rule(
      unsigned long search_engine_id,
      const char* host_postfix,
      const char* regexp_val,
      unsigned long dec_depth,
      const char* enc,
      const char* post_enc)
      /*throw(Exception)*/;

  private:
    static bool
    html_unicode_decode(const String::SubString& in, std::string& buf)
      /*throw(eh::Exception)*/;

  private:
    struct UrlMatchElement: public ReferenceCounting::DefaultImpl<>
    {
      enum SpecialCases
      {
        SC_NONE = 0,
        SC_JS,
        SC_HTML
      };

      UrlMatchElement(
        unsigned long search_engine_id_val,
        const char* regexp_val,
        unsigned long dec_depth,
        const char* enc,
        const char* post_enc)
        /*throw(String::RegEx::Exception, eh::Exception)*/;

      const unsigned long search_engine_id;
      const String::RegEx regexp;
      const unsigned long decoding_depth;
      std::string charset;
      char special;
      SpecialCases special_way;

    protected:
      virtual ~UrlMatchElement() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<UrlMatchElement> UrlMatchElement_var;
    typedef std::list<UrlMatchElement_var> UrlMatchElementList;

    // hold hostname part matchers (separated by dot) in reverse order
    struct HostnameMatcher: public ReferenceCounting::DefaultImpl<>
    {
      typedef Generics::GnuHashTable<Generics::StringHashAdapter,
        ReferenceCounting::SmartPtr<HostnameMatcher> >
        HostnameMatchMap;

      HostnameMatchMap hostname_matchers;
      UrlMatchElementList regexps;

    protected:
      virtual ~HostnameMatcher() noexcept
      {}
    };

    typedef ReferenceCounting::SmartPtr<HostnameMatcher> HostnameMatcher_var;

  private:
    virtual ~UrlMatcher() noexcept {}

    static bool
    match_hostname_(
      unsigned long& search_engine_id,
      std::string& match_result,
      const HostnameMatcher* hostname_matcher,
      const String::SubString& hostname,
      const String::SubString& path_and_query,
      const Language::Segmentor::SegmentorInterface* segmentor)
      /*throw(Exception)*/;

    static bool
    match_list_(
      unsigned long& search_engine_id,
      std::string& match_result,
      const UrlMatchElementList& match_elements,
      const String::SubString& path_and_query,
      const Language::Segmentor::SegmentorInterface* segmentor)
      /*throw(Exception)*/;

  private:
    HostnameMatcher_var root_matcher_;
  };

  typedef ReferenceCounting::SmartPtr<UrlMatcher> UrlMatcher_var;

  /** PlatformMatcher */
  class PlatformMatcher: public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    typedef std::set<unsigned long> PlatformIdSet;
    typedef std::set<std::string> PlatformNameSet;

    PlatformMatcher() /*throw(eh::Exception)*/;

    bool
    match(
      PlatformIdSet* platform_id,
      std::string& platform,
      std::string& full_platform,
      const String::SubString& user_agent,
      bool application = false) const
      /*throw(eh::Exception)*/;

    bool
    match(
      PlatformIdSet* platform_id,
      PlatformNameSet* platform_names,
      std::string& platform,
      std::string& full_platform,
      const String::SubString& user_agent,
      bool application = false) const
      /*throw(eh::Exception)*/;

    void
    add_rule(
      unsigned long platform_id,
      const String::SubString& name,
      const char* type, // empty type used for OS
      const char* full_name,
      const char* marker,
      const char* match_regexp,
      const char* output_regexp,
      unsigned long priority)
      /*throw(String::RegEx::Exception)*/;

  private:
    struct MatchElement
    {
      MatchElement(
        unsigned long platform_id,
        const String::SubString& name,
        const char* full_name,
        const char* marker,
        const char* regexp,
        const char* output_regexp,
        unsigned long priority_val)
        /*throw(String::RegEx::Exception)*/;

      MatchElement(const MatchElement& init) noexcept;

      unsigned long platform_id;
      std::string name;
      std::string full_name;
      std::string marker;
      std::unique_ptr<String::RegEx> match_regexp;
      std::unique_ptr<String::RegEx> output_regexp;
      unsigned long priority;
    };

    typedef AhoCorasik<const MatchElement*> MultiStringMatcher;

    class OnMatch
    {
    public:
      OnMatch(unsigned long max_priority) noexcept;

      bool
      operator() (const MultiStringMatcher::MatchDetails& details)
        /*throw(String::RegEx::Exception)*/;

      const MatchElement*
      get_matched_element() const noexcept;

    private:
      const unsigned long max_priority_;
      const MatchElement* matched_element_;
    };

    class CategoryMatcher : public ReferenceCounting::AtomicImpl
    {
    public:
      CategoryMatcher() noexcept;

      const MatchElement*
      match(const String::SubString& user_agent) const
        /*throw(eh::Exception, String::RegEx::Exception)*/;

      void
      add_rule(
        unsigned long platform_id,
        const String::SubString& name,
        const char* full_name,
        const char* marker,
        const char* match_regexp,
        const char* output_regexp,
        unsigned long priority)
        /*throw(String::RegEx::Exception)*/;
	
    protected:
      virtual
      ~CategoryMatcher() noexcept {}

    private:
      unsigned long max_priority_;
      std::list<MatchElement> elements_;
      MultiStringMatcher matcher_;
      std::list<MatchElement> empty_marker_elements_;
    };

    typedef ReferenceCounting::SmartPtr<CategoryMatcher> CategoryMatcher_var;

  protected:
    virtual ~PlatformMatcher() noexcept {}

    bool
    match_(
      PlatformIdSet* platform_ids,
      PlatformNameSet* platform_names,
      std::string* platform,
      std::string* full_platform,
      const CategoryMatcher& matchers,
      const String::SubString& low_user_agent) const
      /*throw(eh::Exception)*/;

  private:
    typedef std::map<std::string, CategoryMatcher_var> CategoryMatcherMap;
    CategoryMatcherMap matchers_;
    CategoryMatcher_var os_matchers_;
    unsigned long application_platform_id_;
  };

  typedef ReferenceCounting::SmartPtr<PlatformMatcher> PlatformMatcher_var;

  /** WebBrowserMatcher */
  class WebBrowserMatcher: public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    
    WebBrowserMatcher() noexcept;

    bool
    match(
      std::string& browser,
      const String::SubString& user_agent) const
      /*throw(eh::Exception)*/;

    void add_rule(
      const String::SubString& name,
      const char* marker,
      const char* regexp,
      bool regexp_required,
      unsigned long priority)
      /*throw(String::RegEx::Exception)*/;

  protected:
    virtual ~WebBrowserMatcher() noexcept {}

  private:
    struct MatchElement
    {
      MatchElement(
        const String::SubString& name,
        const char* marker,
        const char* re,
        bool regexp_required_val,
        unsigned long priority_val)
        /*throw(String::RegEx::Exception)*/;

      MatchElement(const MatchElement& init) noexcept;

      std::string name;
      std::string marker;
      std::unique_ptr<String::RegEx> regexp;
      bool regexp_required;
      unsigned long priority;
    };

    typedef AhoCorasik<const MatchElement*> MultiStringMatcher;

    class OnMatch
    {
    public:
      OnMatch(const String::SubString& user_agent,
        unsigned long max_priority) noexcept;

      bool
      operator() (const MultiStringMatcher::MatchDetails& details)
        /*throw(String::RegEx::Exception)*/;

      const MatchElement*
      get_matched_element() /*throw(String::RegEx::Exception)*/;

      const String::RegEx::Result&
      get_sub_strs() const noexcept;

    private:
      String::SubString user_agent_;
      unsigned long max_priority_;
      const MatchElement* matched_element_;
      String::RegEx::Result sub_strs_;
      typedef std::multimap<unsigned long, const MatchElement*> MatchElements;
      MatchElements candidates_;

    private:
      bool
      match_(const MatchElement* element) /*throw(String::RegEx::Exception)*/;
    };

    typedef std::list<MatchElement> MatchElementList;

  private:
    unsigned long max_priority_;
    MatchElementList elements_;
    MultiStringMatcher matcher_;
    MatchElementList empty_marker_elements_;
  };

  typedef ReferenceCounting::SmartPtr<WebBrowserMatcher> WebBrowserMatcher_var;

  // IPMatcher
  class IPMatcher: public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(InvalidParameter, Exception);

    IPMatcher() noexcept;

    struct MatchResult
    {
      unsigned long colo_id;
      bool profile_referer;
    };

    bool
    match(
      MatchResult& result,
      const String::SubString& ip,
      const String::SubString& cohort) const
      /*throw(InvalidParameter)*/;

    void
    add_rule(
      const String::SubString& ip_masks,
      const String::SubString& cohorts,
      const MatchResult& match_result)
      /*throw(InvalidParameter)*/;

  protected:
    class CohortMaskHashAdapter
    {
    public:
      CohortMaskHashAdapter(
        const String::SubString& cohort_val,
        uint32_t ip_mask_val)
        noexcept;

      size_t
      hash() const noexcept;

      bool
      operator==(const CohortMaskHashAdapter& right) const
        noexcept;

      const String::SubString cohort;
      const uint32_t ip_mask;

    protected:
      size_t hash_;
    };

    typedef Generics::GnuHashTable<
      CohortMaskHashAdapter,
      MatchResult>
      CohortMaskMap;

    struct BitsMaskMatcher
    {
      uint32_t bits_mask;
      CohortMaskMap cohort_masks;
    };

    typedef std::vector<BitsMaskMatcher> BitsMaskMatcherArray;

    typedef std::set<std::string> CohortSet;

    class BitsMaskMatcherLess;

  protected:
    virtual ~IPMatcher() noexcept {}

    static uint32_t
    str_to_ip_(const String::SubString& ip)
      /*throw(InvalidParameter)*/;

  private:
    BitsMaskMatcherArray bits_mask_matchers_;
    CohortSet cohorts_holder_;
  };

  typedef ReferenceCounting::SmartPtr<IPMatcher>
    IPMatcher_var;

  // CountryFilter
  class CountryFilter: public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(InvalidParameter, Exception);

    CountryFilter() noexcept;

    bool
    enabled(const String::SubString& country_code) const
      noexcept;

    void
    enable_country(
      const String::SubString& country)
      noexcept;

  protected:
    typedef Generics::GnuHashSet<
      Generics::SubStringHashAdapter>
      CountrySet;

  protected:
    virtual ~CountryFilter() noexcept {}

  private:
    CountrySet enabled_countries_;
    std::set<std::string> country_holders_;
  };

  typedef ReferenceCounting::SmartPtr<CountryFilter>
    CountryFilter_var;
}

//
// Implementations
//

namespace FrontendCommons
{
  inline
  UrlMatcher::
  UrlMatchElement::UrlMatchElement(
    unsigned long search_engine_id_val,
    const char* regexp_val,
    unsigned long dec_depth,
    const char* enc,
    const char* post_enc)
    /*throw(String::RegEx::Exception, eh::Exception)*/
    : search_engine_id(search_engine_id_val),
      regexp(String::SubString(regexp_val)),
      decoding_depth(dec_depth),
      special(0),
      special_way(SC_NONE)
  {
    if(enc)
    {
      charset = enc;
      String::AsciiStringManip::to_upper(charset);
    }
    if (post_enc)
    {
      static const char* JS_UNICODE = "js_unicode:";
      if(strlen(post_enc) > strlen(JS_UNICODE) &&
         strncmp(post_enc, JS_UNICODE, strlen(JS_UNICODE)) == 0)
      {
        special = *(post_enc + strlen(JS_UNICODE));
        special_way = SC_JS;
      }
      else if(strcmp(post_enc, "html") == 0)
      {
        special_way = SC_HTML;
      }
    }
  }

  inline
  PlatformMatcher::
  MatchElement::MatchElement(
    unsigned long platform_id_val,
    const String::SubString& name_val,
    const char* full_name_val,
    const char* marker_val,
    const char* match_regexp_val,
    const char* output_regexp_val,
    unsigned long priority_val)
    /*throw(String::RegEx::Exception)*/
    : platform_id(platform_id_val),
      name(name_val.str()),
      full_name(full_name_val[0] ? std::string(full_name_val) : name_val.str()),
      marker(marker_val),
      match_regexp(match_regexp_val[0] ? new String::RegEx(String::SubString(match_regexp_val)) : 0),
      output_regexp(output_regexp_val[0] ? new String::RegEx(String::SubString(output_regexp_val)) : 0),
      priority(priority_val)
  {}

  inline
  PlatformMatcher::
  MatchElement::MatchElement(
    const MatchElement& init)
    noexcept
    : platform_id(init.platform_id),
      name(init.name),
      full_name(init.full_name),
      marker(init.marker),
      match_regexp(init.match_regexp.get() ? new String::RegEx(*init.match_regexp) : 0),
      output_regexp(init.output_regexp.get() ? new String::RegEx(*init.output_regexp) : 0),
      priority(init.priority)
  {}

  inline
  WebBrowserMatcher::
  MatchElement::MatchElement(
    const String::SubString& name_val,
    const char* marker_val,
    const char* re,
    bool regexp_required_val,
    unsigned long priority_val)
    /*throw(String::RegEx::Exception)*/
      : name(name_val.str()),
      marker(marker_val),
      regexp(re[0] ? new String::RegEx(String::SubString(re)) : 0),
      regexp_required(regexp_required_val),
      priority(priority_val)
  {}

  inline
  WebBrowserMatcher::
  MatchElement::MatchElement(
    const MatchElement& init)
    noexcept
    : name(init.name),
      marker(init.marker),
      regexp(init.regexp.get() ? new String::RegEx(*init.regexp) : 0),
      regexp_required(init.regexp_required),
      priority(init.priority)
  {}
}

#endif /*_FRONTENDCOMMONS_REQUESTMATCHERS_HPP_*/
