
#ifndef __AUTOTESTS_COMMONS_STATS_CHANNELTRIGGERSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_CHANNELTRIGGERSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class ChannelTriggerStats:    
      public DiffStats<ChannelTriggerStats, 3>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          HITS = 0,          
          IMPS,          
          CLICKS          
        };        
        typedef DiffStats<ChannelTriggerStats, 3> Base;        
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
            int channel_trigger_id_;            
            bool channel_trigger_id_used_;            
            bool channel_trigger_id_null_;            
            std::string trigger_type_;            
            bool trigger_type_used_;            
            bool trigger_type_null_;            
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
            Key& channel_trigger_id(const int& value);            
            Key& channel_trigger_id_set_null(bool is_null = true);            
            const int& channel_trigger_id() const;            
            bool channel_trigger_id_used() const;            
            bool channel_trigger_id_is_null() const;            
            Key& trigger_type(const std::string& value);            
            Key& trigger_type_set_null(bool is_null = true);            
            const std::string& trigger_type() const;            
            bool trigger_type_used() const;            
            bool trigger_type_is_null() const;            
            Key ();            
            Key (            
              const AutoTest::Time& sdate,              
              const int& colo_id,              
              const int& channel_id,              
              const int& channel_trigger_id,              
              const std::string& trigger_type              
            );            
        };        
        stats_value_type hits () const;        
        stats_value_type imps () const;        
        stats_value_type clicks () const;        
        void print_idname (std::ostream& out) const;        
                
        ChannelTriggerStats (const Key& value);        
        ChannelTriggerStats (        
          const AutoTest::Time& sdate,          
          const int& colo_id,          
          const int& channel_id,          
          const int& channel_trigger_id,          
          const std::string& trigger_type          
        );        
        ChannelTriggerStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const AutoTest::Time& sdate,          
          const int& colo_id,          
          const int& channel_id,          
          const int& channel_trigger_id,          
          const std::string& trigger_type          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<ChannelTriggerStats, 3>::Diffs    
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
                
        Diffs& hits (const stats_diff_type& value);        
        const stats_diff_type& hits () const;        
        Diffs& imps (const stats_diff_type& value);        
        const stats_diff_type& imps () const;        
        Diffs& clicks (const stats_diff_type& value);        
        const stats_diff_type& clicks () const;        
      protected:      
        stats_diff_type diffs[3];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ChannelTriggerStats    
    inline    
    ChannelTriggerStats::ChannelTriggerStats ()    
      :Base("ChannelTriggerStats")    
    {}    
    inline    
    ChannelTriggerStats::ChannelTriggerStats (const ChannelTriggerStats::Key& value)    
      :Base("ChannelTriggerStats")    
    {    
      key_ = value;      
    }    
    inline    
    ChannelTriggerStats::ChannelTriggerStats (    
      const AutoTest::Time& sdate,      
      const int& colo_id,      
      const int& channel_id,      
      const int& channel_trigger_id,      
      const std::string& trigger_type      
    )    
      :Base("ChannelTriggerStats")    
    {    
      key_ = Key (      
        sdate,        
        colo_id,        
        channel_id,        
        channel_trigger_id,        
        trigger_type        
      );      
    }    
    inline    
    ChannelTriggerStats::Key::Key ()    
      :sdate_(default_date()),colo_id_(0),channel_id_(0),channel_trigger_id_(0)    
    {    
      sdate_used_ = false;      
      sdate_null_ = false;      
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      channel_id_used_ = false;      
      channel_id_null_ = false;      
      channel_trigger_id_used_ = false;      
      channel_trigger_id_null_ = false;      
      trigger_type_used_ = false;      
      trigger_type_null_ = false;      
    }    
    inline    
    ChannelTriggerStats::Key::Key (    
      const AutoTest::Time& sdate,      
      const int& colo_id,      
      const int& channel_id,      
      const int& channel_trigger_id,      
      const std::string& trigger_type      
    )    
      :sdate_(sdate),colo_id_(colo_id),channel_id_(channel_id),channel_trigger_id_(channel_trigger_id)    
    {    
      sdate_used_ = true;      
      sdate_null_ = false;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      channel_id_used_ = true;      
      channel_id_null_ = false;      
      channel_trigger_id_used_ = true;      
      channel_trigger_id_null_ = false;      
      trigger_type_ = trigger_type;      
      trigger_type_used_ = true;      
      trigger_type_null_ = false;      
    }    
    inline    
    ChannelTriggerStats::Key&    
    ChannelTriggerStats::Key::sdate(const AutoTest::Time& value)    
    {    
      sdate_ = value;      
      sdate_used_ = true;      
      sdate_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelTriggerStats::Key&    
    ChannelTriggerStats::Key::sdate_set_null(bool is_null)    
    {    
      sdate_used_ = true;      
      sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    ChannelTriggerStats::Key::sdate() const    
    {    
      return sdate_;      
    }    
    inline    
    bool    
    ChannelTriggerStats::Key::sdate_used() const    
    {    
      return sdate_used_;      
    }    
    inline    
    bool    
    ChannelTriggerStats::Key::sdate_is_null() const    
    {    
      return sdate_null_;      
    }    
    inline    
    ChannelTriggerStats::Key&    
    ChannelTriggerStats::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelTriggerStats::Key&    
    ChannelTriggerStats::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelTriggerStats::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    ChannelTriggerStats::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    ChannelTriggerStats::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    ChannelTriggerStats::Key&    
    ChannelTriggerStats::Key::channel_id(const int& value)    
    {    
      channel_id_ = value;      
      channel_id_used_ = true;      
      channel_id_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelTriggerStats::Key&    
    ChannelTriggerStats::Key::channel_id_set_null(bool is_null)    
    {    
      channel_id_used_ = true;      
      channel_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelTriggerStats::Key::channel_id() const    
    {    
      return channel_id_;      
    }    
    inline    
    bool    
    ChannelTriggerStats::Key::channel_id_used() const    
    {    
      return channel_id_used_;      
    }    
    inline    
    bool    
    ChannelTriggerStats::Key::channel_id_is_null() const    
    {    
      return channel_id_null_;      
    }    
    inline    
    ChannelTriggerStats::Key&    
    ChannelTriggerStats::Key::channel_trigger_id(const int& value)    
    {    
      channel_trigger_id_ = value;      
      channel_trigger_id_used_ = true;      
      channel_trigger_id_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelTriggerStats::Key&    
    ChannelTriggerStats::Key::channel_trigger_id_set_null(bool is_null)    
    {    
      channel_trigger_id_used_ = true;      
      channel_trigger_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelTriggerStats::Key::channel_trigger_id() const    
    {    
      return channel_trigger_id_;      
    }    
    inline    
    bool    
    ChannelTriggerStats::Key::channel_trigger_id_used() const    
    {    
      return channel_trigger_id_used_;      
    }    
    inline    
    bool    
    ChannelTriggerStats::Key::channel_trigger_id_is_null() const    
    {    
      return channel_trigger_id_null_;      
    }    
    inline    
    ChannelTriggerStats::Key&    
    ChannelTriggerStats::Key::trigger_type(const std::string& value)    
    {    
      trigger_type_ = value;      
      trigger_type_used_ = true;      
      trigger_type_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelTriggerStats::Key&    
    ChannelTriggerStats::Key::trigger_type_set_null(bool is_null)    
    {    
      trigger_type_used_ = true;      
      trigger_type_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    ChannelTriggerStats::Key::trigger_type() const    
    {    
      return trigger_type_;      
    }    
    inline    
    bool    
    ChannelTriggerStats::Key::trigger_type_used() const    
    {    
      return trigger_type_used_;      
    }    
    inline    
    bool    
    ChannelTriggerStats::Key::trigger_type_is_null() const    
    {    
      return trigger_type_null_;      
    }    
    inline    
    ChannelTriggerStats::Key&    
    ChannelTriggerStats::key (    
      const AutoTest::Time& sdate,      
      const int& colo_id,      
      const int& channel_id,      
      const int& channel_trigger_id,      
      const std::string& trigger_type      
    )    
    {    
      key_ = Key (      
        sdate,        
        colo_id,        
        channel_id,        
        channel_trigger_id,        
        trigger_type        
      );      
      return key_;      
    }    
    inline    
    ChannelTriggerStats::Key&    
    ChannelTriggerStats::key ()    
    {    
      return key_;      
    }    
    inline    
    ChannelTriggerStats::Key&    
    ChannelTriggerStats::key (const ChannelTriggerStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<ChannelTriggerStats, 3>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<ChannelTriggerStats, 3>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<ChannelTriggerStats, 3>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<ChannelTriggerStats, 3>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ChannelTriggerStats, 3>::Diffs&     
    DiffStats<ChannelTriggerStats, 3>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelTriggerStats, 3>::Diffs&     
    DiffStats<ChannelTriggerStats, 3>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelTriggerStats, 3>::Diffs&     
    DiffStats<ChannelTriggerStats, 3>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelTriggerStats, 3>::Diffs     
    DiffStats<ChannelTriggerStats, 3>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<ChannelTriggerStats, 3>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<ChannelTriggerStats, 3>::Diffs     
    DiffStats<ChannelTriggerStats, 3>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<ChannelTriggerStats, 3>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<ChannelTriggerStats, 3>::Diffs::const_iterator    
    DiffStats<ChannelTriggerStats, 3>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ChannelTriggerStats, 3>::Diffs::const_iterator    
    DiffStats<ChannelTriggerStats, 3>::Diffs::end () const    
    {    
      return diffs + 3;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<ChannelTriggerStats, 3>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<ChannelTriggerStats, 3>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<ChannelTriggerStats, 3>::Diffs::size () const    
    {    
      return 3;      
    }    
    inline    
    void    
    DiffStats<ChannelTriggerStats, 3>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 3; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    ChannelTriggerStats::hits () const    
    {    
      return values[ChannelTriggerStats::HITS];      
    }    
    inline    
    DiffStats<ChannelTriggerStats, 3>::Diffs&     
    DiffStats<ChannelTriggerStats, 3>::Diffs::hits (const stats_diff_type& value)    
    {    
      diffs[ChannelTriggerStats::HITS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelTriggerStats, 3>::Diffs::hits () const    
    {    
      return diffs[ChannelTriggerStats::HITS];      
    }    
    inline    
    stats_value_type    
    ChannelTriggerStats::imps () const    
    {    
      return values[ChannelTriggerStats::IMPS];      
    }    
    inline    
    DiffStats<ChannelTriggerStats, 3>::Diffs&     
    DiffStats<ChannelTriggerStats, 3>::Diffs::imps (const stats_diff_type& value)    
    {    
      diffs[ChannelTriggerStats::IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelTriggerStats, 3>::Diffs::imps () const    
    {    
      return diffs[ChannelTriggerStats::IMPS];      
    }    
    inline    
    stats_value_type    
    ChannelTriggerStats::clicks () const    
    {    
      return values[ChannelTriggerStats::CLICKS];      
    }    
    inline    
    DiffStats<ChannelTriggerStats, 3>::Diffs&     
    DiffStats<ChannelTriggerStats, 3>::Diffs::clicks (const stats_diff_type& value)    
    {    
      diffs[ChannelTriggerStats::CLICKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelTriggerStats, 3>::Diffs::clicks () const    
    {    
      return diffs[ChannelTriggerStats::CLICKS];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_CHANNELTRIGGERSTATS_HPP

