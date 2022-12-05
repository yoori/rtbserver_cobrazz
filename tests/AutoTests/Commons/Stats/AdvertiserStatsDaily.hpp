
#ifndef __AUTOTESTS_COMMONS_STATS_ADVERTISERSTATSDAILY_HPP
#define __AUTOTESTS_COMMONS_STATS_ADVERTISERSTATSDAILY_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class AdvertiserStatsDaily:    
      public DiffStats<AdvertiserStatsDaily, 9>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          REQUESTS = 0,          
          IMPS,          
          CLICKS,          
          ACTIONS,          
          ADV_AMOUNT,          
          ADV_COMM_AMOUNT,          
          PUB_AMOUNT_ADV,          
          PUB_COMM_AMOUNT_ADV,          
          ADV_AMOUNT_CMP          
        };        
        typedef DiffStats<AdvertiserStatsDaily, 9> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            AutoTest::Time adv_sdate_;            
            bool adv_sdate_used_;            
            bool adv_sdate_null_;            
            int cc_id_;            
            bool cc_id_used_;            
            bool cc_id_null_;            
            int creative_id_;            
            bool creative_id_used_;            
            bool creative_id_null_;            
            int campaign_id_;            
            bool campaign_id_used_;            
            bool campaign_id_null_;            
            int ccg_id_;            
            bool ccg_id_used_;            
            bool ccg_id_null_;            
            int adv_account_id_;            
            bool adv_account_id_used_;            
            bool adv_account_id_null_;            
            int agency_account_id_;            
            bool agency_account_id_used_;            
            bool agency_account_id_null_;            
            std::string country_code_;            
            bool country_code_used_;            
            bool country_code_null_;            
            bool walled_garden_;            
            bool walled_garden_used_;            
            bool walled_garden_null_;            
          public:          
            Key& adv_sdate(const AutoTest::Time& value);            
            Key& adv_sdate_set_null(bool is_null = true);            
            const AutoTest::Time& adv_sdate() const;            
            bool adv_sdate_used() const;            
            bool adv_sdate_is_null() const;            
            Key& cc_id(const int& value);            
            Key& cc_id_set_null(bool is_null = true);            
            const int& cc_id() const;            
            bool cc_id_used() const;            
            bool cc_id_is_null() const;            
            Key& creative_id(const int& value);            
            Key& creative_id_set_null(bool is_null = true);            
            const int& creative_id() const;            
            bool creative_id_used() const;            
            bool creative_id_is_null() const;            
            Key& campaign_id(const int& value);            
            Key& campaign_id_set_null(bool is_null = true);            
            const int& campaign_id() const;            
            bool campaign_id_used() const;            
            bool campaign_id_is_null() const;            
            Key& ccg_id(const int& value);            
            Key& ccg_id_set_null(bool is_null = true);            
            const int& ccg_id() const;            
            bool ccg_id_used() const;            
            bool ccg_id_is_null() const;            
            Key& adv_account_id(const int& value);            
            Key& adv_account_id_set_null(bool is_null = true);            
            const int& adv_account_id() const;            
            bool adv_account_id_used() const;            
            bool adv_account_id_is_null() const;            
            Key& agency_account_id(const int& value);            
            Key& agency_account_id_set_null(bool is_null = true);            
            const int& agency_account_id() const;            
            bool agency_account_id_used() const;            
            bool agency_account_id_is_null() const;            
            Key& country_code(const std::string& value);            
            Key& country_code_set_null(bool is_null = true);            
            const std::string& country_code() const;            
            bool country_code_used() const;            
            bool country_code_is_null() const;            
            Key& walled_garden(const bool& value);            
            Key& walled_garden_set_null(bool is_null = true);            
            const bool& walled_garden() const;            
            bool walled_garden_used() const;            
            bool walled_garden_is_null() const;            
            Key ();            
            Key (            
              const AutoTest::Time& adv_sdate,              
              const int& cc_id,              
              const int& creative_id,              
              const int& campaign_id,              
              const int& ccg_id,              
              const int& adv_account_id,              
              const int& agency_account_id,              
              const std::string& country_code,              
              const bool& walled_garden              
            );            
        };        
        stats_value_type requests () const;        
        stats_value_type imps () const;        
        stats_value_type clicks () const;        
        stats_value_type actions () const;        
        stats_value_type adv_amount () const;        
        stats_value_type adv_comm_amount () const;        
        stats_value_type pub_amount_adv () const;        
        stats_value_type pub_comm_amount_adv () const;        
        stats_value_type adv_amount_cmp () const;        
        void print_idname (std::ostream& out) const;        
                
        AdvertiserStatsDaily (const Key& value);        
        AdvertiserStatsDaily (        
          const AutoTest::Time& adv_sdate,          
          const int& cc_id,          
          const int& creative_id,          
          const int& campaign_id,          
          const int& ccg_id,          
          const int& adv_account_id,          
          const int& agency_account_id,          
          const std::string& country_code,          
          const bool& walled_garden          
        );        
        AdvertiserStatsDaily ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const AutoTest::Time& adv_sdate,          
          const int& cc_id,          
          const int& creative_id,          
          const int& campaign_id,          
          const int& ccg_id,          
          const int& adv_account_id,          
          const int& agency_account_id,          
          const std::string& country_code,          
          const bool& walled_garden          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<AdvertiserStatsDaily, 9>::Diffs    
    {    
      public:      
        typedef stats_diff_type array_type[9];        
        typedef const stats_diff_type const_array_type[9];        
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
                
        Diffs& requests (const stats_diff_type& value);        
        const stats_diff_type& requests () const;        
        Diffs& imps (const stats_diff_type& value);        
        const stats_diff_type& imps () const;        
        Diffs& clicks (const stats_diff_type& value);        
        const stats_diff_type& clicks () const;        
        Diffs& actions (const stats_diff_type& value);        
        const stats_diff_type& actions () const;        
        Diffs& adv_amount (const stats_diff_type& value);        
        const stats_diff_type& adv_amount () const;        
        Diffs& adv_comm_amount (const stats_diff_type& value);        
        const stats_diff_type& adv_comm_amount () const;        
        Diffs& pub_amount_adv (const stats_diff_type& value);        
        const stats_diff_type& pub_amount_adv () const;        
        Diffs& pub_comm_amount_adv (const stats_diff_type& value);        
        const stats_diff_type& pub_comm_amount_adv () const;        
        Diffs& adv_amount_cmp (const stats_diff_type& value);        
        const stats_diff_type& adv_amount_cmp () const;        
      protected:      
        stats_diff_type diffs[9];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// AdvertiserStatsDaily    
    inline    
    AdvertiserStatsDaily::AdvertiserStatsDaily ()    
      :Base("AdvertiserStatsDaily")    
    {}    
    inline    
    AdvertiserStatsDaily::AdvertiserStatsDaily (const AdvertiserStatsDaily::Key& value)    
      :Base("AdvertiserStatsDaily")    
    {    
      key_ = value;      
    }    
    inline    
    AdvertiserStatsDaily::AdvertiserStatsDaily (    
      const AutoTest::Time& adv_sdate,      
      const int& cc_id,      
      const int& creative_id,      
      const int& campaign_id,      
      const int& ccg_id,      
      const int& adv_account_id,      
      const int& agency_account_id,      
      const std::string& country_code,      
      const bool& walled_garden      
    )    
      :Base("AdvertiserStatsDaily")    
    {    
      key_ = Key (      
        adv_sdate,        
        cc_id,        
        creative_id,        
        campaign_id,        
        ccg_id,        
        adv_account_id,        
        agency_account_id,        
        country_code,        
        walled_garden        
      );      
    }    
    inline    
    AdvertiserStatsDaily::Key::Key ()    
      :adv_sdate_(default_date()),cc_id_(0),creative_id_(0),campaign_id_(0),ccg_id_(0),adv_account_id_(0),agency_account_id_(0),walled_garden_(false)    
    {    
      adv_sdate_used_ = false;      
      adv_sdate_null_ = false;      
      cc_id_used_ = false;      
      cc_id_null_ = false;      
      creative_id_used_ = false;      
      creative_id_null_ = false;      
      campaign_id_used_ = false;      
      campaign_id_null_ = false;      
      ccg_id_used_ = false;      
      ccg_id_null_ = false;      
      adv_account_id_used_ = false;      
      adv_account_id_null_ = false;      
      agency_account_id_used_ = false;      
      agency_account_id_null_ = false;      
      country_code_used_ = false;      
      country_code_null_ = false;      
      walled_garden_used_ = false;      
      walled_garden_null_ = false;      
    }    
    inline    
    AdvertiserStatsDaily::Key::Key (    
      const AutoTest::Time& adv_sdate,      
      const int& cc_id,      
      const int& creative_id,      
      const int& campaign_id,      
      const int& ccg_id,      
      const int& adv_account_id,      
      const int& agency_account_id,      
      const std::string& country_code,      
      const bool& walled_garden      
    )    
      :adv_sdate_(adv_sdate),cc_id_(cc_id),creative_id_(creative_id),campaign_id_(campaign_id),ccg_id_(ccg_id),adv_account_id_(adv_account_id),agency_account_id_(agency_account_id),walled_garden_(walled_garden)    
    {    
      adv_sdate_used_ = true;      
      adv_sdate_null_ = false;      
      cc_id_used_ = true;      
      cc_id_null_ = false;      
      creative_id_used_ = true;      
      creative_id_null_ = false;      
      campaign_id_used_ = true;      
      campaign_id_null_ = false;      
      ccg_id_used_ = true;      
      ccg_id_null_ = false;      
      adv_account_id_used_ = true;      
      adv_account_id_null_ = false;      
      agency_account_id_used_ = true;      
      agency_account_id_null_ = false;      
      country_code_ = country_code;      
      country_code_used_ = true;      
      country_code_null_ = false;      
      walled_garden_used_ = true;      
      walled_garden_null_ = false;      
    }    
    inline    
    AdvertiserStatsDaily::Key&    
    AdvertiserStatsDaily::Key::adv_sdate(const AutoTest::Time& value)    
    {    
      adv_sdate_ = value;      
      adv_sdate_used_ = true;      
      adv_sdate_null_ = false;      
      return *this;      
    }    
    inline    
    AdvertiserStatsDaily::Key&    
    AdvertiserStatsDaily::Key::adv_sdate_set_null(bool is_null)    
    {    
      adv_sdate_used_ = true;      
      adv_sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    AdvertiserStatsDaily::Key::adv_sdate() const    
    {    
      return adv_sdate_;      
    }    
    inline    
    bool    
    AdvertiserStatsDaily::Key::adv_sdate_used() const    
    {    
      return adv_sdate_used_;      
    }    
    inline    
    bool    
    AdvertiserStatsDaily::Key::adv_sdate_is_null() const    
    {    
      return adv_sdate_null_;      
    }    
    inline    
    AdvertiserStatsDaily::Key&    
    AdvertiserStatsDaily::Key::cc_id(const int& value)    
    {    
      cc_id_ = value;      
      cc_id_used_ = true;      
      cc_id_null_ = false;      
      return *this;      
    }    
    inline    
    AdvertiserStatsDaily::Key&    
    AdvertiserStatsDaily::Key::cc_id_set_null(bool is_null)    
    {    
      cc_id_used_ = true;      
      cc_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    AdvertiserStatsDaily::Key::cc_id() const    
    {    
      return cc_id_;      
    }    
    inline    
    bool    
    AdvertiserStatsDaily::Key::cc_id_used() const    
    {    
      return cc_id_used_;      
    }    
    inline    
    bool    
    AdvertiserStatsDaily::Key::cc_id_is_null() const    
    {    
      return cc_id_null_;      
    }    
    inline    
    AdvertiserStatsDaily::Key&    
    AdvertiserStatsDaily::Key::creative_id(const int& value)    
    {    
      creative_id_ = value;      
      creative_id_used_ = true;      
      creative_id_null_ = false;      
      return *this;      
    }    
    inline    
    AdvertiserStatsDaily::Key&    
    AdvertiserStatsDaily::Key::creative_id_set_null(bool is_null)    
    {    
      creative_id_used_ = true;      
      creative_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    AdvertiserStatsDaily::Key::creative_id() const    
    {    
      return creative_id_;      
    }    
    inline    
    bool    
    AdvertiserStatsDaily::Key::creative_id_used() const    
    {    
      return creative_id_used_;      
    }    
    inline    
    bool    
    AdvertiserStatsDaily::Key::creative_id_is_null() const    
    {    
      return creative_id_null_;      
    }    
    inline    
    AdvertiserStatsDaily::Key&    
    AdvertiserStatsDaily::Key::campaign_id(const int& value)    
    {    
      campaign_id_ = value;      
      campaign_id_used_ = true;      
      campaign_id_null_ = false;      
      return *this;      
    }    
    inline    
    AdvertiserStatsDaily::Key&    
    AdvertiserStatsDaily::Key::campaign_id_set_null(bool is_null)    
    {    
      campaign_id_used_ = true;      
      campaign_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    AdvertiserStatsDaily::Key::campaign_id() const    
    {    
      return campaign_id_;      
    }    
    inline    
    bool    
    AdvertiserStatsDaily::Key::campaign_id_used() const    
    {    
      return campaign_id_used_;      
    }    
    inline    
    bool    
    AdvertiserStatsDaily::Key::campaign_id_is_null() const    
    {    
      return campaign_id_null_;      
    }    
    inline    
    AdvertiserStatsDaily::Key&    
    AdvertiserStatsDaily::Key::ccg_id(const int& value)    
    {    
      ccg_id_ = value;      
      ccg_id_used_ = true;      
      ccg_id_null_ = false;      
      return *this;      
    }    
    inline    
    AdvertiserStatsDaily::Key&    
    AdvertiserStatsDaily::Key::ccg_id_set_null(bool is_null)    
    {    
      ccg_id_used_ = true;      
      ccg_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    AdvertiserStatsDaily::Key::ccg_id() const    
    {    
      return ccg_id_;      
    }    
    inline    
    bool    
    AdvertiserStatsDaily::Key::ccg_id_used() const    
    {    
      return ccg_id_used_;      
    }    
    inline    
    bool    
    AdvertiserStatsDaily::Key::ccg_id_is_null() const    
    {    
      return ccg_id_null_;      
    }    
    inline    
    AdvertiserStatsDaily::Key&    
    AdvertiserStatsDaily::Key::adv_account_id(const int& value)    
    {    
      adv_account_id_ = value;      
      adv_account_id_used_ = true;      
      adv_account_id_null_ = false;      
      return *this;      
    }    
    inline    
    AdvertiserStatsDaily::Key&    
    AdvertiserStatsDaily::Key::adv_account_id_set_null(bool is_null)    
    {    
      adv_account_id_used_ = true;      
      adv_account_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    AdvertiserStatsDaily::Key::adv_account_id() const    
    {    
      return adv_account_id_;      
    }    
    inline    
    bool    
    AdvertiserStatsDaily::Key::adv_account_id_used() const    
    {    
      return adv_account_id_used_;      
    }    
    inline    
    bool    
    AdvertiserStatsDaily::Key::adv_account_id_is_null() const    
    {    
      return adv_account_id_null_;      
    }    
    inline    
    AdvertiserStatsDaily::Key&    
    AdvertiserStatsDaily::Key::agency_account_id(const int& value)    
    {    
      agency_account_id_ = value;      
      agency_account_id_used_ = true;      
      agency_account_id_null_ = false;      
      return *this;      
    }    
    inline    
    AdvertiserStatsDaily::Key&    
    AdvertiserStatsDaily::Key::agency_account_id_set_null(bool is_null)    
    {    
      agency_account_id_used_ = true;      
      agency_account_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    AdvertiserStatsDaily::Key::agency_account_id() const    
    {    
      return agency_account_id_;      
    }    
    inline    
    bool    
    AdvertiserStatsDaily::Key::agency_account_id_used() const    
    {    
      return agency_account_id_used_;      
    }    
    inline    
    bool    
    AdvertiserStatsDaily::Key::agency_account_id_is_null() const    
    {    
      return agency_account_id_null_;      
    }    
    inline    
    AdvertiserStatsDaily::Key&    
    AdvertiserStatsDaily::Key::country_code(const std::string& value)    
    {    
      country_code_ = value;      
      country_code_used_ = true;      
      country_code_null_ = false;      
      return *this;      
    }    
    inline    
    AdvertiserStatsDaily::Key&    
    AdvertiserStatsDaily::Key::country_code_set_null(bool is_null)    
    {    
      country_code_used_ = true;      
      country_code_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    AdvertiserStatsDaily::Key::country_code() const    
    {    
      return country_code_;      
    }    
    inline    
    bool    
    AdvertiserStatsDaily::Key::country_code_used() const    
    {    
      return country_code_used_;      
    }    
    inline    
    bool    
    AdvertiserStatsDaily::Key::country_code_is_null() const    
    {    
      return country_code_null_;      
    }    
    inline    
    AdvertiserStatsDaily::Key&    
    AdvertiserStatsDaily::Key::walled_garden(const bool& value)    
    {    
      walled_garden_ = value;      
      walled_garden_used_ = true;      
      walled_garden_null_ = false;      
      return *this;      
    }    
    inline    
    AdvertiserStatsDaily::Key&    
    AdvertiserStatsDaily::Key::walled_garden_set_null(bool is_null)    
    {    
      walled_garden_used_ = true;      
      walled_garden_null_ = is_null;      
      return *this;      
    }    
    inline    
    const bool&    
    AdvertiserStatsDaily::Key::walled_garden() const    
    {    
      return walled_garden_;      
    }    
    inline    
    bool    
    AdvertiserStatsDaily::Key::walled_garden_used() const    
    {    
      return walled_garden_used_;      
    }    
    inline    
    bool    
    AdvertiserStatsDaily::Key::walled_garden_is_null() const    
    {    
      return walled_garden_null_;      
    }    
    inline    
    AdvertiserStatsDaily::Key&    
    AdvertiserStatsDaily::key (    
      const AutoTest::Time& adv_sdate,      
      const int& cc_id,      
      const int& creative_id,      
      const int& campaign_id,      
      const int& ccg_id,      
      const int& adv_account_id,      
      const int& agency_account_id,      
      const std::string& country_code,      
      const bool& walled_garden      
    )    
    {    
      key_ = Key (      
        adv_sdate,        
        cc_id,        
        creative_id,        
        campaign_id,        
        ccg_id,        
        adv_account_id,        
        agency_account_id,        
        country_code,        
        walled_garden        
      );      
      return key_;      
    }    
    inline    
    AdvertiserStatsDaily::Key&    
    AdvertiserStatsDaily::key ()    
    {    
      return key_;      
    }    
    inline    
    AdvertiserStatsDaily::Key&    
    AdvertiserStatsDaily::key (const AdvertiserStatsDaily::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 9; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 9; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 9; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs&     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 9; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs&     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 9; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs&     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 9; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<AdvertiserStatsDaily, 9>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<AdvertiserStatsDaily, 9>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::const_iterator    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::const_iterator    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::end () const    
    {    
      return diffs + 9;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::size () const    
    {    
      return 9;      
    }    
    inline    
    void    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 9; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    AdvertiserStatsDaily::requests () const    
    {    
      return values[AdvertiserStatsDaily::REQUESTS];      
    }    
    inline    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs&     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::requests (const stats_diff_type& value)    
    {    
      diffs[AdvertiserStatsDaily::REQUESTS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::requests () const    
    {    
      return diffs[AdvertiserStatsDaily::REQUESTS];      
    }    
    inline    
    stats_value_type    
    AdvertiserStatsDaily::imps () const    
    {    
      return values[AdvertiserStatsDaily::IMPS];      
    }    
    inline    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs&     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::imps (const stats_diff_type& value)    
    {    
      diffs[AdvertiserStatsDaily::IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::imps () const    
    {    
      return diffs[AdvertiserStatsDaily::IMPS];      
    }    
    inline    
    stats_value_type    
    AdvertiserStatsDaily::clicks () const    
    {    
      return values[AdvertiserStatsDaily::CLICKS];      
    }    
    inline    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs&     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::clicks (const stats_diff_type& value)    
    {    
      diffs[AdvertiserStatsDaily::CLICKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::clicks () const    
    {    
      return diffs[AdvertiserStatsDaily::CLICKS];      
    }    
    inline    
    stats_value_type    
    AdvertiserStatsDaily::actions () const    
    {    
      return values[AdvertiserStatsDaily::ACTIONS];      
    }    
    inline    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs&     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::actions (const stats_diff_type& value)    
    {    
      diffs[AdvertiserStatsDaily::ACTIONS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::actions () const    
    {    
      return diffs[AdvertiserStatsDaily::ACTIONS];      
    }    
    inline    
    stats_value_type    
    AdvertiserStatsDaily::adv_amount () const    
    {    
      return values[AdvertiserStatsDaily::ADV_AMOUNT];      
    }    
    inline    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs&     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::adv_amount (const stats_diff_type& value)    
    {    
      diffs[AdvertiserStatsDaily::ADV_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::adv_amount () const    
    {    
      return diffs[AdvertiserStatsDaily::ADV_AMOUNT];      
    }    
    inline    
    stats_value_type    
    AdvertiserStatsDaily::adv_comm_amount () const    
    {    
      return values[AdvertiserStatsDaily::ADV_COMM_AMOUNT];      
    }    
    inline    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs&     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::adv_comm_amount (const stats_diff_type& value)    
    {    
      diffs[AdvertiserStatsDaily::ADV_COMM_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::adv_comm_amount () const    
    {    
      return diffs[AdvertiserStatsDaily::ADV_COMM_AMOUNT];      
    }    
    inline    
    stats_value_type    
    AdvertiserStatsDaily::pub_amount_adv () const    
    {    
      return values[AdvertiserStatsDaily::PUB_AMOUNT_ADV];      
    }    
    inline    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs&     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::pub_amount_adv (const stats_diff_type& value)    
    {    
      diffs[AdvertiserStatsDaily::PUB_AMOUNT_ADV] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::pub_amount_adv () const    
    {    
      return diffs[AdvertiserStatsDaily::PUB_AMOUNT_ADV];      
    }    
    inline    
    stats_value_type    
    AdvertiserStatsDaily::pub_comm_amount_adv () const    
    {    
      return values[AdvertiserStatsDaily::PUB_COMM_AMOUNT_ADV];      
    }    
    inline    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs&     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::pub_comm_amount_adv (const stats_diff_type& value)    
    {    
      diffs[AdvertiserStatsDaily::PUB_COMM_AMOUNT_ADV] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::pub_comm_amount_adv () const    
    {    
      return diffs[AdvertiserStatsDaily::PUB_COMM_AMOUNT_ADV];      
    }    
    inline    
    stats_value_type    
    AdvertiserStatsDaily::adv_amount_cmp () const    
    {    
      return values[AdvertiserStatsDaily::ADV_AMOUNT_CMP];      
    }    
    inline    
    DiffStats<AdvertiserStatsDaily, 9>::Diffs&     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::adv_amount_cmp (const stats_diff_type& value)    
    {    
      diffs[AdvertiserStatsDaily::ADV_AMOUNT_CMP] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<AdvertiserStatsDaily, 9>::Diffs::adv_amount_cmp () const    
    {    
      return diffs[AdvertiserStatsDaily::ADV_AMOUNT_CMP];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_ADVERTISERSTATSDAILY_HPP

