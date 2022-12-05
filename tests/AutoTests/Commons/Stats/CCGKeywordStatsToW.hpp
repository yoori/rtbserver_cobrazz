
#ifndef __AUTOTESTS_COMMONS_STATS_CCGKEYWORDSTATSTOW_HPP
#define __AUTOTESTS_COMMONS_STATS_CCGKEYWORDSTATSTOW_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class CCGKeywordStatsToW:    
      public DiffStats<CCGKeywordStatsToW, 5>    
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
        typedef DiffStats<CCGKeywordStatsToW, 5> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            int day_of_week_;            
            bool day_of_week_used_;            
            bool day_of_week_null_;            
            int hour_;            
            bool hour_used_;            
            bool hour_null_;            
            int ccg_keyword_id_;            
            bool ccg_keyword_id_used_;            
            bool ccg_keyword_id_null_;            
            int cc_id_;            
            bool cc_id_used_;            
            bool cc_id_null_;            
          public:          
            Key& day_of_week(const int& value);            
            Key& day_of_week_set_null(bool is_null = true);            
            const int& day_of_week() const;            
            bool day_of_week_used() const;            
            bool day_of_week_is_null() const;            
            Key& hour(const int& value);            
            Key& hour_set_null(bool is_null = true);            
            const int& hour() const;            
            bool hour_used() const;            
            bool hour_is_null() const;            
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
              const int& day_of_week,              
              const int& hour,              
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
                
        CCGKeywordStatsToW (const Key& value);        
        CCGKeywordStatsToW (        
          const int& day_of_week,          
          const int& hour,          
          const int& ccg_keyword_id,          
          const int& cc_id          
        );        
        CCGKeywordStatsToW ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& day_of_week,          
          const int& hour,          
          const int& ccg_keyword_id,          
          const int& cc_id          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<CCGKeywordStatsToW, 5>::Diffs    
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
    ///////////////////////////////// CCGKeywordStatsToW    
    inline    
    CCGKeywordStatsToW::CCGKeywordStatsToW ()    
      :Base("CCGKeywordStatsToW")    
    {}    
    inline    
    CCGKeywordStatsToW::CCGKeywordStatsToW (const CCGKeywordStatsToW::Key& value)    
      :Base("CCGKeywordStatsToW")    
    {    
      key_ = value;      
    }    
    inline    
    CCGKeywordStatsToW::CCGKeywordStatsToW (    
      const int& day_of_week,      
      const int& hour,      
      const int& ccg_keyword_id,      
      const int& cc_id      
    )    
      :Base("CCGKeywordStatsToW")    
    {    
      key_ = Key (      
        day_of_week,        
        hour,        
        ccg_keyword_id,        
        cc_id        
      );      
    }    
    inline    
    CCGKeywordStatsToW::Key::Key ()    
      :day_of_week_(0),hour_(0),ccg_keyword_id_(0),cc_id_(0)    
    {    
      day_of_week_used_ = false;      
      day_of_week_null_ = false;      
      hour_used_ = false;      
      hour_null_ = false;      
      ccg_keyword_id_used_ = false;      
      ccg_keyword_id_null_ = false;      
      cc_id_used_ = false;      
      cc_id_null_ = false;      
    }    
    inline    
    CCGKeywordStatsToW::Key::Key (    
      const int& day_of_week,      
      const int& hour,      
      const int& ccg_keyword_id,      
      const int& cc_id      
    )    
      :day_of_week_(day_of_week),hour_(hour),ccg_keyword_id_(ccg_keyword_id),cc_id_(cc_id)    
    {    
      day_of_week_used_ = true;      
      day_of_week_null_ = false;      
      hour_used_ = true;      
      hour_null_ = false;      
      ccg_keyword_id_used_ = true;      
      ccg_keyword_id_null_ = false;      
      cc_id_used_ = true;      
      cc_id_null_ = false;      
    }    
    inline    
    CCGKeywordStatsToW::Key&    
    CCGKeywordStatsToW::Key::day_of_week(const int& value)    
    {    
      day_of_week_ = value;      
      day_of_week_used_ = true;      
      day_of_week_null_ = false;      
      return *this;      
    }    
    inline    
    CCGKeywordStatsToW::Key&    
    CCGKeywordStatsToW::Key::day_of_week_set_null(bool is_null)    
    {    
      day_of_week_used_ = true;      
      day_of_week_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CCGKeywordStatsToW::Key::day_of_week() const    
    {    
      return day_of_week_;      
    }    
    inline    
    bool    
    CCGKeywordStatsToW::Key::day_of_week_used() const    
    {    
      return day_of_week_used_;      
    }    
    inline    
    bool    
    CCGKeywordStatsToW::Key::day_of_week_is_null() const    
    {    
      return day_of_week_null_;      
    }    
    inline    
    CCGKeywordStatsToW::Key&    
    CCGKeywordStatsToW::Key::hour(const int& value)    
    {    
      hour_ = value;      
      hour_used_ = true;      
      hour_null_ = false;      
      return *this;      
    }    
    inline    
    CCGKeywordStatsToW::Key&    
    CCGKeywordStatsToW::Key::hour_set_null(bool is_null)    
    {    
      hour_used_ = true;      
      hour_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CCGKeywordStatsToW::Key::hour() const    
    {    
      return hour_;      
    }    
    inline    
    bool    
    CCGKeywordStatsToW::Key::hour_used() const    
    {    
      return hour_used_;      
    }    
    inline    
    bool    
    CCGKeywordStatsToW::Key::hour_is_null() const    
    {    
      return hour_null_;      
    }    
    inline    
    CCGKeywordStatsToW::Key&    
    CCGKeywordStatsToW::Key::ccg_keyword_id(const int& value)    
    {    
      ccg_keyword_id_ = value;      
      ccg_keyword_id_used_ = true;      
      ccg_keyword_id_null_ = false;      
      return *this;      
    }    
    inline    
    CCGKeywordStatsToW::Key&    
    CCGKeywordStatsToW::Key::ccg_keyword_id_set_null(bool is_null)    
    {    
      ccg_keyword_id_used_ = true;      
      ccg_keyword_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CCGKeywordStatsToW::Key::ccg_keyword_id() const    
    {    
      return ccg_keyword_id_;      
    }    
    inline    
    bool    
    CCGKeywordStatsToW::Key::ccg_keyword_id_used() const    
    {    
      return ccg_keyword_id_used_;      
    }    
    inline    
    bool    
    CCGKeywordStatsToW::Key::ccg_keyword_id_is_null() const    
    {    
      return ccg_keyword_id_null_;      
    }    
    inline    
    CCGKeywordStatsToW::Key&    
    CCGKeywordStatsToW::Key::cc_id(const int& value)    
    {    
      cc_id_ = value;      
      cc_id_used_ = true;      
      cc_id_null_ = false;      
      return *this;      
    }    
    inline    
    CCGKeywordStatsToW::Key&    
    CCGKeywordStatsToW::Key::cc_id_set_null(bool is_null)    
    {    
      cc_id_used_ = true;      
      cc_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CCGKeywordStatsToW::Key::cc_id() const    
    {    
      return cc_id_;      
    }    
    inline    
    bool    
    CCGKeywordStatsToW::Key::cc_id_used() const    
    {    
      return cc_id_used_;      
    }    
    inline    
    bool    
    CCGKeywordStatsToW::Key::cc_id_is_null() const    
    {    
      return cc_id_null_;      
    }    
    inline    
    CCGKeywordStatsToW::Key&    
    CCGKeywordStatsToW::key (    
      const int& day_of_week,      
      const int& hour,      
      const int& ccg_keyword_id,      
      const int& cc_id      
    )    
    {    
      key_ = Key (      
        day_of_week,        
        hour,        
        ccg_keyword_id,        
        cc_id        
      );      
      return key_;      
    }    
    inline    
    CCGKeywordStatsToW::Key&    
    CCGKeywordStatsToW::key ()    
    {    
      return key_;      
    }    
    inline    
    CCGKeywordStatsToW::Key&    
    CCGKeywordStatsToW::key (const CCGKeywordStatsToW::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs&     
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs&     
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs&     
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 5; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs     
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<CCGKeywordStatsToW, 5>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs     
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<CCGKeywordStatsToW, 5>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::const_iterator    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::const_iterator    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::end () const    
    {    
      return diffs + 5;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::size () const    
    {    
      return 5;      
    }    
    inline    
    void    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 5; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    CCGKeywordStatsToW::imps () const    
    {    
      return values[CCGKeywordStatsToW::IMPS];      
    }    
    inline    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs&     
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::imps (const stats_diff_type& value)    
    {    
      diffs[CCGKeywordStatsToW::IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::imps () const    
    {    
      return diffs[CCGKeywordStatsToW::IMPS];      
    }    
    inline    
    stats_value_type    
    CCGKeywordStatsToW::clicks () const    
    {    
      return values[CCGKeywordStatsToW::CLICKS];      
    }    
    inline    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs&     
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::clicks (const stats_diff_type& value)    
    {    
      diffs[CCGKeywordStatsToW::CLICKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::clicks () const    
    {    
      return diffs[CCGKeywordStatsToW::CLICKS];      
    }    
    inline    
    stats_value_type    
    CCGKeywordStatsToW::adv_amount () const    
    {    
      return values[CCGKeywordStatsToW::ADV_AMOUNT];      
    }    
    inline    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs&     
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::adv_amount (const stats_diff_type& value)    
    {    
      diffs[CCGKeywordStatsToW::ADV_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::adv_amount () const    
    {    
      return diffs[CCGKeywordStatsToW::ADV_AMOUNT];      
    }    
    inline    
    stats_value_type    
    CCGKeywordStatsToW::adv_comm_amount () const    
    {    
      return values[CCGKeywordStatsToW::ADV_COMM_AMOUNT];      
    }    
    inline    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs&     
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::adv_comm_amount (const stats_diff_type& value)    
    {    
      diffs[CCGKeywordStatsToW::ADV_COMM_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::adv_comm_amount () const    
    {    
      return diffs[CCGKeywordStatsToW::ADV_COMM_AMOUNT];      
    }    
    inline    
    stats_value_type    
    CCGKeywordStatsToW::pub_amount_adv () const    
    {    
      return values[CCGKeywordStatsToW::PUB_AMOUNT_ADV];      
    }    
    inline    
    DiffStats<CCGKeywordStatsToW, 5>::Diffs&     
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::pub_amount_adv (const stats_diff_type& value)    
    {    
      diffs[CCGKeywordStatsToW::PUB_AMOUNT_ADV] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CCGKeywordStatsToW, 5>::Diffs::pub_amount_adv () const    
    {    
      return diffs[CCGKeywordStatsToW::PUB_AMOUNT_ADV];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_CCGKEYWORDSTATSTOW_HPP

