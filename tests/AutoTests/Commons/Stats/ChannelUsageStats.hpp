
#ifndef __AUTOTESTS_COMMONS_STATS_CHANNELUSAGESTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_CHANNELUSAGESTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class ChannelUsageStats:    
      public DiffStats<ChannelUsageStats, 4>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          IMPS = 0,          
          CLICKS,          
          ACTIONS,          
          REVENUE          
        };        
        typedef DiffStats<ChannelUsageStats, 4> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            int channel_id_;            
            bool channel_id_used_;            
            bool channel_id_null_;            
            int colo_id_;            
            bool colo_id_used_;            
            bool colo_id_null_;            
            int ccg_id_;            
            bool ccg_id_used_;            
            bool ccg_id_null_;            
            AutoTest::Time sdate_;            
            bool sdate_used_;            
            bool sdate_null_;            
          public:          
            Key& channel_id(const int& value);            
            Key& channel_id_set_null(bool is_null = true);            
            const int& channel_id() const;            
            bool channel_id_used() const;            
            bool channel_id_is_null() const;            
            Key& colo_id(const int& value);            
            Key& colo_id_set_null(bool is_null = true);            
            const int& colo_id() const;            
            bool colo_id_used() const;            
            bool colo_id_is_null() const;            
            Key& ccg_id(const int& value);            
            Key& ccg_id_set_null(bool is_null = true);            
            const int& ccg_id() const;            
            bool ccg_id_used() const;            
            bool ccg_id_is_null() const;            
            Key& sdate(const AutoTest::Time& value);            
            Key& sdate_set_null(bool is_null = true);            
            const AutoTest::Time& sdate() const;            
            bool sdate_used() const;            
            bool sdate_is_null() const;            
            Key ();            
            Key (            
              const int& channel_id,              
              const int& colo_id,              
              const int& ccg_id,              
              const AutoTest::Time& sdate              
            );            
        };        
        stats_value_type imps () const;        
        stats_value_type clicks () const;        
        stats_value_type actions () const;        
        stats_value_type revenue () const;        
        void print_idname (std::ostream& out) const;        
                
        ChannelUsageStats (const Key& value);        
        ChannelUsageStats (        
          const int& channel_id,          
          const int& colo_id,          
          const int& ccg_id,          
          const AutoTest::Time& sdate          
        );        
        ChannelUsageStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& channel_id,          
          const int& colo_id,          
          const int& ccg_id,          
          const AutoTest::Time& sdate          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<ChannelUsageStats, 4>::Diffs    
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
                
        Diffs& imps (const stats_diff_type& value);        
        const stats_diff_type& imps () const;        
        Diffs& clicks (const stats_diff_type& value);        
        const stats_diff_type& clicks () const;        
        Diffs& actions (const stats_diff_type& value);        
        const stats_diff_type& actions () const;        
        Diffs& revenue (const stats_diff_type& value);        
        const stats_diff_type& revenue () const;        
      protected:      
        stats_diff_type diffs[4];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ChannelUsageStats    
    inline    
    ChannelUsageStats::ChannelUsageStats ()    
      :Base("ChannelUsageStats")    
    {}    
    inline    
    ChannelUsageStats::ChannelUsageStats (const ChannelUsageStats::Key& value)    
      :Base("ChannelUsageStats")    
    {    
      key_ = value;      
    }    
    inline    
    ChannelUsageStats::ChannelUsageStats (    
      const int& channel_id,      
      const int& colo_id,      
      const int& ccg_id,      
      const AutoTest::Time& sdate      
    )    
      :Base("ChannelUsageStats")    
    {    
      key_ = Key (      
        channel_id,        
        colo_id,        
        ccg_id,        
        sdate        
      );      
    }    
    inline    
    ChannelUsageStats::Key::Key ()    
      :channel_id_(0),colo_id_(0),ccg_id_(0),sdate_(default_date())    
    {    
      channel_id_used_ = false;      
      channel_id_null_ = false;      
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      ccg_id_used_ = false;      
      ccg_id_null_ = false;      
      sdate_used_ = false;      
      sdate_null_ = false;      
    }    
    inline    
    ChannelUsageStats::Key::Key (    
      const int& channel_id,      
      const int& colo_id,      
      const int& ccg_id,      
      const AutoTest::Time& sdate      
    )    
      :channel_id_(channel_id),colo_id_(colo_id),ccg_id_(ccg_id),sdate_(sdate)    
    {    
      channel_id_used_ = true;      
      channel_id_null_ = false;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      ccg_id_used_ = true;      
      ccg_id_null_ = false;      
      sdate_used_ = true;      
      sdate_null_ = false;      
    }    
    inline    
    ChannelUsageStats::Key&    
    ChannelUsageStats::Key::channel_id(const int& value)    
    {    
      channel_id_ = value;      
      channel_id_used_ = true;      
      channel_id_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelUsageStats::Key&    
    ChannelUsageStats::Key::channel_id_set_null(bool is_null)    
    {    
      channel_id_used_ = true;      
      channel_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelUsageStats::Key::channel_id() const    
    {    
      return channel_id_;      
    }    
    inline    
    bool    
    ChannelUsageStats::Key::channel_id_used() const    
    {    
      return channel_id_used_;      
    }    
    inline    
    bool    
    ChannelUsageStats::Key::channel_id_is_null() const    
    {    
      return channel_id_null_;      
    }    
    inline    
    ChannelUsageStats::Key&    
    ChannelUsageStats::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelUsageStats::Key&    
    ChannelUsageStats::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelUsageStats::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    ChannelUsageStats::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    ChannelUsageStats::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    ChannelUsageStats::Key&    
    ChannelUsageStats::Key::ccg_id(const int& value)    
    {    
      ccg_id_ = value;      
      ccg_id_used_ = true;      
      ccg_id_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelUsageStats::Key&    
    ChannelUsageStats::Key::ccg_id_set_null(bool is_null)    
    {    
      ccg_id_used_ = true;      
      ccg_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelUsageStats::Key::ccg_id() const    
    {    
      return ccg_id_;      
    }    
    inline    
    bool    
    ChannelUsageStats::Key::ccg_id_used() const    
    {    
      return ccg_id_used_;      
    }    
    inline    
    bool    
    ChannelUsageStats::Key::ccg_id_is_null() const    
    {    
      return ccg_id_null_;      
    }    
    inline    
    ChannelUsageStats::Key&    
    ChannelUsageStats::Key::sdate(const AutoTest::Time& value)    
    {    
      sdate_ = value;      
      sdate_used_ = true;      
      sdate_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelUsageStats::Key&    
    ChannelUsageStats::Key::sdate_set_null(bool is_null)    
    {    
      sdate_used_ = true;      
      sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    ChannelUsageStats::Key::sdate() const    
    {    
      return sdate_;      
    }    
    inline    
    bool    
    ChannelUsageStats::Key::sdate_used() const    
    {    
      return sdate_used_;      
    }    
    inline    
    bool    
    ChannelUsageStats::Key::sdate_is_null() const    
    {    
      return sdate_null_;      
    }    
    inline    
    ChannelUsageStats::Key&    
    ChannelUsageStats::key (    
      const int& channel_id,      
      const int& colo_id,      
      const int& ccg_id,      
      const AutoTest::Time& sdate      
    )    
    {    
      key_ = Key (      
        channel_id,        
        colo_id,        
        ccg_id,        
        sdate        
      );      
      return key_;      
    }    
    inline    
    ChannelUsageStats::Key&    
    ChannelUsageStats::key ()    
    {    
      return key_;      
    }    
    inline    
    ChannelUsageStats::Key&    
    ChannelUsageStats::key (const ChannelUsageStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<ChannelUsageStats, 4>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<ChannelUsageStats, 4>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<ChannelUsageStats, 4>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<ChannelUsageStats, 4>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ChannelUsageStats, 4>::Diffs&     
    DiffStats<ChannelUsageStats, 4>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelUsageStats, 4>::Diffs&     
    DiffStats<ChannelUsageStats, 4>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelUsageStats, 4>::Diffs&     
    DiffStats<ChannelUsageStats, 4>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelUsageStats, 4>::Diffs     
    DiffStats<ChannelUsageStats, 4>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<ChannelUsageStats, 4>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<ChannelUsageStats, 4>::Diffs     
    DiffStats<ChannelUsageStats, 4>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<ChannelUsageStats, 4>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<ChannelUsageStats, 4>::Diffs::const_iterator    
    DiffStats<ChannelUsageStats, 4>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ChannelUsageStats, 4>::Diffs::const_iterator    
    DiffStats<ChannelUsageStats, 4>::Diffs::end () const    
    {    
      return diffs + 4;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<ChannelUsageStats, 4>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<ChannelUsageStats, 4>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<ChannelUsageStats, 4>::Diffs::size () const    
    {    
      return 4;      
    }    
    inline    
    void    
    DiffStats<ChannelUsageStats, 4>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 4; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    ChannelUsageStats::imps () const    
    {    
      return values[ChannelUsageStats::IMPS];      
    }    
    inline    
    DiffStats<ChannelUsageStats, 4>::Diffs&     
    DiffStats<ChannelUsageStats, 4>::Diffs::imps (const stats_diff_type& value)    
    {    
      diffs[ChannelUsageStats::IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelUsageStats, 4>::Diffs::imps () const    
    {    
      return diffs[ChannelUsageStats::IMPS];      
    }    
    inline    
    stats_value_type    
    ChannelUsageStats::clicks () const    
    {    
      return values[ChannelUsageStats::CLICKS];      
    }    
    inline    
    DiffStats<ChannelUsageStats, 4>::Diffs&     
    DiffStats<ChannelUsageStats, 4>::Diffs::clicks (const stats_diff_type& value)    
    {    
      diffs[ChannelUsageStats::CLICKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelUsageStats, 4>::Diffs::clicks () const    
    {    
      return diffs[ChannelUsageStats::CLICKS];      
    }    
    inline    
    stats_value_type    
    ChannelUsageStats::actions () const    
    {    
      return values[ChannelUsageStats::ACTIONS];      
    }    
    inline    
    DiffStats<ChannelUsageStats, 4>::Diffs&     
    DiffStats<ChannelUsageStats, 4>::Diffs::actions (const stats_diff_type& value)    
    {    
      diffs[ChannelUsageStats::ACTIONS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelUsageStats, 4>::Diffs::actions () const    
    {    
      return diffs[ChannelUsageStats::ACTIONS];      
    }    
    inline    
    stats_value_type    
    ChannelUsageStats::revenue () const    
    {    
      return values[ChannelUsageStats::REVENUE];      
    }    
    inline    
    DiffStats<ChannelUsageStats, 4>::Diffs&     
    DiffStats<ChannelUsageStats, 4>::Diffs::revenue (const stats_diff_type& value)    
    {    
      diffs[ChannelUsageStats::REVENUE] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelUsageStats, 4>::Diffs::revenue () const    
    {    
      return diffs[ChannelUsageStats::REVENUE];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_CHANNELUSAGESTATS_HPP

