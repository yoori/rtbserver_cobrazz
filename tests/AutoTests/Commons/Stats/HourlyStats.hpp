
#ifndef __AUTOTESTS_COMMONS_STATS_HOURLYSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_HOURLYSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class HourlyStats:    
      public DiffStats<HourlyStats, 9>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          IMPS = 0,          
          CLICKS,          
          ACTIONS,          
          REQUESTS,          
          PUB_AMOUNT,          
          PUB_COMM_AMOUNT,          
          ADV_AMOUNT,          
          ADV_COMM_AMOUNT,          
          ISP_AMOUNT          
        };        
        enum Table        
        {        
          RequestStatsHourly = 0,          
          RequestStatsHourlyTest          
        };        
        typedef DiffStats<HourlyStats, 9> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            int cc_id_;            
            bool cc_id_used_;            
            bool cc_id_null_;            
            int tag_id_;            
            bool tag_id_used_;            
            bool tag_id_null_;            
            int colo_id_;            
            bool colo_id_used_;            
            bool colo_id_null_;            
            int adv_account_id_;            
            bool adv_account_id_used_;            
            bool adv_account_id_null_;            
            int pub_account_id_;            
            bool pub_account_id_used_;            
            bool pub_account_id_null_;            
            int isp_account_id_;            
            bool isp_account_id_used_;            
            bool isp_account_id_null_;            
            int num_shown_;            
            bool num_shown_used_;            
            bool num_shown_null_;            
            bool fraud_correction_;            
            bool fraud_correction_used_;            
            bool fraud_correction_null_;            
            AutoTest::Time stimestamp_;            
            bool stimestamp_used_;            
            bool stimestamp_null_;            
          public:          
            Key& cc_id(const int& value);            
            Key& cc_id_set_null(bool is_null = true);            
            const int& cc_id() const;            
            bool cc_id_used() const;            
            bool cc_id_is_null() const;            
            Key& tag_id(const int& value);            
            Key& tag_id_set_null(bool is_null = true);            
            const int& tag_id() const;            
            bool tag_id_used() const;            
            bool tag_id_is_null() const;            
            Key& colo_id(const int& value);            
            Key& colo_id_set_null(bool is_null = true);            
            const int& colo_id() const;            
            bool colo_id_used() const;            
            bool colo_id_is_null() const;            
            Key& adv_account_id(const int& value);            
            Key& adv_account_id_set_null(bool is_null = true);            
            const int& adv_account_id() const;            
            bool adv_account_id_used() const;            
            bool adv_account_id_is_null() const;            
            Key& pub_account_id(const int& value);            
            Key& pub_account_id_set_null(bool is_null = true);            
            const int& pub_account_id() const;            
            bool pub_account_id_used() const;            
            bool pub_account_id_is_null() const;            
            Key& isp_account_id(const int& value);            
            Key& isp_account_id_set_null(bool is_null = true);            
            const int& isp_account_id() const;            
            bool isp_account_id_used() const;            
            bool isp_account_id_is_null() const;            
            Key& num_shown(const int& value);            
            Key& num_shown_set_null(bool is_null = true);            
            const int& num_shown() const;            
            bool num_shown_used() const;            
            bool num_shown_is_null() const;            
            Key& fraud_correction(const bool& value);            
            Key& fraud_correction_set_null(bool is_null = true);            
            const bool& fraud_correction() const;            
            bool fraud_correction_used() const;            
            bool fraud_correction_is_null() const;            
            Key& stimestamp(const AutoTest::Time& value);            
            Key& stimestamp_set_null(bool is_null = true);            
            const AutoTest::Time& stimestamp() const;            
            bool stimestamp_used() const;            
            bool stimestamp_is_null() const;            
            Key ();            
            Key (            
              const int& cc_id,              
              const int& tag_id,              
              const int& colo_id,              
              const int& adv_account_id,              
              const int& pub_account_id,              
              const int& isp_account_id,              
              const int& num_shown,              
              const bool& fraud_correction,              
              const AutoTest::Time& stimestamp              
            );            
        };        
        stats_value_type imps () const;        
        stats_value_type clicks () const;        
        stats_value_type actions () const;        
        stats_value_type requests () const;        
        stats_value_type pub_amount () const;        
        stats_value_type pub_comm_amount () const;        
        stats_value_type adv_amount () const;        
        stats_value_type adv_comm_amount () const;        
        stats_value_type isp_amount () const;        
        void print_idname (std::ostream& out) const;        
                
        HourlyStats (const Key& value);        
        HourlyStats (        
          const int& cc_id,          
          const int& tag_id,          
          const int& colo_id,          
          const int& adv_account_id,          
          const int& pub_account_id,          
          const int& isp_account_id,          
          const int& num_shown,          
          const bool& fraud_correction,          
          const AutoTest::Time& stimestamp          
        );        
        HourlyStats ();        
        HourlyStats& table(const Table&);        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& cc_id,          
          const int& tag_id,          
          const int& colo_id,          
          const int& adv_account_id,          
          const int& pub_account_id,          
          const int& isp_account_id,          
          const int& num_shown,          
          const bool& fraud_correction,          
          const AutoTest::Time& stimestamp          
        );        
                
      protected:      
        Key key_;        
        Table table_;        
        static const char* tables[2];        
    };    
        
    template<>    
    class DiffStats<HourlyStats, 9>::Diffs    
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
                
        Diffs& imps (const stats_diff_type& value);        
        const stats_diff_type& imps () const;        
        Diffs& clicks (const stats_diff_type& value);        
        const stats_diff_type& clicks () const;        
        Diffs& actions (const stats_diff_type& value);        
        const stats_diff_type& actions () const;        
        Diffs& requests (const stats_diff_type& value);        
        const stats_diff_type& requests () const;        
        Diffs& pub_amount (const stats_diff_type& value);        
        const stats_diff_type& pub_amount () const;        
        Diffs& pub_comm_amount (const stats_diff_type& value);        
        const stats_diff_type& pub_comm_amount () const;        
        Diffs& adv_amount (const stats_diff_type& value);        
        const stats_diff_type& adv_amount () const;        
        Diffs& adv_comm_amount (const stats_diff_type& value);        
        const stats_diff_type& adv_comm_amount () const;        
        Diffs& isp_amount (const stats_diff_type& value);        
        const stats_diff_type& isp_amount () const;        
      protected:      
        stats_diff_type diffs[9];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// HourlyStats    
    inline    
    HourlyStats::HourlyStats ()    
      :Base("HourlyStats")    
    { table_ = RequestStatsHourly; }    
    inline    
    HourlyStats::HourlyStats (const HourlyStats::Key& value)    
      :Base("HourlyStats")    
    {    
      table_ = RequestStatsHourly;      
      key_ = value;      
    }    
    inline    
    HourlyStats::HourlyStats (    
      const int& cc_id,      
      const int& tag_id,      
      const int& colo_id,      
      const int& adv_account_id,      
      const int& pub_account_id,      
      const int& isp_account_id,      
      const int& num_shown,      
      const bool& fraud_correction,      
      const AutoTest::Time& stimestamp      
    )    
      :Base("HourlyStats")    
    {    
      table_ = RequestStatsHourly;      
      key_ = Key (      
        cc_id,        
        tag_id,        
        colo_id,        
        adv_account_id,        
        pub_account_id,        
        isp_account_id,        
        num_shown,        
        fraud_correction,        
        stimestamp        
      );      
    }    
    inline    
    HourlyStats&    
    HourlyStats::table(const Table& value)    
    {    
      table_ = value;      
      return *this;      
    }    
    inline    
    HourlyStats::Key::Key ()    
      :cc_id_(0),tag_id_(0),colo_id_(0),adv_account_id_(0),pub_account_id_(0),isp_account_id_(0),num_shown_(0),fraud_correction_(false),stimestamp_(default_date())    
    {    
      cc_id_used_ = false;      
      cc_id_null_ = false;      
      tag_id_used_ = false;      
      tag_id_null_ = false;      
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      adv_account_id_used_ = false;      
      adv_account_id_null_ = false;      
      pub_account_id_used_ = false;      
      pub_account_id_null_ = false;      
      isp_account_id_used_ = false;      
      isp_account_id_null_ = false;      
      num_shown_used_ = false;      
      num_shown_null_ = false;      
      fraud_correction_used_ = false;      
      fraud_correction_null_ = false;      
      stimestamp_used_ = false;      
      stimestamp_null_ = false;      
    }    
    inline    
    HourlyStats::Key::Key (    
      const int& cc_id,      
      const int& tag_id,      
      const int& colo_id,      
      const int& adv_account_id,      
      const int& pub_account_id,      
      const int& isp_account_id,      
      const int& num_shown,      
      const bool& fraud_correction,      
      const AutoTest::Time& stimestamp      
    )    
      :cc_id_(cc_id),tag_id_(tag_id),colo_id_(colo_id),adv_account_id_(adv_account_id),pub_account_id_(pub_account_id),isp_account_id_(isp_account_id),num_shown_(num_shown),fraud_correction_(fraud_correction),stimestamp_(stimestamp)    
    {    
      cc_id_used_ = true;      
      cc_id_null_ = false;      
      tag_id_used_ = true;      
      tag_id_null_ = false;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      adv_account_id_used_ = true;      
      adv_account_id_null_ = false;      
      pub_account_id_used_ = true;      
      pub_account_id_null_ = false;      
      isp_account_id_used_ = true;      
      isp_account_id_null_ = false;      
      num_shown_used_ = true;      
      num_shown_null_ = false;      
      fraud_correction_used_ = true;      
      fraud_correction_null_ = false;      
      stimestamp_used_ = true;      
      stimestamp_null_ = false;      
    }    
    inline    
    HourlyStats::Key&    
    HourlyStats::Key::cc_id(const int& value)    
    {    
      cc_id_ = value;      
      cc_id_used_ = true;      
      cc_id_null_ = false;      
      return *this;      
    }    
    inline    
    HourlyStats::Key&    
    HourlyStats::Key::cc_id_set_null(bool is_null)    
    {    
      cc_id_used_ = true;      
      cc_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    HourlyStats::Key::cc_id() const    
    {    
      return cc_id_;      
    }    
    inline    
    bool    
    HourlyStats::Key::cc_id_used() const    
    {    
      return cc_id_used_;      
    }    
    inline    
    bool    
    HourlyStats::Key::cc_id_is_null() const    
    {    
      return cc_id_null_;      
    }    
    inline    
    HourlyStats::Key&    
    HourlyStats::Key::tag_id(const int& value)    
    {    
      tag_id_ = value;      
      tag_id_used_ = true;      
      tag_id_null_ = false;      
      return *this;      
    }    
    inline    
    HourlyStats::Key&    
    HourlyStats::Key::tag_id_set_null(bool is_null)    
    {    
      tag_id_used_ = true;      
      tag_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    HourlyStats::Key::tag_id() const    
    {    
      return tag_id_;      
    }    
    inline    
    bool    
    HourlyStats::Key::tag_id_used() const    
    {    
      return tag_id_used_;      
    }    
    inline    
    bool    
    HourlyStats::Key::tag_id_is_null() const    
    {    
      return tag_id_null_;      
    }    
    inline    
    HourlyStats::Key&    
    HourlyStats::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    HourlyStats::Key&    
    HourlyStats::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    HourlyStats::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    HourlyStats::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    HourlyStats::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    HourlyStats::Key&    
    HourlyStats::Key::adv_account_id(const int& value)    
    {    
      adv_account_id_ = value;      
      adv_account_id_used_ = true;      
      adv_account_id_null_ = false;      
      return *this;      
    }    
    inline    
    HourlyStats::Key&    
    HourlyStats::Key::adv_account_id_set_null(bool is_null)    
    {    
      adv_account_id_used_ = true;      
      adv_account_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    HourlyStats::Key::adv_account_id() const    
    {    
      return adv_account_id_;      
    }    
    inline    
    bool    
    HourlyStats::Key::adv_account_id_used() const    
    {    
      return adv_account_id_used_;      
    }    
    inline    
    bool    
    HourlyStats::Key::adv_account_id_is_null() const    
    {    
      return adv_account_id_null_;      
    }    
    inline    
    HourlyStats::Key&    
    HourlyStats::Key::pub_account_id(const int& value)    
    {    
      pub_account_id_ = value;      
      pub_account_id_used_ = true;      
      pub_account_id_null_ = false;      
      return *this;      
    }    
    inline    
    HourlyStats::Key&    
    HourlyStats::Key::pub_account_id_set_null(bool is_null)    
    {    
      pub_account_id_used_ = true;      
      pub_account_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    HourlyStats::Key::pub_account_id() const    
    {    
      return pub_account_id_;      
    }    
    inline    
    bool    
    HourlyStats::Key::pub_account_id_used() const    
    {    
      return pub_account_id_used_;      
    }    
    inline    
    bool    
    HourlyStats::Key::pub_account_id_is_null() const    
    {    
      return pub_account_id_null_;      
    }    
    inline    
    HourlyStats::Key&    
    HourlyStats::Key::isp_account_id(const int& value)    
    {    
      isp_account_id_ = value;      
      isp_account_id_used_ = true;      
      isp_account_id_null_ = false;      
      return *this;      
    }    
    inline    
    HourlyStats::Key&    
    HourlyStats::Key::isp_account_id_set_null(bool is_null)    
    {    
      isp_account_id_used_ = true;      
      isp_account_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    HourlyStats::Key::isp_account_id() const    
    {    
      return isp_account_id_;      
    }    
    inline    
    bool    
    HourlyStats::Key::isp_account_id_used() const    
    {    
      return isp_account_id_used_;      
    }    
    inline    
    bool    
    HourlyStats::Key::isp_account_id_is_null() const    
    {    
      return isp_account_id_null_;      
    }    
    inline    
    HourlyStats::Key&    
    HourlyStats::Key::num_shown(const int& value)    
    {    
      num_shown_ = value;      
      num_shown_used_ = true;      
      num_shown_null_ = false;      
      return *this;      
    }    
    inline    
    HourlyStats::Key&    
    HourlyStats::Key::num_shown_set_null(bool is_null)    
    {    
      num_shown_used_ = true;      
      num_shown_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    HourlyStats::Key::num_shown() const    
    {    
      return num_shown_;      
    }    
    inline    
    bool    
    HourlyStats::Key::num_shown_used() const    
    {    
      return num_shown_used_;      
    }    
    inline    
    bool    
    HourlyStats::Key::num_shown_is_null() const    
    {    
      return num_shown_null_;      
    }    
    inline    
    HourlyStats::Key&    
    HourlyStats::Key::fraud_correction(const bool& value)    
    {    
      fraud_correction_ = value;      
      fraud_correction_used_ = true;      
      fraud_correction_null_ = false;      
      return *this;      
    }    
    inline    
    HourlyStats::Key&    
    HourlyStats::Key::fraud_correction_set_null(bool is_null)    
    {    
      fraud_correction_used_ = true;      
      fraud_correction_null_ = is_null;      
      return *this;      
    }    
    inline    
    const bool&    
    HourlyStats::Key::fraud_correction() const    
    {    
      return fraud_correction_;      
    }    
    inline    
    bool    
    HourlyStats::Key::fraud_correction_used() const    
    {    
      return fraud_correction_used_;      
    }    
    inline    
    bool    
    HourlyStats::Key::fraud_correction_is_null() const    
    {    
      return fraud_correction_null_;      
    }    
    inline    
    HourlyStats::Key&    
    HourlyStats::Key::stimestamp(const AutoTest::Time& value)    
    {    
      stimestamp_ = value;      
      stimestamp_used_ = true;      
      stimestamp_null_ = false;      
      return *this;      
    }    
    inline    
    HourlyStats::Key&    
    HourlyStats::Key::stimestamp_set_null(bool is_null)    
    {    
      stimestamp_used_ = true;      
      stimestamp_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    HourlyStats::Key::stimestamp() const    
    {    
      return stimestamp_;      
    }    
    inline    
    bool    
    HourlyStats::Key::stimestamp_used() const    
    {    
      return stimestamp_used_;      
    }    
    inline    
    bool    
    HourlyStats::Key::stimestamp_is_null() const    
    {    
      return stimestamp_null_;      
    }    
    inline    
    HourlyStats::Key&    
    HourlyStats::key (    
      const int& cc_id,      
      const int& tag_id,      
      const int& colo_id,      
      const int& adv_account_id,      
      const int& pub_account_id,      
      const int& isp_account_id,      
      const int& num_shown,      
      const bool& fraud_correction,      
      const AutoTest::Time& stimestamp      
    )    
    {    
      key_ = Key (      
        cc_id,        
        tag_id,        
        colo_id,        
        adv_account_id,        
        pub_account_id,        
        isp_account_id,        
        num_shown,        
        fraud_correction,        
        stimestamp        
      );      
      return key_;      
    }    
    inline    
    HourlyStats::Key&    
    HourlyStats::key ()    
    {    
      return key_;      
    }    
    inline    
    HourlyStats::Key&    
    HourlyStats::key (const HourlyStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<HourlyStats, 9>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 9; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<HourlyStats, 9>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 9; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<HourlyStats, 9>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 9; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<HourlyStats, 9>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<HourlyStats, 9>::Diffs&     
    DiffStats<HourlyStats, 9>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 9; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<HourlyStats, 9>::Diffs&     
    DiffStats<HourlyStats, 9>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 9; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<HourlyStats, 9>::Diffs&     
    DiffStats<HourlyStats, 9>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 9; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<HourlyStats, 9>::Diffs     
    DiffStats<HourlyStats, 9>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<HourlyStats, 9>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<HourlyStats, 9>::Diffs     
    DiffStats<HourlyStats, 9>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<HourlyStats, 9>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<HourlyStats, 9>::Diffs::const_iterator    
    DiffStats<HourlyStats, 9>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<HourlyStats, 9>::Diffs::const_iterator    
    DiffStats<HourlyStats, 9>::Diffs::end () const    
    {    
      return diffs + 9;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<HourlyStats, 9>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<HourlyStats, 9>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<HourlyStats, 9>::Diffs::size () const    
    {    
      return 9;      
    }    
    inline    
    void    
    DiffStats<HourlyStats, 9>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 9; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    HourlyStats::imps () const    
    {    
      return values[HourlyStats::IMPS];      
    }    
    inline    
    DiffStats<HourlyStats, 9>::Diffs&     
    DiffStats<HourlyStats, 9>::Diffs::imps (const stats_diff_type& value)    
    {    
      diffs[HourlyStats::IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<HourlyStats, 9>::Diffs::imps () const    
    {    
      return diffs[HourlyStats::IMPS];      
    }    
    inline    
    stats_value_type    
    HourlyStats::clicks () const    
    {    
      return values[HourlyStats::CLICKS];      
    }    
    inline    
    DiffStats<HourlyStats, 9>::Diffs&     
    DiffStats<HourlyStats, 9>::Diffs::clicks (const stats_diff_type& value)    
    {    
      diffs[HourlyStats::CLICKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<HourlyStats, 9>::Diffs::clicks () const    
    {    
      return diffs[HourlyStats::CLICKS];      
    }    
    inline    
    stats_value_type    
    HourlyStats::actions () const    
    {    
      return values[HourlyStats::ACTIONS];      
    }    
    inline    
    DiffStats<HourlyStats, 9>::Diffs&     
    DiffStats<HourlyStats, 9>::Diffs::actions (const stats_diff_type& value)    
    {    
      diffs[HourlyStats::ACTIONS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<HourlyStats, 9>::Diffs::actions () const    
    {    
      return diffs[HourlyStats::ACTIONS];      
    }    
    inline    
    stats_value_type    
    HourlyStats::requests () const    
    {    
      return values[HourlyStats::REQUESTS];      
    }    
    inline    
    DiffStats<HourlyStats, 9>::Diffs&     
    DiffStats<HourlyStats, 9>::Diffs::requests (const stats_diff_type& value)    
    {    
      diffs[HourlyStats::REQUESTS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<HourlyStats, 9>::Diffs::requests () const    
    {    
      return diffs[HourlyStats::REQUESTS];      
    }    
    inline    
    stats_value_type    
    HourlyStats::pub_amount () const    
    {    
      return values[HourlyStats::PUB_AMOUNT];      
    }    
    inline    
    DiffStats<HourlyStats, 9>::Diffs&     
    DiffStats<HourlyStats, 9>::Diffs::pub_amount (const stats_diff_type& value)    
    {    
      diffs[HourlyStats::PUB_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<HourlyStats, 9>::Diffs::pub_amount () const    
    {    
      return diffs[HourlyStats::PUB_AMOUNT];      
    }    
    inline    
    stats_value_type    
    HourlyStats::pub_comm_amount () const    
    {    
      return values[HourlyStats::PUB_COMM_AMOUNT];      
    }    
    inline    
    DiffStats<HourlyStats, 9>::Diffs&     
    DiffStats<HourlyStats, 9>::Diffs::pub_comm_amount (const stats_diff_type& value)    
    {    
      diffs[HourlyStats::PUB_COMM_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<HourlyStats, 9>::Diffs::pub_comm_amount () const    
    {    
      return diffs[HourlyStats::PUB_COMM_AMOUNT];      
    }    
    inline    
    stats_value_type    
    HourlyStats::adv_amount () const    
    {    
      return values[HourlyStats::ADV_AMOUNT];      
    }    
    inline    
    DiffStats<HourlyStats, 9>::Diffs&     
    DiffStats<HourlyStats, 9>::Diffs::adv_amount (const stats_diff_type& value)    
    {    
      diffs[HourlyStats::ADV_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<HourlyStats, 9>::Diffs::adv_amount () const    
    {    
      return diffs[HourlyStats::ADV_AMOUNT];      
    }    
    inline    
    stats_value_type    
    HourlyStats::adv_comm_amount () const    
    {    
      return values[HourlyStats::ADV_COMM_AMOUNT];      
    }    
    inline    
    DiffStats<HourlyStats, 9>::Diffs&     
    DiffStats<HourlyStats, 9>::Diffs::adv_comm_amount (const stats_diff_type& value)    
    {    
      diffs[HourlyStats::ADV_COMM_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<HourlyStats, 9>::Diffs::adv_comm_amount () const    
    {    
      return diffs[HourlyStats::ADV_COMM_AMOUNT];      
    }    
    inline    
    stats_value_type    
    HourlyStats::isp_amount () const    
    {    
      return values[HourlyStats::ISP_AMOUNT];      
    }    
    inline    
    DiffStats<HourlyStats, 9>::Diffs&     
    DiffStats<HourlyStats, 9>::Diffs::isp_amount (const stats_diff_type& value)    
    {    
      diffs[HourlyStats::ISP_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<HourlyStats, 9>::Diffs::isp_amount () const    
    {    
      return diffs[HourlyStats::ISP_AMOUNT];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_HOURLYSTATS_HPP

