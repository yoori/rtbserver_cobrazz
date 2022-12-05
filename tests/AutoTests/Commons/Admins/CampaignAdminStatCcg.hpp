
#ifndef __AUTOTESTS_COMMONS_ADMINS_CAMPAIGNADMINSTATCCG_HPP
#define __AUTOTESTS_COMMONS_ADMINS_CAMPAIGNADMINSTATCCG_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class CampaignAdminStatCcg:  
    public BaseAdminCmd<CampaignAdminStatCcg, 11>  
  {  
    public:    
      class Expected      
      {      
        friend class CampaignAdminStatCcg;        
        public:        
          Expected() {}          
          Expected& ccg_id (const std::string& val);          
          Expected& ccg_id(unsigned long val);          
          Expected& impressions (const std::string& val);          
          Expected& clicks (const std::string& val);          
          Expected& actions (const std::string& val);          
          Expected& amount (const std::string& val);          
          Expected& comm_amount (const std::string& val);          
          Expected& daily_amount (const std::string& val);          
          Expected& daily_comm_amount (const std::string& val);          
          Expected& creatives (const std::string& val);          
          Expected& publishers (const std::string& val);          
          Expected& tags (const std::string& val);          
                
          enum FieldName          
          {          
            CCG_ID = 0,            
            IMPRESSIONS,            
            CLICKS,            
            ACTIONS,            
            AMOUNT,            
            COMM_AMOUNT,            
            DAILY_AMOUNT,            
            DAILY_COMM_AMOUNT,            
            CREATIVES,            
            PUBLISHERS,            
            TAGS            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<CampaignAdminStatCcg, FIELDS_COUNT> Base;    
        
      void make_cmd (const char* address,      
        const char* id      
      )      
      {      
        address_ = address;        
        AdminParams params;        
        if (id)        
        {        
          params.push_back(AdminParamPair("id=", id));          
        }        
        make_admin_cmd(*this, "stat_ccg", address, params, static_cast<size_t>(CampaignServer));      
      }      
      CampaignAdminStatCcg(      
        const char* address,      
        const char* id      
      )      
      {      
        make_cmd(address, id);      
      }      
      CampaignAdminStatCcg(      
        const std::string& address,      
        const char* id      
      )      
      {      
        make_cmd(address.c_str(), id);      
      }      
      CampaignAdminStatCcg(      
        const char* address,      
        const std::string& id      
      )      
      {      
        make_cmd(address, id.c_str());      
      }      
      CampaignAdminStatCcg(      
        const std::string& address,      
        const std::string& id      
      )      
      {      
        make_cmd(address.c_str(), id.c_str());      
      }      
      CampaignAdminStatCcg(      
        const char* address,      
        unsigned long id      
      )      
      {      
        make_cmd(address, strof(id).c_str());      
      }      
      CampaignAdminStatCcg(      
        const std::string& address,      
        unsigned long id      
      )      
      {      
        make_cmd(address.c_str(), strof(id).c_str());      
      }      
      const char* ccg_id (unsigned int i = 0);      
      const char* impressions (unsigned int i = 0);      
      const char* clicks (unsigned int i = 0);      
      const char* actions (unsigned int i = 0);      
      const char* amount (unsigned int i = 0);      
      const char* comm_amount (unsigned int i = 0);      
      const char* daily_amount (unsigned int i = 0);      
      const char* daily_comm_amount (unsigned int i = 0);      
      const char* creatives (unsigned int i = 0);      
      const char* publishers (unsigned int i = 0);      
      const char* tags (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  CampaignAdminStatCcg::Expected&  
  CampaignAdminStatCcg::Expected::ccg_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(CCG_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdminStatCcg::ccg_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CCG_ID].c_str();  
  }  
  inline  
  CampaignAdminStatCcg::Expected&  
  CampaignAdminStatCcg::Expected::ccg_id(unsigned long val)  
  {  
     values_.insert(std::make_pair(CCG_ID, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  CampaignAdminStatCcg::Expected&  
  CampaignAdminStatCcg::Expected::impressions(const std::string& val)  
  {  
     values_.insert(std::make_pair(IMPRESSIONS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdminStatCcg::impressions (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::IMPRESSIONS].c_str();  
  }  
  inline  
  CampaignAdminStatCcg::Expected&  
  CampaignAdminStatCcg::Expected::clicks(const std::string& val)  
  {  
     values_.insert(std::make_pair(CLICKS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdminStatCcg::clicks (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CLICKS].c_str();  
  }  
  inline  
  CampaignAdminStatCcg::Expected&  
  CampaignAdminStatCcg::Expected::actions(const std::string& val)  
  {  
     values_.insert(std::make_pair(ACTIONS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdminStatCcg::actions (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::ACTIONS].c_str();  
  }  
  inline  
  CampaignAdminStatCcg::Expected&  
  CampaignAdminStatCcg::Expected::amount(const std::string& val)  
  {  
     values_.insert(std::make_pair(AMOUNT, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdminStatCcg::amount (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::AMOUNT].c_str();  
  }  
  inline  
  CampaignAdminStatCcg::Expected&  
  CampaignAdminStatCcg::Expected::comm_amount(const std::string& val)  
  {  
     values_.insert(std::make_pair(COMM_AMOUNT, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdminStatCcg::comm_amount (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::COMM_AMOUNT].c_str();  
  }  
  inline  
  CampaignAdminStatCcg::Expected&  
  CampaignAdminStatCcg::Expected::daily_amount(const std::string& val)  
  {  
     values_.insert(std::make_pair(DAILY_AMOUNT, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdminStatCcg::daily_amount (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::DAILY_AMOUNT].c_str();  
  }  
  inline  
  CampaignAdminStatCcg::Expected&  
  CampaignAdminStatCcg::Expected::daily_comm_amount(const std::string& val)  
  {  
     values_.insert(std::make_pair(DAILY_COMM_AMOUNT, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdminStatCcg::daily_comm_amount (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::DAILY_COMM_AMOUNT].c_str();  
  }  
  inline  
  CampaignAdminStatCcg::Expected&  
  CampaignAdminStatCcg::Expected::creatives(const std::string& val)  
  {  
     values_.insert(std::make_pair(CREATIVES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdminStatCcg::creatives (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CREATIVES].c_str();  
  }  
  inline  
  CampaignAdminStatCcg::Expected&  
  CampaignAdminStatCcg::Expected::publishers(const std::string& val)  
  {  
     values_.insert(std::make_pair(PUBLISHERS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdminStatCcg::publishers (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::PUBLISHERS].c_str();  
  }  
  inline  
  CampaignAdminStatCcg::Expected&  
  CampaignAdminStatCcg::Expected::tags(const std::string& val)  
  {  
     values_.insert(std::make_pair(TAGS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdminStatCcg::tags (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::TAGS].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_CAMPAIGNADMINSTATCCG_HPP

