
#ifndef __AUTOTESTS_COMMONS_STATS_GLOBALCOLOUSERSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_GLOBALCOLOUSERSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class GlobalColoUserStats:    
      public DiffStats<GlobalColoUserStats, 16>    
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
          CONTROL_UNIQUE_USERS_1,          
          CONTROL_UNIQUE_USERS_2,          
          CONTROL_NETWORK_UNIQUE_USERS_1,          
          CONTROL_NETWORK_UNIQUE_USERS_2,          
          CONTROL_PROFILING_UNIQUE_USERS_1,          
          CONTROL_PROFILING_UNIQUE_USERS_2,          
          CONTROL_UNIQUE_HIDS_1,          
          CONTROL_UNIQUE_HIDS_2,          
          NEG_CONTROL_UNIQUE_USERS,          
          NEG_CONTROL_NETWORK_UNIQUE_USERS,          
          NEG_CONTROL_PROFILING_UNIQUE_USERS,          
          NEG_CONTROL_UNIQUE_HIDS          
        };        
        typedef DiffStats<GlobalColoUserStats, 16> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            int colo_id_;            
            bool colo_id_used_;            
            bool colo_id_null_;            
            AutoTest::Time create_date_;            
            bool create_date_used_;            
            bool create_date_null_;            
            AutoTest::Time last_appearance_date_;            
            bool last_appearance_date_used_;            
            bool last_appearance_date_null_;            
            AutoTest::Time global_sdate_;            
            bool global_sdate_used_;            
            bool global_sdate_null_;            
          public:          
            Key& colo_id(const int& value);            
            Key& colo_id_set_null(bool is_null = true);            
            const int& colo_id() const;            
            bool colo_id_used() const;            
            bool colo_id_is_null() const;            
            Key& create_date(const AutoTest::Time& value);            
            Key& create_date_set_null(bool is_null = true);            
            const AutoTest::Time& create_date() const;            
            bool create_date_used() const;            
            bool create_date_is_null() const;            
            Key& last_appearance_date(const AutoTest::Time& value);            
            Key& last_appearance_date_set_null(bool is_null = true);            
            const AutoTest::Time& last_appearance_date() const;            
            bool last_appearance_date_used() const;            
            bool last_appearance_date_is_null() const;            
            Key& global_sdate(const AutoTest::Time& value);            
            Key& global_sdate_set_null(bool is_null = true);            
            const AutoTest::Time& global_sdate() const;            
            bool global_sdate_used() const;            
            bool global_sdate_is_null() const;            
            Key ();            
            Key (            
              const int& colo_id,              
              const AutoTest::Time& create_date,              
              const AutoTest::Time& last_appearance_date,              
              const AutoTest::Time& global_sdate              
            );            
        };        
        stats_value_type unique_users () const;        
        stats_value_type network_unique_users () const;        
        stats_value_type profiling_unique_users () const;        
        stats_value_type unique_hids () const;        
        stats_value_type control_unique_users_1 () const;        
        stats_value_type control_unique_users_2 () const;        
        stats_value_type control_network_unique_users_1 () const;        
        stats_value_type control_network_unique_users_2 () const;        
        stats_value_type control_profiling_unique_users_1 () const;        
        stats_value_type control_profiling_unique_users_2 () const;        
        stats_value_type control_unique_hids_1 () const;        
        stats_value_type control_unique_hids_2 () const;        
        stats_value_type neg_control_unique_users () const;        
        stats_value_type neg_control_network_unique_users () const;        
        stats_value_type neg_control_profiling_unique_users () const;        
        stats_value_type neg_control_unique_hids () const;        
        void print_idname (std::ostream& out) const;        
                
        GlobalColoUserStats (const Key& value);        
        GlobalColoUserStats (        
          const int& colo_id,          
          const AutoTest::Time& create_date,          
          const AutoTest::Time& last_appearance_date,          
          const AutoTest::Time& global_sdate          
        );        
        GlobalColoUserStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& colo_id,          
          const AutoTest::Time& create_date,          
          const AutoTest::Time& last_appearance_date,          
          const AutoTest::Time& global_sdate          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<GlobalColoUserStats, 16>::Diffs    
    {    
      public:      
        typedef stats_diff_type array_type[16];        
        typedef const stats_diff_type const_array_type[16];        
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
        Diffs& control_unique_users_1 (const stats_diff_type& value);        
        const stats_diff_type& control_unique_users_1 () const;        
        Diffs& control_unique_users_2 (const stats_diff_type& value);        
        const stats_diff_type& control_unique_users_2 () const;        
        Diffs& control_network_unique_users_1 (const stats_diff_type& value);        
        const stats_diff_type& control_network_unique_users_1 () const;        
        Diffs& control_network_unique_users_2 (const stats_diff_type& value);        
        const stats_diff_type& control_network_unique_users_2 () const;        
        Diffs& control_profiling_unique_users_1 (const stats_diff_type& value);        
        const stats_diff_type& control_profiling_unique_users_1 () const;        
        Diffs& control_profiling_unique_users_2 (const stats_diff_type& value);        
        const stats_diff_type& control_profiling_unique_users_2 () const;        
        Diffs& control_unique_hids_1 (const stats_diff_type& value);        
        const stats_diff_type& control_unique_hids_1 () const;        
        Diffs& control_unique_hids_2 (const stats_diff_type& value);        
        const stats_diff_type& control_unique_hids_2 () const;        
        Diffs& neg_control_unique_users (const stats_diff_type& value);        
        const stats_diff_type& neg_control_unique_users () const;        
        Diffs& neg_control_network_unique_users (const stats_diff_type& value);        
        const stats_diff_type& neg_control_network_unique_users () const;        
        Diffs& neg_control_profiling_unique_users (const stats_diff_type& value);        
        const stats_diff_type& neg_control_profiling_unique_users () const;        
        Diffs& neg_control_unique_hids (const stats_diff_type& value);        
        const stats_diff_type& neg_control_unique_hids () const;        
      protected:      
        stats_diff_type diffs[16];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// GlobalColoUserStats    
    inline    
    GlobalColoUserStats::GlobalColoUserStats ()    
      :Base("GlobalColoUserStats")    
    {}    
    inline    
    GlobalColoUserStats::GlobalColoUserStats (const GlobalColoUserStats::Key& value)    
      :Base("GlobalColoUserStats")    
    {    
      key_ = value;      
    }    
    inline    
    GlobalColoUserStats::GlobalColoUserStats (    
      const int& colo_id,      
      const AutoTest::Time& create_date,      
      const AutoTest::Time& last_appearance_date,      
      const AutoTest::Time& global_sdate      
    )    
      :Base("GlobalColoUserStats")    
    {    
      key_ = Key (      
        colo_id,        
        create_date,        
        last_appearance_date,        
        global_sdate        
      );      
    }    
    inline    
    GlobalColoUserStats::Key::Key ()    
      :colo_id_(0),create_date_(default_date()),last_appearance_date_(default_date()),global_sdate_(default_date())    
    {    
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      create_date_used_ = false;      
      create_date_null_ = false;      
      last_appearance_date_used_ = false;      
      last_appearance_date_null_ = false;      
      global_sdate_used_ = false;      
      global_sdate_null_ = false;      
    }    
    inline    
    GlobalColoUserStats::Key::Key (    
      const int& colo_id,      
      const AutoTest::Time& create_date,      
      const AutoTest::Time& last_appearance_date,      
      const AutoTest::Time& global_sdate      
    )    
      :colo_id_(colo_id),create_date_(create_date),last_appearance_date_(last_appearance_date),global_sdate_(global_sdate)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      create_date_used_ = true;      
      create_date_null_ = false;      
      last_appearance_date_used_ = true;      
      last_appearance_date_null_ = false;      
      global_sdate_used_ = true;      
      global_sdate_null_ = false;      
    }    
    inline    
    GlobalColoUserStats::Key&    
    GlobalColoUserStats::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    GlobalColoUserStats::Key&    
    GlobalColoUserStats::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    GlobalColoUserStats::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    GlobalColoUserStats::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    GlobalColoUserStats::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    GlobalColoUserStats::Key&    
    GlobalColoUserStats::Key::create_date(const AutoTest::Time& value)    
    {    
      create_date_ = value;      
      create_date_used_ = true;      
      create_date_null_ = false;      
      return *this;      
    }    
    inline    
    GlobalColoUserStats::Key&    
    GlobalColoUserStats::Key::create_date_set_null(bool is_null)    
    {    
      create_date_used_ = true;      
      create_date_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    GlobalColoUserStats::Key::create_date() const    
    {    
      return create_date_;      
    }    
    inline    
    bool    
    GlobalColoUserStats::Key::create_date_used() const    
    {    
      return create_date_used_;      
    }    
    inline    
    bool    
    GlobalColoUserStats::Key::create_date_is_null() const    
    {    
      return create_date_null_;      
    }    
    inline    
    GlobalColoUserStats::Key&    
    GlobalColoUserStats::Key::last_appearance_date(const AutoTest::Time& value)    
    {    
      last_appearance_date_ = value;      
      last_appearance_date_used_ = true;      
      last_appearance_date_null_ = false;      
      return *this;      
    }    
    inline    
    GlobalColoUserStats::Key&    
    GlobalColoUserStats::Key::last_appearance_date_set_null(bool is_null)    
    {    
      last_appearance_date_used_ = true;      
      last_appearance_date_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    GlobalColoUserStats::Key::last_appearance_date() const    
    {    
      return last_appearance_date_;      
    }    
    inline    
    bool    
    GlobalColoUserStats::Key::last_appearance_date_used() const    
    {    
      return last_appearance_date_used_;      
    }    
    inline    
    bool    
    GlobalColoUserStats::Key::last_appearance_date_is_null() const    
    {    
      return last_appearance_date_null_;      
    }    
    inline    
    GlobalColoUserStats::Key&    
    GlobalColoUserStats::Key::global_sdate(const AutoTest::Time& value)    
    {    
      global_sdate_ = value;      
      global_sdate_used_ = true;      
      global_sdate_null_ = false;      
      return *this;      
    }    
    inline    
    GlobalColoUserStats::Key&    
    GlobalColoUserStats::Key::global_sdate_set_null(bool is_null)    
    {    
      global_sdate_used_ = true;      
      global_sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    GlobalColoUserStats::Key::global_sdate() const    
    {    
      return global_sdate_;      
    }    
    inline    
    bool    
    GlobalColoUserStats::Key::global_sdate_used() const    
    {    
      return global_sdate_used_;      
    }    
    inline    
    bool    
    GlobalColoUserStats::Key::global_sdate_is_null() const    
    {    
      return global_sdate_null_;      
    }    
    inline    
    GlobalColoUserStats::Key&    
    GlobalColoUserStats::key (    
      const int& colo_id,      
      const AutoTest::Time& create_date,      
      const AutoTest::Time& last_appearance_date,      
      const AutoTest::Time& global_sdate      
    )    
    {    
      key_ = Key (      
        colo_id,        
        create_date,        
        last_appearance_date,        
        global_sdate        
      );      
      return key_;      
    }    
    inline    
    GlobalColoUserStats::Key&    
    GlobalColoUserStats::key ()    
    {    
      return key_;      
    }    
    inline    
    GlobalColoUserStats::Key&    
    GlobalColoUserStats::key (const GlobalColoUserStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 16; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 16; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 16; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 16; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 16; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 16; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs     
    DiffStats<GlobalColoUserStats, 16>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<GlobalColoUserStats, 16>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs     
    DiffStats<GlobalColoUserStats, 16>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<GlobalColoUserStats, 16>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs::const_iterator    
    DiffStats<GlobalColoUserStats, 16>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs::const_iterator    
    DiffStats<GlobalColoUserStats, 16>::Diffs::end () const    
    {    
      return diffs + 16;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<GlobalColoUserStats, 16>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<GlobalColoUserStats, 16>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<GlobalColoUserStats, 16>::Diffs::size () const    
    {    
      return 16;      
    }    
    inline    
    void    
    DiffStats<GlobalColoUserStats, 16>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 16; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    GlobalColoUserStats::unique_users () const    
    {    
      return values[GlobalColoUserStats::UNIQUE_USERS];      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::unique_users (const stats_diff_type& value)    
    {    
      diffs[GlobalColoUserStats::UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::unique_users () const    
    {    
      return diffs[GlobalColoUserStats::UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    GlobalColoUserStats::network_unique_users () const    
    {    
      return values[GlobalColoUserStats::NETWORK_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::network_unique_users (const stats_diff_type& value)    
    {    
      diffs[GlobalColoUserStats::NETWORK_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::network_unique_users () const    
    {    
      return diffs[GlobalColoUserStats::NETWORK_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    GlobalColoUserStats::profiling_unique_users () const    
    {    
      return values[GlobalColoUserStats::PROFILING_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::profiling_unique_users (const stats_diff_type& value)    
    {    
      diffs[GlobalColoUserStats::PROFILING_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::profiling_unique_users () const    
    {    
      return diffs[GlobalColoUserStats::PROFILING_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    GlobalColoUserStats::unique_hids () const    
    {    
      return values[GlobalColoUserStats::UNIQUE_HIDS];      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::unique_hids (const stats_diff_type& value)    
    {    
      diffs[GlobalColoUserStats::UNIQUE_HIDS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::unique_hids () const    
    {    
      return diffs[GlobalColoUserStats::UNIQUE_HIDS];      
    }    
    inline    
    stats_value_type    
    GlobalColoUserStats::control_unique_users_1 () const    
    {    
      return values[GlobalColoUserStats::CONTROL_UNIQUE_USERS_1];      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::control_unique_users_1 (const stats_diff_type& value)    
    {    
      diffs[GlobalColoUserStats::CONTROL_UNIQUE_USERS_1] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::control_unique_users_1 () const    
    {    
      return diffs[GlobalColoUserStats::CONTROL_UNIQUE_USERS_1];      
    }    
    inline    
    stats_value_type    
    GlobalColoUserStats::control_unique_users_2 () const    
    {    
      return values[GlobalColoUserStats::CONTROL_UNIQUE_USERS_2];      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::control_unique_users_2 (const stats_diff_type& value)    
    {    
      diffs[GlobalColoUserStats::CONTROL_UNIQUE_USERS_2] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::control_unique_users_2 () const    
    {    
      return diffs[GlobalColoUserStats::CONTROL_UNIQUE_USERS_2];      
    }    
    inline    
    stats_value_type    
    GlobalColoUserStats::control_network_unique_users_1 () const    
    {    
      return values[GlobalColoUserStats::CONTROL_NETWORK_UNIQUE_USERS_1];      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::control_network_unique_users_1 (const stats_diff_type& value)    
    {    
      diffs[GlobalColoUserStats::CONTROL_NETWORK_UNIQUE_USERS_1] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::control_network_unique_users_1 () const    
    {    
      return diffs[GlobalColoUserStats::CONTROL_NETWORK_UNIQUE_USERS_1];      
    }    
    inline    
    stats_value_type    
    GlobalColoUserStats::control_network_unique_users_2 () const    
    {    
      return values[GlobalColoUserStats::CONTROL_NETWORK_UNIQUE_USERS_2];      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::control_network_unique_users_2 (const stats_diff_type& value)    
    {    
      diffs[GlobalColoUserStats::CONTROL_NETWORK_UNIQUE_USERS_2] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::control_network_unique_users_2 () const    
    {    
      return diffs[GlobalColoUserStats::CONTROL_NETWORK_UNIQUE_USERS_2];      
    }    
    inline    
    stats_value_type    
    GlobalColoUserStats::control_profiling_unique_users_1 () const    
    {    
      return values[GlobalColoUserStats::CONTROL_PROFILING_UNIQUE_USERS_1];      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::control_profiling_unique_users_1 (const stats_diff_type& value)    
    {    
      diffs[GlobalColoUserStats::CONTROL_PROFILING_UNIQUE_USERS_1] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::control_profiling_unique_users_1 () const    
    {    
      return diffs[GlobalColoUserStats::CONTROL_PROFILING_UNIQUE_USERS_1];      
    }    
    inline    
    stats_value_type    
    GlobalColoUserStats::control_profiling_unique_users_2 () const    
    {    
      return values[GlobalColoUserStats::CONTROL_PROFILING_UNIQUE_USERS_2];      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::control_profiling_unique_users_2 (const stats_diff_type& value)    
    {    
      diffs[GlobalColoUserStats::CONTROL_PROFILING_UNIQUE_USERS_2] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::control_profiling_unique_users_2 () const    
    {    
      return diffs[GlobalColoUserStats::CONTROL_PROFILING_UNIQUE_USERS_2];      
    }    
    inline    
    stats_value_type    
    GlobalColoUserStats::control_unique_hids_1 () const    
    {    
      return values[GlobalColoUserStats::CONTROL_UNIQUE_HIDS_1];      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::control_unique_hids_1 (const stats_diff_type& value)    
    {    
      diffs[GlobalColoUserStats::CONTROL_UNIQUE_HIDS_1] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::control_unique_hids_1 () const    
    {    
      return diffs[GlobalColoUserStats::CONTROL_UNIQUE_HIDS_1];      
    }    
    inline    
    stats_value_type    
    GlobalColoUserStats::control_unique_hids_2 () const    
    {    
      return values[GlobalColoUserStats::CONTROL_UNIQUE_HIDS_2];      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::control_unique_hids_2 (const stats_diff_type& value)    
    {    
      diffs[GlobalColoUserStats::CONTROL_UNIQUE_HIDS_2] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::control_unique_hids_2 () const    
    {    
      return diffs[GlobalColoUserStats::CONTROL_UNIQUE_HIDS_2];      
    }    
    inline    
    stats_value_type    
    GlobalColoUserStats::neg_control_unique_users () const    
    {    
      return values[GlobalColoUserStats::NEG_CONTROL_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::neg_control_unique_users (const stats_diff_type& value)    
    {    
      diffs[GlobalColoUserStats::NEG_CONTROL_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::neg_control_unique_users () const    
    {    
      return diffs[GlobalColoUserStats::NEG_CONTROL_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    GlobalColoUserStats::neg_control_network_unique_users () const    
    {    
      return values[GlobalColoUserStats::NEG_CONTROL_NETWORK_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::neg_control_network_unique_users (const stats_diff_type& value)    
    {    
      diffs[GlobalColoUserStats::NEG_CONTROL_NETWORK_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::neg_control_network_unique_users () const    
    {    
      return diffs[GlobalColoUserStats::NEG_CONTROL_NETWORK_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    GlobalColoUserStats::neg_control_profiling_unique_users () const    
    {    
      return values[GlobalColoUserStats::NEG_CONTROL_PROFILING_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::neg_control_profiling_unique_users (const stats_diff_type& value)    
    {    
      diffs[GlobalColoUserStats::NEG_CONTROL_PROFILING_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::neg_control_profiling_unique_users () const    
    {    
      return diffs[GlobalColoUserStats::NEG_CONTROL_PROFILING_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    GlobalColoUserStats::neg_control_unique_hids () const    
    {    
      return values[GlobalColoUserStats::NEG_CONTROL_UNIQUE_HIDS];      
    }    
    inline    
    DiffStats<GlobalColoUserStats, 16>::Diffs&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::neg_control_unique_hids (const stats_diff_type& value)    
    {    
      diffs[GlobalColoUserStats::NEG_CONTROL_UNIQUE_HIDS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<GlobalColoUserStats, 16>::Diffs::neg_control_unique_hids () const    
    {    
      return diffs[GlobalColoUserStats::NEG_CONTROL_UNIQUE_HIDS];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_GLOBALCOLOUSERSTATS_HPP

