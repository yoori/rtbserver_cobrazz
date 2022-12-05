
#ifndef __AUTOTESTS_COMMONS_STATS_ADVERTISERUSERSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_ADVERTISERUSERSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class AdvertiserUserStats:    
      public DiffStats<AdvertiserUserStats, 4>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          UNIQUE_USERS = 0,          
          TEXT_UNIQUE_USERS,          
          DISPLAY_UNIQUE_USERS,          
          CONTROL_SUM          
        };        
        typedef DiffStats<AdvertiserUserStats, 4> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            int adv_account_id_;            
            bool adv_account_id_used_;            
            bool adv_account_id_null_;            
            AutoTest::Time last_appearance_date_;            
            bool last_appearance_date_used_;            
            bool last_appearance_date_null_;            
            AutoTest::Time adv_sdate_;            
            bool adv_sdate_used_;            
            bool adv_sdate_null_;            
          public:          
            Key& adv_account_id(const int& value);            
            Key& adv_account_id_set_null(bool is_null = true);            
            const int& adv_account_id() const;            
            bool adv_account_id_used() const;            
            bool adv_account_id_is_null() const;            
            Key& last_appearance_date(const AutoTest::Time& value);            
            Key& last_appearance_date_set_null(bool is_null = true);            
            const AutoTest::Time& last_appearance_date() const;            
            bool last_appearance_date_used() const;            
            bool last_appearance_date_is_null() const;            
            Key& adv_sdate(const AutoTest::Time& value);            
            Key& adv_sdate_set_null(bool is_null = true);            
            const AutoTest::Time& adv_sdate() const;            
            bool adv_sdate_used() const;            
            bool adv_sdate_is_null() const;            
            Key ();            
            Key (            
              const int& adv_account_id,              
              const AutoTest::Time& last_appearance_date,              
              const AutoTest::Time& adv_sdate              
            );            
        };        
        stats_value_type unique_users () const;        
        stats_value_type text_unique_users () const;        
        stats_value_type display_unique_users () const;        
        stats_value_type control_sum () const;        
        void print_idname (std::ostream& out) const;        
                
        AdvertiserUserStats (const Key& value);        
        AdvertiserUserStats (        
          const int& adv_account_id,          
          const AutoTest::Time& last_appearance_date,          
          const AutoTest::Time& adv_sdate          
        );        
        AdvertiserUserStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& adv_account_id,          
          const AutoTest::Time& last_appearance_date,          
          const AutoTest::Time& adv_sdate          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<AdvertiserUserStats, 4>::Diffs    
    {    
      public:      
        typedef stats_diff_type array_type[4];        
        typedef const stats_diff_type const_array_type[4];        
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
        Diffs& text_unique_users (const stats_diff_type& value);        
        const stats_diff_type& text_unique_users () const;        
        Diffs& display_unique_users (const stats_diff_type& value);        
        const stats_diff_type& display_unique_users () const;        
        Diffs& control_sum (const stats_diff_type& value);        
        const stats_diff_type& control_sum () const;        
      protected:      
        stats_diff_type diffs[4];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// AdvertiserUserStats    
    inline    
    AdvertiserUserStats::AdvertiserUserStats ()    
      :Base("AdvertiserUserStats")    
    {}    
    inline    
    AdvertiserUserStats::AdvertiserUserStats (const AdvertiserUserStats::Key& value)    
      :Base("AdvertiserUserStats")    
    {    
      key_ = value;      
    }    
    inline    
    AdvertiserUserStats::AdvertiserUserStats (    
      const int& adv_account_id,      
      const AutoTest::Time& last_appearance_date,      
      const AutoTest::Time& adv_sdate      
    )    
      :Base("AdvertiserUserStats")    
    {    
      key_ = Key (      
        adv_account_id,        
        last_appearance_date,        
        adv_sdate        
      );      
    }    
    inline    
    AdvertiserUserStats::Key::Key ()    
      :adv_account_id_(0),last_appearance_date_(default_date()),adv_sdate_(default_date())    
    {    
      adv_account_id_used_ = false;      
      adv_account_id_null_ = false;      
      last_appearance_date_used_ = false;      
      last_appearance_date_null_ = false;      
      adv_sdate_used_ = false;      
      adv_sdate_null_ = false;      
    }    
    inline    
    AdvertiserUserStats::Key::Key (    
      const int& adv_account_id,      
      const AutoTest::Time& last_appearance_date,      
      const AutoTest::Time& adv_sdate      
    )    
      :adv_account_id_(adv_account_id),last_appearance_date_(last_appearance_date),adv_sdate_(adv_sdate)    
    {    
      adv_account_id_used_ = true;      
      adv_account_id_null_ = false;      
      last_appearance_date_used_ = true;      
      last_appearance_date_null_ = false;      
      adv_sdate_used_ = true;      
      adv_sdate_null_ = false;      
    }    
    inline    
    AdvertiserUserStats::Key&    
    AdvertiserUserStats::Key::adv_account_id(const int& value)    
    {    
      adv_account_id_ = value;      
      adv_account_id_used_ = true;      
      adv_account_id_null_ = false;      
      return *this;      
    }    
    inline    
    AdvertiserUserStats::Key&    
    AdvertiserUserStats::Key::adv_account_id_set_null(bool is_null)    
    {    
      adv_account_id_used_ = true;      
      adv_account_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    AdvertiserUserStats::Key::adv_account_id() const    
    {    
      return adv_account_id_;      
    }    
    inline    
    bool    
    AdvertiserUserStats::Key::adv_account_id_used() const    
    {    
      return adv_account_id_used_;      
    }    
    inline    
    bool    
    AdvertiserUserStats::Key::adv_account_id_is_null() const    
    {    
      return adv_account_id_null_;      
    }    
    inline    
    AdvertiserUserStats::Key&    
    AdvertiserUserStats::Key::last_appearance_date(const AutoTest::Time& value)    
    {    
      last_appearance_date_ = value;      
      last_appearance_date_used_ = true;      
      last_appearance_date_null_ = false;      
      return *this;      
    }    
    inline    
    AdvertiserUserStats::Key&    
    AdvertiserUserStats::Key::last_appearance_date_set_null(bool is_null)    
    {    
      last_appearance_date_used_ = true;      
      last_appearance_date_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    AdvertiserUserStats::Key::last_appearance_date() const    
    {    
      return last_appearance_date_;      
    }    
    inline    
    bool    
    AdvertiserUserStats::Key::last_appearance_date_used() const    
    {    
      return last_appearance_date_used_;      
    }    
    inline    
    bool    
    AdvertiserUserStats::Key::last_appearance_date_is_null() const    
    {    
      return last_appearance_date_null_;      
    }    
    inline    
    AdvertiserUserStats::Key&    
    AdvertiserUserStats::Key::adv_sdate(const AutoTest::Time& value)    
    {    
      adv_sdate_ = value;      
      adv_sdate_used_ = true;      
      adv_sdate_null_ = false;      
      return *this;      
    }    
    inline    
    AdvertiserUserStats::Key&    
    AdvertiserUserStats::Key::adv_sdate_set_null(bool is_null)    
    {    
      adv_sdate_used_ = true;      
      adv_sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    AdvertiserUserStats::Key::adv_sdate() const    
    {    
      return adv_sdate_;      
    }    
    inline    
    bool    
    AdvertiserUserStats::Key::adv_sdate_used() const    
    {    
      return adv_sdate_used_;      
    }    
    inline    
    bool    
    AdvertiserUserStats::Key::adv_sdate_is_null() const    
    {    
      return adv_sdate_null_;      
    }    
    inline    
    AdvertiserUserStats::Key&    
    AdvertiserUserStats::key (    
      const int& adv_account_id,      
      const AutoTest::Time& last_appearance_date,      
      const AutoTest::Time& adv_sdate      
    )    
    {    
      key_ = Key (      
        adv_account_id,        
        last_appearance_date,        
        adv_sdate        
      );      
      return key_;      
    }    
    inline    
    AdvertiserUserStats::Key&    
    AdvertiserUserStats::key ()    
    {    
      return key_;      
    }    
    inline    
    AdvertiserUserStats::Key&    
    AdvertiserUserStats::key (const AdvertiserUserStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<AdvertiserUserStats, 4>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<AdvertiserUserStats, 4>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<AdvertiserUserStats, 4>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<AdvertiserUserStats, 4>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<AdvertiserUserStats, 4>::Diffs&     
    DiffStats<AdvertiserUserStats, 4>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<AdvertiserUserStats, 4>::Diffs&     
    DiffStats<AdvertiserUserStats, 4>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<AdvertiserUserStats, 4>::Diffs&     
    DiffStats<AdvertiserUserStats, 4>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<AdvertiserUserStats, 4>::Diffs     
    DiffStats<AdvertiserUserStats, 4>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<AdvertiserUserStats, 4>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<AdvertiserUserStats, 4>::Diffs     
    DiffStats<AdvertiserUserStats, 4>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<AdvertiserUserStats, 4>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<AdvertiserUserStats, 4>::Diffs::const_iterator    
    DiffStats<AdvertiserUserStats, 4>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<AdvertiserUserStats, 4>::Diffs::const_iterator    
    DiffStats<AdvertiserUserStats, 4>::Diffs::end () const    
    {    
      return diffs + 4;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<AdvertiserUserStats, 4>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<AdvertiserUserStats, 4>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<AdvertiserUserStats, 4>::Diffs::size () const    
    {    
      return 4;      
    }    
    inline    
    void    
    DiffStats<AdvertiserUserStats, 4>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 4; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    AdvertiserUserStats::unique_users () const    
    {    
      return values[AdvertiserUserStats::UNIQUE_USERS];      
    }    
    inline    
    DiffStats<AdvertiserUserStats, 4>::Diffs&     
    DiffStats<AdvertiserUserStats, 4>::Diffs::unique_users (const stats_diff_type& value)    
    {    
      diffs[AdvertiserUserStats::UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<AdvertiserUserStats, 4>::Diffs::unique_users () const    
    {    
      return diffs[AdvertiserUserStats::UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    AdvertiserUserStats::text_unique_users () const    
    {    
      return values[AdvertiserUserStats::TEXT_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<AdvertiserUserStats, 4>::Diffs&     
    DiffStats<AdvertiserUserStats, 4>::Diffs::text_unique_users (const stats_diff_type& value)    
    {    
      diffs[AdvertiserUserStats::TEXT_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<AdvertiserUserStats, 4>::Diffs::text_unique_users () const    
    {    
      return diffs[AdvertiserUserStats::TEXT_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    AdvertiserUserStats::display_unique_users () const    
    {    
      return values[AdvertiserUserStats::DISPLAY_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<AdvertiserUserStats, 4>::Diffs&     
    DiffStats<AdvertiserUserStats, 4>::Diffs::display_unique_users (const stats_diff_type& value)    
    {    
      diffs[AdvertiserUserStats::DISPLAY_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<AdvertiserUserStats, 4>::Diffs::display_unique_users () const    
    {    
      return diffs[AdvertiserUserStats::DISPLAY_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    AdvertiserUserStats::control_sum () const    
    {    
      return values[AdvertiserUserStats::CONTROL_SUM];      
    }    
    inline    
    DiffStats<AdvertiserUserStats, 4>::Diffs&     
    DiffStats<AdvertiserUserStats, 4>::Diffs::control_sum (const stats_diff_type& value)    
    {    
      diffs[AdvertiserUserStats::CONTROL_SUM] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<AdvertiserUserStats, 4>::Diffs::control_sum () const    
    {    
      return diffs[AdvertiserUserStats::CONTROL_SUM];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_ADVERTISERUSERSTATS_HPP

