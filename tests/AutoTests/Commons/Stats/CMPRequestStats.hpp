
#ifndef __AUTOTESTS_COMMONS_STATS_CMPREQUESTSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_CMPREQUESTSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class CMPRequestStats:    
      public DiffStats<CMPRequestStats, 5>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          IMPS = 0,          
          ADV_AMOUNT_CMP,          
          CMP_AMOUNT,          
          CMP_AMOUNT_GLOBAL,          
          CLICKS          
        };        
        typedef DiffStats<CMPRequestStats, 5> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            AutoTest::Time sdate_;            
            bool sdate_used_;            
            bool sdate_null_;            
            AutoTest::Time adv_sdate_;            
            bool adv_sdate_used_;            
            bool adv_sdate_null_;            
            AutoTest::Time cmp_sdate_;            
            bool cmp_sdate_used_;            
            bool cmp_sdate_null_;            
            int adv_account_id_;            
            bool adv_account_id_used_;            
            bool adv_account_id_null_;            
            int cmp_account_id_;            
            bool cmp_account_id_used_;            
            bool cmp_account_id_null_;            
            int ccg_id_;            
            bool ccg_id_used_;            
            bool ccg_id_null_;            
            int colo_id_;            
            bool colo_id_used_;            
            bool colo_id_null_;            
            int channel_id_;            
            bool channel_id_used_;            
            bool channel_id_null_;            
            int channel_rate_id_;            
            bool channel_rate_id_used_;            
            bool channel_rate_id_null_;            
            int currency_exchange_id_;            
            bool currency_exchange_id_used_;            
            bool currency_exchange_id_null_;            
            bool fraud_correction_;            
            bool fraud_correction_used_;            
            bool fraud_correction_null_;            
            int cc_id_;            
            bool cc_id_used_;            
            bool cc_id_null_;            
            std::string country_code_;            
            bool country_code_used_;            
            bool country_code_null_;            
            bool walled_garden_;            
            bool walled_garden_used_;            
            bool walled_garden_null_;            
          public:          
            Key& sdate(const AutoTest::Time& value);            
            Key& sdate_set_null(bool is_null = true);            
            const AutoTest::Time& sdate() const;            
            bool sdate_used() const;            
            bool sdate_is_null() const;            
            Key& adv_sdate(const AutoTest::Time& value);            
            Key& adv_sdate_set_null(bool is_null = true);            
            const AutoTest::Time& adv_sdate() const;            
            bool adv_sdate_used() const;            
            bool adv_sdate_is_null() const;            
            Key& cmp_sdate(const AutoTest::Time& value);            
            Key& cmp_sdate_set_null(bool is_null = true);            
            const AutoTest::Time& cmp_sdate() const;            
            bool cmp_sdate_used() const;            
            bool cmp_sdate_is_null() const;            
            Key& adv_account_id(const int& value);            
            Key& adv_account_id_set_null(bool is_null = true);            
            const int& adv_account_id() const;            
            bool adv_account_id_used() const;            
            bool adv_account_id_is_null() const;            
            Key& cmp_account_id(const int& value);            
            Key& cmp_account_id_set_null(bool is_null = true);            
            const int& cmp_account_id() const;            
            bool cmp_account_id_used() const;            
            bool cmp_account_id_is_null() const;            
            Key& ccg_id(const int& value);            
            Key& ccg_id_set_null(bool is_null = true);            
            const int& ccg_id() const;            
            bool ccg_id_used() const;            
            bool ccg_id_is_null() const;            
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
            Key& channel_rate_id(const int& value);            
            Key& channel_rate_id_set_null(bool is_null = true);            
            const int& channel_rate_id() const;            
            bool channel_rate_id_used() const;            
            bool channel_rate_id_is_null() const;            
            Key& currency_exchange_id(const int& value);            
            Key& currency_exchange_id_set_null(bool is_null = true);            
            const int& currency_exchange_id() const;            
            bool currency_exchange_id_used() const;            
            bool currency_exchange_id_is_null() const;            
            Key& fraud_correction(const bool& value);            
            Key& fraud_correction_set_null(bool is_null = true);            
            const bool& fraud_correction() const;            
            bool fraud_correction_used() const;            
            bool fraud_correction_is_null() const;            
            Key& cc_id(const int& value);            
            Key& cc_id_set_null(bool is_null = true);            
            const int& cc_id() const;            
            bool cc_id_used() const;            
            bool cc_id_is_null() const;            
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
              const AutoTest::Time& sdate,              
              const AutoTest::Time& adv_sdate,              
              const AutoTest::Time& cmp_sdate,              
              const int& adv_account_id,              
              const int& cmp_account_id,              
              const int& ccg_id,              
              const int& colo_id,              
              const int& channel_id,              
              const int& channel_rate_id,              
              const int& currency_exchange_id,              
              const bool& fraud_correction,              
              const int& cc_id,              
              const std::string& country_code,              
              const bool& walled_garden              
            );            
        };        
        stats_value_type imps () const;        
        stats_value_type adv_amount_cmp () const;        
        stats_value_type cmp_amount () const;        
        stats_value_type cmp_amount_global () const;        
        stats_value_type clicks () const;        
        void print_idname (std::ostream& out) const;        
                
        CMPRequestStats (const Key& value);        
        CMPRequestStats (        
          const AutoTest::Time& sdate,          
          const AutoTest::Time& adv_sdate,          
          const AutoTest::Time& cmp_sdate,          
          const int& adv_account_id,          
          const int& cmp_account_id,          
          const int& ccg_id,          
          const int& colo_id,          
          const int& channel_id,          
          const int& channel_rate_id,          
          const int& currency_exchange_id,          
          const bool& fraud_correction,          
          const int& cc_id,          
          const std::string& country_code,          
          const bool& walled_garden          
        );        
        CMPRequestStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const AutoTest::Time& sdate,          
          const AutoTest::Time& adv_sdate,          
          const AutoTest::Time& cmp_sdate,          
          const int& adv_account_id,          
          const int& cmp_account_id,          
          const int& ccg_id,          
          const int& colo_id,          
          const int& channel_id,          
          const int& channel_rate_id,          
          const int& currency_exchange_id,          
          const bool& fraud_correction,          
          const int& cc_id,          
          const std::string& country_code,          
          const bool& walled_garden          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<CMPRequestStats, 5>::Diffs    
    {    
      public:      
        typedef stats_diff_type array_type[5];        
        typedef const stats_diff_type const_array_type[5];        
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
        Diffs& adv_amount_cmp (const stats_diff_type& value);        
        const stats_diff_type& adv_amount_cmp () const;        
        Diffs& cmp_amount (const stats_diff_type& value);        
        const stats_diff_type& cmp_amount () const;        
        Diffs& cmp_amount_global (const stats_diff_type& value);        
        const stats_diff_type& cmp_amount_global () const;        
        Diffs& clicks (const stats_diff_type& value);        
        const stats_diff_type& clicks () const;        
      protected:      
        stats_diff_type diffs[5];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// CMPRequestStats    
    inline    
    CMPRequestStats::CMPRequestStats ()    
      :Base("CMPRequestStats")    
    {}    
    inline    
    CMPRequestStats::CMPRequestStats (const CMPRequestStats::Key& value)    
      :Base("CMPRequestStats")    
    {    
      key_ = value;      
    }    
    inline    
    CMPRequestStats::CMPRequestStats (    
      const AutoTest::Time& sdate,      
      const AutoTest::Time& adv_sdate,      
      const AutoTest::Time& cmp_sdate,      
      const int& adv_account_id,      
      const int& cmp_account_id,      
      const int& ccg_id,      
      const int& colo_id,      
      const int& channel_id,      
      const int& channel_rate_id,      
      const int& currency_exchange_id,      
      const bool& fraud_correction,      
      const int& cc_id,      
      const std::string& country_code,      
      const bool& walled_garden      
    )    
      :Base("CMPRequestStats")    
    {    
      key_ = Key (      
        sdate,        
        adv_sdate,        
        cmp_sdate,        
        adv_account_id,        
        cmp_account_id,        
        ccg_id,        
        colo_id,        
        channel_id,        
        channel_rate_id,        
        currency_exchange_id,        
        fraud_correction,        
        cc_id,        
        country_code,        
        walled_garden        
      );      
    }    
    inline    
    CMPRequestStats::Key::Key ()    
      :sdate_(default_date()),adv_sdate_(default_date()),cmp_sdate_(default_date()),adv_account_id_(0),cmp_account_id_(0),ccg_id_(0),colo_id_(0),channel_id_(0),channel_rate_id_(0),currency_exchange_id_(0),fraud_correction_(false),cc_id_(0),walled_garden_(false)    
    {    
      sdate_used_ = false;      
      sdate_null_ = false;      
      adv_sdate_used_ = false;      
      adv_sdate_null_ = false;      
      cmp_sdate_used_ = false;      
      cmp_sdate_null_ = false;      
      adv_account_id_used_ = false;      
      adv_account_id_null_ = false;      
      cmp_account_id_used_ = false;      
      cmp_account_id_null_ = false;      
      ccg_id_used_ = false;      
      ccg_id_null_ = false;      
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      channel_id_used_ = false;      
      channel_id_null_ = false;      
      channel_rate_id_used_ = false;      
      channel_rate_id_null_ = false;      
      currency_exchange_id_used_ = false;      
      currency_exchange_id_null_ = false;      
      fraud_correction_used_ = false;      
      fraud_correction_null_ = false;      
      cc_id_used_ = false;      
      cc_id_null_ = false;      
      country_code_used_ = false;      
      country_code_null_ = false;      
      walled_garden_used_ = false;      
      walled_garden_null_ = false;      
    }    
    inline    
    CMPRequestStats::Key::Key (    
      const AutoTest::Time& sdate,      
      const AutoTest::Time& adv_sdate,      
      const AutoTest::Time& cmp_sdate,      
      const int& adv_account_id,      
      const int& cmp_account_id,      
      const int& ccg_id,      
      const int& colo_id,      
      const int& channel_id,      
      const int& channel_rate_id,      
      const int& currency_exchange_id,      
      const bool& fraud_correction,      
      const int& cc_id,      
      const std::string& country_code,      
      const bool& walled_garden      
    )    
      :sdate_(sdate),adv_sdate_(adv_sdate),cmp_sdate_(cmp_sdate),adv_account_id_(adv_account_id),cmp_account_id_(cmp_account_id),ccg_id_(ccg_id),colo_id_(colo_id),channel_id_(channel_id),channel_rate_id_(channel_rate_id),currency_exchange_id_(currency_exchange_id),fraud_correction_(fraud_correction),cc_id_(cc_id),walled_garden_(walled_garden)    
    {    
      sdate_used_ = true;      
      sdate_null_ = false;      
      adv_sdate_used_ = true;      
      adv_sdate_null_ = false;      
      cmp_sdate_used_ = true;      
      cmp_sdate_null_ = false;      
      adv_account_id_used_ = true;      
      adv_account_id_null_ = false;      
      cmp_account_id_used_ = true;      
      cmp_account_id_null_ = false;      
      ccg_id_used_ = true;      
      ccg_id_null_ = false;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      channel_id_used_ = true;      
      channel_id_null_ = false;      
      channel_rate_id_used_ = true;      
      channel_rate_id_null_ = false;      
      currency_exchange_id_used_ = true;      
      currency_exchange_id_null_ = false;      
      fraud_correction_used_ = true;      
      fraud_correction_null_ = false;      
      cc_id_used_ = true;      
      cc_id_null_ = false;      
      country_code_ = country_code;      
      country_code_used_ = true;      
      country_code_null_ = false;      
      walled_garden_used_ = true;      
      walled_garden_null_ = false;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::sdate(const AutoTest::Time& value)    
    {    
      sdate_ = value;      
      sdate_used_ = true;      
      sdate_null_ = false;      
      return *this;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::sdate_set_null(bool is_null)    
    {    
      sdate_used_ = true;      
      sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    CMPRequestStats::Key::sdate() const    
    {    
      return sdate_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::sdate_used() const    
    {    
      return sdate_used_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::sdate_is_null() const    
    {    
      return sdate_null_;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::adv_sdate(const AutoTest::Time& value)    
    {    
      adv_sdate_ = value;      
      adv_sdate_used_ = true;      
      adv_sdate_null_ = false;      
      return *this;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::adv_sdate_set_null(bool is_null)    
    {    
      adv_sdate_used_ = true;      
      adv_sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    CMPRequestStats::Key::adv_sdate() const    
    {    
      return adv_sdate_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::adv_sdate_used() const    
    {    
      return adv_sdate_used_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::adv_sdate_is_null() const    
    {    
      return adv_sdate_null_;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::cmp_sdate(const AutoTest::Time& value)    
    {    
      cmp_sdate_ = value;      
      cmp_sdate_used_ = true;      
      cmp_sdate_null_ = false;      
      return *this;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::cmp_sdate_set_null(bool is_null)    
    {    
      cmp_sdate_used_ = true;      
      cmp_sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    CMPRequestStats::Key::cmp_sdate() const    
    {    
      return cmp_sdate_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::cmp_sdate_used() const    
    {    
      return cmp_sdate_used_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::cmp_sdate_is_null() const    
    {    
      return cmp_sdate_null_;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::adv_account_id(const int& value)    
    {    
      adv_account_id_ = value;      
      adv_account_id_used_ = true;      
      adv_account_id_null_ = false;      
      return *this;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::adv_account_id_set_null(bool is_null)    
    {    
      adv_account_id_used_ = true;      
      adv_account_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CMPRequestStats::Key::adv_account_id() const    
    {    
      return adv_account_id_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::adv_account_id_used() const    
    {    
      return adv_account_id_used_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::adv_account_id_is_null() const    
    {    
      return adv_account_id_null_;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::cmp_account_id(const int& value)    
    {    
      cmp_account_id_ = value;      
      cmp_account_id_used_ = true;      
      cmp_account_id_null_ = false;      
      return *this;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::cmp_account_id_set_null(bool is_null)    
    {    
      cmp_account_id_used_ = true;      
      cmp_account_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CMPRequestStats::Key::cmp_account_id() const    
    {    
      return cmp_account_id_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::cmp_account_id_used() const    
    {    
      return cmp_account_id_used_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::cmp_account_id_is_null() const    
    {    
      return cmp_account_id_null_;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::ccg_id(const int& value)    
    {    
      ccg_id_ = value;      
      ccg_id_used_ = true;      
      ccg_id_null_ = false;      
      return *this;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::ccg_id_set_null(bool is_null)    
    {    
      ccg_id_used_ = true;      
      ccg_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CMPRequestStats::Key::ccg_id() const    
    {    
      return ccg_id_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::ccg_id_used() const    
    {    
      return ccg_id_used_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::ccg_id_is_null() const    
    {    
      return ccg_id_null_;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CMPRequestStats::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::channel_id(const int& value)    
    {    
      channel_id_ = value;      
      channel_id_used_ = true;      
      channel_id_null_ = false;      
      return *this;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::channel_id_set_null(bool is_null)    
    {    
      channel_id_used_ = true;      
      channel_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CMPRequestStats::Key::channel_id() const    
    {    
      return channel_id_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::channel_id_used() const    
    {    
      return channel_id_used_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::channel_id_is_null() const    
    {    
      return channel_id_null_;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::channel_rate_id(const int& value)    
    {    
      channel_rate_id_ = value;      
      channel_rate_id_used_ = true;      
      channel_rate_id_null_ = false;      
      return *this;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::channel_rate_id_set_null(bool is_null)    
    {    
      channel_rate_id_used_ = true;      
      channel_rate_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CMPRequestStats::Key::channel_rate_id() const    
    {    
      return channel_rate_id_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::channel_rate_id_used() const    
    {    
      return channel_rate_id_used_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::channel_rate_id_is_null() const    
    {    
      return channel_rate_id_null_;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::currency_exchange_id(const int& value)    
    {    
      currency_exchange_id_ = value;      
      currency_exchange_id_used_ = true;      
      currency_exchange_id_null_ = false;      
      return *this;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::currency_exchange_id_set_null(bool is_null)    
    {    
      currency_exchange_id_used_ = true;      
      currency_exchange_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CMPRequestStats::Key::currency_exchange_id() const    
    {    
      return currency_exchange_id_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::currency_exchange_id_used() const    
    {    
      return currency_exchange_id_used_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::currency_exchange_id_is_null() const    
    {    
      return currency_exchange_id_null_;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::fraud_correction(const bool& value)    
    {    
      fraud_correction_ = value;      
      fraud_correction_used_ = true;      
      fraud_correction_null_ = false;      
      return *this;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::fraud_correction_set_null(bool is_null)    
    {    
      fraud_correction_used_ = true;      
      fraud_correction_null_ = is_null;      
      return *this;      
    }    
    inline    
    const bool&    
    CMPRequestStats::Key::fraud_correction() const    
    {    
      return fraud_correction_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::fraud_correction_used() const    
    {    
      return fraud_correction_used_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::fraud_correction_is_null() const    
    {    
      return fraud_correction_null_;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::cc_id(const int& value)    
    {    
      cc_id_ = value;      
      cc_id_used_ = true;      
      cc_id_null_ = false;      
      return *this;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::cc_id_set_null(bool is_null)    
    {    
      cc_id_used_ = true;      
      cc_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CMPRequestStats::Key::cc_id() const    
    {    
      return cc_id_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::cc_id_used() const    
    {    
      return cc_id_used_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::cc_id_is_null() const    
    {    
      return cc_id_null_;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::country_code(const std::string& value)    
    {    
      country_code_ = value;      
      country_code_used_ = true;      
      country_code_null_ = false;      
      return *this;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::country_code_set_null(bool is_null)    
    {    
      country_code_used_ = true;      
      country_code_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    CMPRequestStats::Key::country_code() const    
    {    
      return country_code_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::country_code_used() const    
    {    
      return country_code_used_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::country_code_is_null() const    
    {    
      return country_code_null_;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::walled_garden(const bool& value)    
    {    
      walled_garden_ = value;      
      walled_garden_used_ = true;      
      walled_garden_null_ = false;      
      return *this;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::Key::walled_garden_set_null(bool is_null)    
    {    
      walled_garden_used_ = true;      
      walled_garden_null_ = is_null;      
      return *this;      
    }    
    inline    
    const bool&    
    CMPRequestStats::Key::walled_garden() const    
    {    
      return walled_garden_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::walled_garden_used() const    
    {    
      return walled_garden_used_;      
    }    
    inline    
    bool    
    CMPRequestStats::Key::walled_garden_is_null() const    
    {    
      return walled_garden_null_;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::key (    
      const AutoTest::Time& sdate,      
      const AutoTest::Time& adv_sdate,      
      const AutoTest::Time& cmp_sdate,      
      const int& adv_account_id,      
      const int& cmp_account_id,      
      const int& ccg_id,      
      const int& colo_id,      
      const int& channel_id,      
      const int& channel_rate_id,      
      const int& currency_exchange_id,      
      const bool& fraud_correction,      
      const int& cc_id,      
      const std::string& country_code,      
      const bool& walled_garden      
    )    
    {    
      key_ = Key (      
        sdate,        
        adv_sdate,        
        cmp_sdate,        
        adv_account_id,        
        cmp_account_id,        
        ccg_id,        
        colo_id,        
        channel_id,        
        channel_rate_id,        
        currency_exchange_id,        
        fraud_correction,        
        cc_id,        
        country_code,        
        walled_garden        
      );      
      return key_;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::key ()    
    {    
      return key_;      
    }    
    inline    
    CMPRequestStats::Key&    
    CMPRequestStats::key (const CMPRequestStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<CMPRequestStats, 5>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<CMPRequestStats, 5>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<CMPRequestStats, 5>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<CMPRequestStats, 5>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<CMPRequestStats, 5>::Diffs&     
    DiffStats<CMPRequestStats, 5>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CMPRequestStats, 5>::Diffs&     
    DiffStats<CMPRequestStats, 5>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CMPRequestStats, 5>::Diffs&     
    DiffStats<CMPRequestStats, 5>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CMPRequestStats, 5>::Diffs     
    DiffStats<CMPRequestStats, 5>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<CMPRequestStats, 5>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<CMPRequestStats, 5>::Diffs     
    DiffStats<CMPRequestStats, 5>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<CMPRequestStats, 5>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<CMPRequestStats, 5>::Diffs::const_iterator    
    DiffStats<CMPRequestStats, 5>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<CMPRequestStats, 5>::Diffs::const_iterator    
    DiffStats<CMPRequestStats, 5>::Diffs::end () const    
    {    
      return diffs + 5;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<CMPRequestStats, 5>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<CMPRequestStats, 5>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<CMPRequestStats, 5>::Diffs::size () const    
    {    
      return 5;      
    }    
    inline    
    void    
    DiffStats<CMPRequestStats, 5>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 5; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    CMPRequestStats::imps () const    
    {    
      return values[CMPRequestStats::IMPS];      
    }    
    inline    
    DiffStats<CMPRequestStats, 5>::Diffs&     
    DiffStats<CMPRequestStats, 5>::Diffs::imps (const stats_diff_type& value)    
    {    
      diffs[CMPRequestStats::IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CMPRequestStats, 5>::Diffs::imps () const    
    {    
      return diffs[CMPRequestStats::IMPS];      
    }    
    inline    
    stats_value_type    
    CMPRequestStats::adv_amount_cmp () const    
    {    
      return values[CMPRequestStats::ADV_AMOUNT_CMP];      
    }    
    inline    
    DiffStats<CMPRequestStats, 5>::Diffs&     
    DiffStats<CMPRequestStats, 5>::Diffs::adv_amount_cmp (const stats_diff_type& value)    
    {    
      diffs[CMPRequestStats::ADV_AMOUNT_CMP] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CMPRequestStats, 5>::Diffs::adv_amount_cmp () const    
    {    
      return diffs[CMPRequestStats::ADV_AMOUNT_CMP];      
    }    
    inline    
    stats_value_type    
    CMPRequestStats::cmp_amount () const    
    {    
      return values[CMPRequestStats::CMP_AMOUNT];      
    }    
    inline    
    DiffStats<CMPRequestStats, 5>::Diffs&     
    DiffStats<CMPRequestStats, 5>::Diffs::cmp_amount (const stats_diff_type& value)    
    {    
      diffs[CMPRequestStats::CMP_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CMPRequestStats, 5>::Diffs::cmp_amount () const    
    {    
      return diffs[CMPRequestStats::CMP_AMOUNT];      
    }    
    inline    
    stats_value_type    
    CMPRequestStats::cmp_amount_global () const    
    {    
      return values[CMPRequestStats::CMP_AMOUNT_GLOBAL];      
    }    
    inline    
    DiffStats<CMPRequestStats, 5>::Diffs&     
    DiffStats<CMPRequestStats, 5>::Diffs::cmp_amount_global (const stats_diff_type& value)    
    {    
      diffs[CMPRequestStats::CMP_AMOUNT_GLOBAL] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CMPRequestStats, 5>::Diffs::cmp_amount_global () const    
    {    
      return diffs[CMPRequestStats::CMP_AMOUNT_GLOBAL];      
    }    
    inline    
    stats_value_type    
    CMPRequestStats::clicks () const    
    {    
      return values[CMPRequestStats::CLICKS];      
    }    
    inline    
    DiffStats<CMPRequestStats, 5>::Diffs&     
    DiffStats<CMPRequestStats, 5>::Diffs::clicks (const stats_diff_type& value)    
    {    
      diffs[CMPRequestStats::CLICKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CMPRequestStats, 5>::Diffs::clicks () const    
    {    
      return diffs[CMPRequestStats::CLICKS];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_CMPREQUESTSTATS_HPP

