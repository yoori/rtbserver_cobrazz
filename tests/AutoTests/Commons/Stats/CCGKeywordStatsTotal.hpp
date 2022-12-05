
#ifndef __AUTOTESTS_COMMONS_STATS_CCGKEYWORDSTATSTOTAL_HPP
#define __AUTOTESTS_COMMONS_STATS_CCGKEYWORDSTATSTOTAL_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class CCGKeywordStatsTotal:    
      public DiffStats<CCGKeywordStatsTotal, 5>    
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
        typedef DiffStats<CCGKeywordStatsTotal, 5> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            int ccg_keyword_id_;            
            bool ccg_keyword_id_used_;            
            bool ccg_keyword_id_null_;            
            int cc_id_;            
            bool cc_id_used_;            
            bool cc_id_null_;            
          public:          
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
                
        CCGKeywordStatsTotal (const Key& value);        
        CCGKeywordStatsTotal (        
          const int& ccg_keyword_id,          
          const int& cc_id          
        );        
        CCGKeywordStatsTotal ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& ccg_keyword_id,          
          const int& cc_id          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<CCGKeywordStatsTotal, 5>::Diffs    
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
    ///////////////////////////////// CCGKeywordStatsTotal    
    inline    
    CCGKeywordStatsTotal::CCGKeywordStatsTotal ()    
      :Base("CCGKeywordStatsTotal")    
    {}    
    inline    
    CCGKeywordStatsTotal::CCGKeywordStatsTotal (const CCGKeywordStatsTotal::Key& value)    
      :Base("CCGKeywordStatsTotal")    
    {    
      key_ = value;      
    }    
    inline    
    CCGKeywordStatsTotal::CCGKeywordStatsTotal (    
      const int& ccg_keyword_id,      
      const int& cc_id      
    )    
      :Base("CCGKeywordStatsTotal")    
    {    
      key_ = Key (      
        ccg_keyword_id,        
        cc_id        
      );      
    }    
    inline    
    CCGKeywordStatsTotal::Key::Key ()    
      :ccg_keyword_id_(0),cc_id_(0)    
    {    
      ccg_keyword_id_used_ = false;      
      ccg_keyword_id_null_ = false;      
      cc_id_used_ = false;      
      cc_id_null_ = false;      
    }    
    inline    
    CCGKeywordStatsTotal::Key::Key (    
      const int& ccg_keyword_id,      
      const int& cc_id      
    )    
      :ccg_keyword_id_(ccg_keyword_id),cc_id_(cc_id)    
    {    
      ccg_keyword_id_used_ = true;      
      ccg_keyword_id_null_ = false;      
      cc_id_used_ = true;      
      cc_id_null_ = false;      
    }    
    inline    
    CCGKeywordStatsTotal::Key&    
    CCGKeywordStatsTotal::Key::ccg_keyword_id(const int& value)    
    {    
      ccg_keyword_id_ = value;      
      ccg_keyword_id_used_ = true;      
      ccg_keyword_id_null_ = false;      
      return *this;      
    }    
    inline    
    CCGKeywordStatsTotal::Key&    
    CCGKeywordStatsTotal::Key::ccg_keyword_id_set_null(bool is_null)    
    {    
      ccg_keyword_id_used_ = true;      
      ccg_keyword_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CCGKeywordStatsTotal::Key::ccg_keyword_id() const    
    {    
      return ccg_keyword_id_;      
    }    
    inline    
    bool    
    CCGKeywordStatsTotal::Key::ccg_keyword_id_used() const    
    {    
      return ccg_keyword_id_used_;      
    }    
    inline    
    bool    
    CCGKeywordStatsTotal::Key::ccg_keyword_id_is_null() const    
    {    
      return ccg_keyword_id_null_;      
    }    
    inline    
    CCGKeywordStatsTotal::Key&    
    CCGKeywordStatsTotal::Key::cc_id(const int& value)    
    {    
      cc_id_ = value;      
      cc_id_used_ = true;      
      cc_id_null_ = false;      
      return *this;      
    }    
    inline    
    CCGKeywordStatsTotal::Key&    
    CCGKeywordStatsTotal::Key::cc_id_set_null(bool is_null)    
    {    
      cc_id_used_ = true;      
      cc_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CCGKeywordStatsTotal::Key::cc_id() const    
    {    
      return cc_id_;      
    }    
    inline    
    bool    
    CCGKeywordStatsTotal::Key::cc_id_used() const    
    {    
      return cc_id_used_;      
    }    
    inline    
    bool    
    CCGKeywordStatsTotal::Key::cc_id_is_null() const    
    {    
      return cc_id_null_;      
    }    
    inline    
    CCGKeywordStatsTotal::Key&    
    CCGKeywordStatsTotal::key (    
      const int& ccg_keyword_id,      
      const int& cc_id      
    )    
    {    
      key_ = Key (      
        ccg_keyword_id,        
        cc_id        
      );      
      return key_;      
    }    
    inline    
    CCGKeywordStatsTotal::Key&    
    CCGKeywordStatsTotal::key ()    
    {    
      return key_;      
    }    
    inline    
    CCGKeywordStatsTotal::Key&    
    CCGKeywordStatsTotal::key (const CCGKeywordStatsTotal::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs&     
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs&     
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs&     
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs     
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<CCGKeywordStatsTotal, 5>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs     
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<CCGKeywordStatsTotal, 5>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::const_iterator    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::const_iterator    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::end () const    
    {    
      return diffs + 5;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::size () const    
    {    
      return 5;      
    }    
    inline    
    void    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 5; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    CCGKeywordStatsTotal::imps () const    
    {    
      return values[CCGKeywordStatsTotal::IMPS];      
    }    
    inline    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs&     
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::imps (const stats_diff_type& value)    
    {    
      diffs[CCGKeywordStatsTotal::IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::imps () const    
    {    
      return diffs[CCGKeywordStatsTotal::IMPS];      
    }    
    inline    
    stats_value_type    
    CCGKeywordStatsTotal::clicks () const    
    {    
      return values[CCGKeywordStatsTotal::CLICKS];      
    }    
    inline    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs&     
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::clicks (const stats_diff_type& value)    
    {    
      diffs[CCGKeywordStatsTotal::CLICKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::clicks () const    
    {    
      return diffs[CCGKeywordStatsTotal::CLICKS];      
    }    
    inline    
    stats_value_type    
    CCGKeywordStatsTotal::adv_amount () const    
    {    
      return values[CCGKeywordStatsTotal::ADV_AMOUNT];      
    }    
    inline    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs&     
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::adv_amount (const stats_diff_type& value)    
    {    
      diffs[CCGKeywordStatsTotal::ADV_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::adv_amount () const    
    {    
      return diffs[CCGKeywordStatsTotal::ADV_AMOUNT];      
    }    
    inline    
    stats_value_type    
    CCGKeywordStatsTotal::adv_comm_amount () const    
    {    
      return values[CCGKeywordStatsTotal::ADV_COMM_AMOUNT];      
    }    
    inline    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs&     
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::adv_comm_amount (const stats_diff_type& value)    
    {    
      diffs[CCGKeywordStatsTotal::ADV_COMM_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::adv_comm_amount () const    
    {    
      return diffs[CCGKeywordStatsTotal::ADV_COMM_AMOUNT];      
    }    
    inline    
    stats_value_type    
    CCGKeywordStatsTotal::pub_amount_adv () const    
    {    
      return values[CCGKeywordStatsTotal::PUB_AMOUNT_ADV];      
    }    
    inline    
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs&     
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::pub_amount_adv (const stats_diff_type& value)    
    {    
      diffs[CCGKeywordStatsTotal::PUB_AMOUNT_ADV] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CCGKeywordStatsTotal, 5>::Diffs::pub_amount_adv () const    
    {    
      return diffs[CCGKeywordStatsTotal::PUB_AMOUNT_ADV];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_CCGKEYWORDSTATSTOTAL_HPP

