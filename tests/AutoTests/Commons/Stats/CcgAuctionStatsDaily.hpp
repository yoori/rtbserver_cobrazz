
#ifndef __AUTOTESTS_COMMONS_STATS_CCGAUCTIONSTATSDAILY_HPP
#define __AUTOTESTS_COMMONS_STATS_CCGAUCTIONSTATSDAILY_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class CcgAuctionStatsDaily:    
      public DiffStats<CcgAuctionStatsDaily, 1>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          AUCTIONS_LOST          
        };        
        typedef DiffStats<CcgAuctionStatsDaily, 1> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            AutoTest::Time adv_sdate_;            
            bool adv_sdate_used_;            
            bool adv_sdate_null_;            
            int colo_id_;            
            bool colo_id_used_;            
            bool colo_id_null_;            
            int ccg_id_;            
            bool ccg_id_used_;            
            bool ccg_id_null_;            
          public:          
            Key& adv_sdate(const AutoTest::Time& value);            
            Key& adv_sdate_set_null(bool is_null = true);            
            const AutoTest::Time& adv_sdate() const;            
            bool adv_sdate_used() const;            
            bool adv_sdate_is_null() const;            
            Key& colo_id(const int& value);            
            Key& colo_id_set_null(bool is_null = true);            
            const int& colo_id() const;            
            bool colo_id_used() const;            
            bool colo_id_is_null() const;            
            Key& ccg_id(const int& value);            
            Key& ccg_id_set_null(bool is_null = true);            
            const int& ccg_id() const;            
            bool ccg_id_used() const;            
            bool ccg_id_is_null() const;            
            Key ();            
            Key (            
              const AutoTest::Time& adv_sdate,              
              const int& colo_id,              
              const int& ccg_id              
            );            
        };        
        stats_value_type auctions_lost () const;        
        void print_idname (std::ostream& out) const;        
                
        CcgAuctionStatsDaily (const Key& value);        
        CcgAuctionStatsDaily (        
          const AutoTest::Time& adv_sdate,          
          const int& colo_id,          
          const int& ccg_id          
        );        
        CcgAuctionStatsDaily ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const AutoTest::Time& adv_sdate,          
          const int& colo_id,          
          const int& ccg_id          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<CcgAuctionStatsDaily, 1>::Diffs    
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
                
        Diffs& auctions_lost (const stats_diff_type& value);        
        const stats_diff_type& auctions_lost () const;        
      protected:      
        stats_diff_type diffs[1];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// CcgAuctionStatsDaily    
    inline    
    CcgAuctionStatsDaily::CcgAuctionStatsDaily ()    
      :Base("CcgAuctionStatsDaily")    
    {}    
    inline    
    CcgAuctionStatsDaily::CcgAuctionStatsDaily (const CcgAuctionStatsDaily::Key& value)    
      :Base("CcgAuctionStatsDaily")    
    {    
      key_ = value;      
    }    
    inline    
    CcgAuctionStatsDaily::CcgAuctionStatsDaily (    
      const AutoTest::Time& adv_sdate,      
      const int& colo_id,      
      const int& ccg_id      
    )    
      :Base("CcgAuctionStatsDaily")    
    {    
      key_ = Key (      
        adv_sdate,        
        colo_id,        
        ccg_id        
      );      
    }    
    inline    
    CcgAuctionStatsDaily::Key::Key ()    
      :adv_sdate_(default_date()),colo_id_(0),ccg_id_(0)    
    {    
      adv_sdate_used_ = false;      
      adv_sdate_null_ = false;      
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      ccg_id_used_ = false;      
      ccg_id_null_ = false;      
    }    
    inline    
    CcgAuctionStatsDaily::Key::Key (    
      const AutoTest::Time& adv_sdate,      
      const int& colo_id,      
      const int& ccg_id      
    )    
      :adv_sdate_(adv_sdate),colo_id_(colo_id),ccg_id_(ccg_id)    
    {    
      adv_sdate_used_ = true;      
      adv_sdate_null_ = false;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      ccg_id_used_ = true;      
      ccg_id_null_ = false;      
    }    
    inline    
    CcgAuctionStatsDaily::Key&    
    CcgAuctionStatsDaily::Key::adv_sdate(const AutoTest::Time& value)    
    {    
      adv_sdate_ = value;      
      adv_sdate_used_ = true;      
      adv_sdate_null_ = false;      
      return *this;      
    }    
    inline    
    CcgAuctionStatsDaily::Key&    
    CcgAuctionStatsDaily::Key::adv_sdate_set_null(bool is_null)    
    {    
      adv_sdate_used_ = true;      
      adv_sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    CcgAuctionStatsDaily::Key::adv_sdate() const    
    {    
      return adv_sdate_;      
    }    
    inline    
    bool    
    CcgAuctionStatsDaily::Key::adv_sdate_used() const    
    {    
      return adv_sdate_used_;      
    }    
    inline    
    bool    
    CcgAuctionStatsDaily::Key::adv_sdate_is_null() const    
    {    
      return adv_sdate_null_;      
    }    
    inline    
    CcgAuctionStatsDaily::Key&    
    CcgAuctionStatsDaily::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    CcgAuctionStatsDaily::Key&    
    CcgAuctionStatsDaily::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CcgAuctionStatsDaily::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    CcgAuctionStatsDaily::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    CcgAuctionStatsDaily::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    CcgAuctionStatsDaily::Key&    
    CcgAuctionStatsDaily::Key::ccg_id(const int& value)    
    {    
      ccg_id_ = value;      
      ccg_id_used_ = true;      
      ccg_id_null_ = false;      
      return *this;      
    }    
    inline    
    CcgAuctionStatsDaily::Key&    
    CcgAuctionStatsDaily::Key::ccg_id_set_null(bool is_null)    
    {    
      ccg_id_used_ = true;      
      ccg_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    CcgAuctionStatsDaily::Key::ccg_id() const    
    {    
      return ccg_id_;      
    }    
    inline    
    bool    
    CcgAuctionStatsDaily::Key::ccg_id_used() const    
    {    
      return ccg_id_used_;      
    }    
    inline    
    bool    
    CcgAuctionStatsDaily::Key::ccg_id_is_null() const    
    {    
      return ccg_id_null_;      
    }    
    inline    
    CcgAuctionStatsDaily::Key&    
    CcgAuctionStatsDaily::key (    
      const AutoTest::Time& adv_sdate,      
      const int& colo_id,      
      const int& ccg_id      
    )    
    {    
      key_ = Key (      
        adv_sdate,        
        colo_id,        
        ccg_id        
      );      
      return key_;      
    }    
    inline    
    CcgAuctionStatsDaily::Key&    
    CcgAuctionStatsDaily::key ()    
    {    
      return key_;      
    }    
    inline    
    CcgAuctionStatsDaily::Key&    
    CcgAuctionStatsDaily::key (const CcgAuctionStatsDaily::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs&     
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs&     
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs&     
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs     
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<CcgAuctionStatsDaily, 1>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs     
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<CcgAuctionStatsDaily, 1>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs::const_iterator    
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs::const_iterator    
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs::end () const    
    {    
      return diffs + 1;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs::size () const    
    {    
      return 1;      
    }    
    inline    
    void    
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 1; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    CcgAuctionStatsDaily::auctions_lost () const    
    {    
      return values[CcgAuctionStatsDaily::AUCTIONS_LOST];      
    }    
    inline    
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs&     
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs::auctions_lost (const stats_diff_type& value)    
    {    
      diffs[CcgAuctionStatsDaily::AUCTIONS_LOST] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<CcgAuctionStatsDaily, 1>::Diffs::auctions_lost () const    
    {    
      return diffs[CcgAuctionStatsDaily::AUCTIONS_LOST];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_CCGAUCTIONSTATSDAILY_HPP

