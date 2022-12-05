
#ifndef __AUTOTESTS_COMMONS_STATS_CHANNELCOUNTSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_CHANNELCOUNTSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class ChannelCountStats:    
      public DiffStats<ChannelCountStats, 1>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          USERS_COUNT          
        };        
        typedef DiffStats<ChannelCountStats, 1> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            int colo_id_;            
            bool colo_id_used_;            
            bool colo_id_null_;            
            std::string channel_type_;            
            bool channel_type_used_;            
            bool channel_type_null_;            
            int channel_count_;            
            bool channel_count_used_;            
            bool channel_count_null_;            
            AutoTest::Time isp_sdate_;            
            bool isp_sdate_used_;            
            bool isp_sdate_null_;            
          public:          
            Key& colo_id(const int& value);            
            Key& colo_id_set_null(bool is_null = true);            
            const int& colo_id() const;            
            bool colo_id_used() const;            
            bool colo_id_is_null() const;            
            Key& channel_type(const std::string& value);            
            Key& channel_type_set_null(bool is_null = true);            
            const std::string& channel_type() const;            
            bool channel_type_used() const;            
            bool channel_type_is_null() const;            
            Key& channel_count(const int& value);            
            Key& channel_count_set_null(bool is_null = true);            
            const int& channel_count() const;            
            bool channel_count_used() const;            
            bool channel_count_is_null() const;            
            Key& isp_sdate(const AutoTest::Time& value);            
            Key& isp_sdate_set_null(bool is_null = true);            
            const AutoTest::Time& isp_sdate() const;            
            bool isp_sdate_used() const;            
            bool isp_sdate_is_null() const;            
            Key ();            
            Key (            
              const int& colo_id,              
              const std::string& channel_type,              
              const int& channel_count,              
              const AutoTest::Time& isp_sdate              
            );            
        };        
        stats_value_type users_count () const;        
        void print_idname (std::ostream& out) const;        
                
        ChannelCountStats (const Key& value);        
        ChannelCountStats (        
          const int& colo_id,          
          const std::string& channel_type,          
          const int& channel_count,          
          const AutoTest::Time& isp_sdate          
        );        
        ChannelCountStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& colo_id,          
          const std::string& channel_type,          
          const int& channel_count,          
          const AutoTest::Time& isp_sdate          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<ChannelCountStats, 1>::Diffs    
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
                
        Diffs& users_count (const stats_diff_type& value);        
        const stats_diff_type& users_count () const;        
      protected:      
        stats_diff_type diffs[1];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ChannelCountStats    
    inline    
    ChannelCountStats::ChannelCountStats ()    
      :Base("ChannelCountStats")    
    {}    
    inline    
    ChannelCountStats::ChannelCountStats (const ChannelCountStats::Key& value)    
      :Base("ChannelCountStats")    
    {    
      key_ = value;      
    }    
    inline    
    ChannelCountStats::ChannelCountStats (    
      const int& colo_id,      
      const std::string& channel_type,      
      const int& channel_count,      
      const AutoTest::Time& isp_sdate      
    )    
      :Base("ChannelCountStats")    
    {    
      key_ = Key (      
        colo_id,        
        channel_type,        
        channel_count,        
        isp_sdate        
      );      
    }    
    inline    
    ChannelCountStats::Key::Key ()    
      :colo_id_(0),channel_count_(0),isp_sdate_(default_date())    
    {    
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      channel_type_used_ = false;      
      channel_type_null_ = false;      
      channel_count_used_ = false;      
      channel_count_null_ = false;      
      isp_sdate_used_ = false;      
      isp_sdate_null_ = false;      
    }    
    inline    
    ChannelCountStats::Key::Key (    
      const int& colo_id,      
      const std::string& channel_type,      
      const int& channel_count,      
      const AutoTest::Time& isp_sdate      
    )    
      :colo_id_(colo_id),channel_count_(channel_count),isp_sdate_(isp_sdate)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      channel_type_ = channel_type;      
      channel_type_used_ = true;      
      channel_type_null_ = false;      
      channel_count_used_ = true;      
      channel_count_null_ = false;      
      isp_sdate_used_ = true;      
      isp_sdate_null_ = false;      
    }    
    inline    
    ChannelCountStats::Key&    
    ChannelCountStats::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelCountStats::Key&    
    ChannelCountStats::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelCountStats::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    ChannelCountStats::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    ChannelCountStats::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    ChannelCountStats::Key&    
    ChannelCountStats::Key::channel_type(const std::string& value)    
    {    
      channel_type_ = value;      
      channel_type_used_ = true;      
      channel_type_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelCountStats::Key&    
    ChannelCountStats::Key::channel_type_set_null(bool is_null)    
    {    
      channel_type_used_ = true;      
      channel_type_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    ChannelCountStats::Key::channel_type() const    
    {    
      return channel_type_;      
    }    
    inline    
    bool    
    ChannelCountStats::Key::channel_type_used() const    
    {    
      return channel_type_used_;      
    }    
    inline    
    bool    
    ChannelCountStats::Key::channel_type_is_null() const    
    {    
      return channel_type_null_;      
    }    
    inline    
    ChannelCountStats::Key&    
    ChannelCountStats::Key::channel_count(const int& value)    
    {    
      channel_count_ = value;      
      channel_count_used_ = true;      
      channel_count_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelCountStats::Key&    
    ChannelCountStats::Key::channel_count_set_null(bool is_null)    
    {    
      channel_count_used_ = true;      
      channel_count_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelCountStats::Key::channel_count() const    
    {    
      return channel_count_;      
    }    
    inline    
    bool    
    ChannelCountStats::Key::channel_count_used() const    
    {    
      return channel_count_used_;      
    }    
    inline    
    bool    
    ChannelCountStats::Key::channel_count_is_null() const    
    {    
      return channel_count_null_;      
    }    
    inline    
    ChannelCountStats::Key&    
    ChannelCountStats::Key::isp_sdate(const AutoTest::Time& value)    
    {    
      isp_sdate_ = value;      
      isp_sdate_used_ = true;      
      isp_sdate_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelCountStats::Key&    
    ChannelCountStats::Key::isp_sdate_set_null(bool is_null)    
    {    
      isp_sdate_used_ = true;      
      isp_sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    ChannelCountStats::Key::isp_sdate() const    
    {    
      return isp_sdate_;      
    }    
    inline    
    bool    
    ChannelCountStats::Key::isp_sdate_used() const    
    {    
      return isp_sdate_used_;      
    }    
    inline    
    bool    
    ChannelCountStats::Key::isp_sdate_is_null() const    
    {    
      return isp_sdate_null_;      
    }    
    inline    
    ChannelCountStats::Key&    
    ChannelCountStats::key (    
      const int& colo_id,      
      const std::string& channel_type,      
      const int& channel_count,      
      const AutoTest::Time& isp_sdate      
    )    
    {    
      key_ = Key (      
        colo_id,        
        channel_type,        
        channel_count,        
        isp_sdate        
      );      
      return key_;      
    }    
    inline    
    ChannelCountStats::Key&    
    ChannelCountStats::key ()    
    {    
      return key_;      
    }    
    inline    
    ChannelCountStats::Key&    
    ChannelCountStats::key (const ChannelCountStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<ChannelCountStats, 1>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<ChannelCountStats, 1>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<ChannelCountStats, 1>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<ChannelCountStats, 1>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ChannelCountStats, 1>::Diffs&     
    DiffStats<ChannelCountStats, 1>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelCountStats, 1>::Diffs&     
    DiffStats<ChannelCountStats, 1>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelCountStats, 1>::Diffs&     
    DiffStats<ChannelCountStats, 1>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelCountStats, 1>::Diffs     
    DiffStats<ChannelCountStats, 1>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<ChannelCountStats, 1>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<ChannelCountStats, 1>::Diffs     
    DiffStats<ChannelCountStats, 1>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<ChannelCountStats, 1>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<ChannelCountStats, 1>::Diffs::const_iterator    
    DiffStats<ChannelCountStats, 1>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ChannelCountStats, 1>::Diffs::const_iterator    
    DiffStats<ChannelCountStats, 1>::Diffs::end () const    
    {    
      return diffs + 1;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<ChannelCountStats, 1>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<ChannelCountStats, 1>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<ChannelCountStats, 1>::Diffs::size () const    
    {    
      return 1;      
    }    
    inline    
    void    
    DiffStats<ChannelCountStats, 1>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 1; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    ChannelCountStats::users_count () const    
    {    
      return values[ChannelCountStats::USERS_COUNT];      
    }    
    inline    
    DiffStats<ChannelCountStats, 1>::Diffs&     
    DiffStats<ChannelCountStats, 1>::Diffs::users_count (const stats_diff_type& value)    
    {    
      diffs[ChannelCountStats::USERS_COUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelCountStats, 1>::Diffs::users_count () const    
    {    
      return diffs[ChannelCountStats::USERS_COUNT];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_CHANNELCOUNTSTATS_HPP

