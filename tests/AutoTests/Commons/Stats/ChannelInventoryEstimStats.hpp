
#ifndef __AUTOTESTS_COMMONS_STATS_CHANNELINVENTORYESTIMSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_CHANNELINVENTORYESTIMSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class ChannelInventoryEstimStats:    
      public DiffStats<ChannelInventoryEstimStats, 2>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          USERS_REGULAR = 0,          
          USERS_FROM_NOW          
        };        
        typedef DiffStats<ChannelInventoryEstimStats, 2> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            AutoTest::Time sdate_;            
            bool sdate_used_;            
            bool sdate_null_;            
            int colo_id_;            
            bool colo_id_used_;            
            bool colo_id_null_;            
            int channel_id_;            
            bool channel_id_used_;            
            bool channel_id_null_;            
            double match_level_;            
            bool match_level_used_;            
            bool match_level_null_;            
          public:          
            Key& sdate(const AutoTest::Time& value);            
            Key& sdate_set_null(bool is_null = true);            
            const AutoTest::Time& sdate() const;            
            bool sdate_used() const;            
            bool sdate_is_null() const;            
            Key& colo_id(const int& value);            
            Key& colo_id_set_null(bool is_null = true);            
            const int& colo_id() const;            
            bool colo_id_used() const;            
            bool colo_id_is_null() const;            
            Key& channel_id(const int& value);            
            Key& channel_id_set_null(bool is_null = true);            
            const int& channel_id() const;            
            bool channel_id_used() const;            
            bool channel_id_is_null() const;            
            Key& match_level(const double& value);            
            Key& match_level_set_null(bool is_null = true);            
            const double& match_level() const;            
            bool match_level_used() const;            
            bool match_level_is_null() const;            
            Key ();            
            Key (            
              const AutoTest::Time& sdate,              
              const int& colo_id,              
              const int& channel_id,              
              const double& match_level              
            );            
        };        
        stats_value_type users_regular () const;        
        stats_value_type users_from_now () const;        
        void print_idname (std::ostream& out) const;        
                
        ChannelInventoryEstimStats (const Key& value);        
        ChannelInventoryEstimStats (        
          const AutoTest::Time& sdate,          
          const int& colo_id,          
          const int& channel_id,          
          const double& match_level          
        );        
        ChannelInventoryEstimStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const AutoTest::Time& sdate,          
          const int& colo_id,          
          const int& channel_id,          
          const double& match_level          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<ChannelInventoryEstimStats, 2>::Diffs    
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
                
        Diffs& users_regular (const stats_diff_type& value);        
        const stats_diff_type& users_regular () const;        
        Diffs& users_from_now (const stats_diff_type& value);        
        const stats_diff_type& users_from_now () const;        
      protected:      
        stats_diff_type diffs[2];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ChannelInventoryEstimStats    
    inline    
    ChannelInventoryEstimStats::ChannelInventoryEstimStats ()    
      :Base("ChannelInventoryEstimStats")    
    {}    
    inline    
    ChannelInventoryEstimStats::ChannelInventoryEstimStats (const ChannelInventoryEstimStats::Key& value)    
      :Base("ChannelInventoryEstimStats")    
    {    
      key_ = value;      
    }    
    inline    
    ChannelInventoryEstimStats::ChannelInventoryEstimStats (    
      const AutoTest::Time& sdate,      
      const int& colo_id,      
      const int& channel_id,      
      const double& match_level      
    )    
      :Base("ChannelInventoryEstimStats")    
    {    
      key_ = Key (      
        sdate,        
        colo_id,        
        channel_id,        
        match_level        
      );      
    }    
    inline    
    ChannelInventoryEstimStats::Key::Key ()    
      :sdate_(default_date()),colo_id_(0),channel_id_(0),match_level_(0)    
    {    
      sdate_used_ = false;      
      sdate_null_ = false;      
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      channel_id_used_ = false;      
      channel_id_null_ = false;      
      match_level_used_ = false;      
      match_level_null_ = false;      
    }    
    inline    
    ChannelInventoryEstimStats::Key::Key (    
      const AutoTest::Time& sdate,      
      const int& colo_id,      
      const int& channel_id,      
      const double& match_level      
    )    
      :sdate_(sdate),colo_id_(colo_id),channel_id_(channel_id),match_level_(match_level)    
    {    
      sdate_used_ = true;      
      sdate_null_ = false;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      channel_id_used_ = true;      
      channel_id_null_ = false;      
      match_level_used_ = true;      
      match_level_null_ = false;      
    }    
    inline    
    ChannelInventoryEstimStats::Key&    
    ChannelInventoryEstimStats::Key::sdate(const AutoTest::Time& value)    
    {    
      sdate_ = value;      
      sdate_used_ = true;      
      sdate_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelInventoryEstimStats::Key&    
    ChannelInventoryEstimStats::Key::sdate_set_null(bool is_null)    
    {    
      sdate_used_ = true;      
      sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    ChannelInventoryEstimStats::Key::sdate() const    
    {    
      return sdate_;      
    }    
    inline    
    bool    
    ChannelInventoryEstimStats::Key::sdate_used() const    
    {    
      return sdate_used_;      
    }    
    inline    
    bool    
    ChannelInventoryEstimStats::Key::sdate_is_null() const    
    {    
      return sdate_null_;      
    }    
    inline    
    ChannelInventoryEstimStats::Key&    
    ChannelInventoryEstimStats::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelInventoryEstimStats::Key&    
    ChannelInventoryEstimStats::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelInventoryEstimStats::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    ChannelInventoryEstimStats::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    ChannelInventoryEstimStats::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    ChannelInventoryEstimStats::Key&    
    ChannelInventoryEstimStats::Key::channel_id(const int& value)    
    {    
      channel_id_ = value;      
      channel_id_used_ = true;      
      channel_id_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelInventoryEstimStats::Key&    
    ChannelInventoryEstimStats::Key::channel_id_set_null(bool is_null)    
    {    
      channel_id_used_ = true;      
      channel_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelInventoryEstimStats::Key::channel_id() const    
    {    
      return channel_id_;      
    }    
    inline    
    bool    
    ChannelInventoryEstimStats::Key::channel_id_used() const    
    {    
      return channel_id_used_;      
    }    
    inline    
    bool    
    ChannelInventoryEstimStats::Key::channel_id_is_null() const    
    {    
      return channel_id_null_;      
    }    
    inline    
    ChannelInventoryEstimStats::Key&    
    ChannelInventoryEstimStats::Key::match_level(const double& value)    
    {    
      match_level_ = value;      
      match_level_used_ = true;      
      match_level_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelInventoryEstimStats::Key&    
    ChannelInventoryEstimStats::Key::match_level_set_null(bool is_null)    
    {    
      match_level_used_ = true;      
      match_level_null_ = is_null;      
      return *this;      
    }    
    inline    
    const double&    
    ChannelInventoryEstimStats::Key::match_level() const    
    {    
      return match_level_;      
    }    
    inline    
    bool    
    ChannelInventoryEstimStats::Key::match_level_used() const    
    {    
      return match_level_used_;      
    }    
    inline    
    bool    
    ChannelInventoryEstimStats::Key::match_level_is_null() const    
    {    
      return match_level_null_;      
    }    
    inline    
    ChannelInventoryEstimStats::Key&    
    ChannelInventoryEstimStats::key (    
      const AutoTest::Time& sdate,      
      const int& colo_id,      
      const int& channel_id,      
      const double& match_level      
    )    
    {    
      key_ = Key (      
        sdate,        
        colo_id,        
        channel_id,        
        match_level        
      );      
      return key_;      
    }    
    inline    
    ChannelInventoryEstimStats::Key&    
    ChannelInventoryEstimStats::key ()    
    {    
      return key_;      
    }    
    inline    
    ChannelInventoryEstimStats::Key&    
    ChannelInventoryEstimStats::key (const ChannelInventoryEstimStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs&     
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs&     
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs&     
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs     
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<ChannelInventoryEstimStats, 2>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs     
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<ChannelInventoryEstimStats, 2>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs::const_iterator    
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs::const_iterator    
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs::end () const    
    {    
      return diffs + 2;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs::size () const    
    {    
      return 2;      
    }    
    inline    
    void    
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 2; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    ChannelInventoryEstimStats::users_regular () const    
    {    
      return values[ChannelInventoryEstimStats::USERS_REGULAR];      
    }    
    inline    
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs&     
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs::users_regular (const stats_diff_type& value)    
    {    
      diffs[ChannelInventoryEstimStats::USERS_REGULAR] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs::users_regular () const    
    {    
      return diffs[ChannelInventoryEstimStats::USERS_REGULAR];      
    }    
    inline    
    stats_value_type    
    ChannelInventoryEstimStats::users_from_now () const    
    {    
      return values[ChannelInventoryEstimStats::USERS_FROM_NOW];      
    }    
    inline    
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs&     
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs::users_from_now (const stats_diff_type& value)    
    {    
      diffs[ChannelInventoryEstimStats::USERS_FROM_NOW] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelInventoryEstimStats, 2>::Diffs::users_from_now () const    
    {    
      return diffs[ChannelInventoryEstimStats::USERS_FROM_NOW];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_CHANNELINVENTORYESTIMSTATS_HPP

