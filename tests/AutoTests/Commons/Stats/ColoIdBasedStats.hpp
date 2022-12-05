
#ifndef __AUTOTESTS_COMMONS_STATS_COLOIDBASEDSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_COLOIDBASEDSTATS_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace ORM  
  {  
    class ColoIdBasedStats:    
      public DiffStats<ColoIdBasedStats, 54>    
    {    
      protected:      
        bool query_select_ (StatsDB::IConn& connection);        
      public:      
        enum FieldName        
        {        
          ACTIONREQUESTS_ACTION_REQUEST_COUNT = 0,          
          ACTIONSTATS_COUNT,          
          CCGKEYWORDSTATSHOURLY_IMPS,          
          CCGKEYWORDSTATSHOURLY_CLICKS,          
          CCGKEYWORDSTATSHOURLY_ADV_AMOUNT,          
          CCGKEYWORDSTATSHOURLY_ADV_COMM_AMOUNT,          
          CCGKEYWORDSTATSHOURLY_PUB_AMOUNT_ADV,          
          CCGUSERSTATS_UNIQUE_USERS,          
          CMPREQUESTSTATSHOURLY_IMPS,          
          CMPREQUESTSTATSHOURLY_ADV_AMOUNT_CMP,          
          CMPREQUESTSTATSHOURLY_CMP_AMOUNT,          
          CMPREQUESTSTATSHOURLY_CMP_AMOUNT_GLOBAL,          
          CMPREQUESTSTATSHOURLY_CLICKS,          
          CAMPAIGNUSERSTATS_UNIQUE_USERS,          
          CCUSERSTATS_UNIQUE_USERS,          
          CHANNELTRIGGERSTATS_HITS,          
          CHANNELUSAGESTATS_IMPS,          
          CHANNELUSAGESTATS_CLICKS,          
          CHANNELUSAGESTATS_ACTIONS,          
          CHANNELUSAGESTATS_REVENUE,          
          COLOUSERSTATS_UNIQUE_USERS,          
          COLOUSERSTATS_NETWORK_UNIQUE_USERS,          
          EXPRESSIONPERFORMANCE_IMPS_VERIFIED,          
          EXPRESSIONPERFORMANCE_CLICKS,          
          EXPRESSIONPERFORMANCE_ACTIONS,          
          GLOBALCOLOUSERSTATS_UNIQUE_USERS,          
          GLOBALCOLOUSERSTATS_NETWORK_UNIQUE_USERS,          
          PASSBACKSTATS_REQUESTS,          
          REQUESTSTATSHOURLY_IMPS,          
          REQUESTSTATSHOURLY_REQUESTS,          
          REQUESTSTATSHOURLY_CLICKS,          
          REQUESTSTATSHOURLY_ACTIONS,          
          REQUESTSTATSHOURLY_ADV_AMOUNT,          
          REQUESTSTATSHOURLY_ADV_AMOUNT_GLOBAL,          
          REQUESTSTATSHOURLY_PUB_AMOUNT,          
          REQUESTSTATSHOURLY_PUB_AMOUNT_GLOBAL,          
          REQUESTSTATSHOURLY_ISP_AMOUNT,          
          REQUESTSTATSHOURLY_ISP_AMOUNT_GLOBAL,          
          REQUESTSTATSHOURLY_ADV_COMM_AMOUNT,          
          REQUESTSTATSHOURLY_PUB_COMM_AMOUNT,          
          REQUESTSTATSHOURLY_ADV_COMM_AMOUNT_GLOBAL,          
          REQUESTSTATSHOURLY_PUB_COMM_AMOUNT_GLOBAL,          
          REQUESTSTATSHOURLY_ADV_INVOICE_COMM_AMOUNT,          
          REQUESTSTATSHOURLY_PASSBACKS,          
          SITECHANNELSTATS_IMPS,          
          SITECHANNELSTATS_ADV_REVENUE,          
          SITECHANNELSTATS_PUB_REVENUE,          
          USERPROPERTYSTATSHOURLY_REQUESTS,          
          USERPROPERTYSTATSHOURLY_IMPS,          
          USERPROPERTYSTATSHOURLY_CLICKS,          
          USERPROPERTYSTATSHOURLY_ACTIONS,          
          USERPROPERTYSTATSHOURLY_IMPS_UNVERIFIED,          
          USERPROPERTYSTATSHOURLY_PROFILING_REQUESTS,          
          TAGAUCTIONSTATS_REQUESTS          
        };        
        typedef DiffStats<ColoIdBasedStats, 54> Base;        
        typedef Base::Diffs Diffs;        
      public:      
        class Key        
        {        
          protected:          
            int colo_id_;            
            bool colo_id_used_;            
            bool colo_id_null_;            
          public:          
            Key& colo_id(const int& value);            
            Key& colo_id_set_null(bool is_null = true);            
            const int& colo_id() const;            
            bool colo_id_used() const;            
            bool colo_id_is_null() const;            
            Key ();            
            Key (            
              const int& colo_id              
            );            
        };        
        stats_value_type actionrequests_action_request_count () const;        
        stats_value_type actionstats_count () const;        
        stats_value_type ccgkeywordstatshourly_imps () const;        
        stats_value_type ccgkeywordstatshourly_clicks () const;        
        stats_value_type ccgkeywordstatshourly_adv_amount () const;        
        stats_value_type ccgkeywordstatshourly_adv_comm_amount () const;        
        stats_value_type ccgkeywordstatshourly_pub_amount_adv () const;        
        stats_value_type ccguserstats_unique_users () const;        
        stats_value_type cmprequeststatshourly_imps () const;        
        stats_value_type cmprequeststatshourly_adv_amount_cmp () const;        
        stats_value_type cmprequeststatshourly_cmp_amount () const;        
        stats_value_type cmprequeststatshourly_cmp_amount_global () const;        
        stats_value_type cmprequeststatshourly_clicks () const;        
        stats_value_type campaignuserstats_unique_users () const;        
        stats_value_type ccuserstats_unique_users () const;        
        stats_value_type channeltriggerstats_hits () const;        
        stats_value_type channelusagestats_imps () const;        
        stats_value_type channelusagestats_clicks () const;        
        stats_value_type channelusagestats_actions () const;        
        stats_value_type channelusagestats_revenue () const;        
        stats_value_type colouserstats_unique_users () const;        
        stats_value_type colouserstats_network_unique_users () const;        
        stats_value_type expressionperformance_imps_verified () const;        
        stats_value_type expressionperformance_clicks () const;        
        stats_value_type expressionperformance_actions () const;        
        stats_value_type globalcolouserstats_unique_users () const;        
        stats_value_type globalcolouserstats_network_unique_users () const;        
        stats_value_type passbackstats_requests () const;        
        stats_value_type requeststatshourly_imps () const;        
        stats_value_type requeststatshourly_requests () const;        
        stats_value_type requeststatshourly_clicks () const;        
        stats_value_type requeststatshourly_actions () const;        
        stats_value_type requeststatshourly_adv_amount () const;        
        stats_value_type requeststatshourly_adv_amount_global () const;        
        stats_value_type requeststatshourly_pub_amount () const;        
        stats_value_type requeststatshourly_pub_amount_global () const;        
        stats_value_type requeststatshourly_isp_amount () const;        
        stats_value_type requeststatshourly_isp_amount_global () const;        
        stats_value_type requeststatshourly_adv_comm_amount () const;        
        stats_value_type requeststatshourly_pub_comm_amount () const;        
        stats_value_type requeststatshourly_adv_comm_amount_global () const;        
        stats_value_type requeststatshourly_pub_comm_amount_global () const;        
        stats_value_type requeststatshourly_adv_invoice_comm_amount () const;        
        stats_value_type requeststatshourly_passbacks () const;        
        stats_value_type sitechannelstats_imps () const;        
        stats_value_type sitechannelstats_adv_revenue () const;        
        stats_value_type sitechannelstats_pub_revenue () const;        
        stats_value_type userpropertystatshourly_requests () const;        
        stats_value_type userpropertystatshourly_imps () const;        
        stats_value_type userpropertystatshourly_clicks () const;        
        stats_value_type userpropertystatshourly_actions () const;        
        stats_value_type userpropertystatshourly_imps_unverified () const;        
        stats_value_type userpropertystatshourly_profiling_requests () const;        
        stats_value_type tagauctionstats_requests () const;        
        void print_idname (std::ostream& out) const;        
                
        ColoIdBasedStats (const Key& value);        
        ColoIdBasedStats (        
          const int& colo_id          
        );        
        ColoIdBasedStats ();        
        Key& key ();        
        Key& key (const Key& value);        
        Key& key (        
          const int& colo_id          
        );        
                
      protected:      
        Key key_;        
    };    
        
    template<>    
    class DiffStats<ColoIdBasedStats, 54>::Diffs    
    {    
      public:      
        typedef stats_diff_type array_type[54];        
        typedef const stats_diff_type const_array_type[54];        
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
                
        Diffs& actionrequests_action_request_count (const stats_diff_type& value);        
        const stats_diff_type& actionrequests_action_request_count () const;        
        Diffs& actionstats_count (const stats_diff_type& value);        
        const stats_diff_type& actionstats_count () const;        
        Diffs& ccgkeywordstatshourly_imps (const stats_diff_type& value);        
        const stats_diff_type& ccgkeywordstatshourly_imps () const;        
        Diffs& ccgkeywordstatshourly_clicks (const stats_diff_type& value);        
        const stats_diff_type& ccgkeywordstatshourly_clicks () const;        
        Diffs& ccgkeywordstatshourly_adv_amount (const stats_diff_type& value);        
        const stats_diff_type& ccgkeywordstatshourly_adv_amount () const;        
        Diffs& ccgkeywordstatshourly_adv_comm_amount (const stats_diff_type& value);        
        const stats_diff_type& ccgkeywordstatshourly_adv_comm_amount () const;        
        Diffs& ccgkeywordstatshourly_pub_amount_adv (const stats_diff_type& value);        
        const stats_diff_type& ccgkeywordstatshourly_pub_amount_adv () const;        
        Diffs& ccguserstats_unique_users (const stats_diff_type& value);        
        const stats_diff_type& ccguserstats_unique_users () const;        
        Diffs& cmprequeststatshourly_imps (const stats_diff_type& value);        
        const stats_diff_type& cmprequeststatshourly_imps () const;        
        Diffs& cmprequeststatshourly_adv_amount_cmp (const stats_diff_type& value);        
        const stats_diff_type& cmprequeststatshourly_adv_amount_cmp () const;        
        Diffs& cmprequeststatshourly_cmp_amount (const stats_diff_type& value);        
        const stats_diff_type& cmprequeststatshourly_cmp_amount () const;        
        Diffs& cmprequeststatshourly_cmp_amount_global (const stats_diff_type& value);        
        const stats_diff_type& cmprequeststatshourly_cmp_amount_global () const;        
        Diffs& cmprequeststatshourly_clicks (const stats_diff_type& value);        
        const stats_diff_type& cmprequeststatshourly_clicks () const;        
        Diffs& campaignuserstats_unique_users (const stats_diff_type& value);        
        const stats_diff_type& campaignuserstats_unique_users () const;        
        Diffs& ccuserstats_unique_users (const stats_diff_type& value);        
        const stats_diff_type& ccuserstats_unique_users () const;        
        Diffs& channeltriggerstats_hits (const stats_diff_type& value);        
        const stats_diff_type& channeltriggerstats_hits () const;        
        Diffs& channelusagestats_imps (const stats_diff_type& value);        
        const stats_diff_type& channelusagestats_imps () const;        
        Diffs& channelusagestats_clicks (const stats_diff_type& value);        
        const stats_diff_type& channelusagestats_clicks () const;        
        Diffs& channelusagestats_actions (const stats_diff_type& value);        
        const stats_diff_type& channelusagestats_actions () const;        
        Diffs& channelusagestats_revenue (const stats_diff_type& value);        
        const stats_diff_type& channelusagestats_revenue () const;        
        Diffs& colouserstats_unique_users (const stats_diff_type& value);        
        const stats_diff_type& colouserstats_unique_users () const;        
        Diffs& colouserstats_network_unique_users (const stats_diff_type& value);        
        const stats_diff_type& colouserstats_network_unique_users () const;        
        Diffs& expressionperformance_imps_verified (const stats_diff_type& value);        
        const stats_diff_type& expressionperformance_imps_verified () const;        
        Diffs& expressionperformance_clicks (const stats_diff_type& value);        
        const stats_diff_type& expressionperformance_clicks () const;        
        Diffs& expressionperformance_actions (const stats_diff_type& value);        
        const stats_diff_type& expressionperformance_actions () const;        
        Diffs& globalcolouserstats_unique_users (const stats_diff_type& value);        
        const stats_diff_type& globalcolouserstats_unique_users () const;        
        Diffs& globalcolouserstats_network_unique_users (const stats_diff_type& value);        
        const stats_diff_type& globalcolouserstats_network_unique_users () const;        
        Diffs& passbackstats_requests (const stats_diff_type& value);        
        const stats_diff_type& passbackstats_requests () const;        
        Diffs& requeststatshourly_imps (const stats_diff_type& value);        
        const stats_diff_type& requeststatshourly_imps () const;        
        Diffs& requeststatshourly_requests (const stats_diff_type& value);        
        const stats_diff_type& requeststatshourly_requests () const;        
        Diffs& requeststatshourly_clicks (const stats_diff_type& value);        
        const stats_diff_type& requeststatshourly_clicks () const;        
        Diffs& requeststatshourly_actions (const stats_diff_type& value);        
        const stats_diff_type& requeststatshourly_actions () const;        
        Diffs& requeststatshourly_adv_amount (const stats_diff_type& value);        
        const stats_diff_type& requeststatshourly_adv_amount () const;        
        Diffs& requeststatshourly_adv_amount_global (const stats_diff_type& value);        
        const stats_diff_type& requeststatshourly_adv_amount_global () const;        
        Diffs& requeststatshourly_pub_amount (const stats_diff_type& value);        
        const stats_diff_type& requeststatshourly_pub_amount () const;        
        Diffs& requeststatshourly_pub_amount_global (const stats_diff_type& value);        
        const stats_diff_type& requeststatshourly_pub_amount_global () const;        
        Diffs& requeststatshourly_isp_amount (const stats_diff_type& value);        
        const stats_diff_type& requeststatshourly_isp_amount () const;        
        Diffs& requeststatshourly_isp_amount_global (const stats_diff_type& value);        
        const stats_diff_type& requeststatshourly_isp_amount_global () const;        
        Diffs& requeststatshourly_adv_comm_amount (const stats_diff_type& value);        
        const stats_diff_type& requeststatshourly_adv_comm_amount () const;        
        Diffs& requeststatshourly_pub_comm_amount (const stats_diff_type& value);        
        const stats_diff_type& requeststatshourly_pub_comm_amount () const;        
        Diffs& requeststatshourly_adv_comm_amount_global (const stats_diff_type& value);        
        const stats_diff_type& requeststatshourly_adv_comm_amount_global () const;        
        Diffs& requeststatshourly_pub_comm_amount_global (const stats_diff_type& value);        
        const stats_diff_type& requeststatshourly_pub_comm_amount_global () const;        
        Diffs& requeststatshourly_adv_invoice_comm_amount (const stats_diff_type& value);        
        const stats_diff_type& requeststatshourly_adv_invoice_comm_amount () const;        
        Diffs& requeststatshourly_passbacks (const stats_diff_type& value);        
        const stats_diff_type& requeststatshourly_passbacks () const;        
        Diffs& sitechannelstats_imps (const stats_diff_type& value);        
        const stats_diff_type& sitechannelstats_imps () const;        
        Diffs& sitechannelstats_adv_revenue (const stats_diff_type& value);        
        const stats_diff_type& sitechannelstats_adv_revenue () const;        
        Diffs& sitechannelstats_pub_revenue (const stats_diff_type& value);        
        const stats_diff_type& sitechannelstats_pub_revenue () const;        
        Diffs& userpropertystatshourly_requests (const stats_diff_type& value);        
        const stats_diff_type& userpropertystatshourly_requests () const;        
        Diffs& userpropertystatshourly_imps (const stats_diff_type& value);        
        const stats_diff_type& userpropertystatshourly_imps () const;        
        Diffs& userpropertystatshourly_clicks (const stats_diff_type& value);        
        const stats_diff_type& userpropertystatshourly_clicks () const;        
        Diffs& userpropertystatshourly_actions (const stats_diff_type& value);        
        const stats_diff_type& userpropertystatshourly_actions () const;        
        Diffs& userpropertystatshourly_imps_unverified (const stats_diff_type& value);        
        const stats_diff_type& userpropertystatshourly_imps_unverified () const;        
        Diffs& userpropertystatshourly_profiling_requests (const stats_diff_type& value);        
        const stats_diff_type& userpropertystatshourly_profiling_requests () const;        
        Diffs& tagauctionstats_requests (const stats_diff_type& value);        
        const stats_diff_type& tagauctionstats_requests () const;        
      protected:      
        stats_diff_type diffs[54];      
    };    
        
  }  
}
namespace AutoTest
{
  namespace ORM  
  {  
    ///////////////////////////////// ColoIdBasedStats    
    inline    
    ColoIdBasedStats::ColoIdBasedStats ()    
      :Base("ColoIdBasedStats")    
    {}    
    inline    
    ColoIdBasedStats::ColoIdBasedStats (const ColoIdBasedStats::Key& value)    
      :Base("ColoIdBasedStats")    
    {    
      key_ = value;      
    }    
    inline    
    ColoIdBasedStats::ColoIdBasedStats (    
      const int& colo_id      
    )    
      :Base("ColoIdBasedStats")    
    {    
      key_ = Key (      
        colo_id        
      );      
    }    
    inline    
    ColoIdBasedStats::Key::Key ()    
      :colo_id_(0)    
    {    
      colo_id_used_ = false;      
      colo_id_null_ = false;      
    }    
    inline    
    ColoIdBasedStats::Key::Key (    
      const int& colo_id      
    )    
      :colo_id_(colo_id)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = false;      
    }    
    inline    
    ColoIdBasedStats::Key&    
    ColoIdBasedStats::Key::colo_id(const int& value)    
    {    
      colo_id_ = value;      
      colo_id_used_ = true;      
      colo_id_null_ = false;      
      return *this;      
    }    
    inline    
    ColoIdBasedStats::Key&    
    ColoIdBasedStats::Key::colo_id_set_null(bool is_null)    
    {    
      colo_id_used_ = true;      
      colo_id_null_ = is_null;      
      return *this;      
    }    
    inline    
    const int&    
    ColoIdBasedStats::Key::colo_id() const    
    {    
      return colo_id_;      
    }    
    inline    
    bool    
    ColoIdBasedStats::Key::colo_id_used() const    
    {    
      return colo_id_used_;      
    }    
    inline    
    bool    
    ColoIdBasedStats::Key::colo_id_is_null() const    
    {    
      return colo_id_null_;      
    }    
    inline    
    ColoIdBasedStats::Key&    
    ColoIdBasedStats::key (    
      const int& colo_id      
    )    
    {    
      key_ = Key (      
        colo_id        
      );      
      return key_;      
    }    
    inline    
    ColoIdBasedStats::Key&    
    ColoIdBasedStats::key ()    
    {    
      return key_;      
    }    
    inline    
    ColoIdBasedStats::Key&    
    ColoIdBasedStats::key (const ColoIdBasedStats::Key& value)    
    {    
      key_ = value;      
      return key_;      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs::Diffs()    
    {    
      for(int i = 0; i < 54; ++i)      
        diffs[i] = any_stats_diff;      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs::Diffs(const stats_diff_type& value)    
    {    
      for(int i = 0; i < 54; ++i)      
        diffs[i] = value;      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs::Diffs(const const_array_type& array)    
    {    
      for(int i = 0; i < 54; ++i)      
        diffs[i] = array[i];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs::operator const_array_type_ref () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::operator=(const const_array_type& array)    
    {    
      for(int i = 0; i < 54; ++i)      
        diffs[i] = array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::operator+=(const const_array_type& array)    
    {    
      for(int i = 0; i < 54; ++i)      
        diffs[i] += array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::operator-=(const const_array_type& array)    
    {    
      for(int i = 0; i < 54; ++i)      
        diffs[i] -= array[i];      
      return *this;      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs     
    DiffStats<ColoIdBasedStats, 54>::Diffs::operator+(const const_array_type& array)    
    {    
      DiffStats<ColoIdBasedStats, 54>::Diffs ret = *this;      
      ret += array;      
      return ret;      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs     
    DiffStats<ColoIdBasedStats, 54>::Diffs::operator-(const const_array_type& array)    
    {    
      DiffStats<ColoIdBasedStats, 54>::Diffs ret = *this;      
      ret -= array;      
      return ret;      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs::const_iterator    
    DiffStats<ColoIdBasedStats, 54>::Diffs::begin () const    
    {    
      return diffs;      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs::const_iterator    
    DiffStats<ColoIdBasedStats, 54>::Diffs::end () const    
    {    
      return diffs + 54;      
    }    
    inline    
    stats_diff_type&    
    DiffStats<ColoIdBasedStats, 54>::Diffs::operator[] (int i)    
    {    
      return diffs[i];      
    }    
    inline    
    const stats_diff_type&    
    DiffStats<ColoIdBasedStats, 54>::Diffs::operator[] (int i) const    
    {    
      return diffs[i];      
    }    
    inline    
    int    
    DiffStats<ColoIdBasedStats, 54>::Diffs::size () const    
    {    
      return 54;      
    }    
    inline    
    void    
    DiffStats<ColoIdBasedStats, 54>::Diffs::clear ()    
    {    
      for (unsigned int i = 0; i < 54; ++i)      
      {      
        diffs[i] = stats_diff_type();        
      }      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::actionrequests_action_request_count () const    
    {    
      return values[ColoIdBasedStats::ACTIONREQUESTS_ACTION_REQUEST_COUNT];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::actionrequests_action_request_count (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::ACTIONREQUESTS_ACTION_REQUEST_COUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::actionrequests_action_request_count () const    
    {    
      return diffs[ColoIdBasedStats::ACTIONREQUESTS_ACTION_REQUEST_COUNT];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::actionstats_count () const    
    {    
      return values[ColoIdBasedStats::ACTIONSTATS_COUNT];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::actionstats_count (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::ACTIONSTATS_COUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::actionstats_count () const    
    {    
      return diffs[ColoIdBasedStats::ACTIONSTATS_COUNT];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::ccgkeywordstatshourly_imps () const    
    {    
      return values[ColoIdBasedStats::CCGKEYWORDSTATSHOURLY_IMPS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::ccgkeywordstatshourly_imps (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::CCGKEYWORDSTATSHOURLY_IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::ccgkeywordstatshourly_imps () const    
    {    
      return diffs[ColoIdBasedStats::CCGKEYWORDSTATSHOURLY_IMPS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::ccgkeywordstatshourly_clicks () const    
    {    
      return values[ColoIdBasedStats::CCGKEYWORDSTATSHOURLY_CLICKS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::ccgkeywordstatshourly_clicks (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::CCGKEYWORDSTATSHOURLY_CLICKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::ccgkeywordstatshourly_clicks () const    
    {    
      return diffs[ColoIdBasedStats::CCGKEYWORDSTATSHOURLY_CLICKS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::ccgkeywordstatshourly_adv_amount () const    
    {    
      return values[ColoIdBasedStats::CCGKEYWORDSTATSHOURLY_ADV_AMOUNT];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::ccgkeywordstatshourly_adv_amount (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::CCGKEYWORDSTATSHOURLY_ADV_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::ccgkeywordstatshourly_adv_amount () const    
    {    
      return diffs[ColoIdBasedStats::CCGKEYWORDSTATSHOURLY_ADV_AMOUNT];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::ccgkeywordstatshourly_adv_comm_amount () const    
    {    
      return values[ColoIdBasedStats::CCGKEYWORDSTATSHOURLY_ADV_COMM_AMOUNT];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::ccgkeywordstatshourly_adv_comm_amount (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::CCGKEYWORDSTATSHOURLY_ADV_COMM_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::ccgkeywordstatshourly_adv_comm_amount () const    
    {    
      return diffs[ColoIdBasedStats::CCGKEYWORDSTATSHOURLY_ADV_COMM_AMOUNT];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::ccgkeywordstatshourly_pub_amount_adv () const    
    {    
      return values[ColoIdBasedStats::CCGKEYWORDSTATSHOURLY_PUB_AMOUNT_ADV];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::ccgkeywordstatshourly_pub_amount_adv (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::CCGKEYWORDSTATSHOURLY_PUB_AMOUNT_ADV] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::ccgkeywordstatshourly_pub_amount_adv () const    
    {    
      return diffs[ColoIdBasedStats::CCGKEYWORDSTATSHOURLY_PUB_AMOUNT_ADV];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::ccguserstats_unique_users () const    
    {    
      return values[ColoIdBasedStats::CCGUSERSTATS_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::ccguserstats_unique_users (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::CCGUSERSTATS_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::ccguserstats_unique_users () const    
    {    
      return diffs[ColoIdBasedStats::CCGUSERSTATS_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::cmprequeststatshourly_imps () const    
    {    
      return values[ColoIdBasedStats::CMPREQUESTSTATSHOURLY_IMPS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::cmprequeststatshourly_imps (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::CMPREQUESTSTATSHOURLY_IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::cmprequeststatshourly_imps () const    
    {    
      return diffs[ColoIdBasedStats::CMPREQUESTSTATSHOURLY_IMPS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::cmprequeststatshourly_adv_amount_cmp () const    
    {    
      return values[ColoIdBasedStats::CMPREQUESTSTATSHOURLY_ADV_AMOUNT_CMP];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::cmprequeststatshourly_adv_amount_cmp (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::CMPREQUESTSTATSHOURLY_ADV_AMOUNT_CMP] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::cmprequeststatshourly_adv_amount_cmp () const    
    {    
      return diffs[ColoIdBasedStats::CMPREQUESTSTATSHOURLY_ADV_AMOUNT_CMP];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::cmprequeststatshourly_cmp_amount () const    
    {    
      return values[ColoIdBasedStats::CMPREQUESTSTATSHOURLY_CMP_AMOUNT];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::cmprequeststatshourly_cmp_amount (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::CMPREQUESTSTATSHOURLY_CMP_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::cmprequeststatshourly_cmp_amount () const    
    {    
      return diffs[ColoIdBasedStats::CMPREQUESTSTATSHOURLY_CMP_AMOUNT];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::cmprequeststatshourly_cmp_amount_global () const    
    {    
      return values[ColoIdBasedStats::CMPREQUESTSTATSHOURLY_CMP_AMOUNT_GLOBAL];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::cmprequeststatshourly_cmp_amount_global (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::CMPREQUESTSTATSHOURLY_CMP_AMOUNT_GLOBAL] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::cmprequeststatshourly_cmp_amount_global () const    
    {    
      return diffs[ColoIdBasedStats::CMPREQUESTSTATSHOURLY_CMP_AMOUNT_GLOBAL];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::cmprequeststatshourly_clicks () const    
    {    
      return values[ColoIdBasedStats::CMPREQUESTSTATSHOURLY_CLICKS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::cmprequeststatshourly_clicks (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::CMPREQUESTSTATSHOURLY_CLICKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::cmprequeststatshourly_clicks () const    
    {    
      return diffs[ColoIdBasedStats::CMPREQUESTSTATSHOURLY_CLICKS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::campaignuserstats_unique_users () const    
    {    
      return values[ColoIdBasedStats::CAMPAIGNUSERSTATS_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::campaignuserstats_unique_users (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::CAMPAIGNUSERSTATS_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::campaignuserstats_unique_users () const    
    {    
      return diffs[ColoIdBasedStats::CAMPAIGNUSERSTATS_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::ccuserstats_unique_users () const    
    {    
      return values[ColoIdBasedStats::CCUSERSTATS_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::ccuserstats_unique_users (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::CCUSERSTATS_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::ccuserstats_unique_users () const    
    {    
      return diffs[ColoIdBasedStats::CCUSERSTATS_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::channeltriggerstats_hits () const    
    {    
      return values[ColoIdBasedStats::CHANNELTRIGGERSTATS_HITS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::channeltriggerstats_hits (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::CHANNELTRIGGERSTATS_HITS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::channeltriggerstats_hits () const    
    {    
      return diffs[ColoIdBasedStats::CHANNELTRIGGERSTATS_HITS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::channelusagestats_imps () const    
    {    
      return values[ColoIdBasedStats::CHANNELUSAGESTATS_IMPS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::channelusagestats_imps (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::CHANNELUSAGESTATS_IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::channelusagestats_imps () const    
    {    
      return diffs[ColoIdBasedStats::CHANNELUSAGESTATS_IMPS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::channelusagestats_clicks () const    
    {    
      return values[ColoIdBasedStats::CHANNELUSAGESTATS_CLICKS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::channelusagestats_clicks (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::CHANNELUSAGESTATS_CLICKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::channelusagestats_clicks () const    
    {    
      return diffs[ColoIdBasedStats::CHANNELUSAGESTATS_CLICKS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::channelusagestats_actions () const    
    {    
      return values[ColoIdBasedStats::CHANNELUSAGESTATS_ACTIONS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::channelusagestats_actions (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::CHANNELUSAGESTATS_ACTIONS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::channelusagestats_actions () const    
    {    
      return diffs[ColoIdBasedStats::CHANNELUSAGESTATS_ACTIONS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::channelusagestats_revenue () const    
    {    
      return values[ColoIdBasedStats::CHANNELUSAGESTATS_REVENUE];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::channelusagestats_revenue (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::CHANNELUSAGESTATS_REVENUE] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::channelusagestats_revenue () const    
    {    
      return diffs[ColoIdBasedStats::CHANNELUSAGESTATS_REVENUE];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::colouserstats_unique_users () const    
    {    
      return values[ColoIdBasedStats::COLOUSERSTATS_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::colouserstats_unique_users (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::COLOUSERSTATS_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::colouserstats_unique_users () const    
    {    
      return diffs[ColoIdBasedStats::COLOUSERSTATS_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::colouserstats_network_unique_users () const    
    {    
      return values[ColoIdBasedStats::COLOUSERSTATS_NETWORK_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::colouserstats_network_unique_users (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::COLOUSERSTATS_NETWORK_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::colouserstats_network_unique_users () const    
    {    
      return diffs[ColoIdBasedStats::COLOUSERSTATS_NETWORK_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::expressionperformance_imps_verified () const    
    {    
      return values[ColoIdBasedStats::EXPRESSIONPERFORMANCE_IMPS_VERIFIED];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::expressionperformance_imps_verified (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::EXPRESSIONPERFORMANCE_IMPS_VERIFIED] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::expressionperformance_imps_verified () const    
    {    
      return diffs[ColoIdBasedStats::EXPRESSIONPERFORMANCE_IMPS_VERIFIED];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::expressionperformance_clicks () const    
    {    
      return values[ColoIdBasedStats::EXPRESSIONPERFORMANCE_CLICKS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::expressionperformance_clicks (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::EXPRESSIONPERFORMANCE_CLICKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::expressionperformance_clicks () const    
    {    
      return diffs[ColoIdBasedStats::EXPRESSIONPERFORMANCE_CLICKS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::expressionperformance_actions () const    
    {    
      return values[ColoIdBasedStats::EXPRESSIONPERFORMANCE_ACTIONS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::expressionperformance_actions (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::EXPRESSIONPERFORMANCE_ACTIONS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::expressionperformance_actions () const    
    {    
      return diffs[ColoIdBasedStats::EXPRESSIONPERFORMANCE_ACTIONS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::globalcolouserstats_unique_users () const    
    {    
      return values[ColoIdBasedStats::GLOBALCOLOUSERSTATS_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::globalcolouserstats_unique_users (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::GLOBALCOLOUSERSTATS_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::globalcolouserstats_unique_users () const    
    {    
      return diffs[ColoIdBasedStats::GLOBALCOLOUSERSTATS_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::globalcolouserstats_network_unique_users () const    
    {    
      return values[ColoIdBasedStats::GLOBALCOLOUSERSTATS_NETWORK_UNIQUE_USERS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::globalcolouserstats_network_unique_users (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::GLOBALCOLOUSERSTATS_NETWORK_UNIQUE_USERS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::globalcolouserstats_network_unique_users () const    
    {    
      return diffs[ColoIdBasedStats::GLOBALCOLOUSERSTATS_NETWORK_UNIQUE_USERS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::passbackstats_requests () const    
    {    
      return values[ColoIdBasedStats::PASSBACKSTATS_REQUESTS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::passbackstats_requests (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::PASSBACKSTATS_REQUESTS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::passbackstats_requests () const    
    {    
      return diffs[ColoIdBasedStats::PASSBACKSTATS_REQUESTS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::requeststatshourly_imps () const    
    {    
      return values[ColoIdBasedStats::REQUESTSTATSHOURLY_IMPS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_imps (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_imps () const    
    {    
      return diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_IMPS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::requeststatshourly_requests () const    
    {    
      return values[ColoIdBasedStats::REQUESTSTATSHOURLY_REQUESTS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_requests (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_REQUESTS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_requests () const    
    {    
      return diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_REQUESTS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::requeststatshourly_clicks () const    
    {    
      return values[ColoIdBasedStats::REQUESTSTATSHOURLY_CLICKS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_clicks (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_CLICKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_clicks () const    
    {    
      return diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_CLICKS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::requeststatshourly_actions () const    
    {    
      return values[ColoIdBasedStats::REQUESTSTATSHOURLY_ACTIONS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_actions (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_ACTIONS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_actions () const    
    {    
      return diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_ACTIONS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::requeststatshourly_adv_amount () const    
    {    
      return values[ColoIdBasedStats::REQUESTSTATSHOURLY_ADV_AMOUNT];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_adv_amount (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_ADV_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_adv_amount () const    
    {    
      return diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_ADV_AMOUNT];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::requeststatshourly_adv_amount_global () const    
    {    
      return values[ColoIdBasedStats::REQUESTSTATSHOURLY_ADV_AMOUNT_GLOBAL];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_adv_amount_global (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_ADV_AMOUNT_GLOBAL] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_adv_amount_global () const    
    {    
      return diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_ADV_AMOUNT_GLOBAL];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::requeststatshourly_pub_amount () const    
    {    
      return values[ColoIdBasedStats::REQUESTSTATSHOURLY_PUB_AMOUNT];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_pub_amount (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_PUB_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_pub_amount () const    
    {    
      return diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_PUB_AMOUNT];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::requeststatshourly_pub_amount_global () const    
    {    
      return values[ColoIdBasedStats::REQUESTSTATSHOURLY_PUB_AMOUNT_GLOBAL];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_pub_amount_global (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_PUB_AMOUNT_GLOBAL] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_pub_amount_global () const    
    {    
      return diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_PUB_AMOUNT_GLOBAL];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::requeststatshourly_isp_amount () const    
    {    
      return values[ColoIdBasedStats::REQUESTSTATSHOURLY_ISP_AMOUNT];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_isp_amount (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_ISP_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_isp_amount () const    
    {    
      return diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_ISP_AMOUNT];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::requeststatshourly_isp_amount_global () const    
    {    
      return values[ColoIdBasedStats::REQUESTSTATSHOURLY_ISP_AMOUNT_GLOBAL];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_isp_amount_global (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_ISP_AMOUNT_GLOBAL] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_isp_amount_global () const    
    {    
      return diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_ISP_AMOUNT_GLOBAL];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::requeststatshourly_adv_comm_amount () const    
    {    
      return values[ColoIdBasedStats::REQUESTSTATSHOURLY_ADV_COMM_AMOUNT];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_adv_comm_amount (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_ADV_COMM_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_adv_comm_amount () const    
    {    
      return diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_ADV_COMM_AMOUNT];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::requeststatshourly_pub_comm_amount () const    
    {    
      return values[ColoIdBasedStats::REQUESTSTATSHOURLY_PUB_COMM_AMOUNT];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_pub_comm_amount (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_PUB_COMM_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_pub_comm_amount () const    
    {    
      return diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_PUB_COMM_AMOUNT];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::requeststatshourly_adv_comm_amount_global () const    
    {    
      return values[ColoIdBasedStats::REQUESTSTATSHOURLY_ADV_COMM_AMOUNT_GLOBAL];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_adv_comm_amount_global (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_ADV_COMM_AMOUNT_GLOBAL] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_adv_comm_amount_global () const    
    {    
      return diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_ADV_COMM_AMOUNT_GLOBAL];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::requeststatshourly_pub_comm_amount_global () const    
    {    
      return values[ColoIdBasedStats::REQUESTSTATSHOURLY_PUB_COMM_AMOUNT_GLOBAL];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_pub_comm_amount_global (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_PUB_COMM_AMOUNT_GLOBAL] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_pub_comm_amount_global () const    
    {    
      return diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_PUB_COMM_AMOUNT_GLOBAL];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::requeststatshourly_adv_invoice_comm_amount () const    
    {    
      return values[ColoIdBasedStats::REQUESTSTATSHOURLY_ADV_INVOICE_COMM_AMOUNT];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_adv_invoice_comm_amount (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_ADV_INVOICE_COMM_AMOUNT] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_adv_invoice_comm_amount () const    
    {    
      return diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_ADV_INVOICE_COMM_AMOUNT];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::requeststatshourly_passbacks () const    
    {    
      return values[ColoIdBasedStats::REQUESTSTATSHOURLY_PASSBACKS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_passbacks (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_PASSBACKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::requeststatshourly_passbacks () const    
    {    
      return diffs[ColoIdBasedStats::REQUESTSTATSHOURLY_PASSBACKS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::sitechannelstats_imps () const    
    {    
      return values[ColoIdBasedStats::SITECHANNELSTATS_IMPS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::sitechannelstats_imps (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::SITECHANNELSTATS_IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::sitechannelstats_imps () const    
    {    
      return diffs[ColoIdBasedStats::SITECHANNELSTATS_IMPS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::sitechannelstats_adv_revenue () const    
    {    
      return values[ColoIdBasedStats::SITECHANNELSTATS_ADV_REVENUE];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::sitechannelstats_adv_revenue (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::SITECHANNELSTATS_ADV_REVENUE] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::sitechannelstats_adv_revenue () const    
    {    
      return diffs[ColoIdBasedStats::SITECHANNELSTATS_ADV_REVENUE];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::sitechannelstats_pub_revenue () const    
    {    
      return values[ColoIdBasedStats::SITECHANNELSTATS_PUB_REVENUE];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::sitechannelstats_pub_revenue (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::SITECHANNELSTATS_PUB_REVENUE] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::sitechannelstats_pub_revenue () const    
    {    
      return diffs[ColoIdBasedStats::SITECHANNELSTATS_PUB_REVENUE];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::userpropertystatshourly_requests () const    
    {    
      return values[ColoIdBasedStats::USERPROPERTYSTATSHOURLY_REQUESTS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::userpropertystatshourly_requests (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::USERPROPERTYSTATSHOURLY_REQUESTS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::userpropertystatshourly_requests () const    
    {    
      return diffs[ColoIdBasedStats::USERPROPERTYSTATSHOURLY_REQUESTS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::userpropertystatshourly_imps () const    
    {    
      return values[ColoIdBasedStats::USERPROPERTYSTATSHOURLY_IMPS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::userpropertystatshourly_imps (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::USERPROPERTYSTATSHOURLY_IMPS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::userpropertystatshourly_imps () const    
    {    
      return diffs[ColoIdBasedStats::USERPROPERTYSTATSHOURLY_IMPS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::userpropertystatshourly_clicks () const    
    {    
      return values[ColoIdBasedStats::USERPROPERTYSTATSHOURLY_CLICKS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::userpropertystatshourly_clicks (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::USERPROPERTYSTATSHOURLY_CLICKS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::userpropertystatshourly_clicks () const    
    {    
      return diffs[ColoIdBasedStats::USERPROPERTYSTATSHOURLY_CLICKS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::userpropertystatshourly_actions () const    
    {    
      return values[ColoIdBasedStats::USERPROPERTYSTATSHOURLY_ACTIONS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::userpropertystatshourly_actions (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::USERPROPERTYSTATSHOURLY_ACTIONS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::userpropertystatshourly_actions () const    
    {    
      return diffs[ColoIdBasedStats::USERPROPERTYSTATSHOURLY_ACTIONS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::userpropertystatshourly_imps_unverified () const    
    {    
      return values[ColoIdBasedStats::USERPROPERTYSTATSHOURLY_IMPS_UNVERIFIED];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::userpropertystatshourly_imps_unverified (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::USERPROPERTYSTATSHOURLY_IMPS_UNVERIFIED] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::userpropertystatshourly_imps_unverified () const    
    {    
      return diffs[ColoIdBasedStats::USERPROPERTYSTATSHOURLY_IMPS_UNVERIFIED];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::userpropertystatshourly_profiling_requests () const    
    {    
      return values[ColoIdBasedStats::USERPROPERTYSTATSHOURLY_PROFILING_REQUESTS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::userpropertystatshourly_profiling_requests (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::USERPROPERTYSTATSHOURLY_PROFILING_REQUESTS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::userpropertystatshourly_profiling_requests () const    
    {    
      return diffs[ColoIdBasedStats::USERPROPERTYSTATSHOURLY_PROFILING_REQUESTS];      
    }    
    inline    
    stats_value_type    
    ColoIdBasedStats::tagauctionstats_requests () const    
    {    
      return values[ColoIdBasedStats::TAGAUCTIONSTATS_REQUESTS];      
    }    
    inline    
    DiffStats<ColoIdBasedStats, 54>::Diffs&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::tagauctionstats_requests (const stats_diff_type& value)    
    {    
      diffs[ColoIdBasedStats::TAGAUCTIONSTATS_REQUESTS] = value;      
      return *this;      
    }    
    inline    
    const stats_diff_type&     
    DiffStats<ColoIdBasedStats, 54>::Diffs::tagauctionstats_requests () const    
    {    
      return diffs[ColoIdBasedStats::TAGAUCTIONSTATS_REQUESTS];      
    }    
  }  
}
#endif  // __AUTOTESTS_COMMONS_STATS_COLOIDBASEDSTATS_HPP

