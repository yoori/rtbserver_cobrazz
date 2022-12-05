
#ifndef __AUTOTESTS_COMMONS_STATS_OPTOUTSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_OPTOUTSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class OptOutStats:    
      public DiffStats<OptOutStats, 1>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          COUNT          
        };        
        typedef DiffStats<OptOutStats, 1> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            AutoTest::Time isp_sdate_;            
            bool isp_sdate_used_;            
            bool isp_sdate_null_;            
            int colo_id_;            
            bool colo_id_used_;            
            bool colo_id_null_;            
            std::string referer_;            
            bool referer_used_;            
            bool referer_null_;            
            std::string operation_;            
            bool operation_used_;            
            bool operation_null_;            
            int status_;            
            bool status_used_;            
            bool status_null_;            
            std::string test_;            
            bool test_used_;            
            bool test_null_;            
          public:          
            Key& isp_sdate(const AutoTest::Time& value);            
            Key& isp_sdate_set_null(bool is_null = true);            
            const AutoTest::Time& isp_sdate() const;            
            bool isp_sdate_used() const;            
            bool isp_sdate_is_null() const;            
            Key& colo_id(const int& value);            
            Key& colo_id_set_null(bool is_null = true);            
            const int& colo_id() const;            
            bool colo_id_used() const;            
            bool colo_id_is_null() const;            
            Key& referer(const std::string& value);            
            Key& referer_set_null(bool is_null = true);            
            const std::string& referer() const;            
            bool referer_used() const;            
            bool referer_is_null() const;            
            Key& operation(const std::string& value);            
            Key& operation_set_null(bool is_null = true);            
            const std::string& operation() const;            
            bool operation_used() const;            
            bool operation_is_null() const;            
            Key& status(const int& value);            
            Key& status_set_null(bool is_null = true);            
            const int& status() const;            
            bool status_used() const;            
            bool status_is_null() const;            
            Key& test(const std::string& value);            
            Key& test_set_null(bool is_null = true);            
            const std::string& test() const;            
            bool test_used() const;            
            bool test_is_null() const;            
            Key ();            
            Key (            
              const AutoTest::Time& isp_sdate,              
              const int& colo_id,              
              const std::string& referer,              
              const std::string& operation,              
              const int& status,              
              const std::string& test              
            );            
        };        
        stats_value_type count () const;        
        void print_idname (std::ostream& out) const;        
                
        OptOutStats (const Key& value);        
        OptOutStats (        
          const AutoTest::Time& isp_sdate,          
          const int& colo_id,          
          const std::string& referer,          
          const std::string& operation,          
          const int& status,          
          const std::string& test          
        );        
        OptOutStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const AutoTest::Time& isp_sdate,          
          const int& colo_id,          
          const std::string& referer,          
          const std::string& operation,          
          const int& status,          
          const std::string& test          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<OptOutStats, 1>::Diffs    
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
                
        Diffs& count (const stats_diff_type& value);        
        const stats_diff_type& count () const;        
      protected:      
        stats_diff_type diffs[1];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// OptOutStats    
    inline    
    OptOutStats::OptOutStats ()    
      :Base("OptOutStats")    
    {}    
    inline    
    OptOutStats::OptOutStats (const OptOutStats::Key& value)    
      :Base("OptOutStats")    
    {    
      key_ = value;      
    }    
    inline    
    OptOutStats::OptOutStats (    
      const AutoTest::Time& isp_sdate,      
      const int& colo_id,      
      const std::string& referer,      
      const std::string& operation,      
      const int& status,      
      const std::string& test      
    )    
      :Base("OptOutStats")    
    {    
      key_ = Key (      
        isp_sdate,        
        colo_id,        
        referer,        
        operation,        
        status,        
        test        
      );      
    }    
    inline    
    OptOutStats::Key::Key ()    
      :isp_sdate_(default_date()),colo_id_(0),status_(0)    
    {    
      isp_sdate_used_ = false;      
      isp_sdate_null_ = false;      
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      referer_used_ = false;      
      referer_null_ = false;      
      operation_used_ = false;      
      operation_null_ = false;      
      status_used_ = false;      
      status_null_ = false;      
      test_used_ = false;      
      test_null_ = false;      
    }    
    inline    
    OptOutStats::Key::Key (    
      const AutoTest::Time& isp_sdate,      
      const int& colo_id,      
      const std::string& referer,      
      const std::string& operation,      
      const int& status,      
      const std::string& test      
    )    
      :isp_sdate_(isp_sdate),colo_id_(colo_id),status_(status)    
    {    
      isp_sdate_used_ = true;      
      isp_sdate_null_ = false;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      referer_ = referer;      
      referer_used_ = true;      
      referer_null_ = false;      
      operation_ = operation;      
      operation_used_ = true;      
      operation_null_ = false;      
      status_used_ = true;      
      status_null_ = false;      
      test_ = test;      
      test_used_ = true;      
      test_null_ = false;      
    }    
    inline    
    OptOutStats::Key&    
    OptOutStats::Key::isp_sdate(const AutoTest::Time& value)    
    {    
      isp_sdate_ = value;      
      isp_sdate_used_ = true;      
      isp_sdate_null_ = false;      
      return *this;      
    }    
    inline    
    OptOutStats::Key&    
    OptOutStats::Key::isp_sdate_set_null(bool is_null)    
    {    
      isp_sdate_used_ = true;      
      isp_sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    OptOutStats::Key::isp_sdate() const    
    {    
      return isp_sdate_;      
    }    
    inline    
    bool    
    OptOutStats::Key::isp_sdate_used() const    
    {    
      return isp_sdate_used_;      
    }    
    inline    
    bool    
    OptOutStats::Key::isp_sdate_is_null() const    
    {    
      return isp_sdate_null_;      
    }    
    inline    
    OptOutStats::Key&    
    OptOutStats::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    OptOutStats::Key&    
    OptOutStats::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    OptOutStats::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    OptOutStats::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    OptOutStats::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    OptOutStats::Key&    
    OptOutStats::Key::referer(const std::string& value)    
    {    
      referer_ = value;      
      referer_used_ = true;      
      referer_null_ = false;      
      return *this;      
    }    
    inline    
    OptOutStats::Key&    
    OptOutStats::Key::referer_set_null(bool is_null)    
    {    
      referer_used_ = true;      
      referer_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    OptOutStats::Key::referer() const    
    {    
      return referer_;      
    }    
    inline    
    bool    
    OptOutStats::Key::referer_used() const    
    {    
      return referer_used_;      
    }    
    inline    
    bool    
    OptOutStats::Key::referer_is_null() const    
    {    
      return referer_null_;      
    }    
    inline    
    OptOutStats::Key&    
    OptOutStats::Key::operation(const std::string& value)    
    {    
      operation_ = value;      
      operation_used_ = true;      
      operation_null_ = false;      
      return *this;      
    }    
    inline    
    OptOutStats::Key&    
    OptOutStats::Key::operation_set_null(bool is_null)    
    {    
      operation_used_ = true;      
      operation_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    OptOutStats::Key::operation() const    
    {    
      return operation_;      
    }    
    inline    
    bool    
    OptOutStats::Key::operation_used() const    
    {    
      return operation_used_;      
    }    
    inline    
    bool    
    OptOutStats::Key::operation_is_null() const    
    {    
      return operation_null_;      
    }    
    inline    
    OptOutStats::Key&    
    OptOutStats::Key::status(const int& value)    
    {    
      status_ = value;      
      status_used_ = true;      
      status_null_ = false;      
      return *this;      
    }    
    inline    
    OptOutStats::Key&    
    OptOutStats::Key::status_set_null(bool is_null)    
    {    
      status_used_ = true;      
      status_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    OptOutStats::Key::status() const    
    {    
      return status_;      
    }    
    inline    
    bool    
    OptOutStats::Key::status_used() const    
    {    
      return status_used_;      
    }    
    inline    
    bool    
    OptOutStats::Key::status_is_null() const    
    {    
      return status_null_;      
    }    
    inline    
    OptOutStats::Key&    
    OptOutStats::Key::test(const std::string& value)    
    {    
      test_ = value;      
      test_used_ = true;      
      test_null_ = false;      
      return *this;      
    }    
    inline    
    OptOutStats::Key&    
    OptOutStats::Key::test_set_null(bool is_null)    
    {    
      test_used_ = true;      
      test_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    OptOutStats::Key::test() const    
    {    
      return test_;      
    }    
    inline    
    bool    
    OptOutStats::Key::test_used() const    
    {    
      return test_used_;      
    }    
    inline    
    bool    
    OptOutStats::Key::test_is_null() const    
    {    
      return test_null_;      
    }    
    inline    
    OptOutStats::Key&    
    OptOutStats::key (    
      const AutoTest::Time& isp_sdate,      
      const int& colo_id,      
      const std::string& referer,      
      const std::string& operation,      
      const int& status,      
      const std::string& test      
    )    
    {    
      key_ = Key (      
        isp_sdate,        
        colo_id,        
        referer,        
        operation,        
        status,        
        test        
      );      
      return key_;      
    }    
    inline    
    OptOutStats::Key&    
    OptOutStats::key ()    
    {    
      return key_;      
    }    
    inline    
    OptOutStats::Key&    
    OptOutStats::key (const OptOutStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<OptOutStats, 1>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<OptOutStats, 1>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<OptOutStats, 1>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<OptOutStats, 1>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<OptOutStats, 1>::Diffs&     
    DiffStats<OptOutStats, 1>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<OptOutStats, 1>::Diffs&     
    DiffStats<OptOutStats, 1>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<OptOutStats, 1>::Diffs&     
    DiffStats<OptOutStats, 1>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 1; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<OptOutStats, 1>::Diffs     
    DiffStats<OptOutStats, 1>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<OptOutStats, 1>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<OptOutStats, 1>::Diffs     
    DiffStats<OptOutStats, 1>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<OptOutStats, 1>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<OptOutStats, 1>::Diffs::const_iterator    
    DiffStats<OptOutStats, 1>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<OptOutStats, 1>::Diffs::const_iterator    
    DiffStats<OptOutStats, 1>::Diffs::end () const    
    {    
      return diffs + 1;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<OptOutStats, 1>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<OptOutStats, 1>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<OptOutStats, 1>::Diffs::size () const    
    {    
      return 1;      
    }    
    inline    
    void    
    DiffStats<OptOutStats, 1>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 1; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    OptOutStats::count () const    
    {    
      return values[OptOutStats::COUNT];      
    }    
    inline    
    DiffStats<OptOutStats, 1>::Diffs&     
    DiffStats<OptOutStats, 1>::Diffs::count (const stats_diff_type& value)    
    {    
      diffs[OptOutStats::COUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<OptOutStats, 1>::Diffs::count () const    
    {    
      return diffs[OptOutStats::COUNT];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_OPTOUTSTATS_HPP

