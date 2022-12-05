
#ifndef __AUTOTESTS_COMMONS_STATS_ACTIONREQUESTS_HPP
#define __AUTOTESTS_COMMONS_STATS_ACTIONREQUESTS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class ActionRequests:    
      public DiffStats<ActionRequests, 2>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          COUNT = 0,          
          CUR_VALUE          
        };        
        typedef DiffStats<ActionRequests, 2> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            int action_id_;            
            bool action_id_used_;            
            bool action_id_null_;            
            int colo_id_;            
            bool colo_id_used_;            
            bool colo_id_null_;            
            std::string country_code_;            
            bool country_code_used_;            
            bool country_code_null_;            
            AutoTest::Time action_date_;            
            bool action_date_used_;            
            bool action_date_null_;            
            std::string action_referrer_url_;            
            bool action_referrer_url_used_;            
            bool action_referrer_url_null_;            
            std::string user_status_;            
            bool user_status_used_;            
            bool user_status_null_;            
          public:          
            Key& action_id(const int& value);            
            Key& action_id_set_null(bool is_null = true);            
            const int& action_id() const;            
            bool action_id_used() const;            
            bool action_id_is_null() const;            
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
            Key& action_date(const AutoTest::Time& value);            
            Key& action_date_set_null(bool is_null = true);            
            const AutoTest::Time& action_date() const;            
            bool action_date_used() const;            
            bool action_date_is_null() const;            
            Key& action_referrer_url(const std::string& value);            
            Key& action_referrer_url_set_null(bool is_null = true);            
            const std::string& action_referrer_url() const;            
            bool action_referrer_url_used() const;            
            bool action_referrer_url_is_null() const;            
            Key& user_status(const std::string& value);            
            Key& user_status_set_null(bool is_null = true);            
            const std::string& user_status() const;            
            bool user_status_used() const;            
            bool user_status_is_null() const;            
            Key ();            
            Key (            
              const int& action_id,              
              const int& colo_id,              
              const std::string& country_code,              
              const AutoTest::Time& action_date,              
              const std::string& action_referrer_url,              
              const std::string& user_status              
            );            
        };        
        stats_value_type count () const;        
        stats_value_type cur_value () const;        
        void print_idname (std::ostream& out) const;        
                
        ActionRequests (const Key& value);        
        ActionRequests (        
          const int& action_id,          
          const int& colo_id,          
          const std::string& country_code,          
          const AutoTest::Time& action_date,          
          const std::string& action_referrer_url,          
          const std::string& user_status          
        );        
        ActionRequests ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& action_id,          
          const int& colo_id,          
          const std::string& country_code,          
          const AutoTest::Time& action_date,          
          const std::string& action_referrer_url,          
          const std::string& user_status          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<ActionRequests, 2>::Diffs    
    {    
      public:      
        typedef stats_diff_type array_type[2];        
        typedef const stats_diff_type const_array_type[2];        
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
        Diffs& cur_value (const stats_diff_type& value);        
        const stats_diff_type& cur_value () const;        
      protected:      
        stats_diff_type diffs[2];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ActionRequests    
    inline    
    ActionRequests::ActionRequests ()    
      :Base("ActionRequests")    
    {}    
    inline    
    ActionRequests::ActionRequests (const ActionRequests::Key& value)    
      :Base("ActionRequests")    
    {    
      key_ = value;      
    }    
    inline    
    ActionRequests::ActionRequests (    
      const int& action_id,      
      const int& colo_id,      
      const std::string& country_code,      
      const AutoTest::Time& action_date,      
      const std::string& action_referrer_url,      
      const std::string& user_status      
    )    
      :Base("ActionRequests")    
    {    
      key_ = Key (      
        action_id,        
        colo_id,        
        country_code,        
        action_date,        
        action_referrer_url,        
        user_status        
      );      
    }    
    inline    
    ActionRequests::Key::Key ()    
      :action_id_(0),colo_id_(0),action_date_(default_date())    
    {    
      action_id_used_ = false;      
      action_id_null_ = false;      
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      country_code_used_ = false;      
      country_code_null_ = false;      
      action_date_used_ = false;      
      action_date_null_ = false;      
      action_referrer_url_used_ = false;      
      action_referrer_url_null_ = false;      
      user_status_used_ = false;      
      user_status_null_ = false;      
    }    
    inline    
    ActionRequests::Key::Key (    
      const int& action_id,      
      const int& colo_id,      
      const std::string& country_code,      
      const AutoTest::Time& action_date,      
      const std::string& action_referrer_url,      
      const std::string& user_status      
    )    
      :action_id_(action_id),colo_id_(colo_id),action_date_(action_date)    
    {    
      action_id_used_ = true;      
      action_id_null_ = false;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      country_code_ = country_code;      
      country_code_used_ = true;      
      country_code_null_ = false;      
      action_date_used_ = true;      
      action_date_null_ = false;      
      action_referrer_url_ = action_referrer_url;      
      action_referrer_url_used_ = true;      
      action_referrer_url_null_ = false;      
      user_status_ = user_status;      
      user_status_used_ = true;      
      user_status_null_ = false;      
    }    
    inline    
    ActionRequests::Key&    
    ActionRequests::Key::action_id(const int& value)    
    {    
      action_id_ = value;      
      action_id_used_ = true;      
      action_id_null_ = false;      
      return *this;      
    }    
    inline    
    ActionRequests::Key&    
    ActionRequests::Key::action_id_set_null(bool is_null)    
    {    
      action_id_used_ = true;      
      action_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ActionRequests::Key::action_id() const    
    {    
      return action_id_;      
    }    
    inline    
    bool    
    ActionRequests::Key::action_id_used() const    
    {    
      return action_id_used_;      
    }    
    inline    
    bool    
    ActionRequests::Key::action_id_is_null() const    
    {    
      return action_id_null_;      
    }    
    inline    
    ActionRequests::Key&    
    ActionRequests::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    ActionRequests::Key&    
    ActionRequests::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ActionRequests::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    ActionRequests::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    ActionRequests::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    ActionRequests::Key&    
    ActionRequests::Key::country_code(const std::string& value)    
    {    
      country_code_ = value;      
      country_code_used_ = true;      
      country_code_null_ = false;      
      return *this;      
    }    
    inline    
    ActionRequests::Key&    
    ActionRequests::Key::country_code_set_null(bool is_null)    
    {    
      country_code_used_ = true;      
      country_code_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    ActionRequests::Key::country_code() const    
    {    
      return country_code_;      
    }    
    inline    
    bool    
    ActionRequests::Key::country_code_used() const    
    {    
      return country_code_used_;      
    }    
    inline    
    bool    
    ActionRequests::Key::country_code_is_null() const    
    {    
      return country_code_null_;      
    }    
    inline    
    ActionRequests::Key&    
    ActionRequests::Key::action_date(const AutoTest::Time& value)    
    {    
      action_date_ = value;      
      action_date_used_ = true;      
      action_date_null_ = false;      
      return *this;      
    }    
    inline    
    ActionRequests::Key&    
    ActionRequests::Key::action_date_set_null(bool is_null)    
    {    
      action_date_used_ = true;      
      action_date_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    ActionRequests::Key::action_date() const    
    {    
      return action_date_;      
    }    
    inline    
    bool    
    ActionRequests::Key::action_date_used() const    
    {    
      return action_date_used_;      
    }    
    inline    
    bool    
    ActionRequests::Key::action_date_is_null() const    
    {    
      return action_date_null_;      
    }    
    inline    
    ActionRequests::Key&    
    ActionRequests::Key::action_referrer_url(const std::string& value)    
    {    
      action_referrer_url_ = value;      
      action_referrer_url_used_ = true;      
      action_referrer_url_null_ = false;      
      return *this;      
    }    
    inline    
    ActionRequests::Key&    
    ActionRequests::Key::action_referrer_url_set_null(bool is_null)    
    {    
      action_referrer_url_used_ = true;      
      action_referrer_url_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    ActionRequests::Key::action_referrer_url() const    
    {    
      return action_referrer_url_;      
    }    
    inline    
    bool    
    ActionRequests::Key::action_referrer_url_used() const    
    {    
      return action_referrer_url_used_;      
    }    
    inline    
    bool    
    ActionRequests::Key::action_referrer_url_is_null() const    
    {    
      return action_referrer_url_null_;      
    }    
    inline    
    ActionRequests::Key&    
    ActionRequests::Key::user_status(const std::string& value)    
    {    
      user_status_ = value;      
      user_status_used_ = true;      
      user_status_null_ = false;      
      return *this;      
    }    
    inline    
    ActionRequests::Key&    
    ActionRequests::Key::user_status_set_null(bool is_null)    
    {    
      user_status_used_ = true;      
      user_status_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    ActionRequests::Key::user_status() const    
    {    
      return user_status_;      
    }    
    inline    
    bool    
    ActionRequests::Key::user_status_used() const    
    {    
      return user_status_used_;      
    }    
    inline    
    bool    
    ActionRequests::Key::user_status_is_null() const    
    {    
      return user_status_null_;      
    }    
    inline    
    ActionRequests::Key&    
    ActionRequests::key (    
      const int& action_id,      
      const int& colo_id,      
      const std::string& country_code,      
      const AutoTest::Time& action_date,      
      const std::string& action_referrer_url,      
      const std::string& user_status      
    )    
    {    
      key_ = Key (      
        action_id,        
        colo_id,        
        country_code,        
        action_date,        
        action_referrer_url,        
        user_status        
      );      
      return key_;      
    }    
    inline    
    ActionRequests::Key&    
    ActionRequests::key ()    
    {    
      return key_;      
    }    
    inline    
    ActionRequests::Key&    
    ActionRequests::key (const ActionRequests::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<ActionRequests, 2>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<ActionRequests, 2>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<ActionRequests, 2>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<ActionRequests, 2>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ActionRequests, 2>::Diffs&     
    DiffStats<ActionRequests, 2>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ActionRequests, 2>::Diffs&     
    DiffStats<ActionRequests, 2>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ActionRequests, 2>::Diffs&     
    DiffStats<ActionRequests, 2>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ActionRequests, 2>::Diffs     
    DiffStats<ActionRequests, 2>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<ActionRequests, 2>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<ActionRequests, 2>::Diffs     
    DiffStats<ActionRequests, 2>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<ActionRequests, 2>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<ActionRequests, 2>::Diffs::const_iterator    
    DiffStats<ActionRequests, 2>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ActionRequests, 2>::Diffs::const_iterator    
    DiffStats<ActionRequests, 2>::Diffs::end () const    
    {    
      return diffs + 2;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<ActionRequests, 2>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<ActionRequests, 2>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<ActionRequests, 2>::Diffs::size () const    
    {    
      return 2;      
    }    
    inline    
    void    
    DiffStats<ActionRequests, 2>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 2; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    ActionRequests::count () const    
    {    
      return values[ActionRequests::COUNT];      
    }    
    inline    
    DiffStats<ActionRequests, 2>::Diffs&     
    DiffStats<ActionRequests, 2>::Diffs::count (const stats_diff_type& value)    
    {    
      diffs[ActionRequests::COUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ActionRequests, 2>::Diffs::count () const    
    {    
      return diffs[ActionRequests::COUNT];      
    }    
    inline    
    stats_value_type    
    ActionRequests::cur_value () const    
    {    
      return values[ActionRequests::CUR_VALUE];      
    }    
    inline    
    DiffStats<ActionRequests, 2>::Diffs&     
    DiffStats<ActionRequests, 2>::Diffs::cur_value (const stats_diff_type& value)    
    {    
      diffs[ActionRequests::CUR_VALUE] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ActionRequests, 2>::Diffs::cur_value () const    
    {    
      return diffs[ActionRequests::CUR_VALUE];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_ACTIONREQUESTS_HPP

