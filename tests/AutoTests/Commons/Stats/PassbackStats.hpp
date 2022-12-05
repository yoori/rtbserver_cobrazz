
#ifndef __AUTOTESTS_COMMONS_STATS_PASSBACKSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_PASSBACKSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class PassbackStats:    
      public DiffStats<PassbackStats, 1>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          REQUESTS          
        };        
        typedef DiffStats<PassbackStats, 1> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            AutoTest::Time sdate_;            
            bool sdate_used_;            
            bool sdate_null_;            
            int tag_id_;            
            bool tag_id_used_;            
            bool tag_id_null_;            
            int colo_id_;            
            bool colo_id_used_;            
            bool colo_id_null_;            
          public:          
            Key& sdate(const AutoTest::Time& value);            
            Key& sdate_set_null(bool is_null = true);            
            const AutoTest::Time& sdate() const;            
            bool sdate_used() const;            
            bool sdate_is_null() const;            
            Key& tag_id(const int& value);            
            Key& tag_id_set_null(bool is_null = true);            
            const int& tag_id() const;            
            bool tag_id_used() const;            
            bool tag_id_is_null() const;            
            Key& colo_id(const int& value);            
            Key& colo_id_set_null(bool is_null = true);            
            const int& colo_id() const;            
            bool colo_id_used() const;            
            bool colo_id_is_null() const;            
            Key ();            
            Key (            
              const AutoTest::Time& sdate,              
              const int& tag_id,              
              const int& colo_id              
            );            
        };        
        stats_value_type requests () const;        
        void print_idname (std::ostream& out) const;        
                
        PassbackStats (const Key& value);        
        PassbackStats (        
          const AutoTest::Time& sdate,          
          const int& tag_id,          
          const int& colo_id          
        );        
        PassbackStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const AutoTest::Time& sdate,          
          const int& tag_id,          
          const int& colo_id          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<PassbackStats, 1>::Diffs    
    {    
      public:      
        typedef stats_diff_type array_type[1];        
        typedef const stats_diff_type const_array_type[1];        
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
                
        Diffs& requests (const stats_diff_type& value);        
        const stats_diff_type& requests () const;        
      protected:      
        stats_diff_type diffs[1];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// PassbackStats    
    inline    
    PassbackStats::PassbackStats ()    
      :Base("PassbackStats")    
    {}    
    inline    
    PassbackStats::PassbackStats (const PassbackStats::Key& value)    
      :Base("PassbackStats")    
    {    
      key_ = value;      
    }    
    inline    
    PassbackStats::PassbackStats (    
      const AutoTest::Time& sdate,      
      const int& tag_id,      
      const int& colo_id      
    )    
      :Base("PassbackStats")    
    {    
      key_ = Key (      
        sdate,        
        tag_id,        
        colo_id        
      );      
    }    
    inline    
    PassbackStats::Key::Key ()    
      :sdate_(default_date()),tag_id_(0),colo_id_(0)    
    {    
      sdate_used_ = false;      
      sdate_null_ = false;      
      tag_id_used_ = false;      
      tag_id_null_ = false;      
      colo_id_used_ = false;      
      colo_id_null_ = false;      
    }    
    inline    
    PassbackStats::Key::Key (    
      const AutoTest::Time& sdate,      
      const int& tag_id,      
      const int& colo_id      
    )    
      :sdate_(sdate),tag_id_(tag_id),colo_id_(colo_id)    
    {    
      sdate_used_ = true;      
      sdate_null_ = false;      
      tag_id_used_ = true;      
      tag_id_null_ = false;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
    }    
    inline    
    PassbackStats::Key&    
    PassbackStats::Key::sdate(const AutoTest::Time& value)    
    {    
      sdate_ = value;      
      sdate_used_ = true;      
      sdate_null_ = false;      
      return *this;      
    }    
    inline    
    PassbackStats::Key&    
    PassbackStats::Key::sdate_set_null(bool is_null)    
    {    
      sdate_used_ = true;      
      sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    PassbackStats::Key::sdate() const    
    {    
      return sdate_;      
    }    
    inline    
    bool    
    PassbackStats::Key::sdate_used() const    
    {    
      return sdate_used_;      
    }    
    inline    
    bool    
    PassbackStats::Key::sdate_is_null() const    
    {    
      return sdate_null_;      
    }    
    inline    
    PassbackStats::Key&    
    PassbackStats::Key::tag_id(const int& value)    
    {    
      tag_id_ = value;      
      tag_id_used_ = true;      
      tag_id_null_ = false;      
      return *this;      
    }    
    inline    
    PassbackStats::Key&    
    PassbackStats::Key::tag_id_set_null(bool is_null)    
    {    
      tag_id_used_ = true;      
      tag_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    PassbackStats::Key::tag_id() const    
    {    
      return tag_id_;      
    }    
    inline    
    bool    
    PassbackStats::Key::tag_id_used() const    
    {    
      return tag_id_used_;      
    }    
    inline    
    bool    
    PassbackStats::Key::tag_id_is_null() const    
    {    
      return tag_id_null_;      
    }    
    inline    
    PassbackStats::Key&    
    PassbackStats::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    PassbackStats::Key&    
    PassbackStats::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    PassbackStats::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    PassbackStats::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    PassbackStats::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    PassbackStats::Key&    
    PassbackStats::key (    
      const AutoTest::Time& sdate,      
      const int& tag_id,      
      const int& colo_id      
    )    
    {    
      key_ = Key (      
        sdate,        
        tag_id,        
        colo_id        
      );      
      return key_;      
    }    
    inline    
    PassbackStats::Key&    
    PassbackStats::key ()    
    {    
      return key_;      
    }    
    inline    
    PassbackStats::Key&    
    PassbackStats::key (const PassbackStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<PassbackStats, 1>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<PassbackStats, 1>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<PassbackStats, 1>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<PassbackStats, 1>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<PassbackStats, 1>::Diffs&     
    DiffStats<PassbackStats, 1>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<PassbackStats, 1>::Diffs&     
    DiffStats<PassbackStats, 1>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<PassbackStats, 1>::Diffs&     
    DiffStats<PassbackStats, 1>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<PassbackStats, 1>::Diffs     
    DiffStats<PassbackStats, 1>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<PassbackStats, 1>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<PassbackStats, 1>::Diffs     
    DiffStats<PassbackStats, 1>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<PassbackStats, 1>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<PassbackStats, 1>::Diffs::const_iterator    
    DiffStats<PassbackStats, 1>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<PassbackStats, 1>::Diffs::const_iterator    
    DiffStats<PassbackStats, 1>::Diffs::end () const    
    {    
      return diffs + 1;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<PassbackStats, 1>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<PassbackStats, 1>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<PassbackStats, 1>::Diffs::size () const    
    {    
      return 1;      
    }    
    inline    
    void    
    DiffStats<PassbackStats, 1>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 1; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    PassbackStats::requests () const    
    {    
      return values[PassbackStats::REQUESTS];      
    }    
    inline    
    DiffStats<PassbackStats, 1>::Diffs&     
    DiffStats<PassbackStats, 1>::Diffs::requests (const stats_diff_type& value)    
    {    
      diffs[PassbackStats::REQUESTS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<PassbackStats, 1>::Diffs::requests () const    
    {    
      return diffs[PassbackStats::REQUESTS];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_PASSBACKSTATS_HPP

