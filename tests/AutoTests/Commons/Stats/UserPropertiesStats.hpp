
#ifndef __AUTOTESTS_COMMONS_STATS_USERPROPERTIESSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_USERPROPERTIESSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class UserPropertiesStats:    
      public DiffStats<UserPropertiesStats, 6>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          IMPS = 0,          
          CLICKS,          
          ACTIONS,          
          REQUESTS,          
          IMPS_UNVERIFIED,          
          PROFILING_REQUESTS          
        };        
        typedef DiffStats<UserPropertiesStats, 6> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            std::string name_;            
            bool name_used_;            
            bool name_null_;            
            std::string value_;            
            bool value_used_;            
            bool value_null_;            
            std::string user_status_;            
            bool user_status_used_;            
            bool user_status_null_;            
            int colo_id_;            
            bool colo_id_used_;            
            bool colo_id_null_;            
            AutoTest::Time stimestamp_;            
            bool stimestamp_used_;            
            bool stimestamp_null_;            
            AutoTest::Time sdate_;            
            bool sdate_used_;            
            bool sdate_null_;            
            int hour_;            
            bool hour_used_;            
            bool hour_null_;            
          public:          
            Key& name(const std::string& value);            
            Key& name_set_null(bool is_null = true);            
            const std::string& name() const;            
            bool name_used() const;            
            bool name_is_null() const;            
            Key& value(const std::string& value);            
            Key& value_set_null(bool is_null = true);            
            const std::string& value() const;            
            bool value_used() const;            
            bool value_is_null() const;            
            Key& user_status(const std::string& value);            
            Key& user_status_set_null(bool is_null = true);            
            const std::string& user_status() const;            
            bool user_status_used() const;            
            bool user_status_is_null() const;            
            Key& colo_id(const int& value);            
            Key& colo_id_set_null(bool is_null = true);            
            const int& colo_id() const;            
            bool colo_id_used() const;            
            bool colo_id_is_null() const;            
            Key& stimestamp(const AutoTest::Time& value);            
            Key& stimestamp_set_null(bool is_null = true);            
            const AutoTest::Time& stimestamp() const;            
            bool stimestamp_used() const;            
            bool stimestamp_is_null() const;            
            Key& sdate(const AutoTest::Time& value);            
            Key& sdate_set_null(bool is_null = true);            
            const AutoTest::Time& sdate() const;            
            bool sdate_used() const;            
            bool sdate_is_null() const;            
            Key& hour(const int& value);            
            Key& hour_set_null(bool is_null = true);            
            const int& hour() const;            
            bool hour_used() const;            
            bool hour_is_null() const;            
            Key ();            
            Key (            
              const std::string& name,              
              const std::string& value,              
              const std::string& user_status,              
              const int& colo_id,              
              const AutoTest::Time& stimestamp,              
              const AutoTest::Time& sdate,              
              const int& hour              
            );            
        };        
        stats_value_type imps () const;        
        stats_value_type clicks () const;        
        stats_value_type actions () const;        
        stats_value_type requests () const;        
        stats_value_type imps_unverified () const;        
        stats_value_type profiling_requests () const;        
        void print_idname (std::ostream& out) const;        
                
        UserPropertiesStats (const Key& value);        
        UserPropertiesStats (        
          const std::string& name,          
          const std::string& value,          
          const std::string& user_status,          
          const int& colo_id,          
          const AutoTest::Time& stimestamp,          
          const AutoTest::Time& sdate,          
          const int& hour          
        );        
        UserPropertiesStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const std::string& name,          
          const std::string& value,          
          const std::string& user_status,          
          const int& colo_id,          
          const AutoTest::Time& stimestamp,          
          const AutoTest::Time& sdate,          
          const int& hour          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<UserPropertiesStats, 6>::Diffs    
    {    
      public:      
        typedef stats_diff_type array_type[6];        
        typedef const stats_diff_type const_array_type[6];        
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
                
        Diffs& imps (const stats_diff_type& value);        
        const stats_diff_type& imps () const;        
        Diffs& clicks (const stats_diff_type& value);        
        const stats_diff_type& clicks () const;        
        Diffs& actions (const stats_diff_type& value);        
        const stats_diff_type& actions () const;        
        Diffs& requests (const stats_diff_type& value);        
        const stats_diff_type& requests () const;        
        Diffs& imps_unverified (const stats_diff_type& value);        
        const stats_diff_type& imps_unverified () const;        
        Diffs& profiling_requests (const stats_diff_type& value);        
        const stats_diff_type& profiling_requests () const;        
      protected:      
        stats_diff_type diffs[6];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// UserPropertiesStats    
    inline    
    UserPropertiesStats::UserPropertiesStats ()    
      :Base("UserPropertiesStats")    
    {}    
    inline    
    UserPropertiesStats::UserPropertiesStats (const UserPropertiesStats::Key& value)    
      :Base("UserPropertiesStats")    
    {    
      key_ = value;      
    }    
    inline    
    UserPropertiesStats::UserPropertiesStats (    
      const std::string& name,      
      const std::string& value,      
      const std::string& user_status,      
      const int& colo_id,      
      const AutoTest::Time& stimestamp,      
      const AutoTest::Time& sdate,      
      const int& hour      
    )    
      :Base("UserPropertiesStats")    
    {    
      key_ = Key (      
        name,        
        value,        
        user_status,        
        colo_id,        
        stimestamp,        
        sdate,        
        hour        
      );      
    }    
    inline    
    UserPropertiesStats::Key::Key ()    
      :colo_id_(0),stimestamp_(default_date()),sdate_(default_date()),hour_(0)    
    {    
      name_used_ = false;      
      name_null_ = false;      
      value_used_ = false;      
      value_null_ = false;      
      user_status_used_ = false;      
      user_status_null_ = false;      
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      stimestamp_used_ = false;      
      stimestamp_null_ = false;      
      sdate_used_ = false;      
      sdate_null_ = false;      
      hour_used_ = false;      
      hour_null_ = false;      
    }    
    inline    
    UserPropertiesStats::Key::Key (    
      const std::string& name,      
      const std::string& value,      
      const std::string& user_status,      
      const int& colo_id,      
      const AutoTest::Time& stimestamp,      
      const AutoTest::Time& sdate,      
      const int& hour      
    )    
      :colo_id_(colo_id),stimestamp_(stimestamp),sdate_(sdate),hour_(hour)    
    {    
      name_ = name;      
      name_used_ = true;      
      name_null_ = false;      
      value_ = value;      
      value_used_ = true;      
      value_null_ = false;      
      user_status_ = user_status;      
      user_status_used_ = true;      
      user_status_null_ = false;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      stimestamp_used_ = true;      
      stimestamp_null_ = false;      
      sdate_used_ = true;      
      sdate_null_ = false;      
      hour_used_ = true;      
      hour_null_ = false;      
    }    
    inline    
    UserPropertiesStats::Key&    
    UserPropertiesStats::Key::name(const std::string& value)    
    {    
      name_ = value;      
      name_used_ = true;      
      name_null_ = false;      
      return *this;      
    }    
    inline    
    UserPropertiesStats::Key&    
    UserPropertiesStats::Key::name_set_null(bool is_null)    
    {    
      name_used_ = true;      
      name_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    UserPropertiesStats::Key::name() const    
    {    
      return name_;      
    }    
    inline    
    bool    
    UserPropertiesStats::Key::name_used() const    
    {    
      return name_used_;      
    }    
    inline    
    bool    
    UserPropertiesStats::Key::name_is_null() const    
    {    
      return name_null_;      
    }    
    inline    
    UserPropertiesStats::Key&    
    UserPropertiesStats::Key::value(const std::string& value)    
    {    
      value_ = value;      
      value_used_ = true;      
      value_null_ = false;      
      return *this;      
    }    
    inline    
    UserPropertiesStats::Key&    
    UserPropertiesStats::Key::value_set_null(bool is_null)    
    {    
      value_used_ = true;      
      value_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    UserPropertiesStats::Key::value() const    
    {    
      return value_;      
    }    
    inline    
    bool    
    UserPropertiesStats::Key::value_used() const    
    {    
      return value_used_;      
    }    
    inline    
    bool    
    UserPropertiesStats::Key::value_is_null() const    
    {    
      return value_null_;      
    }    
    inline    
    UserPropertiesStats::Key&    
    UserPropertiesStats::Key::user_status(const std::string& value)    
    {    
      user_status_ = value;      
      user_status_used_ = true;      
      user_status_null_ = false;      
      return *this;      
    }    
    inline    
    UserPropertiesStats::Key&    
    UserPropertiesStats::Key::user_status_set_null(bool is_null)    
    {    
      user_status_used_ = true;      
      user_status_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    UserPropertiesStats::Key::user_status() const    
    {    
      return user_status_;      
    }    
    inline    
    bool    
    UserPropertiesStats::Key::user_status_used() const    
    {    
      return user_status_used_;      
    }    
    inline    
    bool    
    UserPropertiesStats::Key::user_status_is_null() const    
    {    
      return user_status_null_;      
    }    
    inline    
    UserPropertiesStats::Key&    
    UserPropertiesStats::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    UserPropertiesStats::Key&    
    UserPropertiesStats::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    UserPropertiesStats::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    UserPropertiesStats::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    UserPropertiesStats::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    UserPropertiesStats::Key&    
    UserPropertiesStats::Key::stimestamp(const AutoTest::Time& value)    
    {    
      stimestamp_ = value;      
      stimestamp_used_ = true;      
      stimestamp_null_ = false;      
      return *this;      
    }    
    inline    
    UserPropertiesStats::Key&    
    UserPropertiesStats::Key::stimestamp_set_null(bool is_null)    
    {    
      stimestamp_used_ = true;      
      stimestamp_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    UserPropertiesStats::Key::stimestamp() const    
    {    
      return stimestamp_;      
    }    
    inline    
    bool    
    UserPropertiesStats::Key::stimestamp_used() const    
    {    
      return stimestamp_used_;      
    }    
    inline    
    bool    
    UserPropertiesStats::Key::stimestamp_is_null() const    
    {    
      return stimestamp_null_;      
    }    
    inline    
    UserPropertiesStats::Key&    
    UserPropertiesStats::Key::sdate(const AutoTest::Time& value)    
    {    
      sdate_ = value;      
      sdate_used_ = true;      
      sdate_null_ = false;      
      return *this;      
    }    
    inline    
    UserPropertiesStats::Key&    
    UserPropertiesStats::Key::sdate_set_null(bool is_null)    
    {    
      sdate_used_ = true;      
      sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    UserPropertiesStats::Key::sdate() const    
    {    
      return sdate_;      
    }    
    inline    
    bool    
    UserPropertiesStats::Key::sdate_used() const    
    {    
      return sdate_used_;      
    }    
    inline    
    bool    
    UserPropertiesStats::Key::sdate_is_null() const    
    {    
      return sdate_null_;      
    }    
    inline    
    UserPropertiesStats::Key&    
    UserPropertiesStats::Key::hour(const int& value)    
    {    
      hour_ = value;      
      hour_used_ = true;      
      hour_null_ = false;      
      return *this;      
    }    
    inline    
    UserPropertiesStats::Key&    
    UserPropertiesStats::Key::hour_set_null(bool is_null)    
    {    
      hour_used_ = true;      
      hour_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    UserPropertiesStats::Key::hour() const    
    {    
      return hour_;      
    }    
    inline    
    bool    
    UserPropertiesStats::Key::hour_used() const    
    {    
      return hour_used_;      
    }    
    inline    
    bool    
    UserPropertiesStats::Key::hour_is_null() const    
    {    
      return hour_null_;      
    }    
    inline    
    UserPropertiesStats::Key&    
    UserPropertiesStats::key (    
      const std::string& name,      
      const std::string& value,      
      const std::string& user_status,      
      const int& colo_id,      
      const AutoTest::Time& stimestamp,      
      const AutoTest::Time& sdate,      
      const int& hour      
    )    
    {    
      key_ = Key (      
        name,        
        value,        
        user_status,        
        colo_id,        
        stimestamp,        
        sdate,        
        hour        
      );      
      return key_;      
    }    
    inline    
    UserPropertiesStats::Key&    
    UserPropertiesStats::key ()    
    {    
      return key_;      
    }    
    inline    
    UserPropertiesStats::Key&    
    UserPropertiesStats::key (const UserPropertiesStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<UserPropertiesStats, 6>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 6; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<UserPropertiesStats, 6>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 6; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<UserPropertiesStats, 6>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 6; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<UserPropertiesStats, 6>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<UserPropertiesStats, 6>::Diffs&     
    DiffStats<UserPropertiesStats, 6>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 6; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<UserPropertiesStats, 6>::Diffs&     
    DiffStats<UserPropertiesStats, 6>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 6; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<UserPropertiesStats, 6>::Diffs&     
    DiffStats<UserPropertiesStats, 6>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 6; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<UserPropertiesStats, 6>::Diffs     
    DiffStats<UserPropertiesStats, 6>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<UserPropertiesStats, 6>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<UserPropertiesStats, 6>::Diffs     
    DiffStats<UserPropertiesStats, 6>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<UserPropertiesStats, 6>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<UserPropertiesStats, 6>::Diffs::const_iterator    
    DiffStats<UserPropertiesStats, 6>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<UserPropertiesStats, 6>::Diffs::const_iterator    
    DiffStats<UserPropertiesStats, 6>::Diffs::end () const    
    {    
      return diffs + 6;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<UserPropertiesStats, 6>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<UserPropertiesStats, 6>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<UserPropertiesStats, 6>::Diffs::size () const    
    {    
      return 6;      
    }    
    inline    
    void    
    DiffStats<UserPropertiesStats, 6>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 6; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    UserPropertiesStats::imps () const    
    {    
      return values[UserPropertiesStats::IMPS];      
    }    
    inline    
    DiffStats<UserPropertiesStats, 6>::Diffs&     
    DiffStats<UserPropertiesStats, 6>::Diffs::imps (const stats_diff_type& value)    
    {    
      diffs[UserPropertiesStats::IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<UserPropertiesStats, 6>::Diffs::imps () const    
    {    
      return diffs[UserPropertiesStats::IMPS];      
    }    
    inline    
    stats_value_type    
    UserPropertiesStats::clicks () const    
    {    
      return values[UserPropertiesStats::CLICKS];      
    }    
    inline    
    DiffStats<UserPropertiesStats, 6>::Diffs&     
    DiffStats<UserPropertiesStats, 6>::Diffs::clicks (const stats_diff_type& value)    
    {    
      diffs[UserPropertiesStats::CLICKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<UserPropertiesStats, 6>::Diffs::clicks () const    
    {    
      return diffs[UserPropertiesStats::CLICKS];      
    }    
    inline    
    stats_value_type    
    UserPropertiesStats::actions () const    
    {    
      return values[UserPropertiesStats::ACTIONS];      
    }    
    inline    
    DiffStats<UserPropertiesStats, 6>::Diffs&     
    DiffStats<UserPropertiesStats, 6>::Diffs::actions (const stats_diff_type& value)    
    {    
      diffs[UserPropertiesStats::ACTIONS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<UserPropertiesStats, 6>::Diffs::actions () const    
    {    
      return diffs[UserPropertiesStats::ACTIONS];      
    }    
    inline    
    stats_value_type    
    UserPropertiesStats::requests () const    
    {    
      return values[UserPropertiesStats::REQUESTS];      
    }    
    inline    
    DiffStats<UserPropertiesStats, 6>::Diffs&     
    DiffStats<UserPropertiesStats, 6>::Diffs::requests (const stats_diff_type& value)    
    {    
      diffs[UserPropertiesStats::REQUESTS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<UserPropertiesStats, 6>::Diffs::requests () const    
    {    
      return diffs[UserPropertiesStats::REQUESTS];      
    }    
    inline    
    stats_value_type    
    UserPropertiesStats::imps_unverified () const    
    {    
      return values[UserPropertiesStats::IMPS_UNVERIFIED];      
    }    
    inline    
    DiffStats<UserPropertiesStats, 6>::Diffs&     
    DiffStats<UserPropertiesStats, 6>::Diffs::imps_unverified (const stats_diff_type& value)    
    {    
      diffs[UserPropertiesStats::IMPS_UNVERIFIED] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<UserPropertiesStats, 6>::Diffs::imps_unverified () const    
    {    
      return diffs[UserPropertiesStats::IMPS_UNVERIFIED];      
    }    
    inline    
    stats_value_type    
    UserPropertiesStats::profiling_requests () const    
    {    
      return values[UserPropertiesStats::PROFILING_REQUESTS];      
    }    
    inline    
    DiffStats<UserPropertiesStats, 6>::Diffs&     
    DiffStats<UserPropertiesStats, 6>::Diffs::profiling_requests (const stats_diff_type& value)    
    {    
      diffs[UserPropertiesStats::PROFILING_REQUESTS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<UserPropertiesStats, 6>::Diffs::profiling_requests () const    
    {    
      return diffs[UserPropertiesStats::PROFILING_REQUESTS];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_USERPROPERTIESSTATS_HPP

