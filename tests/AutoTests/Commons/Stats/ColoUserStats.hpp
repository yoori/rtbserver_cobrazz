
#ifndef __AUTOTESTS_COMMONS_STATS_COLOUSERSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_COLOUSERSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class ColoUserStats:    
      public DiffStats<ColoUserStats, 12>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          UNIQUE_USERS = 0,          
          NETWORK_UNIQUE_USERS,          
          PROFILING_UNIQUE_USERS,          
          UNIQUE_HIDS,          
          CONTROL_UNIQUE_USERS,          
          CONTROL_NETWORK_UNIQUE_USERS,          
          CONTROL_PROFILING_UNIQUE_USERS,          
          CONTROL_UNIQUE_HIDS,          
          NEG_CONTROL_UNIQUE_USERS,          
          NEG_CONTROL_NETWORK_UNIQUE_USERS,          
          NEG_CONTROL_PROFILING_UNIQUE_USERS,          
          NEG_CONTROL_UNIQUE_HIDS          
        };        
        typedef DiffStats<ColoUserStats, 12> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            int colo_id_;            
            bool colo_id_used_;            
            bool colo_id_null_;            
            AutoTest::Time last_appearance_date_;            
            bool last_appearance_date_used_;            
            bool last_appearance_date_null_;            
            AutoTest::Time isp_sdate_;            
            bool isp_sdate_used_;            
            bool isp_sdate_null_;            
          public:          
            Key& colo_id(const int& value);            
            Key& colo_id_set_null(bool is_null = true);            
            const int& colo_id() const;            
            bool colo_id_used() const;            
            bool colo_id_is_null() const;            
            Key& last_appearance_date(const AutoTest::Time& value);            
            Key& last_appearance_date_set_null(bool is_null = true);            
            const AutoTest::Time& last_appearance_date() const;            
            bool last_appearance_date_used() const;            
            bool last_appearance_date_is_null() const;            
            Key& isp_sdate(const AutoTest::Time& value);            
            Key& isp_sdate_set_null(bool is_null = true);            
            const AutoTest::Time& isp_sdate() const;            
            bool isp_sdate_used() const;            
            bool isp_sdate_is_null() const;            
            Key ();            
            Key (            
              const int& colo_id,              
              const AutoTest::Time& last_appearance_date,              
              const AutoTest::Time& isp_sdate              
            );            
        };        
        stats_value_type unique_users () const;        
        stats_value_type network_unique_users () const;        
        stats_value_type profiling_unique_users () const;        
        stats_value_type unique_hids () const;        
        stats_value_type control_unique_users () const;        
        stats_value_type control_network_unique_users () const;        
        stats_value_type control_profiling_unique_users () const;        
        stats_value_type control_unique_hids () const;        
        stats_value_type neg_control_unique_users () const;        
        stats_value_type neg_control_network_unique_users () const;        
        stats_value_type neg_control_profiling_unique_users () const;        
        stats_value_type neg_control_unique_hids () const;        
        void print_idname (std::ostream& out) const;        
                
        ColoUserStats (const Key& value);        
        ColoUserStats (        
          const int& colo_id,          
          const AutoTest::Time& last_appearance_date,          
          const AutoTest::Time& isp_sdate          
        );        
        ColoUserStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& colo_id,          
          const AutoTest::Time& last_appearance_date,          
          const AutoTest::Time& isp_sdate          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<ColoUserStats, 12>::Diffs    
    {    
      public:      
        typedef stats_diff_type array_type[12];        
        typedef const stats_diff_type const_array_type[12];        
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
                
        Diffs& unique_users (const stats_diff_type& value);        
        const stats_diff_type& unique_users () const;        
        Diffs& network_unique_users (const stats_diff_type& value);        
        const stats_diff_type& network_unique_users () const;        
        Diffs& profiling_unique_users (const stats_diff_type& value);        
        const stats_diff_type& profiling_unique_users () const;        
        Diffs& unique_hids (const stats_diff_type& value);        
        const stats_diff_type& unique_hids () const;        
        Diffs& control_unique_users (const stats_diff_type& value);        
        const stats_diff_type& control_unique_users () const;        
        Diffs& control_network_unique_users (const stats_diff_type& value);        
        const stats_diff_type& control_network_unique_users () const;        
        Diffs& control_profiling_unique_users (const stats_diff_type& value);        
        const stats_diff_type& control_profiling_unique_users () const;        
        Diffs& control_unique_hids (const stats_diff_type& value);        
        const stats_diff_type& control_unique_hids () const;        
        Diffs& neg_control_unique_users (const stats_diff_type& value);        
        const stats_diff_type& neg_control_unique_users () const;        
        Diffs& neg_control_network_unique_users (const stats_diff_type& value);        
        const stats_diff_type& neg_control_network_unique_users () const;        
        Diffs& neg_control_profiling_unique_users (const stats_diff_type& value);        
        const stats_diff_type& neg_control_profiling_unique_users () const;        
        Diffs& neg_control_unique_hids (const stats_diff_type& value);        
        const stats_diff_type& neg_control_unique_hids () const;        
      protected:      
        stats_diff_type diffs[12];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ColoUserStats    
    inline    
    ColoUserStats::ColoUserStats ()    
      :Base("ColoUserStats")    
    {}    
    inline    
    ColoUserStats::ColoUserStats (const ColoUserStats::Key& value)    
      :Base("ColoUserStats")    
    {    
      key_ = value;      
    }    
    inline    
    ColoUserStats::ColoUserStats (    
      const int& colo_id,      
      const AutoTest::Time& last_appearance_date,      
      const AutoTest::Time& isp_sdate      
    )    
      :Base("ColoUserStats")    
    {    
      key_ = Key (      
        colo_id,        
        last_appearance_date,        
        isp_sdate        
      );      
    }    
    inline    
    ColoUserStats::Key::Key ()    
      :colo_id_(0),last_appearance_date_(default_date()),isp_sdate_(default_date())    
    {    
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      last_appearance_date_used_ = false;      
      last_appearance_date_null_ = false;      
      isp_sdate_used_ = false;      
      isp_sdate_null_ = false;      
    }    
    inline    
    ColoUserStats::Key::Key (    
      const int& colo_id,      
      const AutoTest::Time& last_appearance_date,      
      const AutoTest::Time& isp_sdate      
    )    
      :colo_id_(colo_id),last_appearance_date_(last_appearance_date),isp_sdate_(isp_sdate)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      last_appearance_date_used_ = true;      
      last_appearance_date_null_ = false;      
      isp_sdate_used_ = true;      
      isp_sdate_null_ = false;      
    }    
    inline    
    ColoUserStats::Key&    
    ColoUserStats::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    ColoUserStats::Key&    
    ColoUserStats::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ColoUserStats::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    ColoUserStats::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    ColoUserStats::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    ColoUserStats::Key&    
    ColoUserStats::Key::last_appearance_date(const AutoTest::Time& value)    
    {    
      last_appearance_date_ = value;      
      last_appearance_date_used_ = true;      
      last_appearance_date_null_ = false;      
      return *this;      
    }    
    inline    
    ColoUserStats::Key&    
    ColoUserStats::Key::last_appearance_date_set_null(bool is_null)    
    {    
      last_appearance_date_used_ = true;      
      last_appearance_date_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    ColoUserStats::Key::last_appearance_date() const    
    {    
      return last_appearance_date_;      
    }    
    inline    
    bool    
    ColoUserStats::Key::last_appearance_date_used() const    
    {    
      return last_appearance_date_used_;      
    }    
    inline    
    bool    
    ColoUserStats::Key::last_appearance_date_is_null() const    
    {    
      return last_appearance_date_null_;      
    }    
    inline    
    ColoUserStats::Key&    
    ColoUserStats::Key::isp_sdate(const AutoTest::Time& value)    
    {    
      isp_sdate_ = value;      
      isp_sdate_used_ = true;      
      isp_sdate_null_ = false;      
      return *this;      
    }    
    inline    
    ColoUserStats::Key&    
    ColoUserStats::Key::isp_sdate_set_null(bool is_null)    
    {    
      isp_sdate_used_ = true;      
      isp_sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    ColoUserStats::Key::isp_sdate() const    
    {    
      return isp_sdate_;      
    }    
    inline    
    bool    
    ColoUserStats::Key::isp_sdate_used() const    
    {    
      return isp_sdate_used_;      
    }    
    inline    
    bool    
    ColoUserStats::Key::isp_sdate_is_null() const    
    {    
      return isp_sdate_null_;      
    }    
    inline    
    ColoUserStats::Key&    
    ColoUserStats::key (    
      const int& colo_id,      
      const AutoTest::Time& last_appearance_date,      
      const AutoTest::Time& isp_sdate      
    )    
    {    
      key_ = Key (      
        colo_id,        
        last_appearance_date,        
        isp_sdate        
      );      
      return key_;      
    }    
    inline    
    ColoUserStats::Key&    
    ColoUserStats::key ()    
    {    
      return key_;      
    }    
    inline    
    ColoUserStats::Key&    
    ColoUserStats::key (const ColoUserStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 12; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 12; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 12; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs&     
    DiffStats<ColoUserStats, 12>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 12; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs&     
    DiffStats<ColoUserStats, 12>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 12; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs&     
    DiffStats<ColoUserStats, 12>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 12; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs     
    DiffStats<ColoUserStats, 12>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<ColoUserStats, 12>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs     
    DiffStats<ColoUserStats, 12>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<ColoUserStats, 12>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs::const_iterator    
    DiffStats<ColoUserStats, 12>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs::const_iterator    
    DiffStats<ColoUserStats, 12>::Diffs::end () const    
    {    
      return diffs + 12;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<ColoUserStats, 12>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<ColoUserStats, 12>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<ColoUserStats, 12>::Diffs::size () const    
    {    
      return 12;      
    }    
    inline    
    void    
    DiffStats<ColoUserStats, 12>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 12; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    ColoUserStats::unique_users () const    
    {    
      return values[ColoUserStats::UNIQUE_USERS];      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs&     
    DiffStats<ColoUserStats, 12>::Diffs::unique_users (const stats_diff_type& value)    
    {    
      diffs[ColoUserStats::UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoUserStats, 12>::Diffs::unique_users () const    
    {    
      return diffs[ColoUserStats::UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    ColoUserStats::network_unique_users () const    
    {    
      return values[ColoUserStats::NETWORK_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs&     
    DiffStats<ColoUserStats, 12>::Diffs::network_unique_users (const stats_diff_type& value)    
    {    
      diffs[ColoUserStats::NETWORK_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoUserStats, 12>::Diffs::network_unique_users () const    
    {    
      return diffs[ColoUserStats::NETWORK_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    ColoUserStats::profiling_unique_users () const    
    {    
      return values[ColoUserStats::PROFILING_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs&     
    DiffStats<ColoUserStats, 12>::Diffs::profiling_unique_users (const stats_diff_type& value)    
    {    
      diffs[ColoUserStats::PROFILING_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoUserStats, 12>::Diffs::profiling_unique_users () const    
    {    
      return diffs[ColoUserStats::PROFILING_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    ColoUserStats::unique_hids () const    
    {    
      return values[ColoUserStats::UNIQUE_HIDS];      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs&     
    DiffStats<ColoUserStats, 12>::Diffs::unique_hids (const stats_diff_type& value)    
    {    
      diffs[ColoUserStats::UNIQUE_HIDS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoUserStats, 12>::Diffs::unique_hids () const    
    {    
      return diffs[ColoUserStats::UNIQUE_HIDS];      
    }    
    inline    
    stats_value_type    
    ColoUserStats::control_unique_users () const    
    {    
      return values[ColoUserStats::CONTROL_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs&     
    DiffStats<ColoUserStats, 12>::Diffs::control_unique_users (const stats_diff_type& value)    
    {    
      diffs[ColoUserStats::CONTROL_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoUserStats, 12>::Diffs::control_unique_users () const    
    {    
      return diffs[ColoUserStats::CONTROL_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    ColoUserStats::control_network_unique_users () const    
    {    
      return values[ColoUserStats::CONTROL_NETWORK_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs&     
    DiffStats<ColoUserStats, 12>::Diffs::control_network_unique_users (const stats_diff_type& value)    
    {    
      diffs[ColoUserStats::CONTROL_NETWORK_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoUserStats, 12>::Diffs::control_network_unique_users () const    
    {    
      return diffs[ColoUserStats::CONTROL_NETWORK_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    ColoUserStats::control_profiling_unique_users () const    
    {    
      return values[ColoUserStats::CONTROL_PROFILING_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs&     
    DiffStats<ColoUserStats, 12>::Diffs::control_profiling_unique_users (const stats_diff_type& value)    
    {    
      diffs[ColoUserStats::CONTROL_PROFILING_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoUserStats, 12>::Diffs::control_profiling_unique_users () const    
    {    
      return diffs[ColoUserStats::CONTROL_PROFILING_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    ColoUserStats::control_unique_hids () const    
    {    
      return values[ColoUserStats::CONTROL_UNIQUE_HIDS];      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs&     
    DiffStats<ColoUserStats, 12>::Diffs::control_unique_hids (const stats_diff_type& value)    
    {    
      diffs[ColoUserStats::CONTROL_UNIQUE_HIDS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoUserStats, 12>::Diffs::control_unique_hids () const    
    {    
      return diffs[ColoUserStats::CONTROL_UNIQUE_HIDS];      
    }    
    inline    
    stats_value_type    
    ColoUserStats::neg_control_unique_users () const    
    {    
      return values[ColoUserStats::NEG_CONTROL_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs&     
    DiffStats<ColoUserStats, 12>::Diffs::neg_control_unique_users (const stats_diff_type& value)    
    {    
      diffs[ColoUserStats::NEG_CONTROL_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoUserStats, 12>::Diffs::neg_control_unique_users () const    
    {    
      return diffs[ColoUserStats::NEG_CONTROL_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    ColoUserStats::neg_control_network_unique_users () const    
    {    
      return values[ColoUserStats::NEG_CONTROL_NETWORK_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs&     
    DiffStats<ColoUserStats, 12>::Diffs::neg_control_network_unique_users (const stats_diff_type& value)    
    {    
      diffs[ColoUserStats::NEG_CONTROL_NETWORK_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoUserStats, 12>::Diffs::neg_control_network_unique_users () const    
    {    
      return diffs[ColoUserStats::NEG_CONTROL_NETWORK_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    ColoUserStats::neg_control_profiling_unique_users () const    
    {    
      return values[ColoUserStats::NEG_CONTROL_PROFILING_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs&     
    DiffStats<ColoUserStats, 12>::Diffs::neg_control_profiling_unique_users (const stats_diff_type& value)    
    {    
      diffs[ColoUserStats::NEG_CONTROL_PROFILING_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoUserStats, 12>::Diffs::neg_control_profiling_unique_users () const    
    {    
      return diffs[ColoUserStats::NEG_CONTROL_PROFILING_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    ColoUserStats::neg_control_unique_hids () const    
    {    
      return values[ColoUserStats::NEG_CONTROL_UNIQUE_HIDS];      
    }    
    inline    
    DiffStats<ColoUserStats, 12>::Diffs&     
    DiffStats<ColoUserStats, 12>::Diffs::neg_control_unique_hids (const stats_diff_type& value)    
    {    
      diffs[ColoUserStats::NEG_CONTROL_UNIQUE_HIDS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoUserStats, 12>::Diffs::neg_control_unique_hids () const    
    {    
      return diffs[ColoUserStats::NEG_CONTROL_UNIQUE_HIDS];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_COLOUSERSTATS_HPP

