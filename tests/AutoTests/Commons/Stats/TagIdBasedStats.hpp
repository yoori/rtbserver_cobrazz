
#ifndef __AUTOTESTS_COMMONS_STATS_TAGIDBASEDSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_TAGIDBASEDSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class TagIdBasedStats:    
      public DiffStats<TagIdBasedStats, 4>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          TAGAUCTIONSTATS_REQUESTS = 0,          
          PUBLISHERINVENTORY_IMPS,          
          PUBLISHERINVENTORY_REQUESTS,          
          PUBLISHERINVENTORY_REVENUE          
        };        
        typedef DiffStats<TagIdBasedStats, 4> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            int tag_id_;            
            bool tag_id_used_;            
            bool tag_id_null_;            
            int exclude_colo_;            
            bool exclude_colo_used_;            
            bool exclude_colo_null_;            
          public:          
            Key& tag_id(const int& value);            
            Key& tag_id_set_null(bool is_null = true);            
            const int& tag_id() const;            
            bool tag_id_used() const;            
            bool tag_id_is_null() const;            
            Key& exclude_colo(const int& value);            
            Key& exclude_colo_set_null(bool is_null = true);            
            const int& exclude_colo() const;            
            bool exclude_colo_used() const;            
            bool exclude_colo_is_null() const;            
            Key ();            
            Key (            
              const int& tag_id,              
              const int& exclude_colo              
            );            
        };        
        stats_value_type tagauctionstats_requests () const;        
        stats_value_type publisherinventory_imps () const;        
        stats_value_type publisherinventory_requests () const;        
        stats_value_type publisherinventory_revenue () const;        
        void print_idname (std::ostream& out) const;        
                
        TagIdBasedStats (const Key& value);        
        TagIdBasedStats (        
          const int& tag_id,          
          const int& exclude_colo          
        );        
        TagIdBasedStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& tag_id,          
          const int& exclude_colo          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<TagIdBasedStats, 4>::Diffs    
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
                
        Diffs& tagauctionstats_requests (const stats_diff_type& value);        
        const stats_diff_type& tagauctionstats_requests () const;        
        Diffs& publisherinventory_imps (const stats_diff_type& value);        
        const stats_diff_type& publisherinventory_imps () const;        
        Diffs& publisherinventory_requests (const stats_diff_type& value);        
        const stats_diff_type& publisherinventory_requests () const;        
        Diffs& publisherinventory_revenue (const stats_diff_type& value);        
        const stats_diff_type& publisherinventory_revenue () const;        
      protected:      
        stats_diff_type diffs[4];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// TagIdBasedStats    
    inline    
    TagIdBasedStats::TagIdBasedStats ()    
      :Base("TagIdBasedStats")    
    {}    
    inline    
    TagIdBasedStats::TagIdBasedStats (const TagIdBasedStats::Key& value)    
      :Base("TagIdBasedStats")    
    {    
      key_ = value;      
    }    
    inline    
    TagIdBasedStats::TagIdBasedStats (    
      const int& tag_id,      
      const int& exclude_colo      
    )    
      :Base("TagIdBasedStats")    
    {    
      key_ = Key (      
        tag_id,        
        exclude_colo        
      );      
    }    
    inline    
    TagIdBasedStats::Key::Key ()    
      :tag_id_(0)    
    {    
      tag_id_used_ = false;      
      tag_id_null_ = false;      
      exclude_colo_used_ = false;      
      exclude_colo_null_ = false;      
    }    
    inline    
    TagIdBasedStats::Key::Key (    
      const int& tag_id,      
      const int& exclude_colo      
    )    
      :tag_id_(tag_id)    
    {    
      tag_id_used_ = true;      
      tag_id_null_ = false;      
      exclude_colo_ = exclude_colo;      
      exclude_colo_used_ = true;      
      exclude_colo_null_ = false;      
    }    
    inline    
    TagIdBasedStats::Key&    
    TagIdBasedStats::Key::tag_id(const int& value)    
    {    
      tag_id_ = value;      
      tag_id_used_ = true;      
      tag_id_null_ = false;      
      return *this;      
    }    
    inline    
    TagIdBasedStats::Key&    
    TagIdBasedStats::Key::tag_id_set_null(bool is_null)    
    {    
      tag_id_used_ = true;      
      tag_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    TagIdBasedStats::Key::tag_id() const    
    {    
      return tag_id_;      
    }    
    inline    
    bool    
    TagIdBasedStats::Key::tag_id_used() const    
    {    
      return tag_id_used_;      
    }    
    inline    
    bool    
    TagIdBasedStats::Key::tag_id_is_null() const    
    {    
      return tag_id_null_;      
    }    
    inline    
    TagIdBasedStats::Key&    
    TagIdBasedStats::Key::exclude_colo(const int& value)    
    {    
      exclude_colo_ = value;      
      exclude_colo_used_ = true;      
      exclude_colo_null_ = false;      
      return *this;      
    }    
    inline    
    TagIdBasedStats::Key&    
    TagIdBasedStats::Key::exclude_colo_set_null(bool is_null)    
    {    
      exclude_colo_used_ = true;      
      exclude_colo_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    TagIdBasedStats::Key::exclude_colo() const    
    {    
      return exclude_colo_;      
    }    
    inline    
    bool    
    TagIdBasedStats::Key::exclude_colo_used() const    
    {    
      return exclude_colo_used_;      
    }    
    inline    
    bool    
    TagIdBasedStats::Key::exclude_colo_is_null() const    
    {    
      return exclude_colo_null_;      
    }    
    inline    
    TagIdBasedStats::Key&    
    TagIdBasedStats::key (    
      const int& tag_id,      
      const int& exclude_colo      
    )    
    {    
      key_ = Key (      
        tag_id,        
        exclude_colo        
      );      
      return key_;      
    }    
    inline    
    TagIdBasedStats::Key&    
    TagIdBasedStats::key ()    
    {    
      return key_;      
    }    
    inline    
    TagIdBasedStats::Key&    
    TagIdBasedStats::key (const TagIdBasedStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<TagIdBasedStats, 4>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<TagIdBasedStats, 4>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<TagIdBasedStats, 4>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<TagIdBasedStats, 4>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<TagIdBasedStats, 4>::Diffs&     
    DiffStats<TagIdBasedStats, 4>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<TagIdBasedStats, 4>::Diffs&     
    DiffStats<TagIdBasedStats, 4>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<TagIdBasedStats, 4>::Diffs&     
    DiffStats<TagIdBasedStats, 4>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<TagIdBasedStats, 4>::Diffs     
    DiffStats<TagIdBasedStats, 4>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<TagIdBasedStats, 4>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<TagIdBasedStats, 4>::Diffs     
    DiffStats<TagIdBasedStats, 4>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<TagIdBasedStats, 4>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<TagIdBasedStats, 4>::Diffs::const_iterator    
    DiffStats<TagIdBasedStats, 4>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<TagIdBasedStats, 4>::Diffs::const_iterator    
    DiffStats<TagIdBasedStats, 4>::Diffs::end () const    
    {    
      return diffs + 4;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<TagIdBasedStats, 4>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<TagIdBasedStats, 4>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<TagIdBasedStats, 4>::Diffs::size () const    
    {    
      return 4;      
    }    
    inline    
    void    
    DiffStats<TagIdBasedStats, 4>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 4; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    TagIdBasedStats::tagauctionstats_requests () const    
    {    
      return values[TagIdBasedStats::TAGAUCTIONSTATS_REQUESTS];      
    }    
    inline    
    DiffStats<TagIdBasedStats, 4>::Diffs&     
    DiffStats<TagIdBasedStats, 4>::Diffs::tagauctionstats_requests (const stats_diff_type& value)    
    {    
      diffs[TagIdBasedStats::TAGAUCTIONSTATS_REQUESTS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<TagIdBasedStats, 4>::Diffs::tagauctionstats_requests () const    
    {    
      return diffs[TagIdBasedStats::TAGAUCTIONSTATS_REQUESTS];      
    }    
    inline    
    stats_value_type    
    TagIdBasedStats::publisherinventory_imps () const    
    {    
      return values[TagIdBasedStats::PUBLISHERINVENTORY_IMPS];      
    }    
    inline    
    DiffStats<TagIdBasedStats, 4>::Diffs&     
    DiffStats<TagIdBasedStats, 4>::Diffs::publisherinventory_imps (const stats_diff_type& value)    
    {    
      diffs[TagIdBasedStats::PUBLISHERINVENTORY_IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<TagIdBasedStats, 4>::Diffs::publisherinventory_imps () const    
    {    
      return diffs[TagIdBasedStats::PUBLISHERINVENTORY_IMPS];      
    }    
    inline    
    stats_value_type    
    TagIdBasedStats::publisherinventory_requests () const    
    {    
      return values[TagIdBasedStats::PUBLISHERINVENTORY_REQUESTS];      
    }    
    inline    
    DiffStats<TagIdBasedStats, 4>::Diffs&     
    DiffStats<TagIdBasedStats, 4>::Diffs::publisherinventory_requests (const stats_diff_type& value)    
    {    
      diffs[TagIdBasedStats::PUBLISHERINVENTORY_REQUESTS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<TagIdBasedStats, 4>::Diffs::publisherinventory_requests () const    
    {    
      return diffs[TagIdBasedStats::PUBLISHERINVENTORY_REQUESTS];      
    }    
    inline    
    stats_value_type    
    TagIdBasedStats::publisherinventory_revenue () const    
    {    
      return values[TagIdBasedStats::PUBLISHERINVENTORY_REVENUE];      
    }    
    inline    
    DiffStats<TagIdBasedStats, 4>::Diffs&     
    DiffStats<TagIdBasedStats, 4>::Diffs::publisherinventory_revenue (const stats_diff_type& value)    
    {    
      diffs[TagIdBasedStats::PUBLISHERINVENTORY_REVENUE] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<TagIdBasedStats, 4>::Diffs::publisherinventory_revenue () const    
    {    
      return diffs[TagIdBasedStats::PUBLISHERINVENTORY_REVENUE];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_TAGIDBASEDSTATS_HPP

