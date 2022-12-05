
#ifndef __AUTOTESTS_COMMONS_STATS_CHANNELINVENTORYSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_CHANNELINVENTORYSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class ChannelInventoryStats:    
      public DiffStats<ChannelInventoryStats, 7>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          ACTIVE_USERS = 0,          
          TOTAL_USERS,          
          SUM_ECPM,          
          HITS,          
          HITS_URLS,          
          HITS_KWS,          
          HITS_SEARCH_KWS          
        };        
        typedef DiffStats<ChannelInventoryStats, 7> Base;        
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
            Key& sdate(const AutoTest::Time& value);            
            Key& sdate_set_null(bool is_null = true);            
            const AutoTest::Time& sdate() const;            
            bool sdate_used() const;            
            bool sdate_is_null() const;            
            Key ();            
            Key (            
              const int& channel_id,              
              const int& colo_id,              
              const AutoTest::Time& sdate              
            );            
        };        
        stats_value_type active_users () const;        
        stats_value_type total_users () const;        
        stats_value_type sum_ecpm () const;        
        stats_value_type hits () const;        
        stats_value_type hits_urls () const;        
        stats_value_type hits_kws () const;        
        stats_value_type hits_search_kws () const;        
        void print_idname (std::ostream& out) const;        
                
        ChannelInventoryStats (const Key& value);        
        ChannelInventoryStats (        
          const int& channel_id,          
          const int& colo_id,          
          const AutoTest::Time& sdate          
        );        
        ChannelInventoryStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& channel_id,          
          const int& colo_id,          
          const AutoTest::Time& sdate          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<ChannelInventoryStats, 7>::Diffs    
    {    
      public:      
        typedef stats_diff_type array_type[7];        
        typedef const stats_diff_type const_array_type[7];        
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
                
        Diffs& active_users (const stats_diff_type& value);        
        const stats_diff_type& active_users () const;        
        Diffs& total_users (const stats_diff_type& value);        
        const stats_diff_type& total_users () const;        
        Diffs& sum_ecpm (const stats_diff_type& value);        
        const stats_diff_type& sum_ecpm () const;        
        Diffs& hits (const stats_diff_type& value);        
        const stats_diff_type& hits () const;        
        Diffs& hits_urls (const stats_diff_type& value);        
        const stats_diff_type& hits_urls () const;        
        Diffs& hits_kws (const stats_diff_type& value);        
        const stats_diff_type& hits_kws () const;        
        Diffs& hits_search_kws (const stats_diff_type& value);        
        const stats_diff_type& hits_search_kws () const;        
      protected:      
        stats_diff_type diffs[7];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ChannelInventoryStats    
    inline    
    ChannelInventoryStats::ChannelInventoryStats ()    
      :Base("ChannelInventoryStats")    
    {}    
    inline    
    ChannelInventoryStats::ChannelInventoryStats (const ChannelInventoryStats::Key& value)    
      :Base("ChannelInventoryStats")    
    {    
      key_ = value;      
    }    
    inline    
    ChannelInventoryStats::ChannelInventoryStats (    
      const int& channel_id,      
      const int& colo_id,      
      const AutoTest::Time& sdate      
    )    
      :Base("ChannelInventoryStats")    
    {    
      key_ = Key (      
        channel_id,        
        colo_id,        
        sdate        
      );      
    }    
    inline    
    ChannelInventoryStats::Key::Key ()    
      :channel_id_(0),colo_id_(0),sdate_(default_date())    
    {    
      channel_id_used_ = false;      
      channel_id_null_ = false;      
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      sdate_used_ = false;      
      sdate_null_ = false;      
    }    
    inline    
    ChannelInventoryStats::Key::Key (    
      const int& channel_id,      
      const int& colo_id,      
      const AutoTest::Time& sdate      
    )    
      :channel_id_(channel_id),colo_id_(colo_id),sdate_(sdate)    
    {    
      channel_id_used_ = true;      
      channel_id_null_ = false;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      sdate_used_ = true;      
      sdate_null_ = false;      
    }    
    inline    
    ChannelInventoryStats::Key&    
    ChannelInventoryStats::Key::channel_id(const int& value)    
    {    
      channel_id_ = value;      
      channel_id_used_ = true;      
      channel_id_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelInventoryStats::Key&    
    ChannelInventoryStats::Key::channel_id_set_null(bool is_null)    
    {    
      channel_id_used_ = true;      
      channel_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelInventoryStats::Key::channel_id() const    
    {    
      return channel_id_;      
    }    
    inline    
    bool    
    ChannelInventoryStats::Key::channel_id_used() const    
    {    
      return channel_id_used_;      
    }    
    inline    
    bool    
    ChannelInventoryStats::Key::channel_id_is_null() const    
    {    
      return channel_id_null_;      
    }    
    inline    
    ChannelInventoryStats::Key&    
    ChannelInventoryStats::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelInventoryStats::Key&    
    ChannelInventoryStats::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelInventoryStats::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    ChannelInventoryStats::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    ChannelInventoryStats::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    ChannelInventoryStats::Key&    
    ChannelInventoryStats::Key::sdate(const AutoTest::Time& value)    
    {    
      sdate_ = value;      
      sdate_used_ = true;      
      sdate_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelInventoryStats::Key&    
    ChannelInventoryStats::Key::sdate_set_null(bool is_null)    
    {    
      sdate_used_ = true;      
      sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    ChannelInventoryStats::Key::sdate() const    
    {    
      return sdate_;      
    }    
    inline    
    bool    
    ChannelInventoryStats::Key::sdate_used() const    
    {    
      return sdate_used_;      
    }    
    inline    
    bool    
    ChannelInventoryStats::Key::sdate_is_null() const    
    {    
      return sdate_null_;      
    }    
    inline    
    ChannelInventoryStats::Key&    
    ChannelInventoryStats::key (    
      const int& channel_id,      
      const int& colo_id,      
      const AutoTest::Time& sdate      
    )    
    {    
      key_ = Key (      
        channel_id,        
        colo_id,        
        sdate        
      );      
      return key_;      
    }    
    inline    
    ChannelInventoryStats::Key&    
    ChannelInventoryStats::key ()    
    {    
      return key_;      
    }    
    inline    
    ChannelInventoryStats::Key&    
    ChannelInventoryStats::key (const ChannelInventoryStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<ChannelInventoryStats, 7>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 7; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<ChannelInventoryStats, 7>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 7; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<ChannelInventoryStats, 7>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 7; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<ChannelInventoryStats, 7>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ChannelInventoryStats, 7>::Diffs&     
    DiffStats<ChannelInventoryStats, 7>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 7; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelInventoryStats, 7>::Diffs&     
    DiffStats<ChannelInventoryStats, 7>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 7; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelInventoryStats, 7>::Diffs&     
    DiffStats<ChannelInventoryStats, 7>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 7; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelInventoryStats, 7>::Diffs     
    DiffStats<ChannelInventoryStats, 7>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<ChannelInventoryStats, 7>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<ChannelInventoryStats, 7>::Diffs     
    DiffStats<ChannelInventoryStats, 7>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<ChannelInventoryStats, 7>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<ChannelInventoryStats, 7>::Diffs::const_iterator    
    DiffStats<ChannelInventoryStats, 7>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ChannelInventoryStats, 7>::Diffs::const_iterator    
    DiffStats<ChannelInventoryStats, 7>::Diffs::end () const    
    {    
      return diffs + 7;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<ChannelInventoryStats, 7>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<ChannelInventoryStats, 7>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<ChannelInventoryStats, 7>::Diffs::size () const    
    {    
      return 7;      
    }    
    inline    
    void    
    DiffStats<ChannelInventoryStats, 7>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 7; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    ChannelInventoryStats::active_users () const    
    {    
      return values[ChannelInventoryStats::ACTIVE_USERS];      
    }    
    inline    
    DiffStats<ChannelInventoryStats, 7>::Diffs&     
    DiffStats<ChannelInventoryStats, 7>::Diffs::active_users (const stats_diff_type& value)    
    {    
      diffs[ChannelInventoryStats::ACTIVE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelInventoryStats, 7>::Diffs::active_users () const    
    {    
      return diffs[ChannelInventoryStats::ACTIVE_USERS];      
    }    
    inline    
    stats_value_type    
    ChannelInventoryStats::total_users () const    
    {    
      return values[ChannelInventoryStats::TOTAL_USERS];      
    }    
    inline    
    DiffStats<ChannelInventoryStats, 7>::Diffs&     
    DiffStats<ChannelInventoryStats, 7>::Diffs::total_users (const stats_diff_type& value)    
    {    
      diffs[ChannelInventoryStats::TOTAL_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelInventoryStats, 7>::Diffs::total_users () const    
    {    
      return diffs[ChannelInventoryStats::TOTAL_USERS];      
    }    
    inline    
    stats_value_type    
    ChannelInventoryStats::sum_ecpm () const    
    {    
      return values[ChannelInventoryStats::SUM_ECPM];      
    }    
    inline    
    DiffStats<ChannelInventoryStats, 7>::Diffs&     
    DiffStats<ChannelInventoryStats, 7>::Diffs::sum_ecpm (const stats_diff_type& value)    
    {    
      diffs[ChannelInventoryStats::SUM_ECPM] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelInventoryStats, 7>::Diffs::sum_ecpm () const    
    {    
      return diffs[ChannelInventoryStats::SUM_ECPM];      
    }    
    inline    
    stats_value_type    
    ChannelInventoryStats::hits () const    
    {    
      return values[ChannelInventoryStats::HITS];      
    }    
    inline    
    DiffStats<ChannelInventoryStats, 7>::Diffs&     
    DiffStats<ChannelInventoryStats, 7>::Diffs::hits (const stats_diff_type& value)    
    {    
      diffs[ChannelInventoryStats::HITS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelInventoryStats, 7>::Diffs::hits () const    
    {    
      return diffs[ChannelInventoryStats::HITS];      
    }    
    inline    
    stats_value_type    
    ChannelInventoryStats::hits_urls () const    
    {    
      return values[ChannelInventoryStats::HITS_URLS];      
    }    
    inline    
    DiffStats<ChannelInventoryStats, 7>::Diffs&     
    DiffStats<ChannelInventoryStats, 7>::Diffs::hits_urls (const stats_diff_type& value)    
    {    
      diffs[ChannelInventoryStats::HITS_URLS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelInventoryStats, 7>::Diffs::hits_urls () const    
    {    
      return diffs[ChannelInventoryStats::HITS_URLS];      
    }    
    inline    
    stats_value_type    
    ChannelInventoryStats::hits_kws () const    
    {    
      return values[ChannelInventoryStats::HITS_KWS];      
    }    
    inline    
    DiffStats<ChannelInventoryStats, 7>::Diffs&     
    DiffStats<ChannelInventoryStats, 7>::Diffs::hits_kws (const stats_diff_type& value)    
    {    
      diffs[ChannelInventoryStats::HITS_KWS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelInventoryStats, 7>::Diffs::hits_kws () const    
    {    
      return diffs[ChannelInventoryStats::HITS_KWS];      
    }    
    inline    
    stats_value_type    
    ChannelInventoryStats::hits_search_kws () const    
    {    
      return values[ChannelInventoryStats::HITS_SEARCH_KWS];      
    }    
    inline    
    DiffStats<ChannelInventoryStats, 7>::Diffs&     
    DiffStats<ChannelInventoryStats, 7>::Diffs::hits_search_kws (const stats_diff_type& value)    
    {    
      diffs[ChannelInventoryStats::HITS_SEARCH_KWS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelInventoryStats, 7>::Diffs::hits_search_kws () const    
    {    
      return diffs[ChannelInventoryStats::HITS_SEARCH_KWS];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_CHANNELINVENTORYSTATS_HPP

