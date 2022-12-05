
#ifndef __AUTOTESTS_COMMONS_STATS_PUBLISHERSTATSDAILY_HPP
#define __AUTOTESTS_COMMONS_STATS_PUBLISHERSTATSDAILY_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class PublisherStatsDaily:    
      public DiffStats<PublisherStatsDaily, 7>    
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
          PUB_AMOUNT,          
          PUB_COMM_AMOUNT,          
          PUB_CREDITED_IMPS          
        };        
        typedef DiffStats<PublisherStatsDaily, 7> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            AutoTest::Time pub_sdate_;            
            bool pub_sdate_used_;            
            bool pub_sdate_null_;            
            int pub_account_id_;            
            bool pub_account_id_used_;            
            bool pub_account_id_null_;            
            int site_id_;            
            bool site_id_used_;            
            bool site_id_null_;            
            int size_id_;            
            bool size_id_used_;            
            bool size_id_null_;            
            int tag_id_;            
            bool tag_id_used_;            
            bool tag_id_null_;            
            int num_shown_;            
            bool num_shown_used_;            
            bool num_shown_null_;            
            std::string country_code_;            
            bool country_code_used_;            
            bool country_code_null_;            
            bool walled_garden_;            
            bool walled_garden_used_;            
            bool walled_garden_null_;            
          public:          
            Key& pub_sdate(const AutoTest::Time& value);            
            Key& pub_sdate_set_null(bool is_null = true);            
            const AutoTest::Time& pub_sdate() const;            
            bool pub_sdate_used() const;            
            bool pub_sdate_is_null() const;            
            Key& pub_account_id(const int& value);            
            Key& pub_account_id_set_null(bool is_null = true);            
            const int& pub_account_id() const;            
            bool pub_account_id_used() const;            
            bool pub_account_id_is_null() const;            
            Key& site_id(const int& value);            
            Key& site_id_set_null(bool is_null = true);            
            const int& site_id() const;            
            bool site_id_used() const;            
            bool site_id_is_null() const;            
            Key& size_id(const int& value);            
            Key& size_id_set_null(bool is_null = true);            
            const int& size_id() const;            
            bool size_id_used() const;            
            bool size_id_is_null() const;            
            Key& tag_id(const int& value);            
            Key& tag_id_set_null(bool is_null = true);            
            const int& tag_id() const;            
            bool tag_id_used() const;            
            bool tag_id_is_null() const;            
            Key& num_shown(const int& value);            
            Key& num_shown_set_null(bool is_null = true);            
            const int& num_shown() const;            
            bool num_shown_used() const;            
            bool num_shown_is_null() const;            
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
              const AutoTest::Time& pub_sdate,              
              const int& pub_account_id,              
              const int& site_id,              
              const int& size_id,              
              const int& tag_id,              
              const int& num_shown,              
              const std::string& country_code,              
              const bool& walled_garden              
            );            
        };        
        stats_value_type requests () const;        
        stats_value_type imps () const;        
        stats_value_type clicks () const;        
        stats_value_type actions () const;        
        stats_value_type pub_amount () const;        
        stats_value_type pub_comm_amount () const;        
        stats_value_type pub_credited_imps () const;        
        void print_idname (std::ostream& out) const;        
                
        PublisherStatsDaily (const Key& value);        
        PublisherStatsDaily (        
          const AutoTest::Time& pub_sdate,          
          const int& pub_account_id,          
          const int& site_id,          
          const int& size_id,          
          const int& tag_id,          
          const int& num_shown,          
          const std::string& country_code,          
          const bool& walled_garden          
        );        
        PublisherStatsDaily ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const AutoTest::Time& pub_sdate,          
          const int& pub_account_id,          
          const int& site_id,          
          const int& size_id,          
          const int& tag_id,          
          const int& num_shown,          
          const std::string& country_code,          
          const bool& walled_garden          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<PublisherStatsDaily, 7>::Diffs    
    {    
      public:      
        typedef stats_diff_type array_type[7];        
        typedef const stats_diff_type const_array_type[7];        
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
        Diffs& pub_amount (const stats_diff_type& value);        
        const stats_diff_type& pub_amount () const;        
        Diffs& pub_comm_amount (const stats_diff_type& value);        
        const stats_diff_type& pub_comm_amount () const;        
        Diffs& pub_credited_imps (const stats_diff_type& value);        
        const stats_diff_type& pub_credited_imps () const;        
      protected:      
        stats_diff_type diffs[7];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// PublisherStatsDaily    
    inline    
    PublisherStatsDaily::PublisherStatsDaily ()    
      :Base("PublisherStatsDaily")    
    {}    
    inline    
    PublisherStatsDaily::PublisherStatsDaily (const PublisherStatsDaily::Key& value)    
      :Base("PublisherStatsDaily")    
    {    
      key_ = value;      
    }    
    inline    
    PublisherStatsDaily::PublisherStatsDaily (    
      const AutoTest::Time& pub_sdate,      
      const int& pub_account_id,      
      const int& site_id,      
      const int& size_id,      
      const int& tag_id,      
      const int& num_shown,      
      const std::string& country_code,      
      const bool& walled_garden      
    )    
      :Base("PublisherStatsDaily")    
    {    
      key_ = Key (      
        pub_sdate,        
        pub_account_id,        
        site_id,        
        size_id,        
        tag_id,        
        num_shown,        
        country_code,        
        walled_garden        
      );      
    }    
    inline    
    PublisherStatsDaily::Key::Key ()    
      :pub_sdate_(default_date()),pub_account_id_(0),site_id_(0),size_id_(0),tag_id_(0),num_shown_(0),walled_garden_(false)    
    {    
      pub_sdate_used_ = false;      
      pub_sdate_null_ = false;      
      pub_account_id_used_ = false;      
      pub_account_id_null_ = false;      
      site_id_used_ = false;      
      site_id_null_ = false;      
      size_id_used_ = false;      
      size_id_null_ = false;      
      tag_id_used_ = false;      
      tag_id_null_ = false;      
      num_shown_used_ = false;      
      num_shown_null_ = false;      
      country_code_used_ = false;      
      country_code_null_ = false;      
      walled_garden_used_ = false;      
      walled_garden_null_ = false;      
    }    
    inline    
    PublisherStatsDaily::Key::Key (    
      const AutoTest::Time& pub_sdate,      
      const int& pub_account_id,      
      const int& site_id,      
      const int& size_id,      
      const int& tag_id,      
      const int& num_shown,      
      const std::string& country_code,      
      const bool& walled_garden      
    )    
      :pub_sdate_(pub_sdate),pub_account_id_(pub_account_id),site_id_(site_id),size_id_(size_id),tag_id_(tag_id),num_shown_(num_shown),walled_garden_(walled_garden)    
    {    
      pub_sdate_used_ = true;      
      pub_sdate_null_ = false;      
      pub_account_id_used_ = true;      
      pub_account_id_null_ = false;      
      site_id_used_ = true;      
      site_id_null_ = false;      
      size_id_used_ = true;      
      size_id_null_ = false;      
      tag_id_used_ = true;      
      tag_id_null_ = false;      
      num_shown_used_ = true;      
      num_shown_null_ = false;      
      country_code_ = country_code;      
      country_code_used_ = true;      
      country_code_null_ = false;      
      walled_garden_used_ = true;      
      walled_garden_null_ = false;      
    }    
    inline    
    PublisherStatsDaily::Key&    
    PublisherStatsDaily::Key::pub_sdate(const AutoTest::Time& value)    
    {    
      pub_sdate_ = value;      
      pub_sdate_used_ = true;      
      pub_sdate_null_ = false;      
      return *this;      
    }    
    inline    
    PublisherStatsDaily::Key&    
    PublisherStatsDaily::Key::pub_sdate_set_null(bool is_null)    
    {    
      pub_sdate_used_ = true;      
      pub_sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    PublisherStatsDaily::Key::pub_sdate() const    
    {    
      return pub_sdate_;      
    }    
    inline    
    bool    
    PublisherStatsDaily::Key::pub_sdate_used() const    
    {    
      return pub_sdate_used_;      
    }    
    inline    
    bool    
    PublisherStatsDaily::Key::pub_sdate_is_null() const    
    {    
      return pub_sdate_null_;      
    }    
    inline    
    PublisherStatsDaily::Key&    
    PublisherStatsDaily::Key::pub_account_id(const int& value)    
    {    
      pub_account_id_ = value;      
      pub_account_id_used_ = true;      
      pub_account_id_null_ = false;      
      return *this;      
    }    
    inline    
    PublisherStatsDaily::Key&    
    PublisherStatsDaily::Key::pub_account_id_set_null(bool is_null)    
    {    
      pub_account_id_used_ = true;      
      pub_account_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    PublisherStatsDaily::Key::pub_account_id() const    
    {    
      return pub_account_id_;      
    }    
    inline    
    bool    
    PublisherStatsDaily::Key::pub_account_id_used() const    
    {    
      return pub_account_id_used_;      
    }    
    inline    
    bool    
    PublisherStatsDaily::Key::pub_account_id_is_null() const    
    {    
      return pub_account_id_null_;      
    }    
    inline    
    PublisherStatsDaily::Key&    
    PublisherStatsDaily::Key::site_id(const int& value)    
    {    
      site_id_ = value;      
      site_id_used_ = true;      
      site_id_null_ = false;      
      return *this;      
    }    
    inline    
    PublisherStatsDaily::Key&    
    PublisherStatsDaily::Key::site_id_set_null(bool is_null)    
    {    
      site_id_used_ = true;      
      site_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    PublisherStatsDaily::Key::site_id() const    
    {    
      return site_id_;      
    }    
    inline    
    bool    
    PublisherStatsDaily::Key::site_id_used() const    
    {    
      return site_id_used_;      
    }    
    inline    
    bool    
    PublisherStatsDaily::Key::site_id_is_null() const    
    {    
      return site_id_null_;      
    }    
    inline    
    PublisherStatsDaily::Key&    
    PublisherStatsDaily::Key::size_id(const int& value)    
    {    
      size_id_ = value;      
      size_id_used_ = true;      
      size_id_null_ = false;      
      return *this;      
    }    
    inline    
    PublisherStatsDaily::Key&    
    PublisherStatsDaily::Key::size_id_set_null(bool is_null)    
    {    
      size_id_used_ = true;      
      size_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    PublisherStatsDaily::Key::size_id() const    
    {    
      return size_id_;      
    }    
    inline    
    bool    
    PublisherStatsDaily::Key::size_id_used() const    
    {    
      return size_id_used_;      
    }    
    inline    
    bool    
    PublisherStatsDaily::Key::size_id_is_null() const    
    {    
      return size_id_null_;      
    }    
    inline    
    PublisherStatsDaily::Key&    
    PublisherStatsDaily::Key::tag_id(const int& value)    
    {    
      tag_id_ = value;      
      tag_id_used_ = true;      
      tag_id_null_ = false;      
      return *this;      
    }    
    inline    
    PublisherStatsDaily::Key&    
    PublisherStatsDaily::Key::tag_id_set_null(bool is_null)    
    {    
      tag_id_used_ = true;      
      tag_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    PublisherStatsDaily::Key::tag_id() const    
    {    
      return tag_id_;      
    }    
    inline    
    bool    
    PublisherStatsDaily::Key::tag_id_used() const    
    {    
      return tag_id_used_;      
    }    
    inline    
    bool    
    PublisherStatsDaily::Key::tag_id_is_null() const    
    {    
      return tag_id_null_;      
    }    
    inline    
    PublisherStatsDaily::Key&    
    PublisherStatsDaily::Key::num_shown(const int& value)    
    {    
      num_shown_ = value;      
      num_shown_used_ = true;      
      num_shown_null_ = false;      
      return *this;      
    }    
    inline    
    PublisherStatsDaily::Key&    
    PublisherStatsDaily::Key::num_shown_set_null(bool is_null)    
    {    
      num_shown_used_ = true;      
      num_shown_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    PublisherStatsDaily::Key::num_shown() const    
    {    
      return num_shown_;      
    }    
    inline    
    bool    
    PublisherStatsDaily::Key::num_shown_used() const    
    {    
      return num_shown_used_;      
    }    
    inline    
    bool    
    PublisherStatsDaily::Key::num_shown_is_null() const    
    {    
      return num_shown_null_;      
    }    
    inline    
    PublisherStatsDaily::Key&    
    PublisherStatsDaily::Key::country_code(const std::string& value)    
    {    
      country_code_ = value;      
      country_code_used_ = true;      
      country_code_null_ = false;      
      return *this;      
    }    
    inline    
    PublisherStatsDaily::Key&    
    PublisherStatsDaily::Key::country_code_set_null(bool is_null)    
    {    
      country_code_used_ = true;      
      country_code_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    PublisherStatsDaily::Key::country_code() const    
    {    
      return country_code_;      
    }    
    inline    
    bool    
    PublisherStatsDaily::Key::country_code_used() const    
    {    
      return country_code_used_;      
    }    
    inline    
    bool    
    PublisherStatsDaily::Key::country_code_is_null() const    
    {    
      return country_code_null_;      
    }    
    inline    
    PublisherStatsDaily::Key&    
    PublisherStatsDaily::Key::walled_garden(const bool& value)    
    {    
      walled_garden_ = value;      
      walled_garden_used_ = true;      
      walled_garden_null_ = false;      
      return *this;      
    }    
    inline    
    PublisherStatsDaily::Key&    
    PublisherStatsDaily::Key::walled_garden_set_null(bool is_null)    
    {    
      walled_garden_used_ = true;      
      walled_garden_null_ = is_null;      
      return *this;      
    }    
    inline    
    const bool&    
    PublisherStatsDaily::Key::walled_garden() const    
    {    
      return walled_garden_;      
    }    
    inline    
    bool    
    PublisherStatsDaily::Key::walled_garden_used() const    
    {    
      return walled_garden_used_;      
    }    
    inline    
    bool    
    PublisherStatsDaily::Key::walled_garden_is_null() const    
    {    
      return walled_garden_null_;      
    }    
    inline    
    PublisherStatsDaily::Key&    
    PublisherStatsDaily::key (    
      const AutoTest::Time& pub_sdate,      
      const int& pub_account_id,      
      const int& site_id,      
      const int& size_id,      
      const int& tag_id,      
      const int& num_shown,      
      const std::string& country_code,      
      const bool& walled_garden      
    )    
    {    
      key_ = Key (      
        pub_sdate,        
        pub_account_id,        
        site_id,        
        size_id,        
        tag_id,        
        num_shown,        
        country_code,        
        walled_garden        
      );      
      return key_;      
    }    
    inline    
    PublisherStatsDaily::Key&    
    PublisherStatsDaily::key ()    
    {    
      return key_;      
    }    
    inline    
    PublisherStatsDaily::Key&    
    PublisherStatsDaily::key (const PublisherStatsDaily::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<PublisherStatsDaily, 7>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 7; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<PublisherStatsDaily, 7>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 7; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<PublisherStatsDaily, 7>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 7; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<PublisherStatsDaily, 7>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<PublisherStatsDaily, 7>::Diffs&     
    DiffStats<PublisherStatsDaily, 7>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 7; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<PublisherStatsDaily, 7>::Diffs&     
    DiffStats<PublisherStatsDaily, 7>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 7; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<PublisherStatsDaily, 7>::Diffs&     
    DiffStats<PublisherStatsDaily, 7>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 7; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<PublisherStatsDaily, 7>::Diffs     
    DiffStats<PublisherStatsDaily, 7>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<PublisherStatsDaily, 7>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<PublisherStatsDaily, 7>::Diffs     
    DiffStats<PublisherStatsDaily, 7>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<PublisherStatsDaily, 7>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<PublisherStatsDaily, 7>::Diffs::const_iterator    
    DiffStats<PublisherStatsDaily, 7>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<PublisherStatsDaily, 7>::Diffs::const_iterator    
    DiffStats<PublisherStatsDaily, 7>::Diffs::end () const    
    {    
      return diffs + 7;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<PublisherStatsDaily, 7>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<PublisherStatsDaily, 7>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<PublisherStatsDaily, 7>::Diffs::size () const    
    {    
      return 7;      
    }    
    inline    
    void    
    DiffStats<PublisherStatsDaily, 7>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 7; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    PublisherStatsDaily::requests () const    
    {    
      return values[PublisherStatsDaily::REQUESTS];      
    }    
    inline    
    DiffStats<PublisherStatsDaily, 7>::Diffs&     
    DiffStats<PublisherStatsDaily, 7>::Diffs::requests (const stats_diff_type& value)    
    {    
      diffs[PublisherStatsDaily::REQUESTS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<PublisherStatsDaily, 7>::Diffs::requests () const    
    {    
      return diffs[PublisherStatsDaily::REQUESTS];      
    }    
    inline    
    stats_value_type    
    PublisherStatsDaily::imps () const    
    {    
      return values[PublisherStatsDaily::IMPS];      
    }    
    inline    
    DiffStats<PublisherStatsDaily, 7>::Diffs&     
    DiffStats<PublisherStatsDaily, 7>::Diffs::imps (const stats_diff_type& value)    
    {    
      diffs[PublisherStatsDaily::IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<PublisherStatsDaily, 7>::Diffs::imps () const    
    {    
      return diffs[PublisherStatsDaily::IMPS];      
    }    
    inline    
    stats_value_type    
    PublisherStatsDaily::clicks () const    
    {    
      return values[PublisherStatsDaily::CLICKS];      
    }    
    inline    
    DiffStats<PublisherStatsDaily, 7>::Diffs&     
    DiffStats<PublisherStatsDaily, 7>::Diffs::clicks (const stats_diff_type& value)    
    {    
      diffs[PublisherStatsDaily::CLICKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<PublisherStatsDaily, 7>::Diffs::clicks () const    
    {    
      return diffs[PublisherStatsDaily::CLICKS];      
    }    
    inline    
    stats_value_type    
    PublisherStatsDaily::actions () const    
    {    
      return values[PublisherStatsDaily::ACTIONS];      
    }    
    inline    
    DiffStats<PublisherStatsDaily, 7>::Diffs&     
    DiffStats<PublisherStatsDaily, 7>::Diffs::actions (const stats_diff_type& value)    
    {    
      diffs[PublisherStatsDaily::ACTIONS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<PublisherStatsDaily, 7>::Diffs::actions () const    
    {    
      return diffs[PublisherStatsDaily::ACTIONS];      
    }    
    inline    
    stats_value_type    
    PublisherStatsDaily::pub_amount () const    
    {    
      return values[PublisherStatsDaily::PUB_AMOUNT];      
    }    
    inline    
    DiffStats<PublisherStatsDaily, 7>::Diffs&     
    DiffStats<PublisherStatsDaily, 7>::Diffs::pub_amount (const stats_diff_type& value)    
    {    
      diffs[PublisherStatsDaily::PUB_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<PublisherStatsDaily, 7>::Diffs::pub_amount () const    
    {    
      return diffs[PublisherStatsDaily::PUB_AMOUNT];      
    }    
    inline    
    stats_value_type    
    PublisherStatsDaily::pub_comm_amount () const    
    {    
      return values[PublisherStatsDaily::PUB_COMM_AMOUNT];      
    }    
    inline    
    DiffStats<PublisherStatsDaily, 7>::Diffs&     
    DiffStats<PublisherStatsDaily, 7>::Diffs::pub_comm_amount (const stats_diff_type& value)    
    {    
      diffs[PublisherStatsDaily::PUB_COMM_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<PublisherStatsDaily, 7>::Diffs::pub_comm_amount () const    
    {    
      return diffs[PublisherStatsDaily::PUB_COMM_AMOUNT];      
    }    
    inline    
    stats_value_type    
    PublisherStatsDaily::pub_credited_imps () const    
    {    
      return values[PublisherStatsDaily::PUB_CREDITED_IMPS];      
    }    
    inline    
    DiffStats<PublisherStatsDaily, 7>::Diffs&     
    DiffStats<PublisherStatsDaily, 7>::Diffs::pub_credited_imps (const stats_diff_type& value)    
    {    
      diffs[PublisherStatsDaily::PUB_CREDITED_IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<PublisherStatsDaily, 7>::Diffs::pub_credited_imps () const    
    {    
      return diffs[PublisherStatsDaily::PUB_CREDITED_IMPS];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_PUBLISHERSTATSDAILY_HPP

