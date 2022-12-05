
#ifndef __AUTOTESTS_COMMONS_STATS_CHANNELIMPINVENTORY_HPP
#define __AUTOTESTS_COMMONS_STATS_CHANNELIMPINVENTORY_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class ChannelImpInventory:    
      public DiffStats<ChannelImpInventory, 13>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          IMPS = 0,          
          CLICKS,          
          ACTIONS,          
          REVENUE,          
          IMPOPS_USER_COUNT,          
          IMPS_USER_COUNT,          
          IMPS_VALUE,          
          IMPS_OTHER,          
          IMPS_OTHER_USER_COUNT,          
          IMPS_OTHER_VALUE,          
          IMPOPS_NO_IMP,          
          IMPOPS_NO_IMP_USER_COUNT,          
          IMPOPS_NO_IMP_VALUE          
        };        
        typedef DiffStats<ChannelImpInventory, 13> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            int channel_id_;            
            bool channel_id_used_;            
            bool channel_id_null_;            
            std::string ccg_type_;            
            bool ccg_type_used_;            
            bool ccg_type_null_;            
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
            Key& ccg_type(const std::string& value);            
            Key& ccg_type_set_null(bool is_null = true);            
            const std::string& ccg_type() const;            
            bool ccg_type_used() const;            
            bool ccg_type_is_null() const;            
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
              const std::string& ccg_type,              
              const int& colo_id,              
              const AutoTest::Time& sdate              
            );            
        };        
        stats_value_type imps () const;        
        stats_value_type clicks () const;        
        stats_value_type actions () const;        
        stats_value_type revenue () const;        
        stats_value_type impops_user_count () const;        
        stats_value_type imps_user_count () const;        
        stats_value_type imps_value () const;        
        stats_value_type imps_other () const;        
        stats_value_type imps_other_user_count () const;        
        stats_value_type imps_other_value () const;        
        stats_value_type impops_no_imp () const;        
        stats_value_type impops_no_imp_user_count () const;        
        stats_value_type impops_no_imp_value () const;        
        void print_idname (std::ostream& out) const;        
                
        ChannelImpInventory (const Key& value);        
        ChannelImpInventory (        
          const int& channel_id,          
          const std::string& ccg_type,          
          const int& colo_id,          
          const AutoTest::Time& sdate          
        );        
        ChannelImpInventory ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& channel_id,          
          const std::string& ccg_type,          
          const int& colo_id,          
          const AutoTest::Time& sdate          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<ChannelImpInventory, 13>::Diffs    
    {    
      public:      
        typedef stats_diff_type array_type[13];        
        typedef const stats_diff_type const_array_type[13];        
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
        Diffs& impops_user_count (const stats_diff_type& value);        
        const stats_diff_type& impops_user_count () const;        
        Diffs& imps_user_count (const stats_diff_type& value);        
        const stats_diff_type& imps_user_count () const;        
        Diffs& imps_value (const stats_diff_type& value);        
        const stats_diff_type& imps_value () const;        
        Diffs& imps_other (const stats_diff_type& value);        
        const stats_diff_type& imps_other () const;        
        Diffs& imps_other_user_count (const stats_diff_type& value);        
        const stats_diff_type& imps_other_user_count () const;        
        Diffs& imps_other_value (const stats_diff_type& value);        
        const stats_diff_type& imps_other_value () const;        
        Diffs& impops_no_imp (const stats_diff_type& value);        
        const stats_diff_type& impops_no_imp () const;        
        Diffs& impops_no_imp_user_count (const stats_diff_type& value);        
        const stats_diff_type& impops_no_imp_user_count () const;        
        Diffs& impops_no_imp_value (const stats_diff_type& value);        
        const stats_diff_type& impops_no_imp_value () const;        
      protected:      
        stats_diff_type diffs[13];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ChannelImpInventory    
    inline    
    ChannelImpInventory::ChannelImpInventory ()    
      :Base("ChannelImpInventory")    
    {}    
    inline    
    ChannelImpInventory::ChannelImpInventory (const ChannelImpInventory::Key& value)    
      :Base("ChannelImpInventory")    
    {    
      key_ = value;      
    }    
    inline    
    ChannelImpInventory::ChannelImpInventory (    
      const int& channel_id,      
      const std::string& ccg_type,      
      const int& colo_id,      
      const AutoTest::Time& sdate      
    )    
      :Base("ChannelImpInventory")    
    {    
      key_ = Key (      
        channel_id,        
        ccg_type,        
        colo_id,        
        sdate        
      );      
    }    
    inline    
    ChannelImpInventory::Key::Key ()    
      :channel_id_(0),colo_id_(0),sdate_(default_date())    
    {    
      channel_id_used_ = false;      
      channel_id_null_ = false;      
      ccg_type_used_ = false;      
      ccg_type_null_ = false;      
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      sdate_used_ = false;      
      sdate_null_ = false;      
    }    
    inline    
    ChannelImpInventory::Key::Key (    
      const int& channel_id,      
      const std::string& ccg_type,      
      const int& colo_id,      
      const AutoTest::Time& sdate      
    )    
      :channel_id_(channel_id),colo_id_(colo_id),sdate_(sdate)    
    {    
      channel_id_used_ = true;      
      channel_id_null_ = false;      
      ccg_type_ = ccg_type;      
      ccg_type_used_ = true;      
      ccg_type_null_ = false;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      sdate_used_ = true;      
      sdate_null_ = false;      
    }    
    inline    
    ChannelImpInventory::Key&    
    ChannelImpInventory::Key::channel_id(const int& value)    
    {    
      channel_id_ = value;      
      channel_id_used_ = true;      
      channel_id_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelImpInventory::Key&    
    ChannelImpInventory::Key::channel_id_set_null(bool is_null)    
    {    
      channel_id_used_ = true;      
      channel_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelImpInventory::Key::channel_id() const    
    {    
      return channel_id_;      
    }    
    inline    
    bool    
    ChannelImpInventory::Key::channel_id_used() const    
    {    
      return channel_id_used_;      
    }    
    inline    
    bool    
    ChannelImpInventory::Key::channel_id_is_null() const    
    {    
      return channel_id_null_;      
    }    
    inline    
    ChannelImpInventory::Key&    
    ChannelImpInventory::Key::ccg_type(const std::string& value)    
    {    
      ccg_type_ = value;      
      ccg_type_used_ = true;      
      ccg_type_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelImpInventory::Key&    
    ChannelImpInventory::Key::ccg_type_set_null(bool is_null)    
    {    
      ccg_type_used_ = true;      
      ccg_type_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    ChannelImpInventory::Key::ccg_type() const    
    {    
      return ccg_type_;      
    }    
    inline    
    bool    
    ChannelImpInventory::Key::ccg_type_used() const    
    {    
      return ccg_type_used_;      
    }    
    inline    
    bool    
    ChannelImpInventory::Key::ccg_type_is_null() const    
    {    
      return ccg_type_null_;      
    }    
    inline    
    ChannelImpInventory::Key&    
    ChannelImpInventory::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelImpInventory::Key&    
    ChannelImpInventory::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ChannelImpInventory::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    ChannelImpInventory::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    ChannelImpInventory::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    ChannelImpInventory::Key&    
    ChannelImpInventory::Key::sdate(const AutoTest::Time& value)    
    {    
      sdate_ = value;      
      sdate_used_ = true;      
      sdate_null_ = false;      
      return *this;      
    }    
    inline    
    ChannelImpInventory::Key&    
    ChannelImpInventory::Key::sdate_set_null(bool is_null)    
    {    
      sdate_used_ = true;      
      sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    ChannelImpInventory::Key::sdate() const    
    {    
      return sdate_;      
    }    
    inline    
    bool    
    ChannelImpInventory::Key::sdate_used() const    
    {    
      return sdate_used_;      
    }    
    inline    
    bool    
    ChannelImpInventory::Key::sdate_is_null() const    
    {    
      return sdate_null_;      
    }    
    inline    
    ChannelImpInventory::Key&    
    ChannelImpInventory::key (    
      const int& channel_id,      
      const std::string& ccg_type,      
      const int& colo_id,      
      const AutoTest::Time& sdate      
    )    
    {    
      key_ = Key (      
        channel_id,        
        ccg_type,        
        colo_id,        
        sdate        
      );      
      return key_;      
    }    
    inline    
    ChannelImpInventory::Key&    
    ChannelImpInventory::key ()    
    {    
      return key_;      
    }    
    inline    
    ChannelImpInventory::Key&    
    ChannelImpInventory::key (const ChannelImpInventory::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 13; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 13; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 13; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs&     
    DiffStats<ChannelImpInventory, 13>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 13; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs&     
    DiffStats<ChannelImpInventory, 13>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 13; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs&     
    DiffStats<ChannelImpInventory, 13>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 13; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs     
    DiffStats<ChannelImpInventory, 13>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<ChannelImpInventory, 13>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs     
    DiffStats<ChannelImpInventory, 13>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<ChannelImpInventory, 13>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs::const_iterator    
    DiffStats<ChannelImpInventory, 13>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs::const_iterator    
    DiffStats<ChannelImpInventory, 13>::Diffs::end () const    
    {    
      return diffs + 13;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<ChannelImpInventory, 13>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<ChannelImpInventory, 13>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<ChannelImpInventory, 13>::Diffs::size () const    
    {    
      return 13;      
    }    
    inline    
    void    
    DiffStats<ChannelImpInventory, 13>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 13; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    ChannelImpInventory::imps () const    
    {    
      return values[ChannelImpInventory::IMPS];      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs&     
    DiffStats<ChannelImpInventory, 13>::Diffs::imps (const stats_diff_type& value)    
    {    
      diffs[ChannelImpInventory::IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelImpInventory, 13>::Diffs::imps () const    
    {    
      return diffs[ChannelImpInventory::IMPS];      
    }    
    inline    
    stats_value_type    
    ChannelImpInventory::clicks () const    
    {    
      return values[ChannelImpInventory::CLICKS];      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs&     
    DiffStats<ChannelImpInventory, 13>::Diffs::clicks (const stats_diff_type& value)    
    {    
      diffs[ChannelImpInventory::CLICKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelImpInventory, 13>::Diffs::clicks () const    
    {    
      return diffs[ChannelImpInventory::CLICKS];      
    }    
    inline    
    stats_value_type    
    ChannelImpInventory::actions () const    
    {    
      return values[ChannelImpInventory::ACTIONS];      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs&     
    DiffStats<ChannelImpInventory, 13>::Diffs::actions (const stats_diff_type& value)    
    {    
      diffs[ChannelImpInventory::ACTIONS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelImpInventory, 13>::Diffs::actions () const    
    {    
      return diffs[ChannelImpInventory::ACTIONS];      
    }    
    inline    
    stats_value_type    
    ChannelImpInventory::revenue () const    
    {    
      return values[ChannelImpInventory::REVENUE];      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs&     
    DiffStats<ChannelImpInventory, 13>::Diffs::revenue (const stats_diff_type& value)    
    {    
      diffs[ChannelImpInventory::REVENUE] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelImpInventory, 13>::Diffs::revenue () const    
    {    
      return diffs[ChannelImpInventory::REVENUE];      
    }    
    inline    
    stats_value_type    
    ChannelImpInventory::impops_user_count () const    
    {    
      return values[ChannelImpInventory::IMPOPS_USER_COUNT];      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs&     
    DiffStats<ChannelImpInventory, 13>::Diffs::impops_user_count (const stats_diff_type& value)    
    {    
      diffs[ChannelImpInventory::IMPOPS_USER_COUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelImpInventory, 13>::Diffs::impops_user_count () const    
    {    
      return diffs[ChannelImpInventory::IMPOPS_USER_COUNT];      
    }    
    inline    
    stats_value_type    
    ChannelImpInventory::imps_user_count () const    
    {    
      return values[ChannelImpInventory::IMPS_USER_COUNT];      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs&     
    DiffStats<ChannelImpInventory, 13>::Diffs::imps_user_count (const stats_diff_type& value)    
    {    
      diffs[ChannelImpInventory::IMPS_USER_COUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelImpInventory, 13>::Diffs::imps_user_count () const    
    {    
      return diffs[ChannelImpInventory::IMPS_USER_COUNT];      
    }    
    inline    
    stats_value_type    
    ChannelImpInventory::imps_value () const    
    {    
      return values[ChannelImpInventory::IMPS_VALUE];      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs&     
    DiffStats<ChannelImpInventory, 13>::Diffs::imps_value (const stats_diff_type& value)    
    {    
      diffs[ChannelImpInventory::IMPS_VALUE] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelImpInventory, 13>::Diffs::imps_value () const    
    {    
      return diffs[ChannelImpInventory::IMPS_VALUE];      
    }    
    inline    
    stats_value_type    
    ChannelImpInventory::imps_other () const    
    {    
      return values[ChannelImpInventory::IMPS_OTHER];      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs&     
    DiffStats<ChannelImpInventory, 13>::Diffs::imps_other (const stats_diff_type& value)    
    {    
      diffs[ChannelImpInventory::IMPS_OTHER] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelImpInventory, 13>::Diffs::imps_other () const    
    {    
      return diffs[ChannelImpInventory::IMPS_OTHER];      
    }    
    inline    
    stats_value_type    
    ChannelImpInventory::imps_other_user_count () const    
    {    
      return values[ChannelImpInventory::IMPS_OTHER_USER_COUNT];      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs&     
    DiffStats<ChannelImpInventory, 13>::Diffs::imps_other_user_count (const stats_diff_type& value)    
    {    
      diffs[ChannelImpInventory::IMPS_OTHER_USER_COUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelImpInventory, 13>::Diffs::imps_other_user_count () const    
    {    
      return diffs[ChannelImpInventory::IMPS_OTHER_USER_COUNT];      
    }    
    inline    
    stats_value_type    
    ChannelImpInventory::imps_other_value () const    
    {    
      return values[ChannelImpInventory::IMPS_OTHER_VALUE];      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs&     
    DiffStats<ChannelImpInventory, 13>::Diffs::imps_other_value (const stats_diff_type& value)    
    {    
      diffs[ChannelImpInventory::IMPS_OTHER_VALUE] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelImpInventory, 13>::Diffs::imps_other_value () const    
    {    
      return diffs[ChannelImpInventory::IMPS_OTHER_VALUE];      
    }    
    inline    
    stats_value_type    
    ChannelImpInventory::impops_no_imp () const    
    {    
      return values[ChannelImpInventory::IMPOPS_NO_IMP];      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs&     
    DiffStats<ChannelImpInventory, 13>::Diffs::impops_no_imp (const stats_diff_type& value)    
    {    
      diffs[ChannelImpInventory::IMPOPS_NO_IMP] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelImpInventory, 13>::Diffs::impops_no_imp () const    
    {    
      return diffs[ChannelImpInventory::IMPOPS_NO_IMP];      
    }    
    inline    
    stats_value_type    
    ChannelImpInventory::impops_no_imp_user_count () const    
    {    
      return values[ChannelImpInventory::IMPOPS_NO_IMP_USER_COUNT];      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs&     
    DiffStats<ChannelImpInventory, 13>::Diffs::impops_no_imp_user_count (const stats_diff_type& value)    
    {    
      diffs[ChannelImpInventory::IMPOPS_NO_IMP_USER_COUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelImpInventory, 13>::Diffs::impops_no_imp_user_count () const    
    {    
      return diffs[ChannelImpInventory::IMPOPS_NO_IMP_USER_COUNT];      
    }    
    inline    
    stats_value_type    
    ChannelImpInventory::impops_no_imp_value () const    
    {    
      return values[ChannelImpInventory::IMPOPS_NO_IMP_VALUE];      
    }    
    inline    
    DiffStats<ChannelImpInventory, 13>::Diffs&     
    DiffStats<ChannelImpInventory, 13>::Diffs::impops_no_imp_value (const stats_diff_type& value)    
    {    
      diffs[ChannelImpInventory::IMPOPS_NO_IMP_VALUE] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ChannelImpInventory, 13>::Diffs::impops_no_imp_value () const    
    {    
      return diffs[ChannelImpInventory::IMPOPS_NO_IMP_VALUE];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_CHANNELIMPINVENTORY_HPP

