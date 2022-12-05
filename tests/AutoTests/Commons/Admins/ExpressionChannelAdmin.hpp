
#ifndef __AUTOTESTS_COMMONS_ADMINS_EXPRESSIONCHANNELADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_EXPRESSIONCHANNELADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class ExpressionChannelAdmin:  
    public BaseAdminCmd<ExpressionChannelAdmin, 6>  
  {  
    public:    
      class Expected      
      {      
        friend class ExpressionChannelAdmin;        
        public:        
          Expected() {}          
          Expected& channel_id (const std::string& val);          
          Expected& channel_id(unsigned long val);          
          Expected& discover_query (const std::string& val);          
          Expected& discover_annotation (const std::string& val);          
          Expected& expression (const std::string& val);          
          Expected& imp_revenue (const std::string& val);          
          Expected& status (const std::string& val);          
          bool has_status () const;          
          std::string status () const;          
                
          enum FieldName          
          {          
            CHANNEL_ID = 0,            
            DISCOVER_QUERY,            
            DISCOVER_ANNOTATION,            
            EXPRESSION,            
            IMP_REVENUE,            
            STATUS            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<ExpressionChannelAdmin, FIELDS_COUNT> Base;    
        
      void make_cmd (const char* address,      
        const char* channel_id      
      )      
      {      
        address_ = address;        
        AdminParams params;        
        if (channel_id)        
        {        
          params.push_back(AdminParamPair("channel_id=", channel_id));          
        }        
        make_admin_cmd(*this, "expression_channel", address, params, static_cast<size_t>(CampaignServer));      
      }      
      ExpressionChannelAdmin(      
        const char* address,      
        const char* channel_id      
      )      
      {      
        make_cmd(address, channel_id);      
      }      
      ExpressionChannelAdmin(      
        const std::string& address,      
        const char* channel_id      
      )      
      {      
        make_cmd(address.c_str(), channel_id);      
      }      
      ExpressionChannelAdmin(      
        const char* address,      
        const std::string& channel_id      
      )      
      {      
        make_cmd(address, channel_id.c_str());      
      }      
      ExpressionChannelAdmin(      
        const std::string& address,      
        const std::string& channel_id      
      )      
      {      
        make_cmd(address.c_str(), channel_id.c_str());      
      }      
      ExpressionChannelAdmin(      
        const char* address,      
        unsigned long channel_id      
      )      
      {      
        make_cmd(address, strof(channel_id).c_str());      
      }      
      ExpressionChannelAdmin(      
        const std::string& address,      
        unsigned long channel_id      
      )      
      {      
        make_cmd(address.c_str(), strof(channel_id).c_str());      
      }      
      const char* channel_id (unsigned int i = 0);      
      const char* discover_query (unsigned int i = 0);      
      const char* discover_annotation (unsigned int i = 0);      
      const char* expression (unsigned int i = 0);      
      const char* imp_revenue (unsigned int i = 0);      
      const char* status (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  ExpressionChannelAdmin::Expected&  
  ExpressionChannelAdmin::Expected::channel_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(CHANNEL_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  ExpressionChannelAdmin::channel_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CHANNEL_ID].c_str();  
  }  
  inline  
  ExpressionChannelAdmin::Expected&  
  ExpressionChannelAdmin::Expected::channel_id(unsigned long val)  
  {  
     values_.insert(std::make_pair(CHANNEL_ID, ComparableRegExp(strof(val))));  
     return *this;  
  }  
  inline  
  ExpressionChannelAdmin::Expected&  
  ExpressionChannelAdmin::Expected::discover_query(const std::string& val)  
  {  
     values_.insert(std::make_pair(DISCOVER_QUERY, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  ExpressionChannelAdmin::discover_query (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::DISCOVER_QUERY].c_str();  
  }  
  inline  
  ExpressionChannelAdmin::Expected&  
  ExpressionChannelAdmin::Expected::discover_annotation(const std::string& val)  
  {  
     values_.insert(std::make_pair(DISCOVER_ANNOTATION, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  ExpressionChannelAdmin::discover_annotation (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::DISCOVER_ANNOTATION].c_str();  
  }  
  inline  
  ExpressionChannelAdmin::Expected&  
  ExpressionChannelAdmin::Expected::expression(const std::string& val)  
  {  
     values_.insert(std::make_pair(EXPRESSION, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  ExpressionChannelAdmin::expression (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::EXPRESSION].c_str();  
  }  
  inline  
  ExpressionChannelAdmin::Expected&  
  ExpressionChannelAdmin::Expected::imp_revenue(const std::string& val)  
  {  
     values_.insert(std::make_pair(IMP_REVENUE, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  ExpressionChannelAdmin::imp_revenue (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::IMP_REVENUE].c_str();  
  }  
  inline  
  ExpressionChannelAdmin::Expected&  
  ExpressionChannelAdmin::Expected::status(const std::string& val)  
  {  
     values_.insert(std::make_pair(STATUS, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  bool  
  ExpressionChannelAdmin::Expected::has_status() const  
  {  
    FieldIndexMap::const_iterator it = values_.find(STATUS);  
    return it != values_.end();  
  }  
  inline  
  std::string  
  ExpressionChannelAdmin::Expected::status() const  
  {  
    FieldIndexMap::const_iterator it = values_.find(STATUS);  
    return it->second->str();  
  }  
  inline  
  const char*  
  ExpressionChannelAdmin::status (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::STATUS].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_EXPRESSIONCHANNELADMIN_HPP

