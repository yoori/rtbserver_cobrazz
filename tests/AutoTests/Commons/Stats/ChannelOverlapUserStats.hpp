
#ifndef __AUTOTESTS_COMMONS_STATS_CHANNELOVERLAPUSERSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_CHANNELOVERLAPUSERSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class ChannelOverlapUserStats:    
      public DiffStats<ChannelOverlapUserStats, 1>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          USERS          
        };        
        typedef DiffStats<ChannelOverlapUserStats, 1> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            int channel1_;            
            bool channel1_used_;            
            bool channel1_null_;            
            int channel2_;            
            bool channel2_used_;            
            bool channel2_null_;            
            AutoTest::Time sdate_;            
            bool sdate_used_;            
            bool sdate_null_;            
          public:          
            Key& channel1(const int& value);            
            Key& channel1_set_null(bool is_null = true);            
            const int& channel1() const;            
            bool channel1_used() const;            
            bool channel1_is_null() const;            
            Key& channel2(const int& value);            
            Key& channel2_set_null(bool is_null = true);            
            const int& channel2() const;            
            bool channel2_used() const;            
            bool channel2_is_null() const;            
            Key& sdate(const AutoTest::Time& value);            
            Key& sdate_set_null(bool is_null = true);            
            const AutoTest::Time& sdate() const;            
            bool sdate_used() const;            
            bool sdate_is_null() const;            
            Key ();            
            Key (            
              const int& channel1,              
              const int& channel2,              
              const AutoTest::Time& sdate              
            );            
        };        
        stats_value_type users () const;        
        void print_idname (std::ostream& out) const;        
                
        ChannelOverlapUserStats (const Key& value);        
        ChannelOverlapUserStats (        
          const int& channel1,          
          const int& channel2,          
          const AutoTest::Time& sdate          
        );        
        ChannelOverlapUserStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& channel1,          
          const int& channel2,          
          const AutoTest::Time& sdate          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<ChannelOverlapUserStats, 1>::Diffs    
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
                
        Diffs& users (const stats_diff_type& value);        
        const stats_diff_type& users () const;        
      protected:      
        stats_diff_type diffs[1];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ChannelOverlapUserStats    
    inline    
    ChannelOverlapUserStats::ChannelOverlapUserStats ()    
      :Base("ChannelOverlapUserStats")    
    {}    
    inline    
    ChannelOverlapUserStats::ChannelOverlapUserStats (const ChannelOverlapUserStats::Key& value)    
      :Base("ChannelOverlapUserStats")    
    {    
      key_ = value;      
    }    
    inline    
    ChannelOverlapUserStats::ChannelOverlapUserStats (    
      const int& channel1,      
      const int& channel2,      
      const AutoTest::Time& sdate      
    )    
      :Base("ChannelOverlapUserStats")    
    {    
      key_ = Key (      
        channel1,        
        channel2,        
        sdate        
      );      
    }    
    inline    
    ChannelOverlapUserStats::Key::Key ()    
      :channel1_(0),channel2_(0),sdate_(default_date())    
    {    
      channel1_used_ = false;      
      channel1_null_ = false;      
      channel2_used_ = false;      
      channel2_null_ = false;      
      sdate_used_ = false;      
      sdate_null_ = false;      
    }    
    inline    
    ChannelOverlapUserStats::Key::Key (    
      const int& channel1,      
      const int& channel2,      
      const AutoTest::Time& sdate      
    )    
      :channel1_(channel1),channel2_(channel2),sdate_(sdate)    
    {    
      channel1_used_ = true;      
      channel1_null_ = false;      
      channel2_used_ = true;      
      channel2_null_ = false;      
      sdate_used_ = true;      
      sdate_null_ = false;      
    }    
    inline    
    ChannelOverlapUserStats::Key&    
    ChannelOverlapUserStats::Key::channel1(const int& value)    
    {    
      channel1_ = value;      
      channel1_used_ = true;      
      channel1_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelOverlapUserStats::Key&    
    ChannelOverlapUserStats::Key::channel1_set_null(bool is_null)    
    {    
      channel1_used_ = true;      
      channel1_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelOverlapUserStats::Key::channel1() const    
    {    
      return channel1_;      
    }    
    inline    
    bool    
    ChannelOverlapUserStats::Key::channel1_used() const    
    {    
      return channel1_used_;      
    }    
    inline    
    bool    
    ChannelOverlapUserStats::Key::channel1_is_null() const    
    {    
      return channel1_null_;      
    }    
    inline    
    ChannelOverlapUserStats::Key&    
    ChannelOverlapUserStats::Key::channel2(const int& value)    
    {    
      channel2_ = value;      
      channel2_used_ = true;      
      channel2_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelOverlapUserStats::Key&    
    ChannelOverlapUserStats::Key::channel2_set_null(bool is_null)    
    {    
      channel2_used_ = true;      
      channel2_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelOverlapUserStats::Key::channel2() const    
    {    
      return channel2_;      
    }    
    inline    
    bool    
    ChannelOverlapUserStats::Key::channel2_used() const    
    {    
      return channel2_used_;      
    }    
    inline    
    bool    
    ChannelOverlapUserStats::Key::channel2_is_null() const    
    {    
      return channel2_null_;      
    }    
    inline    
    ChannelOverlapUserStats::Key&    
    ChannelOverlapUserStats::Key::sdate(const AutoTest::Time& value)    
    {    
      sdate_ = value;      
      sdate_used_ = true;      
      sdate_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelOverlapUserStats::Key&    
    ChannelOverlapUserStats::Key::sdate_set_null(bool is_null)    
    {    
      sdate_used_ = true;      
      sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    ChannelOverlapUserStats::Key::sdate() const    
    {    
      return sdate_;      
    }    
    inline    
    bool    
    ChannelOverlapUserStats::Key::sdate_used() const    
    {    
      return sdate_used_;      
    }    
    inline    
    bool    
    ChannelOverlapUserStats::Key::sdate_is_null() const    
    {    
      return sdate_null_;      
    }    
    inline    
    ChannelOverlapUserStats::Key&    
    ChannelOverlapUserStats::key (    
      const int& channel1,      
      const int& channel2,      
      const AutoTest::Time& sdate      
    )    
    {    
      key_ = Key (      
        channel1,        
        channel2,        
        sdate        
      );      
      return key_;      
    }    
    inline    
    ChannelOverlapUserStats::Key&    
    ChannelOverlapUserStats::key ()    
    {    
      return key_;      
    }    
    inline    
    ChannelOverlapUserStats::Key&    
    ChannelOverlapUserStats::key (const ChannelOverlapUserStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<ChannelOverlapUserStats, 1>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<ChannelOverlapUserStats, 1>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<ChannelOverlapUserStats, 1>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<ChannelOverlapUserStats, 1>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ChannelOverlapUserStats, 1>::Diffs&     
    DiffStats<ChannelOverlapUserStats, 1>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelOverlapUserStats, 1>::Diffs&     
    DiffStats<ChannelOverlapUserStats, 1>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelOverlapUserStats, 1>::Diffs&     
    DiffStats<ChannelOverlapUserStats, 1>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelOverlapUserStats, 1>::Diffs     
    DiffStats<ChannelOverlapUserStats, 1>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<ChannelOverlapUserStats, 1>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<ChannelOverlapUserStats, 1>::Diffs     
    DiffStats<ChannelOverlapUserStats, 1>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<ChannelOverlapUserStats, 1>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<ChannelOverlapUserStats, 1>::Diffs::const_iterator    
    DiffStats<ChannelOverlapUserStats, 1>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ChannelOverlapUserStats, 1>::Diffs::const_iterator    
    DiffStats<ChannelOverlapUserStats, 1>::Diffs::end () const    
    {    
      return diffs + 1;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<ChannelOverlapUserStats, 1>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<ChannelOverlapUserStats, 1>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<ChannelOverlapUserStats, 1>::Diffs::size () const    
    {    
      return 1;      
    }    
    inline    
    void    
    DiffStats<ChannelOverlapUserStats, 1>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 1; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    ChannelOverlapUserStats::users () const    
    {    
      return values[ChannelOverlapUserStats::USERS];      
    }    
    inline    
    DiffStats<ChannelOverlapUserStats, 1>::Diffs&     
    DiffStats<ChannelOverlapUserStats, 1>::Diffs::users (const stats_diff_type& value)    
    {    
      diffs[ChannelOverlapUserStats::USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelOverlapUserStats, 1>::Diffs::users () const    
    {    
      return diffs[ChannelOverlapUserStats::USERS];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_CHANNELOVERLAPUSERSTATS_HPP

