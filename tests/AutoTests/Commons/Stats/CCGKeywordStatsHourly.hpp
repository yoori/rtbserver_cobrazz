
#ifndef __AUTOTESTS_COMMONS_STATS_CCGKEYWORDSTATSHOURLY_HPP
#define __AUTOTESTS_COMMONS_STATS_CCGKEYWORDSTATSHOURLY_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class CCGKeywordStatsHourly:    
      public DiffStats<CCGKeywordStatsHourly, 5>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          IMPS = 0,          
          CLICKS,          
          ADV_AMOUNT,          
          ADV_COMM_AMOUNT,          
          PUB_AMOUNT_ADV          
        };        
        typedef DiffStats<CCGKeywordStatsHourly, 5> Base;        
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
            int ccg_keyword_id_;            
            bool ccg_keyword_id_used_;            
            bool ccg_keyword_id_null_;            
            int currency_exchange_id_;            
            bool currency_exchange_id_used_;            
            bool currency_exchange_id_null_;            
            int colo_id_;            
            bool colo_id_used_;            
            bool colo_id_null_;            
            int cc_id_;            
            bool cc_id_used_;            
            bool cc_id_null_;            
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
            Key& ccg_keyword_id(const int& value);            
            Key& ccg_keyword_id_set_null(bool is_null = true);            
            const int& ccg_keyword_id() const;            
            bool ccg_keyword_id_used() const;            
            bool ccg_keyword_id_is_null() const;            
            Key& currency_exchange_id(const int& value);            
            Key& currency_exchange_id_set_null(bool is_null = true);            
            const int& currency_exchange_id() const;            
            bool currency_exchange_id_used() const;            
            bool currency_exchange_id_is_null() const;            
            Key& colo_id(const int& value);            
            Key& colo_id_set_null(bool is_null = true);            
            const int& colo_id() const;            
            bool colo_id_used() const;            
            bool colo_id_is_null() const;            
            Key& cc_id(const int& value);            
            Key& cc_id_set_null(bool is_null = true);            
            const int& cc_id() const;            
            bool cc_id_used() const;            
            bool cc_id_is_null() const;            
            Key ();            
            Key (            
              const AutoTest::Time& sdate,              
              const AutoTest::Time& adv_sdate,              
              const int& ccg_keyword_id,              
              const int& currency_exchange_id,              
              const int& colo_id,              
              const int& cc_id              
            );            
        };        
        stats_value_type imps () const;        
        stats_value_type clicks () const;        
        stats_value_type adv_amount () const;        
        stats_value_type adv_comm_amount () const;        
        stats_value_type pub_amount_adv () const;        
        void print_idname (std::ostream& out) const;        
                
        CCGKeywordStatsHourly (const Key& value);        
        CCGKeywordStatsHourly (        
          const AutoTest::Time& sdate,          
          const AutoTest::Time& adv_sdate,          
          const int& ccg_keyword_id,          
          const int& currency_exchange_id,          
          const int& colo_id,          
          const int& cc_id          
        );        
        CCGKeywordStatsHourly ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const AutoTest::Time& sdate,          
          const AutoTest::Time& adv_sdate,          
          const int& ccg_keyword_id,          
          const int& currency_exchange_id,          
          const int& colo_id,          
          const int& cc_id          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<CCGKeywordStatsHourly, 5>::Diffs    
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
        Diffs& clicks (const stats_diff_type& value);        
        const stats_diff_type& clicks () const;        
        Diffs& adv_amount (const stats_diff_type& value);        
        const stats_diff_type& adv_amount () const;        
        Diffs& adv_comm_amount (const stats_diff_type& value);        
        const stats_diff_type& adv_comm_amount () const;        
        Diffs& pub_amount_adv (const stats_diff_type& value);        
        const stats_diff_type& pub_amount_adv () const;        
      protected:      
        stats_diff_type diffs[5];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// CCGKeywordStatsHourly    
    inline    
    CCGKeywordStatsHourly::CCGKeywordStatsHourly ()    
      :Base("CCGKeywordStatsHourly")    
    {}    
    inline    
    CCGKeywordStatsHourly::CCGKeywordStatsHourly (const CCGKeywordStatsHourly::Key& value)    
      :Base("CCGKeywordStatsHourly")    
    {    
      key_ = value;      
    }    
    inline    
    CCGKeywordStatsHourly::CCGKeywordStatsHourly (    
      const AutoTest::Time& sdate,      
      const AutoTest::Time& adv_sdate,      
      const int& ccg_keyword_id,      
      const int& currency_exchange_id,      
      const int& colo_id,      
      const int& cc_id      
    )    
      :Base("CCGKeywordStatsHourly")    
    {    
      key_ = Key (      
        sdate,        
        adv_sdate,        
        ccg_keyword_id,        
        currency_exchange_id,        
        colo_id,        
        cc_id        
      );      
    }    
    inline    
    CCGKeywordStatsHourly::Key::Key ()    
      :sdate_(default_date()),adv_sdate_(default_date()),ccg_keyword_id_(0),currency_exchange_id_(0),colo_id_(0),cc_id_(0)    
    {    
      sdate_used_ = false;      
      sdate_null_ = false;      
      adv_sdate_used_ = false;      
      adv_sdate_null_ = false;      
      ccg_keyword_id_used_ = false;      
      ccg_keyword_id_null_ = false;      
      currency_exchange_id_used_ = false;      
      currency_exchange_id_null_ = false;      
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      cc_id_used_ = false;      
      cc_id_null_ = false;      
    }    
    inline    
    CCGKeywordStatsHourly::Key::Key (    
      const AutoTest::Time& sdate,      
      const AutoTest::Time& adv_sdate,      
      const int& ccg_keyword_id,      
      const int& currency_exchange_id,      
      const int& colo_id,      
      const int& cc_id      
    )    
      :sdate_(sdate),adv_sdate_(adv_sdate),ccg_keyword_id_(ccg_keyword_id),currency_exchange_id_(currency_exchange_id),colo_id_(colo_id),cc_id_(cc_id)    
    {    
      sdate_used_ = true;      
      sdate_null_ = false;      
      adv_sdate_used_ = true;      
      adv_sdate_null_ = false;      
      ccg_keyword_id_used_ = true;      
      ccg_keyword_id_null_ = false;      
      currency_exchange_id_used_ = true;      
      currency_exchange_id_null_ = false;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      cc_id_used_ = true;      
      cc_id_null_ = false;      
    }    
    inline    
    CCGKeywordStatsHourly::Key&    
    CCGKeywordStatsHourly::Key::sdate(const AutoTest::Time& value)    
    {    
      sdate_ = value;      
      sdate_used_ = true;      
      sdate_null_ = false;      
      return *this;      
    }    
    inline    
    CCGKeywordStatsHourly::Key&    
    CCGKeywordStatsHourly::Key::sdate_set_null(bool is_null)    
    {    
      sdate_used_ = true;      
      sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    CCGKeywordStatsHourly::Key::sdate() const    
    {    
      return sdate_;      
    }    
    inline    
    bool    
    CCGKeywordStatsHourly::Key::sdate_used() const    
    {    
      return sdate_used_;      
    }    
    inline    
    bool    
    CCGKeywordStatsHourly::Key::sdate_is_null() const    
    {    
      return sdate_null_;      
    }    
    inline    
    CCGKeywordStatsHourly::Key&    
    CCGKeywordStatsHourly::Key::adv_sdate(const AutoTest::Time& value)    
    {    
      adv_sdate_ = value;      
      adv_sdate_used_ = true;      
      adv_sdate_null_ = false;      
      return *this;      
    }    
    inline    
    CCGKeywordStatsHourly::Key&    
    CCGKeywordStatsHourly::Key::adv_sdate_set_null(bool is_null)    
    {    
      adv_sdate_used_ = true;      
      adv_sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    CCGKeywordStatsHourly::Key::adv_sdate() const    
    {    
      return adv_sdate_;      
    }    
    inline    
    bool    
    CCGKeywordStatsHourly::Key::adv_sdate_used() const    
    {    
      return adv_sdate_used_;      
    }    
    inline    
    bool    
    CCGKeywordStatsHourly::Key::adv_sdate_is_null() const    
    {    
      return adv_sdate_null_;      
    }    
    inline    
    CCGKeywordStatsHourly::Key&    
    CCGKeywordStatsHourly::Key::ccg_keyword_id(const int& value)    
    {    
      ccg_keyword_id_ = value;      
      ccg_keyword_id_used_ = true;      
      ccg_keyword_id_null_ = false;      
      return *this;      
    }    
    inline    
    CCGKeywordStatsHourly::Key&    
    CCGKeywordStatsHourly::Key::ccg_keyword_id_set_null(bool is_null)    
    {    
      ccg_keyword_id_used_ = true;      
      ccg_keyword_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CCGKeywordStatsHourly::Key::ccg_keyword_id() const    
    {    
      return ccg_keyword_id_;      
    }    
    inline    
    bool    
    CCGKeywordStatsHourly::Key::ccg_keyword_id_used() const    
    {    
      return ccg_keyword_id_used_;      
    }    
    inline    
    bool    
    CCGKeywordStatsHourly::Key::ccg_keyword_id_is_null() const    
    {    
      return ccg_keyword_id_null_;      
    }    
    inline    
    CCGKeywordStatsHourly::Key&    
    CCGKeywordStatsHourly::Key::currency_exchange_id(const int& value)    
    {    
      currency_exchange_id_ = value;      
      currency_exchange_id_used_ = true;      
      currency_exchange_id_null_ = false;      
      return *this;      
    }    
    inline    
    CCGKeywordStatsHourly::Key&    
    CCGKeywordStatsHourly::Key::currency_exchange_id_set_null(bool is_null)    
    {    
      currency_exchange_id_used_ = true;      
      currency_exchange_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CCGKeywordStatsHourly::Key::currency_exchange_id() const    
    {    
      return currency_exchange_id_;      
    }    
    inline    
    bool    
    CCGKeywordStatsHourly::Key::currency_exchange_id_used() const    
    {    
      return currency_exchange_id_used_;      
    }    
    inline    
    bool    
    CCGKeywordStatsHourly::Key::currency_exchange_id_is_null() const    
    {    
      return currency_exchange_id_null_;      
    }    
    inline    
    CCGKeywordStatsHourly::Key&    
    CCGKeywordStatsHourly::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    CCGKeywordStatsHourly::Key&    
    CCGKeywordStatsHourly::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CCGKeywordStatsHourly::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    CCGKeywordStatsHourly::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    CCGKeywordStatsHourly::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    CCGKeywordStatsHourly::Key&    
    CCGKeywordStatsHourly::Key::cc_id(const int& value)    
    {    
      cc_id_ = value;      
      cc_id_used_ = true;      
      cc_id_null_ = false;      
      return *this;      
    }    
    inline    
    CCGKeywordStatsHourly::Key&    
    CCGKeywordStatsHourly::Key::cc_id_set_null(bool is_null)    
    {    
      cc_id_used_ = true;      
      cc_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CCGKeywordStatsHourly::Key::cc_id() const    
    {    
      return cc_id_;      
    }    
    inline    
    bool    
    CCGKeywordStatsHourly::Key::cc_id_used() const    
    {    
      return cc_id_used_;      
    }    
    inline    
    bool    
    CCGKeywordStatsHourly::Key::cc_id_is_null() const    
    {    
      return cc_id_null_;      
    }    
    inline    
    CCGKeywordStatsHourly::Key&    
    CCGKeywordStatsHourly::key (    
      const AutoTest::Time& sdate,      
      const AutoTest::Time& adv_sdate,      
      const int& ccg_keyword_id,      
      const int& currency_exchange_id,      
      const int& colo_id,      
      const int& cc_id      
    )    
    {    
      key_ = Key (      
        sdate,        
        adv_sdate,        
        ccg_keyword_id,        
        currency_exchange_id,        
        colo_id,        
        cc_id        
      );      
      return key_;      
    }    
    inline    
    CCGKeywordStatsHourly::Key&    
    CCGKeywordStatsHourly::key ()    
    {    
      return key_;      
    }    
    inline    
    CCGKeywordStatsHourly::Key&    
    CCGKeywordStatsHourly::key (const CCGKeywordStatsHourly::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs&     
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs&     
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs&     
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs     
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<CCGKeywordStatsHourly, 5>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs     
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<CCGKeywordStatsHourly, 5>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::const_iterator    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::const_iterator    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::end () const    
    {    
      return diffs + 5;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::size () const    
    {    
      return 5;      
    }    
    inline    
    void    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 5; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    CCGKeywordStatsHourly::imps () const    
    {    
      return values[CCGKeywordStatsHourly::IMPS];      
    }    
    inline    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs&     
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::imps (const stats_diff_type& value)    
    {    
      diffs[CCGKeywordStatsHourly::IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::imps () const    
    {    
      return diffs[CCGKeywordStatsHourly::IMPS];      
    }    
    inline    
    stats_value_type    
    CCGKeywordStatsHourly::clicks () const    
    {    
      return values[CCGKeywordStatsHourly::CLICKS];      
    }    
    inline    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs&     
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::clicks (const stats_diff_type& value)    
    {    
      diffs[CCGKeywordStatsHourly::CLICKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::clicks () const    
    {    
      return diffs[CCGKeywordStatsHourly::CLICKS];      
    }    
    inline    
    stats_value_type    
    CCGKeywordStatsHourly::adv_amount () const    
    {    
      return values[CCGKeywordStatsHourly::ADV_AMOUNT];      
    }    
    inline    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs&     
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::adv_amount (const stats_diff_type& value)    
    {    
      diffs[CCGKeywordStatsHourly::ADV_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::adv_amount () const    
    {    
      return diffs[CCGKeywordStatsHourly::ADV_AMOUNT];      
    }    
    inline    
    stats_value_type    
    CCGKeywordStatsHourly::adv_comm_amount () const    
    {    
      return values[CCGKeywordStatsHourly::ADV_COMM_AMOUNT];      
    }    
    inline    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs&     
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::adv_comm_amount (const stats_diff_type& value)    
    {    
      diffs[CCGKeywordStatsHourly::ADV_COMM_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::adv_comm_amount () const    
    {    
      return diffs[CCGKeywordStatsHourly::ADV_COMM_AMOUNT];      
    }    
    inline    
    stats_value_type    
    CCGKeywordStatsHourly::pub_amount_adv () const    
    {    
      return values[CCGKeywordStatsHourly::PUB_AMOUNT_ADV];      
    }    
    inline    
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs&     
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::pub_amount_adv (const stats_diff_type& value)    
    {    
      diffs[CCGKeywordStatsHourly::PUB_AMOUNT_ADV] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CCGKeywordStatsHourly, 5>::Diffs::pub_amount_adv () const    
    {    
      return diffs[CCGKeywordStatsHourly::PUB_AMOUNT_ADV];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_CCGKEYWORDSTATSHOURLY_HPP

