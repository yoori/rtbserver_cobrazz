
#ifndef __AUTOTESTS_COMMONS_STATS_CHANNELINVENTORYBYCPMSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_CHANNELINVENTORYBYCPMSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class ChannelInventoryByCPMStats:    
      public DiffStats<ChannelInventoryByCPMStats, 2>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          USER_COUNT = 0,          
          IMPOPS          
        };        
        typedef DiffStats<ChannelInventoryByCPMStats, 2> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            int channel_id_;            
            bool channel_id_used_;            
            bool channel_id_null_;            
            AutoTest::Time sdate_;            
            bool sdate_used_;            
            bool sdate_null_;            
            int creative_size_id_;            
            bool creative_size_id_used_;            
            bool creative_size_id_null_;            
            int colo_id_;            
            bool colo_id_used_;            
            bool colo_id_null_;            
            std::string country_code_;            
            bool country_code_used_;            
            bool country_code_null_;            
            double ecpm_;            
            bool ecpm_used_;            
            bool ecpm_null_;            
          public:          
            Key& channel_id(const int& value);            
            Key& channel_id_set_null(bool is_null = true);            
            const int& channel_id() const;            
            bool channel_id_used() const;            
            bool channel_id_is_null() const;            
            Key& sdate(const AutoTest::Time& value);            
            Key& sdate_set_null(bool is_null = true);            
            const AutoTest::Time& sdate() const;            
            bool sdate_used() const;            
            bool sdate_is_null() const;            
            Key& creative_size_id(const int& value);            
            Key& creative_size_id_set_null(bool is_null = true);            
            const int& creative_size_id() const;            
            bool creative_size_id_used() const;            
            bool creative_size_id_is_null() const;            
            Key& colo_id(const int& value);            
            Key& colo_id_set_null(bool is_null = true);            
            const int& colo_id() const;            
            bool colo_id_used() const;            
            bool colo_id_is_null() const;            
            Key& country_code(const std::string& value);            
            Key& country_code_set_null(bool is_null = true);            
            const std::string& country_code() const;            
            bool country_code_used() const;            
            bool country_code_is_null() const;            
            Key& ecpm(const double& value);            
            Key& ecpm_set_null(bool is_null = true);            
            const double& ecpm() const;            
            bool ecpm_used() const;            
            bool ecpm_is_null() const;            
            Key ();            
            Key (            
              const int& channel_id,              
              const AutoTest::Time& sdate,              
              const int& creative_size_id,              
              const int& colo_id,              
              const std::string& country_code,              
              const double& ecpm              
            );            
        };        
        stats_value_type user_count () const;        
        stats_value_type impops () const;        
        void print_idname (std::ostream& out) const;        
                
        ChannelInventoryByCPMStats (const Key& value);        
        ChannelInventoryByCPMStats (        
          const int& channel_id,          
          const AutoTest::Time& sdate,          
          const int& creative_size_id,          
          const int& colo_id,          
          const std::string& country_code,          
          const double& ecpm          
        );        
        ChannelInventoryByCPMStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& channel_id,          
          const AutoTest::Time& sdate,          
          const int& creative_size_id,          
          const int& colo_id,          
          const std::string& country_code,          
          const double& ecpm          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<ChannelInventoryByCPMStats, 2>::Diffs    
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
                
        Diffs& user_count (const stats_diff_type& value);        
        const stats_diff_type& user_count () const;        
        Diffs& impops (const stats_diff_type& value);        
        const stats_diff_type& impops () const;        
      protected:      
        stats_diff_type diffs[2];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ChannelInventoryByCPMStats    
    inline    
    ChannelInventoryByCPMStats::ChannelInventoryByCPMStats ()    
      :Base("ChannelInventoryByCPMStats")    
    {}    
    inline    
    ChannelInventoryByCPMStats::ChannelInventoryByCPMStats (const ChannelInventoryByCPMStats::Key& value)    
      :Base("ChannelInventoryByCPMStats")    
    {    
      key_ = value;      
    }    
    inline    
    ChannelInventoryByCPMStats::ChannelInventoryByCPMStats (    
      const int& channel_id,      
      const AutoTest::Time& sdate,      
      const int& creative_size_id,      
      const int& colo_id,      
      const std::string& country_code,      
      const double& ecpm      
    )    
      :Base("ChannelInventoryByCPMStats")    
    {    
      key_ = Key (      
        channel_id,        
        sdate,        
        creative_size_id,        
        colo_id,        
        country_code,        
        ecpm        
      );      
    }    
    inline    
    ChannelInventoryByCPMStats::Key::Key ()    
      :channel_id_(0),sdate_(default_date()),creative_size_id_(0),colo_id_(0),ecpm_(0)    
    {    
      channel_id_used_ = false;      
      channel_id_null_ = false;      
      sdate_used_ = false;      
      sdate_null_ = false;      
      creative_size_id_used_ = false;      
      creative_size_id_null_ = false;      
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      country_code_used_ = false;      
      country_code_null_ = false;      
      ecpm_used_ = false;      
      ecpm_null_ = false;      
    }    
    inline    
    ChannelInventoryByCPMStats::Key::Key (    
      const int& channel_id,      
      const AutoTest::Time& sdate,      
      const int& creative_size_id,      
      const int& colo_id,      
      const std::string& country_code,      
      const double& ecpm      
    )    
      :channel_id_(channel_id),sdate_(sdate),creative_size_id_(creative_size_id),colo_id_(colo_id),ecpm_(ecpm)    
    {    
      channel_id_used_ = true;      
      channel_id_null_ = false;      
      sdate_used_ = true;      
      sdate_null_ = false;      
      creative_size_id_used_ = true;      
      creative_size_id_null_ = false;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      country_code_ = country_code;      
      country_code_used_ = true;      
      country_code_null_ = false;      
      ecpm_used_ = true;      
      ecpm_null_ = false;      
    }    
    inline    
    ChannelInventoryByCPMStats::Key&    
    ChannelInventoryByCPMStats::Key::channel_id(const int& value)    
    {    
      channel_id_ = value;      
      channel_id_used_ = true;      
      channel_id_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelInventoryByCPMStats::Key&    
    ChannelInventoryByCPMStats::Key::channel_id_set_null(bool is_null)    
    {    
      channel_id_used_ = true;      
      channel_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelInventoryByCPMStats::Key::channel_id() const    
    {    
      return channel_id_;      
    }    
    inline    
    bool    
    ChannelInventoryByCPMStats::Key::channel_id_used() const    
    {    
      return channel_id_used_;      
    }    
    inline    
    bool    
    ChannelInventoryByCPMStats::Key::channel_id_is_null() const    
    {    
      return channel_id_null_;      
    }    
    inline    
    ChannelInventoryByCPMStats::Key&    
    ChannelInventoryByCPMStats::Key::sdate(const AutoTest::Time& value)    
    {    
      sdate_ = value;      
      sdate_used_ = true;      
      sdate_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelInventoryByCPMStats::Key&    
    ChannelInventoryByCPMStats::Key::sdate_set_null(bool is_null)    
    {    
      sdate_used_ = true;      
      sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    ChannelInventoryByCPMStats::Key::sdate() const    
    {    
      return sdate_;      
    }    
    inline    
    bool    
    ChannelInventoryByCPMStats::Key::sdate_used() const    
    {    
      return sdate_used_;      
    }    
    inline    
    bool    
    ChannelInventoryByCPMStats::Key::sdate_is_null() const    
    {    
      return sdate_null_;      
    }    
    inline    
    ChannelInventoryByCPMStats::Key&    
    ChannelInventoryByCPMStats::Key::creative_size_id(const int& value)    
    {    
      creative_size_id_ = value;      
      creative_size_id_used_ = true;      
      creative_size_id_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelInventoryByCPMStats::Key&    
    ChannelInventoryByCPMStats::Key::creative_size_id_set_null(bool is_null)    
    {    
      creative_size_id_used_ = true;      
      creative_size_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelInventoryByCPMStats::Key::creative_size_id() const    
    {    
      return creative_size_id_;      
    }    
    inline    
    bool    
    ChannelInventoryByCPMStats::Key::creative_size_id_used() const    
    {    
      return creative_size_id_used_;      
    }    
    inline    
    bool    
    ChannelInventoryByCPMStats::Key::creative_size_id_is_null() const    
    {    
      return creative_size_id_null_;      
    }    
    inline    
    ChannelInventoryByCPMStats::Key&    
    ChannelInventoryByCPMStats::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelInventoryByCPMStats::Key&    
    ChannelInventoryByCPMStats::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelInventoryByCPMStats::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    ChannelInventoryByCPMStats::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    ChannelInventoryByCPMStats::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    ChannelInventoryByCPMStats::Key&    
    ChannelInventoryByCPMStats::Key::country_code(const std::string& value)    
    {    
      country_code_ = value;      
      country_code_used_ = true;      
      country_code_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelInventoryByCPMStats::Key&    
    ChannelInventoryByCPMStats::Key::country_code_set_null(bool is_null)    
    {    
      country_code_used_ = true;      
      country_code_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    ChannelInventoryByCPMStats::Key::country_code() const    
    {    
      return country_code_;      
    }    
    inline    
    bool    
    ChannelInventoryByCPMStats::Key::country_code_used() const    
    {    
      return country_code_used_;      
    }    
    inline    
    bool    
    ChannelInventoryByCPMStats::Key::country_code_is_null() const    
    {    
      return country_code_null_;      
    }    
    inline    
    ChannelInventoryByCPMStats::Key&    
    ChannelInventoryByCPMStats::Key::ecpm(const double& value)    
    {    
      ecpm_ = value;      
      ecpm_used_ = true;      
      ecpm_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelInventoryByCPMStats::Key&    
    ChannelInventoryByCPMStats::Key::ecpm_set_null(bool is_null)    
    {    
      ecpm_used_ = true;      
      ecpm_null_ = is_null;      
      return *this;      
    }    
    inline    
    const double&    
    ChannelInventoryByCPMStats::Key::ecpm() const    
    {    
      return ecpm_;      
    }    
    inline    
    bool    
    ChannelInventoryByCPMStats::Key::ecpm_used() const    
    {    
      return ecpm_used_;      
    }    
    inline    
    bool    
    ChannelInventoryByCPMStats::Key::ecpm_is_null() const    
    {    
      return ecpm_null_;      
    }    
    inline    
    ChannelInventoryByCPMStats::Key&    
    ChannelInventoryByCPMStats::key (    
      const int& channel_id,      
      const AutoTest::Time& sdate,      
      const int& creative_size_id,      
      const int& colo_id,      
      const std::string& country_code,      
      const double& ecpm      
    )    
    {    
      key_ = Key (      
        channel_id,        
        sdate,        
        creative_size_id,        
        colo_id,        
        country_code,        
        ecpm        
      );      
      return key_;      
    }    
    inline    
    ChannelInventoryByCPMStats::Key&    
    ChannelInventoryByCPMStats::key ()    
    {    
      return key_;      
    }    
    inline    
    ChannelInventoryByCPMStats::Key&    
    ChannelInventoryByCPMStats::key (const ChannelInventoryByCPMStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs&     
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs&     
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs&     
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs     
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<ChannelInventoryByCPMStats, 2>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs     
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<ChannelInventoryByCPMStats, 2>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs::const_iterator    
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs::const_iterator    
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs::end () const    
    {    
      return diffs + 2;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs::size () const    
    {    
      return 2;      
    }    
    inline    
    void    
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 2; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    ChannelInventoryByCPMStats::user_count () const    
    {    
      return values[ChannelInventoryByCPMStats::USER_COUNT];      
    }    
    inline    
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs&     
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs::user_count (const stats_diff_type& value)    
    {    
      diffs[ChannelInventoryByCPMStats::USER_COUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs::user_count () const    
    {    
      return diffs[ChannelInventoryByCPMStats::USER_COUNT];      
    }    
    inline    
    stats_value_type    
    ChannelInventoryByCPMStats::impops () const    
    {    
      return values[ChannelInventoryByCPMStats::IMPOPS];      
    }    
    inline    
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs&     
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs::impops (const stats_diff_type& value)    
    {    
      diffs[ChannelInventoryByCPMStats::IMPOPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelInventoryByCPMStats, 2>::Diffs::impops () const    
    {    
      return diffs[ChannelInventoryByCPMStats::IMPOPS];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_CHANNELINVENTORYBYCPMSTATS_HPP

