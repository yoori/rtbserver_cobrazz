
#ifndef __AUTOTESTS_COMMONS_STATS_SITECHANNELSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_SITECHANNELSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class SiteChannelStats:    
      public DiffStats<SiteChannelStats, 3>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          IMPS = 0,          
          ADV_REVENUE,          
          PUB_REVENUE          
        };        
        typedef DiffStats<SiteChannelStats, 3> Base;        
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
            int channel_id_;            
            bool channel_id_used_;            
            bool channel_id_null_;            
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
            Key ();            
            Key (            
              const AutoTest::Time& sdate,              
              const int& tag_id,              
              const int& channel_id,              
              const int& colo_id              
            );            
        };        
        stats_value_type imps () const;        
        stats_value_type adv_revenue () const;        
        stats_value_type pub_revenue () const;        
        void print_idname (std::ostream& out) const;        
                
        SiteChannelStats (const Key& value);        
        SiteChannelStats (        
          const AutoTest::Time& sdate,          
          const int& tag_id,          
          const int& channel_id,          
          const int& colo_id          
        );        
        SiteChannelStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const AutoTest::Time& sdate,          
          const int& tag_id,          
          const int& channel_id,          
          const int& colo_id          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<SiteChannelStats, 3>::Diffs    
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
                
        Diffs& imps (const stats_diff_type& value);        
        const stats_diff_type& imps () const;        
        Diffs& adv_revenue (const stats_diff_type& value);        
        const stats_diff_type& adv_revenue () const;        
        Diffs& pub_revenue (const stats_diff_type& value);        
        const stats_diff_type& pub_revenue () const;        
      protected:      
        stats_diff_type diffs[3];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// SiteChannelStats    
    inline    
    SiteChannelStats::SiteChannelStats ()    
      :Base("SiteChannelStats")    
    {}    
    inline    
    SiteChannelStats::SiteChannelStats (const SiteChannelStats::Key& value)    
      :Base("SiteChannelStats")    
    {    
      key_ = value;      
    }    
    inline    
    SiteChannelStats::SiteChannelStats (    
      const AutoTest::Time& sdate,      
      const int& tag_id,      
      const int& channel_id,      
      const int& colo_id      
    )    
      :Base("SiteChannelStats")    
    {    
      key_ = Key (      
        sdate,        
        tag_id,        
        channel_id,        
        colo_id        
      );      
    }    
    inline    
    SiteChannelStats::Key::Key ()    
      :sdate_(default_date()),tag_id_(0),channel_id_(0),colo_id_(0)    
    {    
      sdate_used_ = false;      
      sdate_null_ = false;      
      tag_id_used_ = false;      
      tag_id_null_ = false;      
      channel_id_used_ = false;      
      channel_id_null_ = false;      
      colo_id_used_ = false;      
      colo_id_null_ = false;      
    }    
    inline    
    SiteChannelStats::Key::Key (    
      const AutoTest::Time& sdate,      
      const int& tag_id,      
      const int& channel_id,      
      const int& colo_id      
    )    
      :sdate_(sdate),tag_id_(tag_id),channel_id_(channel_id),colo_id_(colo_id)    
    {    
      sdate_used_ = true;      
      sdate_null_ = false;      
      tag_id_used_ = true;      
      tag_id_null_ = false;      
      channel_id_used_ = true;      
      channel_id_null_ = false;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
    }    
    inline    
    SiteChannelStats::Key&    
    SiteChannelStats::Key::sdate(const AutoTest::Time& value)    
    {    
      sdate_ = value;      
      sdate_used_ = true;      
      sdate_null_ = false;      
      return *this;      
    }    
    inline    
    SiteChannelStats::Key&    
    SiteChannelStats::Key::sdate_set_null(bool is_null)    
    {    
      sdate_used_ = true;      
      sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    SiteChannelStats::Key::sdate() const    
    {    
      return sdate_;      
    }    
    inline    
    bool    
    SiteChannelStats::Key::sdate_used() const    
    {    
      return sdate_used_;      
    }    
    inline    
    bool    
    SiteChannelStats::Key::sdate_is_null() const    
    {    
      return sdate_null_;      
    }    
    inline    
    SiteChannelStats::Key&    
    SiteChannelStats::Key::tag_id(const int& value)    
    {    
      tag_id_ = value;      
      tag_id_used_ = true;      
      tag_id_null_ = false;      
      return *this;      
    }    
    inline    
    SiteChannelStats::Key&    
    SiteChannelStats::Key::tag_id_set_null(bool is_null)    
    {    
      tag_id_used_ = true;      
      tag_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    SiteChannelStats::Key::tag_id() const    
    {    
      return tag_id_;      
    }    
    inline    
    bool    
    SiteChannelStats::Key::tag_id_used() const    
    {    
      return tag_id_used_;      
    }    
    inline    
    bool    
    SiteChannelStats::Key::tag_id_is_null() const    
    {    
      return tag_id_null_;      
    }    
    inline    
    SiteChannelStats::Key&    
    SiteChannelStats::Key::channel_id(const int& value)    
    {    
      channel_id_ = value;      
      channel_id_used_ = true;      
      channel_id_null_ = false;      
      return *this;      
    }    
    inline    
    SiteChannelStats::Key&    
    SiteChannelStats::Key::channel_id_set_null(bool is_null)    
    {    
      channel_id_used_ = true;      
      channel_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    SiteChannelStats::Key::channel_id() const    
    {    
      return channel_id_;      
    }    
    inline    
    bool    
    SiteChannelStats::Key::channel_id_used() const    
    {    
      return channel_id_used_;      
    }    
    inline    
    bool    
    SiteChannelStats::Key::channel_id_is_null() const    
    {    
      return channel_id_null_;      
    }    
    inline    
    SiteChannelStats::Key&    
    SiteChannelStats::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    SiteChannelStats::Key&    
    SiteChannelStats::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    SiteChannelStats::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    SiteChannelStats::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    SiteChannelStats::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    SiteChannelStats::Key&    
    SiteChannelStats::key (    
      const AutoTest::Time& sdate,      
      const int& tag_id,      
      const int& channel_id,      
      const int& colo_id      
    )    
    {    
      key_ = Key (      
        sdate,        
        tag_id,        
        channel_id,        
        colo_id        
      );      
      return key_;      
    }    
    inline    
    SiteChannelStats::Key&    
    SiteChannelStats::key ()    
    {    
      return key_;      
    }    
    inline    
    SiteChannelStats::Key&    
    SiteChannelStats::key (const SiteChannelStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<SiteChannelStats, 3>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<SiteChannelStats, 3>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<SiteChannelStats, 3>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<SiteChannelStats, 3>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<SiteChannelStats, 3>::Diffs&     
    DiffStats<SiteChannelStats, 3>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<SiteChannelStats, 3>::Diffs&     
    DiffStats<SiteChannelStats, 3>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<SiteChannelStats, 3>::Diffs&     
    DiffStats<SiteChannelStats, 3>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<SiteChannelStats, 3>::Diffs     
    DiffStats<SiteChannelStats, 3>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<SiteChannelStats, 3>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<SiteChannelStats, 3>::Diffs     
    DiffStats<SiteChannelStats, 3>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<SiteChannelStats, 3>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<SiteChannelStats, 3>::Diffs::const_iterator    
    DiffStats<SiteChannelStats, 3>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<SiteChannelStats, 3>::Diffs::const_iterator    
    DiffStats<SiteChannelStats, 3>::Diffs::end () const    
    {    
      return diffs + 3;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<SiteChannelStats, 3>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<SiteChannelStats, 3>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<SiteChannelStats, 3>::Diffs::size () const    
    {    
      return 3;      
    }    
    inline    
    void    
    DiffStats<SiteChannelStats, 3>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 3; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    SiteChannelStats::imps () const    
    {    
      return values[SiteChannelStats::IMPS];      
    }    
    inline    
    DiffStats<SiteChannelStats, 3>::Diffs&     
    DiffStats<SiteChannelStats, 3>::Diffs::imps (const stats_diff_type& value)    
    {    
      diffs[SiteChannelStats::IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<SiteChannelStats, 3>::Diffs::imps () const    
    {    
      return diffs[SiteChannelStats::IMPS];      
    }    
    inline    
    stats_value_type    
    SiteChannelStats::adv_revenue () const    
    {    
      return values[SiteChannelStats::ADV_REVENUE];      
    }    
    inline    
    DiffStats<SiteChannelStats, 3>::Diffs&     
    DiffStats<SiteChannelStats, 3>::Diffs::adv_revenue (const stats_diff_type& value)    
    {    
      diffs[SiteChannelStats::ADV_REVENUE] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<SiteChannelStats, 3>::Diffs::adv_revenue () const    
    {    
      return diffs[SiteChannelStats::ADV_REVENUE];      
    }    
    inline    
    stats_value_type    
    SiteChannelStats::pub_revenue () const    
    {    
      return values[SiteChannelStats::PUB_REVENUE];      
    }    
    inline    
    DiffStats<SiteChannelStats, 3>::Diffs&     
    DiffStats<SiteChannelStats, 3>::Diffs::pub_revenue (const stats_diff_type& value)    
    {    
      diffs[SiteChannelStats::PUB_REVENUE] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<SiteChannelStats, 3>::Diffs::pub_revenue () const    
    {    
      return diffs[SiteChannelStats::PUB_REVENUE];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_SITECHANNELSTATS_HPP

