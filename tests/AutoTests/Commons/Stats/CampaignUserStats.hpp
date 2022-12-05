
#ifndef __AUTOTESTS_COMMONS_STATS_CAMPAIGNUSERSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_CAMPAIGNUSERSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class CampaignUserStats:    
      public DiffStats<CampaignUserStats, 2>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          UNIQUE_USERS = 0,          
          CONTROL_SUM          
        };        
        typedef DiffStats<CampaignUserStats, 2> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            int campaign_id_;            
            bool campaign_id_used_;            
            bool campaign_id_null_;            
            int colo_id_;            
            bool colo_id_used_;            
            bool colo_id_null_;            
            AutoTest::Time last_appearance_date_;            
            bool last_appearance_date_used_;            
            bool last_appearance_date_null_;            
            AutoTest::Time adv_sdate_;            
            bool adv_sdate_used_;            
            bool adv_sdate_null_;            
          public:          
            Key& campaign_id(const int& value);            
            Key& campaign_id_set_null(bool is_null = true);            
            const int& campaign_id() const;            
            bool campaign_id_used() const;            
            bool campaign_id_is_null() const;            
            Key& colo_id(const int& value);            
            Key& colo_id_set_null(bool is_null = true);            
            const int& colo_id() const;            
            bool colo_id_used() const;            
            bool colo_id_is_null() const;            
            Key& last_appearance_date(const AutoTest::Time& value);            
            Key& last_appearance_date_set_null(bool is_null = true);            
            const AutoTest::Time& last_appearance_date() const;            
            bool last_appearance_date_used() const;            
            bool last_appearance_date_is_null() const;            
            Key& adv_sdate(const AutoTest::Time& value);            
            Key& adv_sdate_set_null(bool is_null = true);            
            const AutoTest::Time& adv_sdate() const;            
            bool adv_sdate_used() const;            
            bool adv_sdate_is_null() const;            
            Key ();            
            Key (            
              const int& campaign_id,              
              const int& colo_id,              
              const AutoTest::Time& last_appearance_date,              
              const AutoTest::Time& adv_sdate              
            );            
        };        
        stats_value_type unique_users () const;        
        stats_value_type control_sum () const;        
        void print_idname (std::ostream& out) const;        
                
        CampaignUserStats (const Key& value);        
        CampaignUserStats (        
          const int& campaign_id,          
          const int& colo_id,          
          const AutoTest::Time& last_appearance_date,          
          const AutoTest::Time& adv_sdate          
        );        
        CampaignUserStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& campaign_id,          
          const int& colo_id,          
          const AutoTest::Time& last_appearance_date,          
          const AutoTest::Time& adv_sdate          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<CampaignUserStats, 2>::Diffs    
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
                
        Diffs& unique_users (const stats_diff_type& value);        
        const stats_diff_type& unique_users () const;        
        Diffs& control_sum (const stats_diff_type& value);        
        const stats_diff_type& control_sum () const;        
      protected:      
        stats_diff_type diffs[2];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// CampaignUserStats    
    inline    
    CampaignUserStats::CampaignUserStats ()    
      :Base("CampaignUserStats")    
    {}    
    inline    
    CampaignUserStats::CampaignUserStats (const CampaignUserStats::Key& value)    
      :Base("CampaignUserStats")    
    {    
      key_ = value;      
    }    
    inline    
    CampaignUserStats::CampaignUserStats (    
      const int& campaign_id,      
      const int& colo_id,      
      const AutoTest::Time& last_appearance_date,      
      const AutoTest::Time& adv_sdate      
    )    
      :Base("CampaignUserStats")    
    {    
      key_ = Key (      
        campaign_id,        
        colo_id,        
        last_appearance_date,        
        adv_sdate        
      );      
    }    
    inline    
    CampaignUserStats::Key::Key ()    
      :campaign_id_(0),colo_id_(0),last_appearance_date_(default_date()),adv_sdate_(default_date())    
    {    
      campaign_id_used_ = false;      
      campaign_id_null_ = false;      
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      last_appearance_date_used_ = false;      
      last_appearance_date_null_ = false;      
      adv_sdate_used_ = false;      
      adv_sdate_null_ = false;      
    }    
    inline    
    CampaignUserStats::Key::Key (    
      const int& campaign_id,      
      const int& colo_id,      
      const AutoTest::Time& last_appearance_date,      
      const AutoTest::Time& adv_sdate      
    )    
      :campaign_id_(campaign_id),colo_id_(colo_id),last_appearance_date_(last_appearance_date),adv_sdate_(adv_sdate)    
    {    
      campaign_id_used_ = true;      
      campaign_id_null_ = false;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      last_appearance_date_used_ = true;      
      last_appearance_date_null_ = false;      
      adv_sdate_used_ = true;      
      adv_sdate_null_ = false;      
    }    
    inline    
    CampaignUserStats::Key&    
    CampaignUserStats::Key::campaign_id(const int& value)    
    {    
      campaign_id_ = value;      
      campaign_id_used_ = true;      
      campaign_id_null_ = false;      
      return *this;      
    }    
    inline    
    CampaignUserStats::Key&    
    CampaignUserStats::Key::campaign_id_set_null(bool is_null)    
    {    
      campaign_id_used_ = true;      
      campaign_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CampaignUserStats::Key::campaign_id() const    
    {    
      return campaign_id_;      
    }    
    inline    
    bool    
    CampaignUserStats::Key::campaign_id_used() const    
    {    
      return campaign_id_used_;      
    }    
    inline    
    bool    
    CampaignUserStats::Key::campaign_id_is_null() const    
    {    
      return campaign_id_null_;      
    }    
    inline    
    CampaignUserStats::Key&    
    CampaignUserStats::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    CampaignUserStats::Key&    
    CampaignUserStats::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CampaignUserStats::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    CampaignUserStats::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    CampaignUserStats::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    CampaignUserStats::Key&    
    CampaignUserStats::Key::last_appearance_date(const AutoTest::Time& value)    
    {    
      last_appearance_date_ = value;      
      last_appearance_date_used_ = true;      
      last_appearance_date_null_ = false;      
      return *this;      
    }    
    inline    
    CampaignUserStats::Key&    
    CampaignUserStats::Key::last_appearance_date_set_null(bool is_null)    
    {    
      last_appearance_date_used_ = true;      
      last_appearance_date_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    CampaignUserStats::Key::last_appearance_date() const    
    {    
      return last_appearance_date_;      
    }    
    inline    
    bool    
    CampaignUserStats::Key::last_appearance_date_used() const    
    {    
      return last_appearance_date_used_;      
    }    
    inline    
    bool    
    CampaignUserStats::Key::last_appearance_date_is_null() const    
    {    
      return last_appearance_date_null_;      
    }    
    inline    
    CampaignUserStats::Key&    
    CampaignUserStats::Key::adv_sdate(const AutoTest::Time& value)    
    {    
      adv_sdate_ = value;      
      adv_sdate_used_ = true;      
      adv_sdate_null_ = false;      
      return *this;      
    }    
    inline    
    CampaignUserStats::Key&    
    CampaignUserStats::Key::adv_sdate_set_null(bool is_null)    
    {    
      adv_sdate_used_ = true;      
      adv_sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    CampaignUserStats::Key::adv_sdate() const    
    {    
      return adv_sdate_;      
    }    
    inline    
    bool    
    CampaignUserStats::Key::adv_sdate_used() const    
    {    
      return adv_sdate_used_;      
    }    
    inline    
    bool    
    CampaignUserStats::Key::adv_sdate_is_null() const    
    {    
      return adv_sdate_null_;      
    }    
    inline    
    CampaignUserStats::Key&    
    CampaignUserStats::key (    
      const int& campaign_id,      
      const int& colo_id,      
      const AutoTest::Time& last_appearance_date,      
      const AutoTest::Time& adv_sdate      
    )    
    {    
      key_ = Key (      
        campaign_id,        
        colo_id,        
        last_appearance_date,        
        adv_sdate        
      );      
      return key_;      
    }    
    inline    
    CampaignUserStats::Key&    
    CampaignUserStats::key ()    
    {    
      return key_;      
    }    
    inline    
    CampaignUserStats::Key&    
    CampaignUserStats::key (const CampaignUserStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<CampaignUserStats, 2>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<CampaignUserStats, 2>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<CampaignUserStats, 2>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<CampaignUserStats, 2>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<CampaignUserStats, 2>::Diffs&     
    DiffStats<CampaignUserStats, 2>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CampaignUserStats, 2>::Diffs&     
    DiffStats<CampaignUserStats, 2>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CampaignUserStats, 2>::Diffs&     
    DiffStats<CampaignUserStats, 2>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 2; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CampaignUserStats, 2>::Diffs     
    DiffStats<CampaignUserStats, 2>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<CampaignUserStats, 2>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<CampaignUserStats, 2>::Diffs     
    DiffStats<CampaignUserStats, 2>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<CampaignUserStats, 2>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<CampaignUserStats, 2>::Diffs::const_iterator    
    DiffStats<CampaignUserStats, 2>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<CampaignUserStats, 2>::Diffs::const_iterator    
    DiffStats<CampaignUserStats, 2>::Diffs::end () const    
    {    
      return diffs + 2;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<CampaignUserStats, 2>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<CampaignUserStats, 2>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<CampaignUserStats, 2>::Diffs::size () const    
    {    
      return 2;      
    }    
    inline    
    void    
    DiffStats<CampaignUserStats, 2>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 2; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    CampaignUserStats::unique_users () const    
    {    
      return values[CampaignUserStats::UNIQUE_USERS];      
    }    
    inline    
    DiffStats<CampaignUserStats, 2>::Diffs&     
    DiffStats<CampaignUserStats, 2>::Diffs::unique_users (const stats_diff_type& value)    
    {    
      diffs[CampaignUserStats::UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CampaignUserStats, 2>::Diffs::unique_users () const    
    {    
      return diffs[CampaignUserStats::UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    CampaignUserStats::control_sum () const    
    {    
      return values[CampaignUserStats::CONTROL_SUM];      
    }    
    inline    
    DiffStats<CampaignUserStats, 2>::Diffs&     
    DiffStats<CampaignUserStats, 2>::Diffs::control_sum (const stats_diff_type& value)    
    {    
      diffs[CampaignUserStats::CONTROL_SUM] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CampaignUserStats, 2>::Diffs::control_sum () const    
    {    
      return diffs[CampaignUserStats::CONTROL_SUM];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_CAMPAIGNUSERSTATS_HPP

