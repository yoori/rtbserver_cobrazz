
#ifndef __AUTOTESTS_COMMONS_STATS_SITEIDBASEDSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_SITEIDBASEDSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class SiteIdBasedStats:    
      public DiffStats<SiteIdBasedStats, 3>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          SITEUSERSTATS_UNIQUE_USERS = 0,          
          PAGELOADSDAILY_PAGE_LOADS,          
          PAGELOADSDAILY_UTILIZED_PAGE_LOADS          
        };        
        typedef DiffStats<SiteIdBasedStats, 3> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            int site_id_;            
            bool site_id_used_;            
            bool site_id_null_;            
            int exclude_colo_;            
            bool exclude_colo_used_;            
            bool exclude_colo_null_;            
          public:          
            Key& site_id(const int& value);            
            Key& site_id_set_null(bool is_null = true);            
            const int& site_id() const;            
            bool site_id_used() const;            
            bool site_id_is_null() const;            
            Key& exclude_colo(const int& value);            
            Key& exclude_colo_set_null(bool is_null = true);            
            const int& exclude_colo() const;            
            bool exclude_colo_used() const;            
            bool exclude_colo_is_null() const;            
            Key ();            
            Key (            
              const int& site_id,              
              const int& exclude_colo              
            );            
        };        
        stats_value_type siteuserstats_unique_users () const;        
        stats_value_type pageloadsdaily_page_loads () const;        
        stats_value_type pageloadsdaily_utilized_page_loads () const;        
        void print_idname (std::ostream& out) const;        
                
        SiteIdBasedStats (const Key& value);        
        SiteIdBasedStats (        
          const int& site_id,          
          const int& exclude_colo          
        );        
        SiteIdBasedStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& site_id,          
          const int& exclude_colo          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<SiteIdBasedStats, 3>::Diffs    
    {    
      public:      
        typedef stats_diff_type array_type[3];        
        typedef const stats_diff_type const_array_type[3];        
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
                
        Diffs& siteuserstats_unique_users (const stats_diff_type& value);        
        const stats_diff_type& siteuserstats_unique_users () const;        
        Diffs& pageloadsdaily_page_loads (const stats_diff_type& value);        
        const stats_diff_type& pageloadsdaily_page_loads () const;        
        Diffs& pageloadsdaily_utilized_page_loads (const stats_diff_type& value);        
        const stats_diff_type& pageloadsdaily_utilized_page_loads () const;        
      protected:      
        stats_diff_type diffs[3];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// SiteIdBasedStats    
    inline    
    SiteIdBasedStats::SiteIdBasedStats ()    
      :Base("SiteIdBasedStats")    
    {}    
    inline    
    SiteIdBasedStats::SiteIdBasedStats (const SiteIdBasedStats::Key& value)    
      :Base("SiteIdBasedStats")    
    {    
      key_ = value;      
    }    
    inline    
    SiteIdBasedStats::SiteIdBasedStats (    
      const int& site_id,      
      const int& exclude_colo      
    )    
      :Base("SiteIdBasedStats")    
    {    
      key_ = Key (      
        site_id,        
        exclude_colo        
      );      
    }    
    inline    
    SiteIdBasedStats::Key::Key ()    
      :site_id_(0)    
    {    
      site_id_used_ = false;      
      site_id_null_ = false;      
      exclude_colo_used_ = false;      
      exclude_colo_null_ = false;      
    }    
    inline    
    SiteIdBasedStats::Key::Key (    
      const int& site_id,      
      const int& exclude_colo      
    )    
      :site_id_(site_id)    
    {    
      site_id_used_ = true;      
      site_id_null_ = false;      
      exclude_colo_ = exclude_colo;      
      exclude_colo_used_ = true;      
      exclude_colo_null_ = false;      
    }    
    inline    
    SiteIdBasedStats::Key&    
    SiteIdBasedStats::Key::site_id(const int& value)    
    {    
      site_id_ = value;      
      site_id_used_ = true;      
      site_id_null_ = false;      
      return *this;      
    }    
    inline    
    SiteIdBasedStats::Key&    
    SiteIdBasedStats::Key::site_id_set_null(bool is_null)    
    {    
      site_id_used_ = true;      
      site_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    SiteIdBasedStats::Key::site_id() const    
    {    
      return site_id_;      
    }    
    inline    
    bool    
    SiteIdBasedStats::Key::site_id_used() const    
    {    
      return site_id_used_;      
    }    
    inline    
    bool    
    SiteIdBasedStats::Key::site_id_is_null() const    
    {    
      return site_id_null_;      
    }    
    inline    
    SiteIdBasedStats::Key&    
    SiteIdBasedStats::Key::exclude_colo(const int& value)    
    {    
      exclude_colo_ = value;      
      exclude_colo_used_ = true;      
      exclude_colo_null_ = false;      
      return *this;      
    }    
    inline    
    SiteIdBasedStats::Key&    
    SiteIdBasedStats::Key::exclude_colo_set_null(bool is_null)    
    {    
      exclude_colo_used_ = true;      
      exclude_colo_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    SiteIdBasedStats::Key::exclude_colo() const    
    {    
      return exclude_colo_;      
    }    
    inline    
    bool    
    SiteIdBasedStats::Key::exclude_colo_used() const    
    {    
      return exclude_colo_used_;      
    }    
    inline    
    bool    
    SiteIdBasedStats::Key::exclude_colo_is_null() const    
    {    
      return exclude_colo_null_;      
    }    
    inline    
    SiteIdBasedStats::Key&    
    SiteIdBasedStats::key (    
      const int& site_id,      
      const int& exclude_colo      
    )    
    {    
      key_ = Key (      
        site_id,        
        exclude_colo        
      );      
      return key_;      
    }    
    inline    
    SiteIdBasedStats::Key&    
    SiteIdBasedStats::key ()    
    {    
      return key_;      
    }    
    inline    
    SiteIdBasedStats::Key&    
    SiteIdBasedStats::key (const SiteIdBasedStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<SiteIdBasedStats, 3>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<SiteIdBasedStats, 3>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<SiteIdBasedStats, 3>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<SiteIdBasedStats, 3>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<SiteIdBasedStats, 3>::Diffs&     
    DiffStats<SiteIdBasedStats, 3>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<SiteIdBasedStats, 3>::Diffs&     
    DiffStats<SiteIdBasedStats, 3>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<SiteIdBasedStats, 3>::Diffs&     
    DiffStats<SiteIdBasedStats, 3>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<SiteIdBasedStats, 3>::Diffs     
    DiffStats<SiteIdBasedStats, 3>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<SiteIdBasedStats, 3>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<SiteIdBasedStats, 3>::Diffs     
    DiffStats<SiteIdBasedStats, 3>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<SiteIdBasedStats, 3>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<SiteIdBasedStats, 3>::Diffs::const_iterator    
    DiffStats<SiteIdBasedStats, 3>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<SiteIdBasedStats, 3>::Diffs::const_iterator    
    DiffStats<SiteIdBasedStats, 3>::Diffs::end () const    
    {    
      return diffs + 3;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<SiteIdBasedStats, 3>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<SiteIdBasedStats, 3>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<SiteIdBasedStats, 3>::Diffs::size () const    
    {    
      return 3;      
    }    
    inline    
    void    
    DiffStats<SiteIdBasedStats, 3>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 3; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    SiteIdBasedStats::siteuserstats_unique_users () const    
    {    
      return values[SiteIdBasedStats::SITEUSERSTATS_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<SiteIdBasedStats, 3>::Diffs&     
    DiffStats<SiteIdBasedStats, 3>::Diffs::siteuserstats_unique_users (const stats_diff_type& value)    
    {    
      diffs[SiteIdBasedStats::SITEUSERSTATS_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<SiteIdBasedStats, 3>::Diffs::siteuserstats_unique_users () const    
    {    
      return diffs[SiteIdBasedStats::SITEUSERSTATS_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    SiteIdBasedStats::pageloadsdaily_page_loads () const    
    {    
      return values[SiteIdBasedStats::PAGELOADSDAILY_PAGE_LOADS];      
    }    
    inline    
    DiffStats<SiteIdBasedStats, 3>::Diffs&     
    DiffStats<SiteIdBasedStats, 3>::Diffs::pageloadsdaily_page_loads (const stats_diff_type& value)    
    {    
      diffs[SiteIdBasedStats::PAGELOADSDAILY_PAGE_LOADS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<SiteIdBasedStats, 3>::Diffs::pageloadsdaily_page_loads () const    
    {    
      return diffs[SiteIdBasedStats::PAGELOADSDAILY_PAGE_LOADS];      
    }    
    inline    
    stats_value_type    
    SiteIdBasedStats::pageloadsdaily_utilized_page_loads () const    
    {    
      return values[SiteIdBasedStats::PAGELOADSDAILY_UTILIZED_PAGE_LOADS];      
    }    
    inline    
    DiffStats<SiteIdBasedStats, 3>::Diffs&     
    DiffStats<SiteIdBasedStats, 3>::Diffs::pageloadsdaily_utilized_page_loads (const stats_diff_type& value)    
    {    
      diffs[SiteIdBasedStats::PAGELOADSDAILY_UTILIZED_PAGE_LOADS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<SiteIdBasedStats, 3>::Diffs::pageloadsdaily_utilized_page_loads () const    
    {    
      return diffs[SiteIdBasedStats::PAGELOADSDAILY_UTILIZED_PAGE_LOADS];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_SITEIDBASEDSTATS_HPP

