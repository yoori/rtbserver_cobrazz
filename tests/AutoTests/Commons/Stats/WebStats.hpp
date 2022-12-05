
#ifndef __AUTOTESTS_COMMONS_STATS_WEBSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_WEBSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class WebStats:    
      public DiffStats<WebStats, 1>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          COUNT          
        };        
        typedef DiffStats<WebStats, 1> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            AutoTest::Time stimestamp_;            
            bool stimestamp_used_;            
            bool stimestamp_null_;            
            AutoTest::Time country_sdate_;            
            bool country_sdate_used_;            
            bool country_sdate_null_;            
            int colo_id_;            
            bool colo_id_used_;            
            bool colo_id_null_;            
            std::string ct_;            
            bool ct_used_;            
            bool ct_null_;            
            std::string curct_;            
            bool curct_used_;            
            bool curct_null_;            
            std::string browser_;            
            bool browser_used_;            
            bool browser_null_;            
            std::string os_;            
            bool os_used_;            
            bool os_null_;            
            std::string app_;            
            bool app_used_;            
            bool app_null_;            
            std::string source_;            
            bool source_used_;            
            bool source_null_;            
            std::string operation_;            
            bool operation_used_;            
            bool operation_null_;            
            std::string result_;            
            bool result_used_;            
            bool result_null_;            
            std::string user_status_;            
            bool user_status_used_;            
            bool user_status_null_;            
            bool test_;            
            bool test_used_;            
            bool test_null_;            
            int cc_id_;            
            bool cc_id_used_;            
            bool cc_id_null_;            
            int tag_id_;            
            bool tag_id_used_;            
            bool tag_id_null_;            
          public:          
            Key& stimestamp(const AutoTest::Time& value);            
            Key& stimestamp_set_null(bool is_null = true);            
            const AutoTest::Time& stimestamp() const;            
            bool stimestamp_used() const;            
            bool stimestamp_is_null() const;            
            Key& country_sdate(const AutoTest::Time& value);            
            Key& country_sdate_set_null(bool is_null = true);            
            const AutoTest::Time& country_sdate() const;            
            bool country_sdate_used() const;            
            bool country_sdate_is_null() const;            
            Key& colo_id(const int& value);            
            Key& colo_id_set_null(bool is_null = true);            
            const int& colo_id() const;            
            bool colo_id_used() const;            
            bool colo_id_is_null() const;            
            Key& ct(const std::string& value);            
            Key& ct_set_null(bool is_null = true);            
            const std::string& ct() const;            
            bool ct_used() const;            
            bool ct_is_null() const;            
            Key& curct(const std::string& value);            
            Key& curct_set_null(bool is_null = true);            
            const std::string& curct() const;            
            bool curct_used() const;            
            bool curct_is_null() const;            
            Key& browser(const std::string& value);            
            Key& browser_set_null(bool is_null = true);            
            const std::string& browser() const;            
            bool browser_used() const;            
            bool browser_is_null() const;            
            Key& os(const std::string& value);            
            Key& os_set_null(bool is_null = true);            
            const std::string& os() const;            
            bool os_used() const;            
            bool os_is_null() const;            
            Key& app(const std::string& value);            
            Key& app_set_null(bool is_null = true);            
            const std::string& app() const;            
            bool app_used() const;            
            bool app_is_null() const;            
            Key& source(const std::string& value);            
            Key& source_set_null(bool is_null = true);            
            const std::string& source() const;            
            bool source_used() const;            
            bool source_is_null() const;            
            Key& operation(const std::string& value);            
            Key& operation_set_null(bool is_null = true);            
            const std::string& operation() const;            
            bool operation_used() const;            
            bool operation_is_null() const;            
            Key& result(const std::string& value);            
            Key& result_set_null(bool is_null = true);            
            const std::string& result() const;            
            bool result_used() const;            
            bool result_is_null() const;            
            Key& user_status(const std::string& value);            
            Key& user_status_set_null(bool is_null = true);            
            const std::string& user_status() const;            
            bool user_status_used() const;            
            bool user_status_is_null() const;            
            Key& test(const bool& value);            
            Key& test_set_null(bool is_null = true);            
            const bool& test() const;            
            bool test_used() const;            
            bool test_is_null() const;            
            Key& cc_id(const int& value);            
            Key& cc_id_set_null(bool is_null = true);            
            const int& cc_id() const;            
            bool cc_id_used() const;            
            bool cc_id_is_null() const;            
            Key& tag_id(const int& value);            
            Key& tag_id_set_null(bool is_null = true);            
            const int& tag_id() const;            
            bool tag_id_used() const;            
            bool tag_id_is_null() const;            
            Key ();            
            Key (            
              const AutoTest::Time& stimestamp,              
              const AutoTest::Time& country_sdate,              
              const int& colo_id,              
              const std::string& ct,              
              const std::string& curct,              
              const std::string& browser,              
              const std::string& os,              
              const std::string& app,              
              const std::string& source,              
              const std::string& operation,              
              const std::string& result,              
              const std::string& user_status,              
              const bool& test,              
              const int& cc_id,              
              const int& tag_id              
            );            
        };        
        stats_value_type count () const;        
        void print_idname (std::ostream& out) const;        
                
        WebStats (const Key& value);        
        WebStats (        
          const AutoTest::Time& stimestamp,          
          const AutoTest::Time& country_sdate,          
          const int& colo_id,          
          const std::string& ct,          
          const std::string& curct,          
          const std::string& browser,          
          const std::string& os,          
          const std::string& app,          
          const std::string& source,          
          const std::string& operation,          
          const std::string& result,          
          const std::string& user_status,          
          const bool& test,          
          const int& cc_id,          
          const int& tag_id          
        );        
        WebStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const AutoTest::Time& stimestamp,          
          const AutoTest::Time& country_sdate,          
          const int& colo_id,          
          const std::string& ct,          
          const std::string& curct,          
          const std::string& browser,          
          const std::string& os,          
          const std::string& app,          
          const std::string& source,          
          const std::string& operation,          
          const std::string& result,          
          const std::string& user_status,          
          const bool& test,          
          const int& cc_id,          
          const int& tag_id          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<WebStats, 1>::Diffs    
    {    
      public:      
        typedef stats_diff_type array_type[1];        
        typedef const stats_diff_type const_array_type[1];        
        typedef const const_array_type& const_array_type_ref;        
        typedef const stats_diff_type* const_iterator;        
                
        Diffs();        
        Diffs(const stats_diff_type& value);        
        Diffs(const const_array_type& array);        
        operator const_array_type_ref () const;        
        Diffs& operator= (const const_array_type& array);        
        Diffs& operator+= (const const_array_type& array);        
        Diffs& operator-= (const const_array_type& array);        
        Diffs operator+ (const const_array_type& array);        
        Diffs operator- (const const_array_type& array);        
                
        stats_diff_type& operator[] (int i);        
        const stats_diff_type& operator[] (int i) const;        
                
        const_iterator begin() const;        
        const_iterator end() const;        
        int size() const;        
        void clear();        
                
        Diffs& count (const stats_diff_type& value);        
        const stats_diff_type& count () const;        
      protected:      
        stats_diff_type diffs[1];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// WebStats    
    inline    
    WebStats::WebStats ()    
      :Base("WebStats")    
    {}    
    inline    
    WebStats::WebStats (const WebStats::Key& value)    
      :Base("WebStats")    
    {    
      key_ = value;      
    }    
    inline    
    WebStats::WebStats (    
      const AutoTest::Time& stimestamp,      
      const AutoTest::Time& country_sdate,      
      const int& colo_id,      
      const std::string& ct,      
      const std::string& curct,      
      const std::string& browser,      
      const std::string& os,      
      const std::string& app,      
      const std::string& source,      
      const std::string& operation,      
      const std::string& result,      
      const std::string& user_status,      
      const bool& test,      
      const int& cc_id,      
      const int& tag_id      
    )    
      :Base("WebStats")    
    {    
      key_ = Key (      
        stimestamp,        
        country_sdate,        
        colo_id,        
        ct,        
        curct,        
        browser,        
        os,        
        app,        
        source,        
        operation,        
        result,        
        user_status,        
        test,        
        cc_id,        
        tag_id        
      );      
    }    
    inline    
    WebStats::Key::Key ()    
      :stimestamp_(default_date()),country_sdate_(default_date()),colo_id_(0),test_(false),cc_id_(0),tag_id_(0)    
    {    
      stimestamp_used_ = false;      
      stimestamp_null_ = false;      
      country_sdate_used_ = false;      
      country_sdate_null_ = false;      
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      ct_used_ = false;      
      ct_null_ = false;      
      curct_used_ = false;      
      curct_null_ = false;      
      browser_used_ = false;      
      browser_null_ = false;      
      os_used_ = false;      
      os_null_ = false;      
      app_used_ = false;      
      app_null_ = false;      
      source_used_ = false;      
      source_null_ = false;      
      operation_used_ = false;      
      operation_null_ = false;      
      result_used_ = false;      
      result_null_ = false;      
      user_status_used_ = false;      
      user_status_null_ = false;      
      test_used_ = false;      
      test_null_ = false;      
      cc_id_used_ = false;      
      cc_id_null_ = false;      
      tag_id_used_ = false;      
      tag_id_null_ = false;      
    }    
    inline    
    WebStats::Key::Key (    
      const AutoTest::Time& stimestamp,      
      const AutoTest::Time& country_sdate,      
      const int& colo_id,      
      const std::string& ct,      
      const std::string& curct,      
      const std::string& browser,      
      const std::string& os,      
      const std::string& app,      
      const std::string& source,      
      const std::string& operation,      
      const std::string& result,      
      const std::string& user_status,      
      const bool& test,      
      const int& cc_id,      
      const int& tag_id      
    )    
      :stimestamp_(stimestamp),country_sdate_(country_sdate),colo_id_(colo_id),test_(test),cc_id_(cc_id),tag_id_(tag_id)    
    {    
      stimestamp_used_ = true;      
      stimestamp_null_ = false;      
      country_sdate_used_ = true;      
      country_sdate_null_ = false;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      ct_ = ct;      
      ct_used_ = true;      
      ct_null_ = false;      
      curct_ = curct;      
      curct_used_ = true;      
      curct_null_ = false;      
      browser_ = browser;      
      browser_used_ = true;      
      browser_null_ = false;      
      os_ = os;      
      os_used_ = true;      
      os_null_ = false;      
      app_ = app;      
      app_used_ = true;      
      app_null_ = false;      
      source_ = source;      
      source_used_ = true;      
      source_null_ = false;      
      operation_ = operation;      
      operation_used_ = true;      
      operation_null_ = false;      
      result_ = result;      
      result_used_ = true;      
      result_null_ = false;      
      user_status_ = user_status;      
      user_status_used_ = true;      
      user_status_null_ = false;      
      test_used_ = true;      
      test_null_ = false;      
      cc_id_used_ = true;      
      cc_id_null_ = false;      
      tag_id_used_ = true;      
      tag_id_null_ = false;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::stimestamp(const AutoTest::Time& value)    
    {    
      stimestamp_ = value;      
      stimestamp_used_ = true;      
      stimestamp_null_ = false;      
      return *this;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::stimestamp_set_null(bool is_null)    
    {    
      stimestamp_used_ = true;      
      stimestamp_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    WebStats::Key::stimestamp() const    
    {    
      return stimestamp_;      
    }    
    inline    
    bool    
    WebStats::Key::stimestamp_used() const    
    {    
      return stimestamp_used_;      
    }    
    inline    
    bool    
    WebStats::Key::stimestamp_is_null() const    
    {    
      return stimestamp_null_;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::country_sdate(const AutoTest::Time& value)    
    {    
      country_sdate_ = value;      
      country_sdate_used_ = true;      
      country_sdate_null_ = false;      
      return *this;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::country_sdate_set_null(bool is_null)    
    {    
      country_sdate_used_ = true;      
      country_sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    WebStats::Key::country_sdate() const    
    {    
      return country_sdate_;      
    }    
    inline    
    bool    
    WebStats::Key::country_sdate_used() const    
    {    
      return country_sdate_used_;      
    }    
    inline    
    bool    
    WebStats::Key::country_sdate_is_null() const    
    {    
      return country_sdate_null_;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    WebStats::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    WebStats::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    WebStats::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::ct(const std::string& value)    
    {    
      ct_ = value;      
      ct_used_ = true;      
      ct_null_ = false;      
      return *this;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::ct_set_null(bool is_null)    
    {    
      ct_used_ = true;      
      ct_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    WebStats::Key::ct() const    
    {    
      return ct_;      
    }    
    inline    
    bool    
    WebStats::Key::ct_used() const    
    {    
      return ct_used_;      
    }    
    inline    
    bool    
    WebStats::Key::ct_is_null() const    
    {    
      return ct_null_;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::curct(const std::string& value)    
    {    
      curct_ = value;      
      curct_used_ = true;      
      curct_null_ = false;      
      return *this;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::curct_set_null(bool is_null)    
    {    
      curct_used_ = true;      
      curct_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    WebStats::Key::curct() const    
    {    
      return curct_;      
    }    
    inline    
    bool    
    WebStats::Key::curct_used() const    
    {    
      return curct_used_;      
    }    
    inline    
    bool    
    WebStats::Key::curct_is_null() const    
    {    
      return curct_null_;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::browser(const std::string& value)    
    {    
      browser_ = value;      
      browser_used_ = true;      
      browser_null_ = false;      
      return *this;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::browser_set_null(bool is_null)    
    {    
      browser_used_ = true;      
      browser_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    WebStats::Key::browser() const    
    {    
      return browser_;      
    }    
    inline    
    bool    
    WebStats::Key::browser_used() const    
    {    
      return browser_used_;      
    }    
    inline    
    bool    
    WebStats::Key::browser_is_null() const    
    {    
      return browser_null_;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::os(const std::string& value)    
    {    
      os_ = value;      
      os_used_ = true;      
      os_null_ = false;      
      return *this;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::os_set_null(bool is_null)    
    {    
      os_used_ = true;      
      os_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    WebStats::Key::os() const    
    {    
      return os_;      
    }    
    inline    
    bool    
    WebStats::Key::os_used() const    
    {    
      return os_used_;      
    }    
    inline    
    bool    
    WebStats::Key::os_is_null() const    
    {    
      return os_null_;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::app(const std::string& value)    
    {    
      app_ = value;      
      app_used_ = true;      
      app_null_ = false;      
      return *this;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::app_set_null(bool is_null)    
    {    
      app_used_ = true;      
      app_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    WebStats::Key::app() const    
    {    
      return app_;      
    }    
    inline    
    bool    
    WebStats::Key::app_used() const    
    {    
      return app_used_;      
    }    
    inline    
    bool    
    WebStats::Key::app_is_null() const    
    {    
      return app_null_;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::source(const std::string& value)    
    {    
      source_ = value;      
      source_used_ = true;      
      source_null_ = false;      
      return *this;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::source_set_null(bool is_null)    
    {    
      source_used_ = true;      
      source_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    WebStats::Key::source() const    
    {    
      return source_;      
    }    
    inline    
    bool    
    WebStats::Key::source_used() const    
    {    
      return source_used_;      
    }    
    inline    
    bool    
    WebStats::Key::source_is_null() const    
    {    
      return source_null_;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::operation(const std::string& value)    
    {    
      operation_ = value;      
      operation_used_ = true;      
      operation_null_ = false;      
      return *this;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::operation_set_null(bool is_null)    
    {    
      operation_used_ = true;      
      operation_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    WebStats::Key::operation() const    
    {    
      return operation_;      
    }    
    inline    
    bool    
    WebStats::Key::operation_used() const    
    {    
      return operation_used_;      
    }    
    inline    
    bool    
    WebStats::Key::operation_is_null() const    
    {    
      return operation_null_;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::result(const std::string& value)    
    {    
      result_ = value;      
      result_used_ = true;      
      result_null_ = false;      
      return *this;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::result_set_null(bool is_null)    
    {    
      result_used_ = true;      
      result_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    WebStats::Key::result() const    
    {    
      return result_;      
    }    
    inline    
    bool    
    WebStats::Key::result_used() const    
    {    
      return result_used_;      
    }    
    inline    
    bool    
    WebStats::Key::result_is_null() const    
    {    
      return result_null_;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::user_status(const std::string& value)    
    {    
      user_status_ = value;      
      user_status_used_ = true;      
      user_status_null_ = false;      
      return *this;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::user_status_set_null(bool is_null)    
    {    
      user_status_used_ = true;      
      user_status_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    WebStats::Key::user_status() const    
    {    
      return user_status_;      
    }    
    inline    
    bool    
    WebStats::Key::user_status_used() const    
    {    
      return user_status_used_;      
    }    
    inline    
    bool    
    WebStats::Key::user_status_is_null() const    
    {    
      return user_status_null_;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::test(const bool& value)    
    {    
      test_ = value;      
      test_used_ = true;      
      test_null_ = false;      
      return *this;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::test_set_null(bool is_null)    
    {    
      test_used_ = true;      
      test_null_ = is_null;      
      return *this;      
    }    
    inline    
    const bool&    
    WebStats::Key::test() const    
    {    
      return test_;      
    }    
    inline    
    bool    
    WebStats::Key::test_used() const    
    {    
      return test_used_;      
    }    
    inline    
    bool    
    WebStats::Key::test_is_null() const    
    {    
      return test_null_;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::cc_id(const int& value)    
    {    
      cc_id_ = value;      
      cc_id_used_ = true;      
      cc_id_null_ = false;      
      return *this;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::cc_id_set_null(bool is_null)    
    {    
      cc_id_used_ = true;      
      cc_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    WebStats::Key::cc_id() const    
    {    
      return cc_id_;      
    }    
    inline    
    bool    
    WebStats::Key::cc_id_used() const    
    {    
      return cc_id_used_;      
    }    
    inline    
    bool    
    WebStats::Key::cc_id_is_null() const    
    {    
      return cc_id_null_;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::tag_id(const int& value)    
    {    
      tag_id_ = value;      
      tag_id_used_ = true;      
      tag_id_null_ = false;      
      return *this;      
    }    
    inline    
    WebStats::Key&    
    WebStats::Key::tag_id_set_null(bool is_null)    
    {    
      tag_id_used_ = true;      
      tag_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    WebStats::Key::tag_id() const    
    {    
      return tag_id_;      
    }    
    inline    
    bool    
    WebStats::Key::tag_id_used() const    
    {    
      return tag_id_used_;      
    }    
    inline    
    bool    
    WebStats::Key::tag_id_is_null() const    
    {    
      return tag_id_null_;      
    }    
    inline    
    WebStats::Key&    
    WebStats::key (    
      const AutoTest::Time& stimestamp,      
      const AutoTest::Time& country_sdate,      
      const int& colo_id,      
      const std::string& ct,      
      const std::string& curct,      
      const std::string& browser,      
      const std::string& os,      
      const std::string& app,      
      const std::string& source,      
      const std::string& operation,      
      const std::string& result,      
      const std::string& user_status,      
      const bool& test,      
      const int& cc_id,      
      const int& tag_id      
    )    
    {    
      key_ = Key (      
        stimestamp,        
        country_sdate,        
        colo_id,        
        ct,        
        curct,        
        browser,        
        os,        
        app,        
        source,        
        operation,        
        result,        
        user_status,        
        test,        
        cc_id,        
        tag_id        
      );      
      return key_;      
    }    
    inline    
    WebStats::Key&    
    WebStats::key ()    
    {    
      return key_;      
    }    
    inline    
    WebStats::Key&    
    WebStats::key (const WebStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<WebStats, 1>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<WebStats, 1>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<WebStats, 1>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<WebStats, 1>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<WebStats, 1>::Diffs&     
    DiffStats<WebStats, 1>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<WebStats, 1>::Diffs&     
    DiffStats<WebStats, 1>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<WebStats, 1>::Diffs&     
    DiffStats<WebStats, 1>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<WebStats, 1>::Diffs     
    DiffStats<WebStats, 1>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<WebStats, 1>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<WebStats, 1>::Diffs     
    DiffStats<WebStats, 1>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<WebStats, 1>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<WebStats, 1>::Diffs::const_iterator    
    DiffStats<WebStats, 1>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<WebStats, 1>::Diffs::const_iterator    
    DiffStats<WebStats, 1>::Diffs::end () const    
    {    
      return diffs + 1;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<WebStats, 1>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<WebStats, 1>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<WebStats, 1>::Diffs::size () const    
    {    
      return 1;      
    }    
    inline    
    void    
    DiffStats<WebStats, 1>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 1; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    WebStats::count () const    
    {    
      return values[WebStats::COUNT];      
    }    
    inline    
    DiffStats<WebStats, 1>::Diffs&     
    DiffStats<WebStats, 1>::Diffs::count (const stats_diff_type& value)    
    {    
      diffs[WebStats::COUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<WebStats, 1>::Diffs::count () const    
    {    
      return diffs[WebStats::COUNT];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_WEBSTATS_HPP

