
#ifndef __AUTOTESTS_COMMONS_ADMINS_CAMPAIGNADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_CAMPAIGNADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class CampaignAdmin:  
    public BaseAdminCmd<CampaignAdmin, 23>  
  {  
    public:    
      class Expected      
      {      
        friend class CampaignAdmin;        
        public:        
          Expected(): force_remote_present_(false) {}          
          Expected& id (const std::string& val);          
          Expected& id(unsigned long val);          
          Expected& account_id (const std::string& val);          
          Expected& account_id(unsigned long val);          
          Expected& account_currency_id (const std::string& val);          
          Expected& campaign_group_name (const std::string& val);          
          Expected& channels (const std::string& val);          
          Expected& ecpm (const std::string& val);          
          Expected& ecpm (const Money& val);          
          Expected& delivery_threshold (const std::string& val);          
          Expected& status (const std::string& val);          
          bool has_status () const;          
          std::string status () const;          
          Expected& eval_status (const std::string& val);          
          bool has_eval_status () const;          
          std::string eval_status () const;          
          Expected& timestamp (const std::string& val);          
          Expected& exclude_pub_accounts (const std::string& val);          
          Expected& exclude_tags (const std::string& val);          
          Expected& date_start (const std::string& val);          
          Expected& date_end (const std::string& val);          
          Expected& budget (const std::string& val);          
          Expected& budget (const Money& val);          
          Expected& daily_budget (const std::string& val);          
          Expected& cmp_date_start (const std::string& val);          
          Expected& cmp_date_end (const std::string& val);          
          Expected& max_pub_share (const std::string& val);          
          Expected& channel (const std::string& val);          
          Expected& flags (const std::string& val);          
          Expected& country (const std::string& val);          
          Expected& sites (const std::string& val);          
          Expected& sites (const ComparableStringList& val);          
          Expected& force_remote_present(bool val);          
          bool force_remote_present() const;          
                
          enum FieldName          
          {          
            ID = 0,            
            ACCOUNT_ID,            
            ACCOUNT_CURRENCY_ID,            
            CAMPAIGN_GROUP_NAME,            
            CHANNELS,            
            ECPM,            
            DELIVERY_THRESHOLD,            
            STATUS,            
            EVAL_STATUS,            
            TIMESTAMP,            
            EXCLUDE_PUB_ACCOUNTS,            
            EXCLUDE_TAGS,            
            DATE_START,            
            DATE_END,            
            BUDGET,            
            DAILY_BUDGET,            
            CMP_DATE_START,            
            CMP_DATE_END,            
            MAX_PUB_SHARE,            
            CHANNEL,            
            FLAGS,            
            COUNTRY,            
            SITES            
          };          
        protected:        
          FieldIndexMap values_;          
          bool force_remote_present_;          
      };      
        
    public:    
      typedef BaseAdminCmd<CampaignAdmin, FIELDS_COUNT> Base;    
        
      enum Modificator      
      {      
        EXPAND = 0        
      };      
      void make_cmd (const char* address,      
        const char* campaign      
      )      
      {      
        address_ = address;        
        AdminParams params;        
        if (campaign)        
        {        
          params.push_back(AdminParamPair("id=", campaign));          
        }        
        make_admin_cmd(*this, "campaign", address, params, static_cast<size_t>(CampaignManager));      
        add_cmd_i(modificator_names_[modificator_]);      
      }      
      CampaignAdmin(      
        const char* address,      
        const char* campaign,      
        Modificator mod = EXPAND      
      )      
      {      
        modificator_ = mod;      
        make_cmd(address, campaign);      
      }      
      CampaignAdmin(      
        const std::string& address,      
        const char* campaign,      
        Modificator mod = EXPAND      
      )      
      {      
        modificator_ = mod;      
        make_cmd(address.c_str(), campaign);      
      }      
      CampaignAdmin(      
        const char* address,      
        const std::string& campaign,      
        Modificator mod = EXPAND      
      )      
      {      
        modificator_ = mod;      
        make_cmd(address, campaign.c_str());      
      }      
      CampaignAdmin(      
        const std::string& address,      
        const std::string& campaign,      
        Modificator mod = EXPAND      
      )      
      {      
        modificator_ = mod;      
        make_cmd(address.c_str(), campaign.c_str());      
      }      
      CampaignAdmin(      
        const char* address,      
        unsigned long campaign,      
        Modificator mod = EXPAND      
      )      
      {      
        modificator_ = mod;      
        make_cmd(address, strof(campaign).c_str());      
      }      
      CampaignAdmin(      
        const std::string& address,      
        unsigned long campaign,      
        Modificator mod = EXPAND      
      )      
      {      
        modificator_ = mod;      
        make_cmd(address.c_str(), strof(campaign).c_str());      
      }      
      const char* id (unsigned int i = 0);      
      const char* account_id (unsigned int i = 0);      
      const char* account_currency_id (unsigned int i = 0);      
      const char* campaign_group_name (unsigned int i = 0);      
      const char* channels (unsigned int i = 0);      
      const char* ecpm (unsigned int i = 0);      
      const char* delivery_threshold (unsigned int i = 0);      
      const char* status (unsigned int i = 0);      
      const char* eval_status (unsigned int i = 0);      
      const char* timestamp (unsigned int i = 0);      
      const char* exclude_pub_accounts (unsigned int i = 0);      
      const char* exclude_tags (unsigned int i = 0);      
      const char* date_start (unsigned int i = 0);      
      const char* date_end (unsigned int i = 0);      
      const char* budget (unsigned int i = 0);      
      const char* daily_budget (unsigned int i = 0);      
      const char* cmp_date_start (unsigned int i = 0);      
      const char* cmp_date_end (unsigned int i = 0);      
      const char* max_pub_share (unsigned int i = 0);      
      const char* channel (unsigned int i = 0);      
      const char* flags (unsigned int i = 0);      
      const char* country (unsigned int i = 0);      
      const char* sites (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
    protected:    
      Modificator modificator_;      
      static const char* modificator_names_[1];      
  };  
    
}
namespace AutoTest
{
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::id(const std::string& val)  
  {  
     values_.insert(std::make_pair(ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdmin::id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::ID].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::id(unsigned long val)  
  {  
     values_.insert(std::make_pair(ID, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::account_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(ACCOUNT_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdmin::account_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::ACCOUNT_ID].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::account_id(unsigned long val)  
  {  
     values_.insert(std::make_pair(ACCOUNT_ID, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::account_currency_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(ACCOUNT_CURRENCY_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdmin::account_currency_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::ACCOUNT_CURRENCY_ID].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::campaign_group_name(const std::string& val)  
  {  
     values_.insert(std::make_pair(CAMPAIGN_GROUP_NAME, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdmin::campaign_group_name (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CAMPAIGN_GROUP_NAME].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::channels(const std::string& val)  
  {  
     values_.insert(std::make_pair(CHANNELS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdmin::channels (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CHANNELS].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::ecpm(const std::string& val)  
  {  
     values_.insert(std::make_pair(ECPM, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdmin::ecpm (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::ECPM].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::ecpm (const Money& val)  
  {  
     values_.insert(std::make_pair(ECPM, val));  
     return *this;  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::delivery_threshold(const std::string& val)  
  {  
     values_.insert(std::make_pair(DELIVERY_THRESHOLD, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdmin::delivery_threshold (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::DELIVERY_THRESHOLD].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::status(const std::string& val)  
  {  
     values_.insert(std::make_pair(STATUS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  bool  
  CampaignAdmin::Expected::has_status() const  
  {  
    FieldIndexMap::const_iterator it = values_.find(STATUS);  
    return it != values_.end();  
  }  
  inline  
  std::string  
  CampaignAdmin::Expected::status() const  
  {  
    FieldIndexMap::const_iterator it = values_.find(STATUS);  
    return it->second->str();  
  }  
  inline  
  const char*  
  CampaignAdmin::status (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::STATUS].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::eval_status(const std::string& val)  
  {  
     values_.insert(std::make_pair(EVAL_STATUS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  bool  
  CampaignAdmin::Expected::has_eval_status() const  
  {  
    FieldIndexMap::const_iterator it = values_.find(EVAL_STATUS);  
    return it != values_.end();  
  }  
  inline  
  std::string  
  CampaignAdmin::Expected::eval_status() const  
  {  
    FieldIndexMap::const_iterator it = values_.find(EVAL_STATUS);  
    return it->second->str();  
  }  
  inline  
  const char*  
  CampaignAdmin::eval_status (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::EVAL_STATUS].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::timestamp(const std::string& val)  
  {  
     values_.insert(std::make_pair(TIMESTAMP, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdmin::timestamp (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::TIMESTAMP].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::exclude_pub_accounts(const std::string& val)  
  {  
     values_.insert(std::make_pair(EXCLUDE_PUB_ACCOUNTS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdmin::exclude_pub_accounts (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::EXCLUDE_PUB_ACCOUNTS].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::exclude_tags(const std::string& val)  
  {  
     values_.insert(std::make_pair(EXCLUDE_TAGS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdmin::exclude_tags (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::EXCLUDE_TAGS].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::date_start(const std::string& val)  
  {  
     values_.insert(std::make_pair(DATE_START, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdmin::date_start (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::DATE_START].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::date_end(const std::string& val)  
  {  
     values_.insert(std::make_pair(DATE_END, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdmin::date_end (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::DATE_END].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::budget(const std::string& val)  
  {  
     values_.insert(std::make_pair(BUDGET, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdmin::budget (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::BUDGET].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::budget (const Money& val)  
  {  
     values_.insert(std::make_pair(BUDGET, val));  
     return *this;  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::daily_budget(const std::string& val)  
  {  
     values_.insert(std::make_pair(DAILY_BUDGET, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdmin::daily_budget (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::DAILY_BUDGET].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::cmp_date_start(const std::string& val)  
  {  
     values_.insert(std::make_pair(CMP_DATE_START, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdmin::cmp_date_start (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CMP_DATE_START].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::cmp_date_end(const std::string& val)  
  {  
     values_.insert(std::make_pair(CMP_DATE_END, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdmin::cmp_date_end (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CMP_DATE_END].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::max_pub_share(const std::string& val)  
  {  
     values_.insert(std::make_pair(MAX_PUB_SHARE, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdmin::max_pub_share (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::MAX_PUB_SHARE].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::channel(const std::string& val)  
  {  
     values_.insert(std::make_pair(CHANNEL, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdmin::channel (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CHANNEL].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::flags(const std::string& val)  
  {  
     values_.insert(std::make_pair(FLAGS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdmin::flags (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::FLAGS].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::country(const std::string& val)  
  {  
     values_.insert(std::make_pair(COUNTRY, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdmin::country (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::COUNTRY].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::sites(const std::string& val)  
  {  
     values_.insert(std::make_pair(SITES, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  CampaignAdmin::sites (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SITES].c_str();  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::sites (const ComparableStringList& val)  
  {  
     values_.insert(std::make_pair(SITES, val));  
     return *this;  
  }  
  inline  
  CampaignAdmin::Expected&  
  CampaignAdmin::Expected::force_remote_present(bool val)  
  {  
     force_remote_present_ = val;  
     return *this;  
  }  
  inline  
  bool  
  CampaignAdmin::Expected::force_remote_present() const  
  {  
    return force_remote_present_;  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_CAMPAIGNADMIN_HPP

