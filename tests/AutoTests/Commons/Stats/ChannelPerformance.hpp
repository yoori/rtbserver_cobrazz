
#ifndef __AUTOTESTS_COMMONS_STATS_CHANNELPERFORMANCE_HPP
#define __AUTOTESTS_COMMONS_STATS_CHANNELPERFORMANCE_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class ChannelPerformance:    
      public DiffStats<ChannelPerformance, 4>    
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
        typedef DiffStats<ChannelPerformance, 4> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            int channel_id_;            
            bool channel_id_used_;            
            bool channel_id_null_;            
            AutoTest::Time last_use_;            
            bool last_use_used_;            
            bool last_use_null_;            
          public:          
            Key& channel_id(const int& value);            
            Key& channel_id_set_null(bool is_null = true);            
            const int& channel_id() const;            
            bool channel_id_used() const;            
            bool channel_id_is_null() const;            
            Key& last_use(const AutoTest::Time& value);            
            Key& last_use_set_null(bool is_null = true);            
            const AutoTest::Time& last_use() const;            
            bool last_use_used() const;            
            Key& last_use_set_used(bool used);            
            bool last_use_is_null() const;            
            Key ();            
            Key (            
              const int& channel_id,              
              const AutoTest::Time& last_use              
            );            
        };        
        stats_value_type imps () const;        
        stats_value_type clicks () const;        
        stats_value_type actions () const;        
        stats_value_type revenue () const;        
        void print_idname (std::ostream& out) const;        
                
        ChannelPerformance (const Key& value);        
        ChannelPerformance (        
          const int& channel_id,          
          const AutoTest::Time& last_use          
        );        
        ChannelPerformance ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& channel_id,          
          const AutoTest::Time& last_use          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<ChannelPerformance, 4>::Diffs    
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
    ///////////////////////////////// ChannelPerformance    
    inline    
    ChannelPerformance::ChannelPerformance ()    
      :Base("ChannelPerformance")    
    {}    
    inline    
    ChannelPerformance::ChannelPerformance (const ChannelPerformance::Key& value)    
      :Base("ChannelPerformance")    
    {    
      key_ = value;      
    }    
    inline    
    ChannelPerformance::ChannelPerformance (    
      const int& channel_id,      
      const AutoTest::Time& last_use      
    )    
      :Base("ChannelPerformance")    
    {    
      key_ = Key (      
        channel_id,        
        last_use        
      );      
    }    
    inline    
    ChannelPerformance::Key::Key ()    
      :channel_id_(0),last_use_(default_date())    
    {    
      channel_id_used_ = false;      
      channel_id_null_ = false;      
      last_use_used_ = false;      
      last_use_null_ = false;      
    }    
    inline    
    ChannelPerformance::Key::Key (    
      const int& channel_id,      
      const AutoTest::Time& last_use      
    )    
      :channel_id_(channel_id),last_use_(last_use)    
    {    
      channel_id_used_ = true;      
      channel_id_null_ = false;      
      last_use_used_ = true;      
      last_use_null_ = false;      
    }    
    inline    
    ChannelPerformance::Key&    
    ChannelPerformance::Key::channel_id(const int& value)    
    {    
      channel_id_ = value;      
      channel_id_used_ = true;      
      channel_id_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelPerformance::Key&    
    ChannelPerformance::Key::channel_id_set_null(bool is_null)    
    {    
      channel_id_used_ = true;      
      channel_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelPerformance::Key::channel_id() const    
    {    
      return channel_id_;      
    }    
    inline    
    bool    
    ChannelPerformance::Key::channel_id_used() const    
    {    
      return channel_id_used_;      
    }    
    inline    
    bool    
    ChannelPerformance::Key::channel_id_is_null() const    
    {    
      return channel_id_null_;      
    }    
    inline    
    ChannelPerformance::Key&    
    ChannelPerformance::Key::last_use(const AutoTest::Time& value)    
    {    
      last_use_ = value;      
      last_use_used_ = true;      
      last_use_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelPerformance::Key&    
    ChannelPerformance::Key::last_use_set_null(bool is_null)    
    {    
      last_use_used_ = true;      
      last_use_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    ChannelPerformance::Key::last_use() const    
    {    
      return last_use_;      
    }    
    inline    
    bool    
    ChannelPerformance::Key::last_use_used() const    
    {    
      return last_use_used_;      
    }    
    inline    
    ChannelPerformance::Key&    
    ChannelPerformance::Key::last_use_set_used(bool used)    
    {    
      last_use_used_ = used;      
      return *this;      
    }    
    inline    
    bool    
    ChannelPerformance::Key::last_use_is_null() const    
    {    
      return last_use_null_;      
    }    
    inline    
    ChannelPerformance::Key&    
    ChannelPerformance::key (    
      const int& channel_id,      
      const AutoTest::Time& last_use      
    )    
    {    
      key_ = Key (      
        channel_id,        
        last_use        
      );      
      return key_;      
    }    
    inline    
    ChannelPerformance::Key&    
    ChannelPerformance::key ()    
    {    
      return key_;      
    }    
    inline    
    ChannelPerformance::Key&    
    ChannelPerformance::key (const ChannelPerformance::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<ChannelPerformance, 4>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<ChannelPerformance, 4>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<ChannelPerformance, 4>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<ChannelPerformance, 4>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ChannelPerformance, 4>::Diffs&     
    DiffStats<ChannelPerformance, 4>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelPerformance, 4>::Diffs&     
    DiffStats<ChannelPerformance, 4>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelPerformance, 4>::Diffs&     
    DiffStats<ChannelPerformance, 4>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 4; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelPerformance, 4>::Diffs     
    DiffStats<ChannelPerformance, 4>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<ChannelPerformance, 4>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<ChannelPerformance, 4>::Diffs     
    DiffStats<ChannelPerformance, 4>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<ChannelPerformance, 4>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<ChannelPerformance, 4>::Diffs::const_iterator    
    DiffStats<ChannelPerformance, 4>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ChannelPerformance, 4>::Diffs::const_iterator    
    DiffStats<ChannelPerformance, 4>::Diffs::end () const    
    {    
      return diffs + 4;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<ChannelPerformance, 4>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<ChannelPerformance, 4>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<ChannelPerformance, 4>::Diffs::size () const    
    {    
      return 4;      
    }    
    inline    
    void    
    DiffStats<ChannelPerformance, 4>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 4; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    ChannelPerformance::imps () const    
    {    
      return values[ChannelPerformance::IMPS];      
    }    
    inline    
    DiffStats<ChannelPerformance, 4>::Diffs&     
    DiffStats<ChannelPerformance, 4>::Diffs::imps (const stats_diff_type& value)    
    {    
      diffs[ChannelPerformance::IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelPerformance, 4>::Diffs::imps () const    
    {    
      return diffs[ChannelPerformance::IMPS];      
    }    
    inline    
    stats_value_type    
    ChannelPerformance::clicks () const    
    {    
      return values[ChannelPerformance::CLICKS];      
    }    
    inline    
    DiffStats<ChannelPerformance, 4>::Diffs&     
    DiffStats<ChannelPerformance, 4>::Diffs::clicks (const stats_diff_type& value)    
    {    
      diffs[ChannelPerformance::CLICKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelPerformance, 4>::Diffs::clicks () const    
    {    
      return diffs[ChannelPerformance::CLICKS];      
    }    
    inline    
    stats_value_type    
    ChannelPerformance::actions () const    
    {    
      return values[ChannelPerformance::ACTIONS];      
    }    
    inline    
    DiffStats<ChannelPerformance, 4>::Diffs&     
    DiffStats<ChannelPerformance, 4>::Diffs::actions (const stats_diff_type& value)    
    {    
      diffs[ChannelPerformance::ACTIONS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelPerformance, 4>::Diffs::actions () const    
    {    
      return diffs[ChannelPerformance::ACTIONS];      
    }    
    inline    
    stats_value_type    
    ChannelPerformance::revenue () const    
    {    
      return values[ChannelPerformance::REVENUE];      
    }    
    inline    
    DiffStats<ChannelPerformance, 4>::Diffs&     
    DiffStats<ChannelPerformance, 4>::Diffs::revenue (const stats_diff_type& value)    
    {    
      diffs[ChannelPerformance::REVENUE] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelPerformance, 4>::Diffs::revenue () const    
    {    
      return diffs[ChannelPerformance::REVENUE];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_CHANNELPERFORMANCE_HPP

