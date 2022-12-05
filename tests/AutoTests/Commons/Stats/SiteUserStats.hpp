
#ifndef __AUTOTESTS_COMMONS_STATS_SITEUSERSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_SITEUSERSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class SiteUserStats:    
      public DiffStats<SiteUserStats, 2>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          UNIQUE_USERS = 0,          
          CONTROL_SUM          
        };        
        typedef DiffStats<SiteUserStats, 2> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            int site_id_;            
            bool site_id_used_;            
            bool site_id_null_;            
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
            Key& site_id(const int& value);            
            Key& site_id_set_null(bool is_null = true);            
            const int& site_id() const;            
            bool site_id_used() const;            
            bool site_id_is_null() const;            
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
              const int& site_id,              
              const int& colo_id,              
              const AutoTest::Time& last_appearance_date,              
              const AutoTest::Time& isp_sdate              
            );            
        };        
        stats_value_type unique_users () const;        
        stats_value_type control_sum () const;        
        void print_idname (std::ostream& out) const;        
                
        SiteUserStats (const Key& value);        
        SiteUserStats (        
          const int& site_id,          
          const int& colo_id,          
          const AutoTest::Time& last_appearance_date,          
          const AutoTest::Time& isp_sdate          
        );        
        SiteUserStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& site_id,          
          const int& colo_id,          
          const AutoTest::Time& last_appearance_date,          
          const AutoTest::Time& isp_sdate          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<SiteUserStats, 2>::Diffs    
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
                
        Diffs& unique_users (const stats_diff_type& value);        
        const stats_diff_type& unique_users () const;        
        Diffs& control_sum (const stats_diff_type& value);        
        const stats_diff_type& control_sum () const;        
      protected:      
        stats_diff_type diffs[2];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// SiteUserStats    
    inline    
    SiteUserStats::SiteUserStats ()    
      :Base("SiteUserStats")    
    {}    
    inline    
    SiteUserStats::SiteUserStats (const SiteUserStats::Key& value)    
      :Base("SiteUserStats")    
    {    
      key_ = value;      
    }    
    inline    
    SiteUserStats::SiteUserStats (    
      const int& site_id,      
      const int& colo_id,      
      const AutoTest::Time& last_appearance_date,      
      const AutoTest::Time& isp_sdate      
    )    
      :Base("SiteUserStats")    
    {    
      key_ = Key (      
        site_id,        
        colo_id,        
        last_appearance_date,        
        isp_sdate        
      );      
    }    
    inline    
    SiteUserStats::Key::Key ()    
      :site_id_(0),colo_id_(0),last_appearance_date_(default_date()),isp_sdate_(default_date())    
    {    
      site_id_used_ = false;      
      site_id_null_ = false;      
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      last_appearance_date_used_ = false;      
      last_appearance_date_null_ = false;      
      isp_sdate_used_ = false;      
      isp_sdate_null_ = false;      
    }    
    inline    
    SiteUserStats::Key::Key (    
      const int& site_id,      
      const int& colo_id,      
      const AutoTest::Time& last_appearance_date,      
      const AutoTest::Time& isp_sdate      
    )    
      :site_id_(site_id),colo_id_(colo_id),last_appearance_date_(last_appearance_date),isp_sdate_(isp_sdate)    
    {    
      site_id_used_ = true;      
      site_id_null_ = false;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      last_appearance_date_used_ = true;      
      last_appearance_date_null_ = false;      
      isp_sdate_used_ = true;      
      isp_sdate_null_ = false;      
    }    
    inline    
    SiteUserStats::Key&    
    SiteUserStats::Key::site_id(const int& value)    
    {    
      site_id_ = value;      
      site_id_used_ = true;      
      site_id_null_ = false;      
      return *this;      
    }    
    inline    
    SiteUserStats::Key&    
    SiteUserStats::Key::site_id_set_null(bool is_null)    
    {    
      site_id_used_ = true;      
      site_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    SiteUserStats::Key::site_id() const    
    {    
      return site_id_;      
    }    
    inline    
    bool    
    SiteUserStats::Key::site_id_used() const    
    {    
      return site_id_used_;      
    }    
    inline    
    bool    
    SiteUserStats::Key::site_id_is_null() const    
    {    
      return site_id_null_;      
    }    
    inline    
    SiteUserStats::Key&    
    SiteUserStats::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    SiteUserStats::Key&    
    SiteUserStats::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    SiteUserStats::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    SiteUserStats::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    SiteUserStats::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    SiteUserStats::Key&    
    SiteUserStats::Key::last_appearance_date(const AutoTest::Time& value)    
    {    
      last_appearance_date_ = value;      
      last_appearance_date_used_ = true;      
      last_appearance_date_null_ = false;      
      return *this;      
    }    
    inline    
    SiteUserStats::Key&    
    SiteUserStats::Key::last_appearance_date_set_null(bool is_null)    
    {    
      last_appearance_date_used_ = true;      
      last_appearance_date_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    SiteUserStats::Key::last_appearance_date() const    
    {    
      return last_appearance_date_;      
    }    
    inline    
    bool    
    SiteUserStats::Key::last_appearance_date_used() const    
    {    
      return last_appearance_date_used_;      
    }    
    inline    
    bool    
    SiteUserStats::Key::last_appearance_date_is_null() const    
    {    
      return last_appearance_date_null_;      
    }    
    inline    
    SiteUserStats::Key&    
    SiteUserStats::Key::isp_sdate(const AutoTest::Time& value)    
    {    
      isp_sdate_ = value;      
      isp_sdate_used_ = true;      
      isp_sdate_null_ = false;      
      return *this;      
    }    
    inline    
    SiteUserStats::Key&    
    SiteUserStats::Key::isp_sdate_set_null(bool is_null)    
    {    
      isp_sdate_used_ = true;      
      isp_sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    SiteUserStats::Key::isp_sdate() const    
    {    
      return isp_sdate_;      
    }    
    inline    
    bool    
    SiteUserStats::Key::isp_sdate_used() const    
    {    
      return isp_sdate_used_;      
    }    
    inline    
    bool    
    SiteUserStats::Key::isp_sdate_is_null() const    
    {    
      return isp_sdate_null_;      
    }    
    inline    
    SiteUserStats::Key&    
    SiteUserStats::key (    
      const int& site_id,      
      const int& colo_id,      
      const AutoTest::Time& last_appearance_date,      
      const AutoTest::Time& isp_sdate      
    )    
    {    
      key_ = Key (      
        site_id,        
        colo_id,        
        last_appearance_date,        
        isp_sdate        
      );      
      return key_;      
    }    
    inline    
    SiteUserStats::Key&    
    SiteUserStats::key ()    
    {    
      return key_;      
    }    
    inline    
    SiteUserStats::Key&    
    SiteUserStats::key (const SiteUserStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<SiteUserStats, 2>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<SiteUserStats, 2>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<SiteUserStats, 2>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<SiteUserStats, 2>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<SiteUserStats, 2>::Diffs&     
    DiffStats<SiteUserStats, 2>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<SiteUserStats, 2>::Diffs&     
    DiffStats<SiteUserStats, 2>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<SiteUserStats, 2>::Diffs&     
    DiffStats<SiteUserStats, 2>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<SiteUserStats, 2>::Diffs     
    DiffStats<SiteUserStats, 2>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<SiteUserStats, 2>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<SiteUserStats, 2>::Diffs     
    DiffStats<SiteUserStats, 2>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<SiteUserStats, 2>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<SiteUserStats, 2>::Diffs::const_iterator    
    DiffStats<SiteUserStats, 2>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<SiteUserStats, 2>::Diffs::const_iterator    
    DiffStats<SiteUserStats, 2>::Diffs::end () const    
    {    
      return diffs + 2;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<SiteUserStats, 2>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<SiteUserStats, 2>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<SiteUserStats, 2>::Diffs::size () const    
    {    
      return 2;      
    }    
    inline    
    void    
    DiffStats<SiteUserStats, 2>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 2; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    SiteUserStats::unique_users () const    
    {    
      return values[SiteUserStats::UNIQUE_USERS];      
    }    
    inline    
    DiffStats<SiteUserStats, 2>::Diffs&     
    DiffStats<SiteUserStats, 2>::Diffs::unique_users (const stats_diff_type& value)    
    {    
      diffs[SiteUserStats::UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<SiteUserStats, 2>::Diffs::unique_users () const    
    {    
      return diffs[SiteUserStats::UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    SiteUserStats::control_sum () const    
    {    
      return values[SiteUserStats::CONTROL_SUM];      
    }    
    inline    
    DiffStats<SiteUserStats, 2>::Diffs&     
    DiffStats<SiteUserStats, 2>::Diffs::control_sum (const stats_diff_type& value)    
    {    
      diffs[SiteUserStats::CONTROL_SUM] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<SiteUserStats, 2>::Diffs::control_sum () const    
    {    
      return diffs[SiteUserStats::CONTROL_SUM];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_SITEUSERSTATS_HPP

