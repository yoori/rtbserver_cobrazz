
#ifndef __AUTOTESTS_COMMONS_STATS_EXPRESSIONPERFORMANCESTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_EXPRESSIONPERFORMANCESTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class ExpressionPerformanceStats:    
      public DiffStats<ExpressionPerformanceStats, 3>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          IMPS = 0,          
          CLICKS,          
          ACTIONS          
        };        
        typedef DiffStats<ExpressionPerformanceStats, 3> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            AutoTest::Time sdate_;            
            bool sdate_used_;            
            bool sdate_null_;            
            int cc_id_;            
            bool cc_id_used_;            
            bool cc_id_null_;            
            int colo_id_;            
            bool colo_id_used_;            
            bool colo_id_null_;            
            std::string expression_;            
            bool expression_used_;            
            bool expression_null_;            
          public:          
            Key& sdate(const AutoTest::Time& value);            
            Key& sdate_set_null(bool is_null = true);            
            const AutoTest::Time& sdate() const;            
            bool sdate_used() const;            
            bool sdate_is_null() const;            
            Key& cc_id(const int& value);            
            Key& cc_id_set_null(bool is_null = true);            
            const int& cc_id() const;            
            bool cc_id_used() const;            
            bool cc_id_is_null() const;            
            Key& colo_id(const int& value);            
            Key& colo_id_set_null(bool is_null = true);            
            const int& colo_id() const;            
            bool colo_id_used() const;            
            bool colo_id_is_null() const;            
            Key& expression(const std::string& value);            
            Key& expression_set_null(bool is_null = true);            
            const std::string& expression() const;            
            bool expression_used() const;            
            bool expression_is_null() const;            
            Key ();            
            Key (            
              const AutoTest::Time& sdate,              
              const int& cc_id,              
              const int& colo_id,              
              const std::string& expression              
            );            
        };        
        stats_value_type imps () const;        
        stats_value_type clicks () const;        
        stats_value_type actions () const;        
        void print_idname (std::ostream& out) const;        
                
        ExpressionPerformanceStats (const Key& value);        
        ExpressionPerformanceStats (        
          const AutoTest::Time& sdate,          
          const int& cc_id,          
          const int& colo_id,          
          const std::string& expression          
        );        
        ExpressionPerformanceStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const AutoTest::Time& sdate,          
          const int& cc_id,          
          const int& colo_id,          
          const std::string& expression          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<ExpressionPerformanceStats, 3>::Diffs    
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
        Diffs& clicks (const stats_diff_type& value);        
        const stats_diff_type& clicks () const;        
        Diffs& actions (const stats_diff_type& value);        
        const stats_diff_type& actions () const;        
      protected:      
        stats_diff_type diffs[3];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ExpressionPerformanceStats    
    inline    
    ExpressionPerformanceStats::ExpressionPerformanceStats ()    
      :Base("ExpressionPerformanceStats")    
    {}    
    inline    
    ExpressionPerformanceStats::ExpressionPerformanceStats (const ExpressionPerformanceStats::Key& value)    
      :Base("ExpressionPerformanceStats")    
    {    
      key_ = value;      
    }    
    inline    
    ExpressionPerformanceStats::ExpressionPerformanceStats (    
      const AutoTest::Time& sdate,      
      const int& cc_id,      
      const int& colo_id,      
      const std::string& expression      
    )    
      :Base("ExpressionPerformanceStats")    
    {    
      key_ = Key (      
        sdate,        
        cc_id,        
        colo_id,        
        expression        
      );      
    }    
    inline    
    ExpressionPerformanceStats::Key::Key ()    
      :sdate_(default_date()),cc_id_(0),colo_id_(0)    
    {    
      sdate_used_ = false;      
      sdate_null_ = false;      
      cc_id_used_ = false;      
      cc_id_null_ = false;      
      colo_id_used_ = false;      
      colo_id_null_ = false;      
      expression_used_ = false;      
      expression_null_ = false;      
    }    
    inline    
    ExpressionPerformanceStats::Key::Key (    
      const AutoTest::Time& sdate,      
      const int& cc_id,      
      const int& colo_id,      
      const std::string& expression      
    )    
      :sdate_(sdate),cc_id_(cc_id),colo_id_(colo_id)    
    {    
      sdate_used_ = true;      
      sdate_null_ = false;      
      cc_id_used_ = true;      
      cc_id_null_ = false;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      expression_ = expression;      
      expression_used_ = true;      
      expression_null_ = false;      
    }    
    inline    
    ExpressionPerformanceStats::Key&    
    ExpressionPerformanceStats::Key::sdate(const AutoTest::Time& value)    
    {    
      sdate_ = value;      
      sdate_used_ = true;      
      sdate_null_ = false;      
      return *this;      
    }    
    inline    
    ExpressionPerformanceStats::Key&    
    ExpressionPerformanceStats::Key::sdate_set_null(bool is_null)    
    {    
      sdate_used_ = true;      
      sdate_null_ = is_null;      
      return *this;      
    }    
    inline    
    const AutoTest::Time&    
    ExpressionPerformanceStats::Key::sdate() const    
    {    
      return sdate_;      
    }    
    inline    
    bool    
    ExpressionPerformanceStats::Key::sdate_used() const    
    {    
      return sdate_used_;      
    }    
    inline    
    bool    
    ExpressionPerformanceStats::Key::sdate_is_null() const    
    {    
      return sdate_null_;      
    }    
    inline    
    ExpressionPerformanceStats::Key&    
    ExpressionPerformanceStats::Key::cc_id(const int& value)    
    {    
      cc_id_ = value;      
      cc_id_used_ = true;      
      cc_id_null_ = false;      
      return *this;      
    }    
    inline    
    ExpressionPerformanceStats::Key&    
    ExpressionPerformanceStats::Key::cc_id_set_null(bool is_null)    
    {    
      cc_id_used_ = true;      
      cc_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ExpressionPerformanceStats::Key::cc_id() const    
    {    
      return cc_id_;      
    }    
    inline    
    bool    
    ExpressionPerformanceStats::Key::cc_id_used() const    
    {    
      return cc_id_used_;      
    }    
    inline    
    bool    
    ExpressionPerformanceStats::Key::cc_id_is_null() const    
    {    
      return cc_id_null_;      
    }    
    inline    
    ExpressionPerformanceStats::Key&    
    ExpressionPerformanceStats::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    ExpressionPerformanceStats::Key&    
    ExpressionPerformanceStats::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ExpressionPerformanceStats::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    ExpressionPerformanceStats::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    ExpressionPerformanceStats::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    ExpressionPerformanceStats::Key&    
    ExpressionPerformanceStats::Key::expression(const std::string& value)    
    {    
      expression_ = value;      
      expression_used_ = true;      
      expression_null_ = false;      
      return *this;      
    }    
    inline    
    ExpressionPerformanceStats::Key&    
    ExpressionPerformanceStats::Key::expression_set_null(bool is_null)    
    {    
      expression_used_ = true;      
      expression_null_ = is_null;      
      return *this;      
    }    
    inline    
    const std::string&    
    ExpressionPerformanceStats::Key::expression() const    
    {    
      return expression_;      
    }    
    inline    
    bool    
    ExpressionPerformanceStats::Key::expression_used() const    
    {    
      return expression_used_;      
    }    
    inline    
    bool    
    ExpressionPerformanceStats::Key::expression_is_null() const    
    {    
      return expression_null_;      
    }    
    inline    
    ExpressionPerformanceStats::Key&    
    ExpressionPerformanceStats::key (    
      const AutoTest::Time& sdate,      
      const int& cc_id,      
      const int& colo_id,      
      const std::string& expression      
    )    
    {    
      key_ = Key (      
        sdate,        
        cc_id,        
        colo_id,        
        expression        
      );      
      return key_;      
    }    
    inline    
    ExpressionPerformanceStats::Key&    
    ExpressionPerformanceStats::key ()    
    {    
      return key_;      
    }    
    inline    
    ExpressionPerformanceStats::Key&    
    ExpressionPerformanceStats::key (const ExpressionPerformanceStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ExpressionPerformanceStats, 3>::Diffs&     
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ExpressionPerformanceStats, 3>::Diffs&     
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ExpressionPerformanceStats, 3>::Diffs&     
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 3; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ExpressionPerformanceStats, 3>::Diffs     
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<ExpressionPerformanceStats, 3>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<ExpressionPerformanceStats, 3>::Diffs     
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<ExpressionPerformanceStats, 3>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::const_iterator    
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::const_iterator    
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::end () const    
    {    
      return diffs + 3;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::size () const    
    {    
      return 3;      
    }    
    inline    
    void    
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 3; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    ExpressionPerformanceStats::imps () const    
    {    
      return values[ExpressionPerformanceStats::IMPS];      
    }    
    inline    
    DiffStats<ExpressionPerformanceStats, 3>::Diffs&     
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::imps (const stats_diff_type& value)    
    {    
      diffs[ExpressionPerformanceStats::IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::imps () const    
    {    
      return diffs[ExpressionPerformanceStats::IMPS];      
    }    
    inline    
    stats_value_type    
    ExpressionPerformanceStats::clicks () const    
    {    
      return values[ExpressionPerformanceStats::CLICKS];      
    }    
    inline    
    DiffStats<ExpressionPerformanceStats, 3>::Diffs&     
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::clicks (const stats_diff_type& value)    
    {    
      diffs[ExpressionPerformanceStats::CLICKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::clicks () const    
    {    
      return diffs[ExpressionPerformanceStats::CLICKS];      
    }    
    inline    
    stats_value_type    
    ExpressionPerformanceStats::actions () const    
    {    
      return values[ExpressionPerformanceStats::ACTIONS];      
    }    
    inline    
    DiffStats<ExpressionPerformanceStats, 3>::Diffs&     
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::actions (const stats_diff_type& value)    
    {    
      diffs[ExpressionPerformanceStats::ACTIONS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ExpressionPerformanceStats, 3>::Diffs::actions () const    
    {    
      return diffs[ExpressionPerformanceStats::ACTIONS];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_EXPRESSIONPERFORMANCESTATS_HPP

