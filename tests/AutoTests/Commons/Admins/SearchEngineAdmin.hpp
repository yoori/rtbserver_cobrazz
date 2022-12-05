
#ifndef __AUTOTESTS_COMMONS_ADMINS_SEARCHENGINEADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_SEARCHENGINEADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class SearchEngineAdmin:  
    public BaseAdminCmd<SearchEngineAdmin, 5>  
  {  
    public:    
      class Expected      
      {      
        friend class SearchEngineAdmin;        
        public:        
          Expected() {}          
          Expected& id (const std::string& val);          
          Expected& regexp (const std::string& val);          
          Expected& encoding (const std::string& val);          
          Expected& decoding_depth (const std::string& val);          
          Expected& timestamp (const std::string& val);          
                
          enum FieldName          
          {          
            ID = 0,            
            REGEXP,            
            ENCODING,            
            DECODING_DEPTH,            
            TIMESTAMP            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<SearchEngineAdmin, FIELDS_COUNT> Base;    
        
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
        make_admin_cmd(*this, "search_engines", address, params, static_cast<size_t>(CampaignServer));      
      }      
      SearchEngineAdmin(      
        const char* address,      
        const char* id      
      )      
      {      
        make_cmd(address, id);      
      }      
      SearchEngineAdmin(      
        const std::string& address,      
        const char* id      
      )      
      {      
        make_cmd(address.c_str(), id);      
      }      
      SearchEngineAdmin(      
        const char* address,      
        const std::string& id      
      )      
      {      
        make_cmd(address, id.c_str());      
      }      
      SearchEngineAdmin(      
        const std::string& address,      
        const std::string& id      
      )      
      {      
        make_cmd(address.c_str(), id.c_str());      
      }      
      SearchEngineAdmin(      
        const char* address,      
        unsigned long id      
      )      
      {      
        make_cmd(address, strof(id).c_str());      
      }      
      SearchEngineAdmin(      
        const std::string& address,      
        unsigned long id      
      )      
      {      
        make_cmd(address.c_str(), strof(id).c_str());      
      }      
      const char* id (unsigned int i = 0);      
      const char* regexp (unsigned int i = 0);      
      const char* encoding (unsigned int i = 0);      
      const char* decoding_depth (unsigned int i = 0);      
      const char* timestamp (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  SearchEngineAdmin::Expected&  
  SearchEngineAdmin::Expected::id(const std::string& val)  
  {  
     values_.insert(std::make_pair(ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  SearchEngineAdmin::id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::ID].c_str();  
  }  
  inline  
  SearchEngineAdmin::Expected&  
  SearchEngineAdmin::Expected::regexp(const std::string& val)  
  {  
     values_.insert(std::make_pair(REGEXP, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  SearchEngineAdmin::regexp (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::REGEXP].c_str();  
  }  
  inline  
  SearchEngineAdmin::Expected&  
  SearchEngineAdmin::Expected::encoding(const std::string& val)  
  {  
     values_.insert(std::make_pair(ENCODING, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  SearchEngineAdmin::encoding (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::ENCODING].c_str();  
  }  
  inline  
  SearchEngineAdmin::Expected&  
  SearchEngineAdmin::Expected::decoding_depth(const std::string& val)  
  {  
     values_.insert(std::make_pair(DECODING_DEPTH, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  SearchEngineAdmin::decoding_depth (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::DECODING_DEPTH].c_str();  
  }  
  inline  
  SearchEngineAdmin::Expected&  
  SearchEngineAdmin::Expected::timestamp(const std::string& val)  
  {  
     values_.insert(std::make_pair(TIMESTAMP, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  SearchEngineAdmin::timestamp (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::TIMESTAMP].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_SEARCHENGINEADMIN_HPP

