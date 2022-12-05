
#ifndef __AUTOTESTS_COMMONS_STATS_PAGELOADSDAILY_HPP
#define __AUTOTESTS_COMMONS_STATS_PAGELOADSDAILY_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class PageLoadsDaily:    
      public DiffStats<PageLoadsDaily, 6>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          PAGE_LOADS = 0,          
          PAGE_LOADS_MIN,          
          PAGE_LOADS_MAX,          
          UTILIZED_PAGE_LOADS,          
          UTILIZED_PAGE_LOADS_MIN,          
          UTILIZED_PAGE_LOADS_MAX          
        };        
        typedef DiffStats<PageLoadsDaily, 6> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            AutoTest::Time country_sdate_;            
            bool country_sdate_used_;            
            bool country_sdate_null_;            
            int colo_id_;            
            bool colo_id_used_;            
            bool colo_id_null_;            
            int site_id_;            
            bool site_id_used_;            
            bool site_id_null_;            
            std::string country_code_;            
            bool country_code_used_;            
            bool country_code_null_;            
            std::string tag_group_;            
            bool tag_group_used_;            
            bool tag_group_null_;            
          public:          
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
            Key& site_id(const int& value);            
            Key& site_id_set_null(bool is_null = true);            
            const int& site_id() const;            
            bool site_id_used() const;            
            bool site_id_is_null() const;            
            Key& country_code(const std::string& value);            
            Key& country_code_set_null(bool is_null = true);            
            const std::string& country_code() const;            
            bool country_code_used() const;            
            bool country_code_is_null() const;            
            Key& tag_group(const std::string& value);            
            Key& tag_group_set_null(bool is_null = true);            
            const std::string& tag_group() const;            
            bool tag_group_used() const;            
            bool tag_group_is_null() const;            
            Key ();            
            Key (            
              const AutoTest::Time& country_sdate,              
              const int& colo_id,              
              const int& site_id,              
              const std::string& country_code,              
              const std::string& tag_group              
            );            
        };        
        stats_value_type page_loads () const;        
        stats_value_type page_loads_min () const;        
        stats_value_type page_loads_max () const;        
        stats_value_type utilized_page_loads () const;        
        stats_value_type utilized_page_loads_min () const;        
        stats_value_type utilized_page_loads_max () const;        
        void print_idname (std::ostream& out) const;        
                
        PageLoadsDaily (const Key& value);        
        PageLoadsDaily (        
          const AutoTest::Time& country_sdate,          
          const int& colo_id,          
          const int& site_id,          
          const std::string& country_code,          
          const std::string& tag_group          
        );        
        PageLoadsDaily ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const AutoTest::Time& country_sdate,          
          const int& colo_id,          
          const int& site_id,          
          const std::string& country_code,          
          const std::string& tag_group          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<PageLoadsDaily, 6>::Diffs    
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
                
        Diffs& page_loads (const stats_diff_type& value);        
        const stats_diff_type& page_loads () const;        
        Diffs& page_loads_min (const stats_diff_type& value);        
        const stats_diff_type& page_loads_min () const;        
        Diffs& page_loads_max (const stats_diff_type& value);        
        const stats_diff_type& page_loads_max () const;        
        Diffs& utilized_page_loads (const stats_diff_type& value);        
        const stats_diff_type& utilized_page_loads () const;        
        Diffs& utilized_page_loads_min (const stats_diff_type& value);        
        const stats_diff_type& utilized_page_loads_min () const;        
        Diffs& utilized_page_loads_max (const stats_diff_type& value);        
        const stats_diff_type& utilized_page_loads_max () const;        
      protected:      
        stats_diff_type diffs[6];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// PageLoadsDaily    
    inline    
    PageLoadsDaily::PageLoadsDaily ()    
      :Base("PageLoadsDaily")    
    {}    
    inline    
    PageLoadsDaily::PageLoadsDaily (const PageLoadsDaily::Key& value)    
      :Base("PageLoadsDaily")    
    {    
      key_ = value;      
    }    
    inline    
    PageLoadsDaily::PageLoadsDaily (    
      const AutoTest::Time& country_sdate,      
      const int& colo_id,      
      const int& site_id,      
      const std::string& country_code,      
      const std::string& tag_group      
    )    
      :Base("PageLoadsDaily")    
    {    
      key_ = Key (      
        country_sdate,        
        colo_id,        
        site_id,        
        country_code,        
        tag_group        
      );      
    }    
    inline    
    PageLoadsDaily::Key::Key ()    
      :country_sdate_(default_date()),colo_id_(0),site_id_(0)    
    {    
      country_sdate_used_ = false;      
      country_sdate_null_ = false;      
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      site_id_used_ = false;      
      site_id_null_ = false;      
      country_code_used_ = false;      
      country_code_null_ = false;      
      tag_group_used_ = false;      
      tag_group_null_ = false;      
    }    
    inline    
    PageLoadsDaily::Key::Key (    
      const AutoTest::Time& country_sdate,      
      const int& colo_id,      
      const int& site_id,      
      const std::string& country_code,      
      const std::string& tag_group      
    )    
      :country_sdate_(country_sdate),colo_id_(colo_id),site_id_(site_id)    
    {    
      country_sdate_used_ = true;      
      country_sdate_null_ = false;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      site_id_used_ = true;      
      site_id_null_ = false;      
      country_code_ = country_code;      
      country_code_used_ = true;      
      country_code_null_ = false;      
      tag_group_ = tag_group;      
      tag_group_used_ = true;      
      tag_group_null_ = false;      
    }    
    inline    
    PageLoadsDaily::Key&    
    PageLoadsDaily::Key::country_sdate(const AutoTest::Time& value)    
    {    
      country_sdate_ = value;      
      country_sdate_used_ = true;      
      country_sdate_null_ = false;      
      return *this;      
    }    
    inline    
    PageLoadsDaily::Key&    
    PageLoadsDaily::Key::country_sdate_set_null(bool is_null)    
    {    
      country_sdate_used_ = true;      
      country_sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    PageLoadsDaily::Key::country_sdate() const    
    {    
      return country_sdate_;      
    }    
    inline    
    bool    
    PageLoadsDaily::Key::country_sdate_used() const    
    {    
      return country_sdate_used_;      
    }    
    inline    
    bool    
    PageLoadsDaily::Key::country_sdate_is_null() const    
    {    
      return country_sdate_null_;      
    }    
    inline    
    PageLoadsDaily::Key&    
    PageLoadsDaily::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    PageLoadsDaily::Key&    
    PageLoadsDaily::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    PageLoadsDaily::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    PageLoadsDaily::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    PageLoadsDaily::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    PageLoadsDaily::Key&    
    PageLoadsDaily::Key::site_id(const int& value)    
    {    
      site_id_ = value;      
      site_id_used_ = true;      
      site_id_null_ = false;      
      return *this;      
    }    
    inline    
    PageLoadsDaily::Key&    
    PageLoadsDaily::Key::site_id_set_null(bool is_null)    
    {    
      site_id_used_ = true;      
      site_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    PageLoadsDaily::Key::site_id() const    
    {    
      return site_id_;      
    }    
    inline    
    bool    
    PageLoadsDaily::Key::site_id_used() const    
    {    
      return site_id_used_;      
    }    
    inline    
    bool    
    PageLoadsDaily::Key::site_id_is_null() const    
    {    
      return site_id_null_;      
    }    
    inline    
    PageLoadsDaily::Key&    
    PageLoadsDaily::Key::country_code(const std::string& value)    
    {    
      country_code_ = value;      
      country_code_used_ = true;      
      country_code_null_ = false;      
      return *this;      
    }    
    inline    
    PageLoadsDaily::Key&    
    PageLoadsDaily::Key::country_code_set_null(bool is_null)    
    {    
      country_code_used_ = true;      
      country_code_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    PageLoadsDaily::Key::country_code() const    
    {    
      return country_code_;      
    }    
    inline    
    bool    
    PageLoadsDaily::Key::country_code_used() const    
    {    
      return country_code_used_;      
    }    
    inline    
    bool    
    PageLoadsDaily::Key::country_code_is_null() const    
    {    
      return country_code_null_;      
    }    
    inline    
    PageLoadsDaily::Key&    
    PageLoadsDaily::Key::tag_group(const std::string& value)    
    {    
      tag_group_ = value;      
      tag_group_used_ = true;      
      tag_group_null_ = false;      
      return *this;      
    }    
    inline    
    PageLoadsDaily::Key&    
    PageLoadsDaily::Key::tag_group_set_null(bool is_null)    
    {    
      tag_group_used_ = true;      
      tag_group_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    PageLoadsDaily::Key::tag_group() const    
    {    
      return tag_group_;      
    }    
    inline    
    bool    
    PageLoadsDaily::Key::tag_group_used() const    
    {    
      return tag_group_used_;      
    }    
    inline    
    bool    
    PageLoadsDaily::Key::tag_group_is_null() const    
    {    
      return tag_group_null_;      
    }    
    inline    
    PageLoadsDaily::Key&    
    PageLoadsDaily::key (    
      const AutoTest::Time& country_sdate,      
      const int& colo_id,      
      const int& site_id,      
      const std::string& country_code,      
      const std::string& tag_group      
    )    
    {    
      key_ = Key (      
        country_sdate,        
        colo_id,        
        site_id,        
        country_code,        
        tag_group        
      );      
      return key_;      
    }    
    inline    
    PageLoadsDaily::Key&    
    PageLoadsDaily::key ()    
    {    
      return key_;      
    }    
    inline    
    PageLoadsDaily::Key&    
    PageLoadsDaily::key (const PageLoadsDaily::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<PageLoadsDaily, 6>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 6; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<PageLoadsDaily, 6>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 6; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<PageLoadsDaily, 6>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 6; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<PageLoadsDaily, 6>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<PageLoadsDaily, 6>::Diffs&     
    DiffStats<PageLoadsDaily, 6>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 6; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<PageLoadsDaily, 6>::Diffs&     
    DiffStats<PageLoadsDaily, 6>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 6; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<PageLoadsDaily, 6>::Diffs&     
    DiffStats<PageLoadsDaily, 6>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 6; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<PageLoadsDaily, 6>::Diffs     
    DiffStats<PageLoadsDaily, 6>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<PageLoadsDaily, 6>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<PageLoadsDaily, 6>::Diffs     
    DiffStats<PageLoadsDaily, 6>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<PageLoadsDaily, 6>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<PageLoadsDaily, 6>::Diffs::const_iterator    
    DiffStats<PageLoadsDaily, 6>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<PageLoadsDaily, 6>::Diffs::const_iterator    
    DiffStats<PageLoadsDaily, 6>::Diffs::end () const    
    {    
      return diffs + 6;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<PageLoadsDaily, 6>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<PageLoadsDaily, 6>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<PageLoadsDaily, 6>::Diffs::size () const    
    {    
      return 6;      
    }    
    inline    
    void    
    DiffStats<PageLoadsDaily, 6>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 6; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    PageLoadsDaily::page_loads () const    
    {    
      return values[PageLoadsDaily::PAGE_LOADS];      
    }    
    inline    
    DiffStats<PageLoadsDaily, 6>::Diffs&     
    DiffStats<PageLoadsDaily, 6>::Diffs::page_loads (const stats_diff_type& value)    
    {    
      diffs[PageLoadsDaily::PAGE_LOADS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<PageLoadsDaily, 6>::Diffs::page_loads () const    
    {    
      return diffs[PageLoadsDaily::PAGE_LOADS];      
    }    
    inline    
    stats_value_type    
    PageLoadsDaily::page_loads_min () const    
    {    
      return values[PageLoadsDaily::PAGE_LOADS_MIN];      
    }    
    inline    
    DiffStats<PageLoadsDaily, 6>::Diffs&     
    DiffStats<PageLoadsDaily, 6>::Diffs::page_loads_min (const stats_diff_type& value)    
    {    
      diffs[PageLoadsDaily::PAGE_LOADS_MIN] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<PageLoadsDaily, 6>::Diffs::page_loads_min () const    
    {    
      return diffs[PageLoadsDaily::PAGE_LOADS_MIN];      
    }    
    inline    
    stats_value_type    
    PageLoadsDaily::page_loads_max () const    
    {    
      return values[PageLoadsDaily::PAGE_LOADS_MAX];      
    }    
    inline    
    DiffStats<PageLoadsDaily, 6>::Diffs&     
    DiffStats<PageLoadsDaily, 6>::Diffs::page_loads_max (const stats_diff_type& value)    
    {    
      diffs[PageLoadsDaily::PAGE_LOADS_MAX] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<PageLoadsDaily, 6>::Diffs::page_loads_max () const    
    {    
      return diffs[PageLoadsDaily::PAGE_LOADS_MAX];      
    }    
    inline    
    stats_value_type    
    PageLoadsDaily::utilized_page_loads () const    
    {    
      return values[PageLoadsDaily::UTILIZED_PAGE_LOADS];      
    }    
    inline    
    DiffStats<PageLoadsDaily, 6>::Diffs&     
    DiffStats<PageLoadsDaily, 6>::Diffs::utilized_page_loads (const stats_diff_type& value)    
    {    
      diffs[PageLoadsDaily::UTILIZED_PAGE_LOADS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<PageLoadsDaily, 6>::Diffs::utilized_page_loads () const    
    {    
      return diffs[PageLoadsDaily::UTILIZED_PAGE_LOADS];      
    }    
    inline    
    stats_value_type    
    PageLoadsDaily::utilized_page_loads_min () const    
    {    
      return values[PageLoadsDaily::UTILIZED_PAGE_LOADS_MIN];      
    }    
    inline    
    DiffStats<PageLoadsDaily, 6>::Diffs&     
    DiffStats<PageLoadsDaily, 6>::Diffs::utilized_page_loads_min (const stats_diff_type& value)    
    {    
      diffs[PageLoadsDaily::UTILIZED_PAGE_LOADS_MIN] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<PageLoadsDaily, 6>::Diffs::utilized_page_loads_min () const    
    {    
      return diffs[PageLoadsDaily::UTILIZED_PAGE_LOADS_MIN];      
    }    
    inline    
    stats_value_type    
    PageLoadsDaily::utilized_page_loads_max () const    
    {    
      return values[PageLoadsDaily::UTILIZED_PAGE_LOADS_MAX];      
    }    
    inline    
    DiffStats<PageLoadsDaily, 6>::Diffs&     
    DiffStats<PageLoadsDaily, 6>::Diffs::utilized_page_loads_max (const stats_diff_type& value)    
    {    
      diffs[PageLoadsDaily::UTILIZED_PAGE_LOADS_MAX] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<PageLoadsDaily, 6>::Diffs::utilized_page_loads_max () const    
    {    
      return diffs[PageLoadsDaily::UTILIZED_PAGE_LOADS_MAX];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_PAGELOADSDAILY_HPP

