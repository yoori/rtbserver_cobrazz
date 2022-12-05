
#ifndef __AUTOTESTS_COMMONS_STATS_CREATEDUSERSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_CREATEDUSERSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class CreatedUserStats:    
      public DiffStats<CreatedUserStats, 16>    
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
        typedef DiffStats<CreatedUserStats, 16> Base;        
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
            AutoTest::Time create_date_;            
            bool create_date_used_;            
            bool create_date_null_;            
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
            Key& create_date(const AutoTest::Time& value);            
            Key& create_date_set_null(bool is_null = true);            
            const AutoTest::Time& create_date() const;            
            bool create_date_used() const;            
            bool create_date_is_null() const;            
            Key& isp_sdate(const AutoTest::Time& value);            
            Key& isp_sdate_set_null(bool is_null = true);            
            const AutoTest::Time& isp_sdate() const;            
            bool isp_sdate_used() const;            
            bool isp_sdate_is_null() const;            
            Key ();            
            Key (            
              const int& colo_id,              
              const AutoTest::Time& last_appearance_date,              
              const AutoTest::Time& create_date,              
              const AutoTest::Time& isp_sdate              
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
                
        CreatedUserStats (const Key& value);        
        CreatedUserStats (        
          const int& colo_id,          
          const AutoTest::Time& last_appearance_date,          
          const AutoTest::Time& create_date,          
          const AutoTest::Time& isp_sdate          
        );        
        CreatedUserStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& colo_id,          
          const AutoTest::Time& last_appearance_date,          
          const AutoTest::Time& create_date,          
          const AutoTest::Time& isp_sdate          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<CreatedUserStats, 16>::Diffs    
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
    ///////////////////////////////// CreatedUserStats    
    inline    
    CreatedUserStats::CreatedUserStats ()    
      :Base("CreatedUserStats")    
    {}    
    inline    
    CreatedUserStats::CreatedUserStats (const CreatedUserStats::Key& value)    
      :Base("CreatedUserStats")    
    {    
      key_ = value;      
    }    
    inline    
    CreatedUserStats::CreatedUserStats (    
      const int& colo_id,      
      const AutoTest::Time& last_appearance_date,      
      const AutoTest::Time& create_date,      
      const AutoTest::Time& isp_sdate      
    )    
      :Base("CreatedUserStats")    
    {    
      key_ = Key (      
        colo_id,        
        last_appearance_date,        
        create_date,        
        isp_sdate        
      );      
    }    
    inline    
    CreatedUserStats::Key::Key ()    
      :colo_id_(0),last_appearance_date_(default_date()),create_date_(default_date()),isp_sdate_(default_date())    
    {    
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      last_appearance_date_used_ = false;      
      last_appearance_date_null_ = false;      
      create_date_used_ = false;      
      create_date_null_ = false;      
      isp_sdate_used_ = false;      
      isp_sdate_null_ = false;      
    }    
    inline    
    CreatedUserStats::Key::Key (    
      const int& colo_id,      
      const AutoTest::Time& last_appearance_date,      
      const AutoTest::Time& create_date,      
      const AutoTest::Time& isp_sdate      
    )    
      :colo_id_(colo_id),last_appearance_date_(last_appearance_date),create_date_(create_date),isp_sdate_(isp_sdate)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      last_appearance_date_used_ = true;      
      last_appearance_date_null_ = false;      
      create_date_used_ = true;      
      create_date_null_ = false;      
      isp_sdate_used_ = true;      
      isp_sdate_null_ = false;      
    }    
    inline    
    CreatedUserStats::Key&    
    CreatedUserStats::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    CreatedUserStats::Key&    
    CreatedUserStats::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CreatedUserStats::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    CreatedUserStats::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    CreatedUserStats::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    CreatedUserStats::Key&    
    CreatedUserStats::Key::last_appearance_date(const AutoTest::Time& value)    
    {    
      last_appearance_date_ = value;      
      last_appearance_date_used_ = true;      
      last_appearance_date_null_ = false;      
      return *this;      
    }    
    inline    
    CreatedUserStats::Key&    
    CreatedUserStats::Key::last_appearance_date_set_null(bool is_null)    
    {    
      last_appearance_date_used_ = true;      
      last_appearance_date_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    CreatedUserStats::Key::last_appearance_date() const    
    {    
      return last_appearance_date_;      
    }    
    inline    
    bool    
    CreatedUserStats::Key::last_appearance_date_used() const    
    {    
      return last_appearance_date_used_;      
    }    
    inline    
    bool    
    CreatedUserStats::Key::last_appearance_date_is_null() const    
    {    
      return last_appearance_date_null_;      
    }    
    inline    
    CreatedUserStats::Key&    
    CreatedUserStats::Key::create_date(const AutoTest::Time& value)    
    {    
      create_date_ = value;      
      create_date_used_ = true;      
      create_date_null_ = false;      
      return *this;      
    }    
    inline    
    CreatedUserStats::Key&    
    CreatedUserStats::Key::create_date_set_null(bool is_null)    
    {    
      create_date_used_ = true;      
      create_date_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    CreatedUserStats::Key::create_date() const    
    {    
      return create_date_;      
    }    
    inline    
    bool    
    CreatedUserStats::Key::create_date_used() const    
    {    
      return create_date_used_;      
    }    
    inline    
    bool    
    CreatedUserStats::Key::create_date_is_null() const    
    {    
      return create_date_null_;      
    }    
    inline    
    CreatedUserStats::Key&    
    CreatedUserStats::Key::isp_sdate(const AutoTest::Time& value)    
    {    
      isp_sdate_ = value;      
      isp_sdate_used_ = true;      
      isp_sdate_null_ = false;      
      return *this;      
    }    
    inline    
    CreatedUserStats::Key&    
    CreatedUserStats::Key::isp_sdate_set_null(bool is_null)    
    {    
      isp_sdate_used_ = true;      
      isp_sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    CreatedUserStats::Key::isp_sdate() const    
    {    
      return isp_sdate_;      
    }    
    inline    
    bool    
    CreatedUserStats::Key::isp_sdate_used() const    
    {    
      return isp_sdate_used_;      
    }    
    inline    
    bool    
    CreatedUserStats::Key::isp_sdate_is_null() const    
    {    
      return isp_sdate_null_;      
    }    
    inline    
    CreatedUserStats::Key&    
    CreatedUserStats::key (    
      const int& colo_id,      
      const AutoTest::Time& last_appearance_date,      
      const AutoTest::Time& create_date,      
      const AutoTest::Time& isp_sdate      
    )    
    {    
      key_ = Key (      
        colo_id,        
        last_appearance_date,        
        create_date,        
        isp_sdate        
      );      
      return key_;      
    }    
    inline    
    CreatedUserStats::Key&    
    CreatedUserStats::key ()    
    {    
      return key_;      
    }    
    inline    
    CreatedUserStats::Key&    
    CreatedUserStats::key (const CreatedUserStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 16; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 16; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 16; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs&     
    DiffStats<CreatedUserStats, 16>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 16; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs&     
    DiffStats<CreatedUserStats, 16>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 16; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs&     
    DiffStats<CreatedUserStats, 16>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 16; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs     
    DiffStats<CreatedUserStats, 16>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<CreatedUserStats, 16>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs     
    DiffStats<CreatedUserStats, 16>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<CreatedUserStats, 16>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs::const_iterator    
    DiffStats<CreatedUserStats, 16>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs::const_iterator    
    DiffStats<CreatedUserStats, 16>::Diffs::end () const    
    {    
      return diffs + 16;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<CreatedUserStats, 16>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<CreatedUserStats, 16>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<CreatedUserStats, 16>::Diffs::size () const    
    {    
      return 16;      
    }    
    inline    
    void    
    DiffStats<CreatedUserStats, 16>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 16; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    CreatedUserStats::unique_users () const    
    {    
      return values[CreatedUserStats::UNIQUE_USERS];      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs&     
    DiffStats<CreatedUserStats, 16>::Diffs::unique_users (const stats_diff_type& value)    
    {    
      diffs[CreatedUserStats::UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CreatedUserStats, 16>::Diffs::unique_users () const    
    {    
      return diffs[CreatedUserStats::UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    CreatedUserStats::network_unique_users () const    
    {    
      return values[CreatedUserStats::NETWORK_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs&     
    DiffStats<CreatedUserStats, 16>::Diffs::network_unique_users (const stats_diff_type& value)    
    {    
      diffs[CreatedUserStats::NETWORK_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CreatedUserStats, 16>::Diffs::network_unique_users () const    
    {    
      return diffs[CreatedUserStats::NETWORK_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    CreatedUserStats::profiling_unique_users () const    
    {    
      return values[CreatedUserStats::PROFILING_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs&     
    DiffStats<CreatedUserStats, 16>::Diffs::profiling_unique_users (const stats_diff_type& value)    
    {    
      diffs[CreatedUserStats::PROFILING_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CreatedUserStats, 16>::Diffs::profiling_unique_users () const    
    {    
      return diffs[CreatedUserStats::PROFILING_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    CreatedUserStats::unique_hids () const    
    {    
      return values[CreatedUserStats::UNIQUE_HIDS];      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs&     
    DiffStats<CreatedUserStats, 16>::Diffs::unique_hids (const stats_diff_type& value)    
    {    
      diffs[CreatedUserStats::UNIQUE_HIDS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CreatedUserStats, 16>::Diffs::unique_hids () const    
    {    
      return diffs[CreatedUserStats::UNIQUE_HIDS];      
    }    
    inline    
    stats_value_type    
    CreatedUserStats::control_unique_users_1 () const    
    {    
      return values[CreatedUserStats::CONTROL_UNIQUE_USERS_1];      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs&     
    DiffStats<CreatedUserStats, 16>::Diffs::control_unique_users_1 (const stats_diff_type& value)    
    {    
      diffs[CreatedUserStats::CONTROL_UNIQUE_USERS_1] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CreatedUserStats, 16>::Diffs::control_unique_users_1 () const    
    {    
      return diffs[CreatedUserStats::CONTROL_UNIQUE_USERS_1];      
    }    
    inline    
    stats_value_type    
    CreatedUserStats::control_unique_users_2 () const    
    {    
      return values[CreatedUserStats::CONTROL_UNIQUE_USERS_2];      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs&     
    DiffStats<CreatedUserStats, 16>::Diffs::control_unique_users_2 (const stats_diff_type& value)    
    {    
      diffs[CreatedUserStats::CONTROL_UNIQUE_USERS_2] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CreatedUserStats, 16>::Diffs::control_unique_users_2 () const    
    {    
      return diffs[CreatedUserStats::CONTROL_UNIQUE_USERS_2];      
    }    
    inline    
    stats_value_type    
    CreatedUserStats::control_network_unique_users_1 () const    
    {    
      return values[CreatedUserStats::CONTROL_NETWORK_UNIQUE_USERS_1];      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs&     
    DiffStats<CreatedUserStats, 16>::Diffs::control_network_unique_users_1 (const stats_diff_type& value)    
    {    
      diffs[CreatedUserStats::CONTROL_NETWORK_UNIQUE_USERS_1] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CreatedUserStats, 16>::Diffs::control_network_unique_users_1 () const    
    {    
      return diffs[CreatedUserStats::CONTROL_NETWORK_UNIQUE_USERS_1];      
    }    
    inline    
    stats_value_type    
    CreatedUserStats::control_network_unique_users_2 () const    
    {    
      return values[CreatedUserStats::CONTROL_NETWORK_UNIQUE_USERS_2];      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs&     
    DiffStats<CreatedUserStats, 16>::Diffs::control_network_unique_users_2 (const stats_diff_type& value)    
    {    
      diffs[CreatedUserStats::CONTROL_NETWORK_UNIQUE_USERS_2] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CreatedUserStats, 16>::Diffs::control_network_unique_users_2 () const    
    {    
      return diffs[CreatedUserStats::CONTROL_NETWORK_UNIQUE_USERS_2];      
    }    
    inline    
    stats_value_type    
    CreatedUserStats::control_profiling_unique_users_1 () const    
    {    
      return values[CreatedUserStats::CONTROL_PROFILING_UNIQUE_USERS_1];      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs&     
    DiffStats<CreatedUserStats, 16>::Diffs::control_profiling_unique_users_1 (const stats_diff_type& value)    
    {    
      diffs[CreatedUserStats::CONTROL_PROFILING_UNIQUE_USERS_1] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CreatedUserStats, 16>::Diffs::control_profiling_unique_users_1 () const    
    {    
      return diffs[CreatedUserStats::CONTROL_PROFILING_UNIQUE_USERS_1];      
    }    
    inline    
    stats_value_type    
    CreatedUserStats::control_profiling_unique_users_2 () const    
    {    
      return values[CreatedUserStats::CONTROL_PROFILING_UNIQUE_USERS_2];      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs&     
    DiffStats<CreatedUserStats, 16>::Diffs::control_profiling_unique_users_2 (const stats_diff_type& value)    
    {    
      diffs[CreatedUserStats::CONTROL_PROFILING_UNIQUE_USERS_2] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CreatedUserStats, 16>::Diffs::control_profiling_unique_users_2 () const    
    {    
      return diffs[CreatedUserStats::CONTROL_PROFILING_UNIQUE_USERS_2];      
    }    
    inline    
    stats_value_type    
    CreatedUserStats::control_unique_hids_1 () const    
    {    
      return values[CreatedUserStats::CONTROL_UNIQUE_HIDS_1];      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs&     
    DiffStats<CreatedUserStats, 16>::Diffs::control_unique_hids_1 (const stats_diff_type& value)    
    {    
      diffs[CreatedUserStats::CONTROL_UNIQUE_HIDS_1] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CreatedUserStats, 16>::Diffs::control_unique_hids_1 () const    
    {    
      return diffs[CreatedUserStats::CONTROL_UNIQUE_HIDS_1];      
    }    
    inline    
    stats_value_type    
    CreatedUserStats::control_unique_hids_2 () const    
    {    
      return values[CreatedUserStats::CONTROL_UNIQUE_HIDS_2];      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs&     
    DiffStats<CreatedUserStats, 16>::Diffs::control_unique_hids_2 (const stats_diff_type& value)    
    {    
      diffs[CreatedUserStats::CONTROL_UNIQUE_HIDS_2] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CreatedUserStats, 16>::Diffs::control_unique_hids_2 () const    
    {    
      return diffs[CreatedUserStats::CONTROL_UNIQUE_HIDS_2];      
    }    
    inline    
    stats_value_type    
    CreatedUserStats::neg_control_unique_users () const    
    {    
      return values[CreatedUserStats::NEG_CONTROL_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs&     
    DiffStats<CreatedUserStats, 16>::Diffs::neg_control_unique_users (const stats_diff_type& value)    
    {    
      diffs[CreatedUserStats::NEG_CONTROL_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CreatedUserStats, 16>::Diffs::neg_control_unique_users () const    
    {    
      return diffs[CreatedUserStats::NEG_CONTROL_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    CreatedUserStats::neg_control_network_unique_users () const    
    {    
      return values[CreatedUserStats::NEG_CONTROL_NETWORK_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs&     
    DiffStats<CreatedUserStats, 16>::Diffs::neg_control_network_unique_users (const stats_diff_type& value)    
    {    
      diffs[CreatedUserStats::NEG_CONTROL_NETWORK_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CreatedUserStats, 16>::Diffs::neg_control_network_unique_users () const    
    {    
      return diffs[CreatedUserStats::NEG_CONTROL_NETWORK_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    CreatedUserStats::neg_control_profiling_unique_users () const    
    {    
      return values[CreatedUserStats::NEG_CONTROL_PROFILING_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs&     
    DiffStats<CreatedUserStats, 16>::Diffs::neg_control_profiling_unique_users (const stats_diff_type& value)    
    {    
      diffs[CreatedUserStats::NEG_CONTROL_PROFILING_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CreatedUserStats, 16>::Diffs::neg_control_profiling_unique_users () const    
    {    
      return diffs[CreatedUserStats::NEG_CONTROL_PROFILING_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    CreatedUserStats::neg_control_unique_hids () const    
    {    
      return values[CreatedUserStats::NEG_CONTROL_UNIQUE_HIDS];      
    }    
    inline    
    DiffStats<CreatedUserStats, 16>::Diffs&     
    DiffStats<CreatedUserStats, 16>::Diffs::neg_control_unique_hids (const stats_diff_type& value)    
    {    
      diffs[CreatedUserStats::NEG_CONTROL_UNIQUE_HIDS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CreatedUserStats, 16>::Diffs::neg_control_unique_hids () const    
    {    
      return diffs[CreatedUserStats::NEG_CONTROL_UNIQUE_HIDS];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_CREATEDUSERSTATS_HPP

