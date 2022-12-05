
#ifndef __AUTOTESTS_COMMONS_STATS_CCGKEYWORDSTATSDAILY_HPP
#define __AUTOTESTS_COMMONS_STATS_CCGKEYWORDSTATSDAILY_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class CCGKeywordStatsDaily:    
      public DiffStats<CCGKeywordStatsDaily, 5>    
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
        typedef DiffStats<CCGKeywordStatsDaily, 5> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            AutoTest::Time adv_sdate_;            
            bool adv_sdate_used_;            
            bool adv_sdate_null_;            
            int ccg_keyword_id_;            
            bool ccg_keyword_id_used_;            
            bool ccg_keyword_id_null_;            
            int cc_id_;            
            bool cc_id_used_;            
            bool cc_id_null_;            
          public:          
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
            Key& cc_id(const int& value);            
            Key& cc_id_set_null(bool is_null = true);            
            const int& cc_id() const;            
            bool cc_id_used() const;            
            bool cc_id_is_null() const;            
            Key ();            
            Key (            
              const AutoTest::Time& adv_sdate,              
              const int& ccg_keyword_id,              
              const int& cc_id              
            );            
        };        
        stats_value_type imps () const;        
        stats_value_type clicks () const;        
        stats_value_type adv_amount () const;        
        stats_value_type adv_comm_amount () const;        
        stats_value_type pub_amount_adv () const;        
        void print_idname (std::ostream& out) const;        
                
        CCGKeywordStatsDaily (const Key& value);        
        CCGKeywordStatsDaily (        
          const AutoTest::Time& adv_sdate,          
          const int& ccg_keyword_id,          
          const int& cc_id          
        );        
        CCGKeywordStatsDaily ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const AutoTest::Time& adv_sdate,          
          const int& ccg_keyword_id,          
          const int& cc_id          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<CCGKeywordStatsDaily, 5>::Diffs    
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
    ///////////////////////////////// CCGKeywordStatsDaily    
    inline    
    CCGKeywordStatsDaily::CCGKeywordStatsDaily ()    
      :Base("CCGKeywordStatsDaily")    
    {}    
    inline    
    CCGKeywordStatsDaily::CCGKeywordStatsDaily (const CCGKeywordStatsDaily::Key& value)    
      :Base("CCGKeywordStatsDaily")    
    {    
      key_ = value;      
    }    
    inline    
    CCGKeywordStatsDaily::CCGKeywordStatsDaily (    
      const AutoTest::Time& adv_sdate,      
      const int& ccg_keyword_id,      
      const int& cc_id      
    )    
      :Base("CCGKeywordStatsDaily")    
    {    
      key_ = Key (      
        adv_sdate,        
        ccg_keyword_id,        
        cc_id        
      );      
    }    
    inline    
    CCGKeywordStatsDaily::Key::Key ()    
      :adv_sdate_(default_date()),ccg_keyword_id_(0),cc_id_(0)    
    {    
      adv_sdate_used_ = false;      
      adv_sdate_null_ = false;      
      ccg_keyword_id_used_ = false;      
      ccg_keyword_id_null_ = false;      
      cc_id_used_ = false;      
      cc_id_null_ = false;      
    }    
    inline    
    CCGKeywordStatsDaily::Key::Key (    
      const AutoTest::Time& adv_sdate,      
      const int& ccg_keyword_id,      
      const int& cc_id      
    )    
      :adv_sdate_(adv_sdate),ccg_keyword_id_(ccg_keyword_id),cc_id_(cc_id)    
    {    
      adv_sdate_used_ = true;      
      adv_sdate_null_ = false;      
      ccg_keyword_id_used_ = true;      
      ccg_keyword_id_null_ = false;      
      cc_id_used_ = true;      
      cc_id_null_ = false;      
    }    
    inline    
    CCGKeywordStatsDaily::Key&    
    CCGKeywordStatsDaily::Key::adv_sdate(const AutoTest::Time& value)    
    {    
      adv_sdate_ = value;      
      adv_sdate_used_ = true;      
      adv_sdate_null_ = false;      
      return *this;      
    }    
    inline    
    CCGKeywordStatsDaily::Key&    
    CCGKeywordStatsDaily::Key::adv_sdate_set_null(bool is_null)    
    {    
      adv_sdate_used_ = true;      
      adv_sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    CCGKeywordStatsDaily::Key::adv_sdate() const    
    {    
      return adv_sdate_;      
    }    
    inline    
    bool    
    CCGKeywordStatsDaily::Key::adv_sdate_used() const    
    {    
      return adv_sdate_used_;      
    }    
    inline    
    bool    
    CCGKeywordStatsDaily::Key::adv_sdate_is_null() const    
    {    
      return adv_sdate_null_;      
    }    
    inline    
    CCGKeywordStatsDaily::Key&    
    CCGKeywordStatsDaily::Key::ccg_keyword_id(const int& value)    
    {    
      ccg_keyword_id_ = value;      
      ccg_keyword_id_used_ = true;      
      ccg_keyword_id_null_ = false;      
      return *this;      
    }    
    inline    
    CCGKeywordStatsDaily::Key&    
    CCGKeywordStatsDaily::Key::ccg_keyword_id_set_null(bool is_null)    
    {    
      ccg_keyword_id_used_ = true;      
      ccg_keyword_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CCGKeywordStatsDaily::Key::ccg_keyword_id() const    
    {    
      return ccg_keyword_id_;      
    }    
    inline    
    bool    
    CCGKeywordStatsDaily::Key::ccg_keyword_id_used() const    
    {    
      return ccg_keyword_id_used_;      
    }    
    inline    
    bool    
    CCGKeywordStatsDaily::Key::ccg_keyword_id_is_null() const    
    {    
      return ccg_keyword_id_null_;      
    }    
    inline    
    CCGKeywordStatsDaily::Key&    
    CCGKeywordStatsDaily::Key::cc_id(const int& value)    
    {    
      cc_id_ = value;      
      cc_id_used_ = true;      
      cc_id_null_ = false;      
      return *this;      
    }    
    inline    
    CCGKeywordStatsDaily::Key&    
    CCGKeywordStatsDaily::Key::cc_id_set_null(bool is_null)    
    {    
      cc_id_used_ = true;      
      cc_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CCGKeywordStatsDaily::Key::cc_id() const    
    {    
      return cc_id_;      
    }    
    inline    
    bool    
    CCGKeywordStatsDaily::Key::cc_id_used() const    
    {    
      return cc_id_used_;      
    }    
    inline    
    bool    
    CCGKeywordStatsDaily::Key::cc_id_is_null() const    
    {    
      return cc_id_null_;      
    }    
    inline    
    CCGKeywordStatsDaily::Key&    
    CCGKeywordStatsDaily::key (    
      const AutoTest::Time& adv_sdate,      
      const int& ccg_keyword_id,      
      const int& cc_id      
    )    
    {    
      key_ = Key (      
        adv_sdate,        
        ccg_keyword_id,        
        cc_id        
      );      
      return key_;      
    }    
    inline    
    CCGKeywordStatsDaily::Key&    
    CCGKeywordStatsDaily::key ()    
    {    
      return key_;      
    }    
    inline    
    CCGKeywordStatsDaily::Key&    
    CCGKeywordStatsDaily::key (const CCGKeywordStatsDaily::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs&     
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs&     
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs&     
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs     
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<CCGKeywordStatsDaily, 5>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs     
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<CCGKeywordStatsDaily, 5>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::const_iterator    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::const_iterator    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::end () const    
    {    
      return diffs + 5;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::size () const    
    {    
      return 5;      
    }    
    inline    
    void    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 5; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    CCGKeywordStatsDaily::imps () const    
    {    
      return values[CCGKeywordStatsDaily::IMPS];      
    }    
    inline    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs&     
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::imps (const stats_diff_type& value)    
    {    
      diffs[CCGKeywordStatsDaily::IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::imps () const    
    {    
      return diffs[CCGKeywordStatsDaily::IMPS];      
    }    
    inline    
    stats_value_type    
    CCGKeywordStatsDaily::clicks () const    
    {    
      return values[CCGKeywordStatsDaily::CLICKS];      
    }    
    inline    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs&     
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::clicks (const stats_diff_type& value)    
    {    
      diffs[CCGKeywordStatsDaily::CLICKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::clicks () const    
    {    
      return diffs[CCGKeywordStatsDaily::CLICKS];      
    }    
    inline    
    stats_value_type    
    CCGKeywordStatsDaily::adv_amount () const    
    {    
      return values[CCGKeywordStatsDaily::ADV_AMOUNT];      
    }    
    inline    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs&     
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::adv_amount (const stats_diff_type& value)    
    {    
      diffs[CCGKeywordStatsDaily::ADV_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::adv_amount () const    
    {    
      return diffs[CCGKeywordStatsDaily::ADV_AMOUNT];      
    }    
    inline    
    stats_value_type    
    CCGKeywordStatsDaily::adv_comm_amount () const    
    {    
      return values[CCGKeywordStatsDaily::ADV_COMM_AMOUNT];      
    }    
    inline    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs&     
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::adv_comm_amount (const stats_diff_type& value)    
    {    
      diffs[CCGKeywordStatsDaily::ADV_COMM_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::adv_comm_amount () const    
    {    
      return diffs[CCGKeywordStatsDaily::ADV_COMM_AMOUNT];      
    }    
    inline    
    stats_value_type    
    CCGKeywordStatsDaily::pub_amount_adv () const    
    {    
      return values[CCGKeywordStatsDaily::PUB_AMOUNT_ADV];      
    }    
    inline    
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs&     
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::pub_amount_adv (const stats_diff_type& value)    
    {    
      diffs[CCGKeywordStatsDaily::PUB_AMOUNT_ADV] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CCGKeywordStatsDaily, 5>::Diffs::pub_amount_adv () const    
    {    
      return diffs[CCGKeywordStatsDaily::PUB_AMOUNT_ADV];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_CCGKEYWORDSTATSDAILY_HPP

