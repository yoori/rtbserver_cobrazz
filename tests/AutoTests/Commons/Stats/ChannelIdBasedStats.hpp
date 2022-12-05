
#ifndef __AUTOTESTS_COMMONS_STATS_CHANNELIDBASEDSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_CHANNELIDBASEDSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class ChannelIdBasedStats:    
      public DiffStats<ChannelIdBasedStats, 22>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          CHANNELIMPINVENTORY_IMPS = 0,          
          CHANNELIMPINVENTORY_CLICKS,          
          CHANNELIMPINVENTORY_ACTIONS,          
          CHANNELIMPINVENTORY_REVENUE,          
          CHANNELIMPINVENTORY_IMPOPS_USER_COUNT,          
          CHANNELIMPINVENTORY_IMPS_USER_COUNT,          
          CHANNELIMPINVENTORY_IMPS_VALUE,          
          CHANNELIMPINVENTORY_IMPS_OTHER,          
          CHANNELIMPINVENTORY_IMPS_OTHER_USER_COUNT,          
          CHANNELIMPINVENTORY_IMPS_OTHER_VALUE,          
          CHANNELIMPINVENTORY_IMPOPS_NO_IMP,          
          CHANNELIMPINVENTORY_IMPOPS_NO_IMP_USER_COUNT,          
          CHANNELIMPINVENTORY_IMPOPS_NO_IMP_VALUE,          
          CHANNELINVENTORY_SUM_ECPM,          
          CHANNELINVENTORY_ACTIVE_USER_COUNT,          
          CHANNELINVENTORY_TOTAL_USER_COUNT,          
          CHANNELINVENTORY_HITS,          
          CHANNELINVENTORY_HITS_URLS,          
          CHANNELINVENTORY_HITS_KWS,          
          CHANNELINVENTORY_HITS_SEARCH_KWS,          
          CHANNELINVENTORY_HITS_URL_KWS,          
          CHANNELINVENTORYBYCPM_USER_COUNT          
        };        
        typedef DiffStats<ChannelIdBasedStats, 22> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            int channel_id_;            
            bool channel_id_used_;            
            bool channel_id_null_;            
            int exclude_colo_;            
            bool exclude_colo_used_;            
            bool exclude_colo_null_;            
          public:          
            Key& channel_id(const int& value);            
            Key& channel_id_set_null(bool is_null = true);            
            const int& channel_id() const;            
            bool channel_id_used() const;            
            bool channel_id_is_null() const;            
            Key& exclude_colo(const int& value);            
            Key& exclude_colo_set_null(bool is_null = true);            
            const int& exclude_colo() const;            
            bool exclude_colo_used() const;            
            bool exclude_colo_is_null() const;            
            Key ();            
            Key (            
              const int& channel_id,              
              const int& exclude_colo              
            );            
        };        
        stats_value_type channelimpinventory_imps () const;        
        stats_value_type channelimpinventory_clicks () const;        
        stats_value_type channelimpinventory_actions () const;        
        stats_value_type channelimpinventory_revenue () const;        
        stats_value_type channelimpinventory_impops_user_count () const;        
        stats_value_type channelimpinventory_imps_user_count () const;        
        stats_value_type channelimpinventory_imps_value () const;        
        stats_value_type channelimpinventory_imps_other () const;        
        stats_value_type channelimpinventory_imps_other_user_count () const;        
        stats_value_type channelimpinventory_imps_other_value () const;        
        stats_value_type channelimpinventory_impops_no_imp () const;        
        stats_value_type channelimpinventory_impops_no_imp_user_count () const;        
        stats_value_type channelimpinventory_impops_no_imp_value () const;        
        stats_value_type channelinventory_sum_ecpm () const;        
        stats_value_type channelinventory_active_user_count () const;        
        stats_value_type channelinventory_total_user_count () const;        
        stats_value_type channelinventory_hits () const;        
        stats_value_type channelinventory_hits_urls () const;        
        stats_value_type channelinventory_hits_kws () const;        
        stats_value_type channelinventory_hits_search_kws () const;        
        stats_value_type channelinventory_hits_url_kws () const;        
        stats_value_type channelinventorybycpm_user_count () const;        
        void print_idname (std::ostream& out) const;        
                
        ChannelIdBasedStats (const Key& value);        
        ChannelIdBasedStats (        
          const int& channel_id,          
          const int& exclude_colo          
        );        
        ChannelIdBasedStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& channel_id,          
          const int& exclude_colo          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<ChannelIdBasedStats, 22>::Diffs    
    {    
      public:      
        typedef stats_diff_type array_type[22];        
        typedef const stats_diff_type const_array_type[22];        
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
                
        Diffs& channelimpinventory_imps (const stats_diff_type& value);        
        const stats_diff_type& channelimpinventory_imps () const;        
        Diffs& channelimpinventory_clicks (const stats_diff_type& value);        
        const stats_diff_type& channelimpinventory_clicks () const;        
        Diffs& channelimpinventory_actions (const stats_diff_type& value);        
        const stats_diff_type& channelimpinventory_actions () const;        
        Diffs& channelimpinventory_revenue (const stats_diff_type& value);        
        const stats_diff_type& channelimpinventory_revenue () const;        
        Diffs& channelimpinventory_impops_user_count (const stats_diff_type& value);        
        const stats_diff_type& channelimpinventory_impops_user_count () const;        
        Diffs& channelimpinventory_imps_user_count (const stats_diff_type& value);        
        const stats_diff_type& channelimpinventory_imps_user_count () const;        
        Diffs& channelimpinventory_imps_value (const stats_diff_type& value);        
        const stats_diff_type& channelimpinventory_imps_value () const;        
        Diffs& channelimpinventory_imps_other (const stats_diff_type& value);        
        const stats_diff_type& channelimpinventory_imps_other () const;        
        Diffs& channelimpinventory_imps_other_user_count (const stats_diff_type& value);        
        const stats_diff_type& channelimpinventory_imps_other_user_count () const;        
        Diffs& channelimpinventory_imps_other_value (const stats_diff_type& value);        
        const stats_diff_type& channelimpinventory_imps_other_value () const;        
        Diffs& channelimpinventory_impops_no_imp (const stats_diff_type& value);        
        const stats_diff_type& channelimpinventory_impops_no_imp () const;        
        Diffs& channelimpinventory_impops_no_imp_user_count (const stats_diff_type& value);        
        const stats_diff_type& channelimpinventory_impops_no_imp_user_count () const;        
        Diffs& channelimpinventory_impops_no_imp_value (const stats_diff_type& value);        
        const stats_diff_type& channelimpinventory_impops_no_imp_value () const;        
        Diffs& channelinventory_sum_ecpm (const stats_diff_type& value);        
        const stats_diff_type& channelinventory_sum_ecpm () const;        
        Diffs& channelinventory_active_user_count (const stats_diff_type& value);        
        const stats_diff_type& channelinventory_active_user_count () const;        
        Diffs& channelinventory_total_user_count (const stats_diff_type& value);        
        const stats_diff_type& channelinventory_total_user_count () const;        
        Diffs& channelinventory_hits (const stats_diff_type& value);        
        const stats_diff_type& channelinventory_hits () const;        
        Diffs& channelinventory_hits_urls (const stats_diff_type& value);        
        const stats_diff_type& channelinventory_hits_urls () const;        
        Diffs& channelinventory_hits_kws (const stats_diff_type& value);        
        const stats_diff_type& channelinventory_hits_kws () const;        
        Diffs& channelinventory_hits_search_kws (const stats_diff_type& value);        
        const stats_diff_type& channelinventory_hits_search_kws () const;        
        Diffs& channelinventory_hits_url_kws (const stats_diff_type& value);        
        const stats_diff_type& channelinventory_hits_url_kws () const;        
        Diffs& channelinventorybycpm_user_count (const stats_diff_type& value);        
        const stats_diff_type& channelinventorybycpm_user_count () const;        
      protected:      
        stats_diff_type diffs[22];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ChannelIdBasedStats    
    inline    
    ChannelIdBasedStats::ChannelIdBasedStats ()    
      :Base("ChannelIdBasedStats")    
    {}    
    inline    
    ChannelIdBasedStats::ChannelIdBasedStats (const ChannelIdBasedStats::Key& value)    
      :Base("ChannelIdBasedStats")    
    {    
      key_ = value;      
    }    
    inline    
    ChannelIdBasedStats::ChannelIdBasedStats (    
      const int& channel_id,      
      const int& exclude_colo      
    )    
      :Base("ChannelIdBasedStats")    
    {    
      key_ = Key (      
        channel_id,        
        exclude_colo        
      );      
    }    
    inline    
    ChannelIdBasedStats::Key::Key ()    
      :channel_id_(0)    
    {    
      channel_id_used_ = false;      
      channel_id_null_ = false;      
      exclude_colo_used_ = false;      
      exclude_colo_null_ = false;      
    }    
    inline    
    ChannelIdBasedStats::Key::Key (    
      const int& channel_id,      
      const int& exclude_colo      
    )    
      :channel_id_(channel_id)    
    {    
      channel_id_used_ = true;      
      channel_id_null_ = false;      
      exclude_colo_ = exclude_colo;      
      exclude_colo_used_ = true;      
      exclude_colo_null_ = false;      
    }    
    inline    
    ChannelIdBasedStats::Key&    
    ChannelIdBasedStats::Key::channel_id(const int& value)    
    {    
      channel_id_ = value;      
      channel_id_used_ = true;      
      channel_id_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelIdBasedStats::Key&    
    ChannelIdBasedStats::Key::channel_id_set_null(bool is_null)    
    {    
      channel_id_used_ = true;      
      channel_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelIdBasedStats::Key::channel_id() const    
    {    
      return channel_id_;      
    }    
    inline    
    bool    
    ChannelIdBasedStats::Key::channel_id_used() const    
    {    
      return channel_id_used_;      
    }    
    inline    
    bool    
    ChannelIdBasedStats::Key::channel_id_is_null() const    
    {    
      return channel_id_null_;      
    }    
    inline    
    ChannelIdBasedStats::Key&    
    ChannelIdBasedStats::Key::exclude_colo(const int& value)    
    {    
      exclude_colo_ = value;      
      exclude_colo_used_ = true;      
      exclude_colo_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelIdBasedStats::Key&    
    ChannelIdBasedStats::Key::exclude_colo_set_null(bool is_null)    
    {    
      exclude_colo_used_ = true;      
      exclude_colo_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelIdBasedStats::Key::exclude_colo() const    
    {    
      return exclude_colo_;      
    }    
    inline    
    bool    
    ChannelIdBasedStats::Key::exclude_colo_used() const    
    {    
      return exclude_colo_used_;      
    }    
    inline    
    bool    
    ChannelIdBasedStats::Key::exclude_colo_is_null() const    
    {    
      return exclude_colo_null_;      
    }    
    inline    
    ChannelIdBasedStats::Key&    
    ChannelIdBasedStats::key (    
      const int& channel_id,      
      const int& exclude_colo      
    )    
    {    
      key_ = Key (      
        channel_id,        
        exclude_colo        
      );      
      return key_;      
    }    
    inline    
    ChannelIdBasedStats::Key&    
    ChannelIdBasedStats::key ()    
    {    
      return key_;      
    }    
    inline    
    ChannelIdBasedStats::Key&    
    ChannelIdBasedStats::key (const ChannelIdBasedStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 22; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 22; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 22; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 22; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 22; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 22; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<ChannelIdBasedStats, 22>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<ChannelIdBasedStats, 22>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs::const_iterator    
    DiffStats<ChannelIdBasedStats, 22>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs::const_iterator    
    DiffStats<ChannelIdBasedStats, 22>::Diffs::end () const    
    {    
      return diffs + 22;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<ChannelIdBasedStats, 22>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<ChannelIdBasedStats, 22>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<ChannelIdBasedStats, 22>::Diffs::size () const    
    {    
      return 22;      
    }    
    inline    
    void    
    DiffStats<ChannelIdBasedStats, 22>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 22; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelimpinventory_imps () const    
    {    
      return values[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPS];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_imps (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_imps () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPS];      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelimpinventory_clicks () const    
    {    
      return values[ChannelIdBasedStats::CHANNELIMPINVENTORY_CLICKS];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_clicks (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_CLICKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_clicks () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_CLICKS];      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelimpinventory_actions () const    
    {    
      return values[ChannelIdBasedStats::CHANNELIMPINVENTORY_ACTIONS];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_actions (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_ACTIONS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_actions () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_ACTIONS];      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelimpinventory_revenue () const    
    {    
      return values[ChannelIdBasedStats::CHANNELIMPINVENTORY_REVENUE];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_revenue (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_REVENUE] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_revenue () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_REVENUE];      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelimpinventory_impops_user_count () const    
    {    
      return values[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPOPS_USER_COUNT];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_impops_user_count (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPOPS_USER_COUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_impops_user_count () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPOPS_USER_COUNT];      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelimpinventory_imps_user_count () const    
    {    
      return values[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPS_USER_COUNT];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_imps_user_count (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPS_USER_COUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_imps_user_count () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPS_USER_COUNT];      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelimpinventory_imps_value () const    
    {    
      return values[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPS_VALUE];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_imps_value (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPS_VALUE] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_imps_value () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPS_VALUE];      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelimpinventory_imps_other () const    
    {    
      return values[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPS_OTHER];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_imps_other (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPS_OTHER] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_imps_other () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPS_OTHER];      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelimpinventory_imps_other_user_count () const    
    {    
      return values[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPS_OTHER_USER_COUNT];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_imps_other_user_count (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPS_OTHER_USER_COUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_imps_other_user_count () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPS_OTHER_USER_COUNT];      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelimpinventory_imps_other_value () const    
    {    
      return values[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPS_OTHER_VALUE];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_imps_other_value (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPS_OTHER_VALUE] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_imps_other_value () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPS_OTHER_VALUE];      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelimpinventory_impops_no_imp () const    
    {    
      return values[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPOPS_NO_IMP];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_impops_no_imp (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPOPS_NO_IMP] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_impops_no_imp () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPOPS_NO_IMP];      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelimpinventory_impops_no_imp_user_count () const    
    {    
      return values[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPOPS_NO_IMP_USER_COUNT];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_impops_no_imp_user_count (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPOPS_NO_IMP_USER_COUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_impops_no_imp_user_count () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPOPS_NO_IMP_USER_COUNT];      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelimpinventory_impops_no_imp_value () const    
    {    
      return values[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPOPS_NO_IMP_VALUE];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_impops_no_imp_value (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPOPS_NO_IMP_VALUE] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelimpinventory_impops_no_imp_value () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELIMPINVENTORY_IMPOPS_NO_IMP_VALUE];      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelinventory_sum_ecpm () const    
    {    
      return values[ChannelIdBasedStats::CHANNELINVENTORY_SUM_ECPM];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelinventory_sum_ecpm (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELINVENTORY_SUM_ECPM] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelinventory_sum_ecpm () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELINVENTORY_SUM_ECPM];      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelinventory_active_user_count () const    
    {    
      return values[ChannelIdBasedStats::CHANNELINVENTORY_ACTIVE_USER_COUNT];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelinventory_active_user_count (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELINVENTORY_ACTIVE_USER_COUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelinventory_active_user_count () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELINVENTORY_ACTIVE_USER_COUNT];      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelinventory_total_user_count () const    
    {    
      return values[ChannelIdBasedStats::CHANNELINVENTORY_TOTAL_USER_COUNT];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelinventory_total_user_count (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELINVENTORY_TOTAL_USER_COUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelinventory_total_user_count () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELINVENTORY_TOTAL_USER_COUNT];      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelinventory_hits () const    
    {    
      return values[ChannelIdBasedStats::CHANNELINVENTORY_HITS];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelinventory_hits (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELINVENTORY_HITS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelinventory_hits () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELINVENTORY_HITS];      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelinventory_hits_urls () const    
    {    
      return values[ChannelIdBasedStats::CHANNELINVENTORY_HITS_URLS];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelinventory_hits_urls (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELINVENTORY_HITS_URLS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelinventory_hits_urls () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELINVENTORY_HITS_URLS];      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelinventory_hits_kws () const    
    {    
      return values[ChannelIdBasedStats::CHANNELINVENTORY_HITS_KWS];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelinventory_hits_kws (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELINVENTORY_HITS_KWS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelinventory_hits_kws () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELINVENTORY_HITS_KWS];      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelinventory_hits_search_kws () const    
    {    
      return values[ChannelIdBasedStats::CHANNELINVENTORY_HITS_SEARCH_KWS];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelinventory_hits_search_kws (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELINVENTORY_HITS_SEARCH_KWS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelinventory_hits_search_kws () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELINVENTORY_HITS_SEARCH_KWS];      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelinventory_hits_url_kws () const    
    {    
      return values[ChannelIdBasedStats::CHANNELINVENTORY_HITS_URL_KWS];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelinventory_hits_url_kws (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELINVENTORY_HITS_URL_KWS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelinventory_hits_url_kws () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELINVENTORY_HITS_URL_KWS];      
    }    
    inline    
    stats_value_type    
    ChannelIdBasedStats::channelinventorybycpm_user_count () const    
    {    
      return values[ChannelIdBasedStats::CHANNELINVENTORYBYCPM_USER_COUNT];      
    }    
    inline    
    DiffStats<ChannelIdBasedStats, 22>::Diffs&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelinventorybycpm_user_count (const stats_diff_type& value)    
    {    
      diffs[ChannelIdBasedStats::CHANNELINVENTORYBYCPM_USER_COUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelIdBasedStats, 22>::Diffs::channelinventorybycpm_user_count () const    
    {    
      return diffs[ChannelIdBasedStats::CHANNELINVENTORYBYCPM_USER_COUNT];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_CHANNELIDBASEDSTATS_HPP

