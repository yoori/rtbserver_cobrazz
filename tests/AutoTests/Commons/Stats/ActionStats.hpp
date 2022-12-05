
#ifndef __AUTOTESTS_COMMONS_STATS_ACTIONSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_ACTIONSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class ActionStats:    
      public DiffStats<ActionStats, 1>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          COUNT          
        };        
        typedef DiffStats<ActionStats, 1> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            int action_request_id_;            
            bool action_request_id_used_;            
            bool action_request_id_null_;            
            int action_id_;            
            bool action_id_used_;            
            bool action_id_null_;            
            int cc_id_;            
            bool cc_id_used_;            
            bool cc_id_null_;            
            int tag_id_;            
            bool tag_id_used_;            
            bool tag_id_null_;            
            int colo_id_;            
            bool colo_id_used_;            
            bool colo_id_null_;            
            std::string country_code_;            
            bool country_code_used_;            
            bool country_code_null_;            
            std::string action_referrer_url_;            
            bool action_referrer_url_used_;            
            bool action_referrer_url_null_;            
            AutoTest::Time action_date_;            
            bool action_date_used_;            
            bool action_date_null_;            
            AutoTest::Time imp_date_;            
            bool imp_date_used_;            
            bool imp_date_null_;            
            AutoTest::Time click_date_;            
            bool click_date_used_;            
            bool click_date_null_;            
            double cur_value_;            
            bool cur_value_used_;            
            bool cur_value_null_;            
            std::string order_id_;            
            bool order_id_used_;            
            bool order_id_null_;            
          public:          
            Key& action_request_id(const int& value);            
            Key& action_request_id_set_null(bool is_null = true);            
            const int& action_request_id() const;            
            bool action_request_id_used() const;            
            bool action_request_id_is_null() const;            
            Key& action_id(const int& value);            
            Key& action_id_set_null(bool is_null = true);            
            const int& action_id() const;            
            bool action_id_used() const;            
            bool action_id_is_null() const;            
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
            Key& colo_id(const int& value);            
            Key& colo_id_set_null(bool is_null = true);            
            const int& colo_id() const;            
            bool colo_id_used() const;            
            bool colo_id_is_null() const;            
            Key& country_code(const std::string& value);            
            Key& country_code_set_null(bool is_null = true);            
            const std::string& country_code() const;            
            bool country_code_used() const;            
            bool country_code_is_null() const;            
            Key& action_referrer_url(const std::string& value);            
            Key& action_referrer_url_set_null(bool is_null = true);            
            const std::string& action_referrer_url() const;            
            bool action_referrer_url_used() const;            
            bool action_referrer_url_is_null() const;            
            Key& action_date(const AutoTest::Time& value);            
            Key& action_date_set_null(bool is_null = true);            
            const AutoTest::Time& action_date() const;            
            bool action_date_used() const;            
            bool action_date_is_null() const;            
            Key& imp_date(const AutoTest::Time& value);            
            Key& imp_date_set_null(bool is_null = true);            
            const AutoTest::Time& imp_date() const;            
            bool imp_date_used() const;            
            bool imp_date_is_null() const;            
            Key& click_date(const AutoTest::Time& value);            
            Key& click_date_set_null(bool is_null = true);            
            const AutoTest::Time& click_date() const;            
            bool click_date_used() const;            
            bool click_date_is_null() const;            
            Key& cur_value(const double& value);            
            Key& cur_value_set_null(bool is_null = true);            
            const double& cur_value() const;            
            bool cur_value_used() const;            
            bool cur_value_is_null() const;            
            Key& order_id(const std::string& value);            
            Key& order_id_set_null(bool is_null = true);            
            const std::string& order_id() const;            
            bool order_id_used() const;            
            bool order_id_is_null() const;            
            Key ();            
            Key (            
              const int& action_request_id,              
              const int& action_id,              
              const int& cc_id,              
              const int& tag_id,              
              const int& colo_id,              
              const std::string& country_code,              
              const std::string& action_referrer_url,              
              const AutoTest::Time& action_date,              
              const AutoTest::Time& imp_date,              
              const AutoTest::Time& click_date,              
              const double& cur_value,              
              const std::string& order_id              
            );            
        };        
        stats_value_type count () const;        
        void print_idname (std::ostream& out) const;        
                
        ActionStats (const Key& value);        
        ActionStats (        
          const int& action_request_id,          
          const int& action_id,          
          const int& cc_id,          
          const int& tag_id,          
          const int& colo_id,          
          const std::string& country_code,          
          const std::string& action_referrer_url,          
          const AutoTest::Time& action_date,          
          const AutoTest::Time& imp_date,          
          const AutoTest::Time& click_date,          
          const double& cur_value,          
          const std::string& order_id          
        );        
        ActionStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& action_request_id,          
          const int& action_id,          
          const int& cc_id,          
          const int& tag_id,          
          const int& colo_id,          
          const std::string& country_code,          
          const std::string& action_referrer_url,          
          const AutoTest::Time& action_date,          
          const AutoTest::Time& imp_date,          
          const AutoTest::Time& click_date,          
          const double& cur_value,          
          const std::string& order_id          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<ActionStats, 1>::Diffs    
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
    ///////////////////////////////// ActionStats    
    inline    
    ActionStats::ActionStats ()    
      :Base("ActionStats")    
    {}    
    inline    
    ActionStats::ActionStats (const ActionStats::Key& value)    
      :Base("ActionStats")    
    {    
      key_ = value;      
    }    
    inline    
    ActionStats::ActionStats (    
      const int& action_request_id,      
      const int& action_id,      
      const int& cc_id,      
      const int& tag_id,      
      const int& colo_id,      
      const std::string& country_code,      
      const std::string& action_referrer_url,      
      const AutoTest::Time& action_date,      
      const AutoTest::Time& imp_date,      
      const AutoTest::Time& click_date,      
      const double& cur_value,      
      const std::string& order_id      
    )    
      :Base("ActionStats")    
    {    
      key_ = Key (      
        action_request_id,        
        action_id,        
        cc_id,        
        tag_id,        
        colo_id,        
        country_code,        
        action_referrer_url,        
        action_date,        
        imp_date,        
        click_date,        
        cur_value,        
        order_id        
      );      
    }    
    inline    
    ActionStats::Key::Key ()    
      :action_request_id_(0),action_id_(0),cc_id_(0),tag_id_(0),colo_id_(0),action_date_(default_date()),imp_date_(default_date()),click_date_(default_date()),cur_value_(0)    
    {    
      action_request_id_used_ = false;      
      action_request_id_null_ = false;      
      action_id_used_ = false;      
      action_id_null_ = false;      
      cc_id_used_ = false;      
      cc_id_null_ = false;      
      tag_id_used_ = false;      
      tag_id_null_ = false;      
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      country_code_used_ = false;      
      country_code_null_ = false;      
      action_referrer_url_used_ = false;      
      action_referrer_url_null_ = false;      
      action_date_used_ = false;      
      action_date_null_ = false;      
      imp_date_used_ = false;      
      imp_date_null_ = false;      
      click_date_used_ = false;      
      click_date_null_ = false;      
      cur_value_used_ = false;      
      cur_value_null_ = false;      
      order_id_used_ = false;      
      order_id_null_ = false;      
    }    
    inline    
    ActionStats::Key::Key (    
      const int& action_request_id,      
      const int& action_id,      
      const int& cc_id,      
      const int& tag_id,      
      const int& colo_id,      
      const std::string& country_code,      
      const std::string& action_referrer_url,      
      const AutoTest::Time& action_date,      
      const AutoTest::Time& imp_date,      
      const AutoTest::Time& click_date,      
      const double& cur_value,      
      const std::string& order_id      
    )    
      :action_request_id_(action_request_id),action_id_(action_id),cc_id_(cc_id),tag_id_(tag_id),colo_id_(colo_id),action_date_(action_date),imp_date_(imp_date),click_date_(click_date),cur_value_(cur_value)    
    {    
      action_request_id_used_ = true;      
      action_request_id_null_ = false;      
      action_id_used_ = true;      
      action_id_null_ = false;      
      cc_id_used_ = true;      
      cc_id_null_ = false;      
      tag_id_used_ = true;      
      tag_id_null_ = false;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      country_code_ = country_code;      
      country_code_used_ = true;      
      country_code_null_ = false;      
      action_referrer_url_ = action_referrer_url;      
      action_referrer_url_used_ = true;      
      action_referrer_url_null_ = false;      
      action_date_used_ = true;      
      action_date_null_ = false;      
      imp_date_used_ = true;      
      imp_date_null_ = false;      
      click_date_used_ = true;      
      click_date_null_ = false;      
      cur_value_used_ = true;      
      cur_value_null_ = false;      
      order_id_ = order_id;      
      order_id_used_ = true;      
      order_id_null_ = false;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::action_request_id(const int& value)    
    {    
      action_request_id_ = value;      
      action_request_id_used_ = true;      
      action_request_id_null_ = false;      
      return *this;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::action_request_id_set_null(bool is_null)    
    {    
      action_request_id_used_ = true;      
      action_request_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ActionStats::Key::action_request_id() const    
    {    
      return action_request_id_;      
    }    
    inline    
    bool    
    ActionStats::Key::action_request_id_used() const    
    {    
      return action_request_id_used_;      
    }    
    inline    
    bool    
    ActionStats::Key::action_request_id_is_null() const    
    {    
      return action_request_id_null_;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::action_id(const int& value)    
    {    
      action_id_ = value;      
      action_id_used_ = true;      
      action_id_null_ = false;      
      return *this;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::action_id_set_null(bool is_null)    
    {    
      action_id_used_ = true;      
      action_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ActionStats::Key::action_id() const    
    {    
      return action_id_;      
    }    
    inline    
    bool    
    ActionStats::Key::action_id_used() const    
    {    
      return action_id_used_;      
    }    
    inline    
    bool    
    ActionStats::Key::action_id_is_null() const    
    {    
      return action_id_null_;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::cc_id(const int& value)    
    {    
      cc_id_ = value;      
      cc_id_used_ = true;      
      cc_id_null_ = false;      
      return *this;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::cc_id_set_null(bool is_null)    
    {    
      cc_id_used_ = true;      
      cc_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ActionStats::Key::cc_id() const    
    {    
      return cc_id_;      
    }    
    inline    
    bool    
    ActionStats::Key::cc_id_used() const    
    {    
      return cc_id_used_;      
    }    
    inline    
    bool    
    ActionStats::Key::cc_id_is_null() const    
    {    
      return cc_id_null_;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::tag_id(const int& value)    
    {    
      tag_id_ = value;      
      tag_id_used_ = true;      
      tag_id_null_ = false;      
      return *this;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::tag_id_set_null(bool is_null)    
    {    
      tag_id_used_ = true;      
      tag_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ActionStats::Key::tag_id() const    
    {    
      return tag_id_;      
    }    
    inline    
    bool    
    ActionStats::Key::tag_id_used() const    
    {    
      return tag_id_used_;      
    }    
    inline    
    bool    
    ActionStats::Key::tag_id_is_null() const    
    {    
      return tag_id_null_;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ActionStats::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    ActionStats::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    ActionStats::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::country_code(const std::string& value)    
    {    
      country_code_ = value;      
      country_code_used_ = true;      
      country_code_null_ = false;      
      return *this;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::country_code_set_null(bool is_null)    
    {    
      country_code_used_ = true;      
      country_code_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    ActionStats::Key::country_code() const    
    {    
      return country_code_;      
    }    
    inline    
    bool    
    ActionStats::Key::country_code_used() const    
    {    
      return country_code_used_;      
    }    
    inline    
    bool    
    ActionStats::Key::country_code_is_null() const    
    {    
      return country_code_null_;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::action_referrer_url(const std::string& value)    
    {    
      action_referrer_url_ = value;      
      action_referrer_url_used_ = true;      
      action_referrer_url_null_ = false;      
      return *this;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::action_referrer_url_set_null(bool is_null)    
    {    
      action_referrer_url_used_ = true;      
      action_referrer_url_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    ActionStats::Key::action_referrer_url() const    
    {    
      return action_referrer_url_;      
    }    
    inline    
    bool    
    ActionStats::Key::action_referrer_url_used() const    
    {    
      return action_referrer_url_used_;      
    }    
    inline    
    bool    
    ActionStats::Key::action_referrer_url_is_null() const    
    {    
      return action_referrer_url_null_;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::action_date(const AutoTest::Time& value)    
    {    
      action_date_ = value;      
      action_date_used_ = true;      
      action_date_null_ = false;      
      return *this;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::action_date_set_null(bool is_null)    
    {    
      action_date_used_ = true;      
      action_date_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    ActionStats::Key::action_date() const    
    {    
      return action_date_;      
    }    
    inline    
    bool    
    ActionStats::Key::action_date_used() const    
    {    
      return action_date_used_;      
    }    
    inline    
    bool    
    ActionStats::Key::action_date_is_null() const    
    {    
      return action_date_null_;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::imp_date(const AutoTest::Time& value)    
    {    
      imp_date_ = value;      
      imp_date_used_ = true;      
      imp_date_null_ = false;      
      return *this;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::imp_date_set_null(bool is_null)    
    {    
      imp_date_used_ = true;      
      imp_date_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    ActionStats::Key::imp_date() const    
    {    
      return imp_date_;      
    }    
    inline    
    bool    
    ActionStats::Key::imp_date_used() const    
    {    
      return imp_date_used_;      
    }    
    inline    
    bool    
    ActionStats::Key::imp_date_is_null() const    
    {    
      return imp_date_null_;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::click_date(const AutoTest::Time& value)    
    {    
      click_date_ = value;      
      click_date_used_ = true;      
      click_date_null_ = false;      
      return *this;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::click_date_set_null(bool is_null)    
    {    
      click_date_used_ = true;      
      click_date_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    ActionStats::Key::click_date() const    
    {    
      return click_date_;      
    }    
    inline    
    bool    
    ActionStats::Key::click_date_used() const    
    {    
      return click_date_used_;      
    }    
    inline    
    bool    
    ActionStats::Key::click_date_is_null() const    
    {    
      return click_date_null_;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::cur_value(const double& value)    
    {    
      cur_value_ = value;      
      cur_value_used_ = true;      
      cur_value_null_ = false;      
      return *this;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::cur_value_set_null(bool is_null)    
    {    
      cur_value_used_ = true;      
      cur_value_null_ = is_null;      
      return *this;      
    }    
    inline    
    const double&    
    ActionStats::Key::cur_value() const    
    {    
      return cur_value_;      
    }    
    inline    
    bool    
    ActionStats::Key::cur_value_used() const    
    {    
      return cur_value_used_;      
    }    
    inline    
    bool    
    ActionStats::Key::cur_value_is_null() const    
    {    
      return cur_value_null_;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::order_id(const std::string& value)    
    {    
      order_id_ = value;      
      order_id_used_ = true;      
      order_id_null_ = false;      
      return *this;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::Key::order_id_set_null(bool is_null)    
    {    
      order_id_used_ = true;      
      order_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    ActionStats::Key::order_id() const    
    {    
      return order_id_;      
    }    
    inline    
    bool    
    ActionStats::Key::order_id_used() const    
    {    
      return order_id_used_;      
    }    
    inline    
    bool    
    ActionStats::Key::order_id_is_null() const    
    {    
      return order_id_null_;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::key (    
      const int& action_request_id,      
      const int& action_id,      
      const int& cc_id,      
      const int& tag_id,      
      const int& colo_id,      
      const std::string& country_code,      
      const std::string& action_referrer_url,      
      const AutoTest::Time& action_date,      
      const AutoTest::Time& imp_date,      
      const AutoTest::Time& click_date,      
      const double& cur_value,      
      const std::string& order_id      
    )    
    {    
      key_ = Key (      
        action_request_id,        
        action_id,        
        cc_id,        
        tag_id,        
        colo_id,        
        country_code,        
        action_referrer_url,        
        action_date,        
        imp_date,        
        click_date,        
        cur_value,        
        order_id        
      );      
      return key_;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::key ()    
    {    
      return key_;      
    }    
    inline    
    ActionStats::Key&    
    ActionStats::key (const ActionStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<ActionStats, 1>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<ActionStats, 1>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<ActionStats, 1>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<ActionStats, 1>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ActionStats, 1>::Diffs&     
    DiffStats<ActionStats, 1>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ActionStats, 1>::Diffs&     
    DiffStats<ActionStats, 1>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ActionStats, 1>::Diffs&     
    DiffStats<ActionStats, 1>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ActionStats, 1>::Diffs     
    DiffStats<ActionStats, 1>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<ActionStats, 1>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<ActionStats, 1>::Diffs     
    DiffStats<ActionStats, 1>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<ActionStats, 1>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<ActionStats, 1>::Diffs::const_iterator    
    DiffStats<ActionStats, 1>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ActionStats, 1>::Diffs::const_iterator    
    DiffStats<ActionStats, 1>::Diffs::end () const    
    {    
      return diffs + 1;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<ActionStats, 1>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<ActionStats, 1>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<ActionStats, 1>::Diffs::size () const    
    {    
      return 1;      
    }    
    inline    
    void    
    DiffStats<ActionStats, 1>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 1; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    ActionStats::count () const    
    {    
      return values[ActionStats::COUNT];      
    }    
    inline    
    DiffStats<ActionStats, 1>::Diffs&     
    DiffStats<ActionStats, 1>::Diffs::count (const stats_diff_type& value)    
    {    
      diffs[ActionStats::COUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ActionStats, 1>::Diffs::count () const    
    {    
      return diffs[ActionStats::COUNT];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_ACTIONSTATS_HPP

