
#ifndef __AUTOTESTS_COMMONS_STATS_PUBLISHERINVENTORY_HPP
#define __AUTOTESTS_COMMONS_STATS_PUBLISHERINVENTORY_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class PublisherInventory:    
      public DiffStats<PublisherInventory, 3>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          IMPS = 0,          
          REQUESTS,          
          REVENUE          
        };        
        typedef DiffStats<PublisherInventory, 3> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            AutoTest::Time pub_sdate_;            
            bool pub_sdate_used_;            
            bool pub_sdate_null_;            
            int tag_id_;            
            bool tag_id_used_;            
            bool tag_id_null_;            
            double cpm_;            
            bool cpm_used_;            
            bool cpm_null_;            
          public:          
            Key& pub_sdate(const AutoTest::Time& value);            
            Key& pub_sdate_set_null(bool is_null = true);            
            const AutoTest::Time& pub_sdate() const;            
            bool pub_sdate_used() const;            
            bool pub_sdate_is_null() const;            
            Key& tag_id(const int& value);            
            Key& tag_id_set_null(bool is_null = true);            
            const int& tag_id() const;            
            bool tag_id_used() const;            
            bool tag_id_is_null() const;            
            Key& cpm(const double& value);            
            Key& cpm_set_null(bool is_null = true);            
            const double& cpm() const;            
            bool cpm_used() const;            
            bool cpm_is_null() const;            
            Key ();            
            Key (            
              const AutoTest::Time& pub_sdate,              
              const int& tag_id,              
              const double& cpm              
            );            
        };        
        stats_value_type imps () const;        
        stats_value_type requests () const;        
        stats_value_type revenue () const;        
        void print_idname (std::ostream& out) const;        
                
        PublisherInventory (const Key& value);        
        PublisherInventory (        
          const AutoTest::Time& pub_sdate,          
          const int& tag_id,          
          const double& cpm          
        );        
        PublisherInventory ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const AutoTest::Time& pub_sdate,          
          const int& tag_id,          
          const double& cpm          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<PublisherInventory, 3>::Diffs    
    {    
      public:      
        typedef stats_diff_type array_type[3];        
        typedef const stats_diff_type const_array_type[3];        
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
        Diffs& requests (const stats_diff_type& value);        
        const stats_diff_type& requests () const;        
        Diffs& revenue (const stats_diff_type& value);        
        const stats_diff_type& revenue () const;        
      protected:      
        stats_diff_type diffs[3];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// PublisherInventory    
    inline    
    PublisherInventory::PublisherInventory ()    
      :Base("PublisherInventory")    
    {}    
    inline    
    PublisherInventory::PublisherInventory (const PublisherInventory::Key& value)    
      :Base("PublisherInventory")    
    {    
      key_ = value;      
    }    
    inline    
    PublisherInventory::PublisherInventory (    
      const AutoTest::Time& pub_sdate,      
      const int& tag_id,      
      const double& cpm      
    )    
      :Base("PublisherInventory")    
    {    
      key_ = Key (      
        pub_sdate,        
        tag_id,        
        cpm        
      );      
    }    
    inline    
    PublisherInventory::Key::Key ()    
      :pub_sdate_(default_date()),tag_id_(0),cpm_(0)    
    {    
      pub_sdate_used_ = false;      
      pub_sdate_null_ = false;      
      tag_id_used_ = false;      
      tag_id_null_ = false;      
      cpm_used_ = false;      
      cpm_null_ = false;      
    }    
    inline    
    PublisherInventory::Key::Key (    
      const AutoTest::Time& pub_sdate,      
      const int& tag_id,      
      const double& cpm      
    )    
      :pub_sdate_(pub_sdate),tag_id_(tag_id),cpm_(cpm)    
    {    
      pub_sdate_used_ = true;      
      pub_sdate_null_ = false;      
      tag_id_used_ = true;      
      tag_id_null_ = false;      
      cpm_used_ = true;      
      cpm_null_ = false;      
    }    
    inline    
    PublisherInventory::Key&    
    PublisherInventory::Key::pub_sdate(const AutoTest::Time& value)    
    {    
      pub_sdate_ = value;      
      pub_sdate_used_ = true;      
      pub_sdate_null_ = false;      
      return *this;      
    }    
    inline    
    PublisherInventory::Key&    
    PublisherInventory::Key::pub_sdate_set_null(bool is_null)    
    {    
      pub_sdate_used_ = true;      
      pub_sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    PublisherInventory::Key::pub_sdate() const    
    {    
      return pub_sdate_;      
    }    
    inline    
    bool    
    PublisherInventory::Key::pub_sdate_used() const    
    {    
      return pub_sdate_used_;      
    }    
    inline    
    bool    
    PublisherInventory::Key::pub_sdate_is_null() const    
    {    
      return pub_sdate_null_;      
    }    
    inline    
    PublisherInventory::Key&    
    PublisherInventory::Key::tag_id(const int& value)    
    {    
      tag_id_ = value;      
      tag_id_used_ = true;      
      tag_id_null_ = false;      
      return *this;      
    }    
    inline    
    PublisherInventory::Key&    
    PublisherInventory::Key::tag_id_set_null(bool is_null)    
    {    
      tag_id_used_ = true;      
      tag_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    PublisherInventory::Key::tag_id() const    
    {    
      return tag_id_;      
    }    
    inline    
    bool    
    PublisherInventory::Key::tag_id_used() const    
    {    
      return tag_id_used_;      
    }    
    inline    
    bool    
    PublisherInventory::Key::tag_id_is_null() const    
    {    
      return tag_id_null_;      
    }    
    inline    
    PublisherInventory::Key&    
    PublisherInventory::Key::cpm(const double& value)    
    {    
      cpm_ = value;      
      cpm_used_ = true;      
      cpm_null_ = false;      
      return *this;      
    }    
    inline    
    PublisherInventory::Key&    
    PublisherInventory::Key::cpm_set_null(bool is_null)    
    {    
      cpm_used_ = true;      
      cpm_null_ = is_null;      
      return *this;      
    }    
    inline    
    const double&    
    PublisherInventory::Key::cpm() const    
    {    
      return cpm_;      
    }    
    inline    
    bool    
    PublisherInventory::Key::cpm_used() const    
    {    
      return cpm_used_;      
    }    
    inline    
    bool    
    PublisherInventory::Key::cpm_is_null() const    
    {    
      return cpm_null_;      
    }    
    inline    
    PublisherInventory::Key&    
    PublisherInventory::key (    
      const AutoTest::Time& pub_sdate,      
      const int& tag_id,      
      const double& cpm      
    )    
    {    
      key_ = Key (      
        pub_sdate,        
        tag_id,        
        cpm        
      );      
      return key_;      
    }    
    inline    
    PublisherInventory::Key&    
    PublisherInventory::key ()    
    {    
      return key_;      
    }    
    inline    
    PublisherInventory::Key&    
    PublisherInventory::key (const PublisherInventory::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<PublisherInventory, 3>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<PublisherInventory, 3>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<PublisherInventory, 3>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<PublisherInventory, 3>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<PublisherInventory, 3>::Diffs&     
    DiffStats<PublisherInventory, 3>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<PublisherInventory, 3>::Diffs&     
    DiffStats<PublisherInventory, 3>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<PublisherInventory, 3>::Diffs&     
    DiffStats<PublisherInventory, 3>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<PublisherInventory, 3>::Diffs     
    DiffStats<PublisherInventory, 3>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<PublisherInventory, 3>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<PublisherInventory, 3>::Diffs     
    DiffStats<PublisherInventory, 3>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<PublisherInventory, 3>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<PublisherInventory, 3>::Diffs::const_iterator    
    DiffStats<PublisherInventory, 3>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<PublisherInventory, 3>::Diffs::const_iterator    
    DiffStats<PublisherInventory, 3>::Diffs::end () const    
    {    
      return diffs + 3;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<PublisherInventory, 3>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<PublisherInventory, 3>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<PublisherInventory, 3>::Diffs::size () const    
    {    
      return 3;      
    }    
    inline    
    void    
    DiffStats<PublisherInventory, 3>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 3; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    PublisherInventory::imps () const    
    {    
      return values[PublisherInventory::IMPS];      
    }    
    inline    
    DiffStats<PublisherInventory, 3>::Diffs&     
    DiffStats<PublisherInventory, 3>::Diffs::imps (const stats_diff_type& value)    
    {    
      diffs[PublisherInventory::IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<PublisherInventory, 3>::Diffs::imps () const    
    {    
      return diffs[PublisherInventory::IMPS];      
    }    
    inline    
    stats_value_type    
    PublisherInventory::requests () const    
    {    
      return values[PublisherInventory::REQUESTS];      
    }    
    inline    
    DiffStats<PublisherInventory, 3>::Diffs&     
    DiffStats<PublisherInventory, 3>::Diffs::requests (const stats_diff_type& value)    
    {    
      diffs[PublisherInventory::REQUESTS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<PublisherInventory, 3>::Diffs::requests () const    
    {    
      return diffs[PublisherInventory::REQUESTS];      
    }    
    inline    
    stats_value_type    
    PublisherInventory::revenue () const    
    {    
      return values[PublisherInventory::REVENUE];      
    }    
    inline    
    DiffStats<PublisherInventory, 3>::Diffs&     
    DiffStats<PublisherInventory, 3>::Diffs::revenue (const stats_diff_type& value)    
    {    
      diffs[PublisherInventory::REVENUE] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<PublisherInventory, 3>::Diffs::revenue () const    
    {    
      return diffs[PublisherInventory::REVENUE];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_PUBLISHERINVENTORY_HPP

