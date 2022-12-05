
#ifndef __AUTOTESTS_COMMONS_ADMINS_TRIGGERADMIN_HPP
#define __AUTOTESTS_COMMONS_ADMINS_TRIGGERADMIN_HPP

#include "Admins.hpp"

namespace AutoTest
{
  class TriggerAdmin:  
    public BaseAdminCmd<TriggerAdmin, 10>  
  {  
    static const unsigned long DEFAULT_SKIP = 7;  
    public:    
      class Expected      
      {      
        friend class TriggerAdmin;        
        public:        
          Expected() {}          
          Expected& channel_id (const std::string& val);          
          Expected& url (const std::string& val);          
          Expected& url (const ComparableStringList& val);          
          Expected& neg_url (const std::string& val);          
          Expected& page_word (const std::string& val);          
          Expected& page_word (const ComparableStringList& val);          
          Expected& neg_page_word (const std::string& val);          
          Expected& search_word (const std::string& val);          
          Expected& search_word (const ComparableStringList& val);          
          Expected& neg_search_word (const std::string& val);          
          Expected& url_keyword (const std::string& val);          
          Expected& url_keyword (const ComparableStringList& val);          
          Expected& neg_url_keyword (const std::string& val);          
          Expected& stamp (const std::string& val);          
                
          enum FieldName          
          {          
            CHANNEL_ID = 0,            
            URL,            
            NEG_URL,            
            PAGE_WORD,            
            NEG_PAGE_WORD,            
            SEARCH_WORD,            
            NEG_SEARCH_WORD,            
            URL_KEYWORD,            
            NEG_URL_KEYWORD,            
            STAMP            
          };          
        protected:        
          FieldIndexMap values_;          
      };      
        
    public:    
      typedef BaseAdminCmd<TriggerAdmin, FIELDS_COUNT> Base;    
        
      void make_cmd (const char* address,      
        const char* trigger,      
        ChannelSrv service      
      )      
      {      
        address_ = address;        
        AdminParams params;        
        if (trigger)        
        {        
          params.push_back(AdminParamPair("-i", trigger));          
        }        
        make_admin_cmd(*this, "update", address, params, static_cast<size_t>(service));      
      }      
      TriggerAdmin(      
        const char* address,      
        const char* trigger,      
        ChannelSrv service = ChannelManagerController      
      )      
        :Base(DEFAULT_SKIP)      
      {      
        make_cmd(address, trigger, service);      
      }      
      TriggerAdmin(      
        const std::string& address,      
        const char* trigger,      
        ChannelSrv service = ChannelManagerController      
      )      
        :Base(DEFAULT_SKIP)      
      {      
        make_cmd(address.c_str(), trigger, service);      
      }      
      TriggerAdmin(      
        const char* address,      
        const std::string& trigger,      
        ChannelSrv service = ChannelManagerController      
      )      
        :Base(DEFAULT_SKIP)      
      {      
        make_cmd(address, trigger.c_str(), service);      
      }      
      TriggerAdmin(      
        const std::string& address,      
        const std::string& trigger,      
        ChannelSrv service = ChannelManagerController      
      )      
        :Base(DEFAULT_SKIP)      
      {      
        make_cmd(address.c_str(), trigger.c_str(), service);      
      }      
      const char* channel_id (unsigned int i = 0);      
      const char* url (unsigned int i = 0);      
      const char* neg_url (unsigned int i = 0);      
      const char* page_word (unsigned int i = 0);      
      const char* neg_page_word (unsigned int i = 0);      
      const char* search_word (unsigned int i = 0);      
      const char* neg_search_word (unsigned int i = 0);      
      const char* url_keyword (unsigned int i = 0);      
      const char* neg_url_keyword (unsigned int i = 0);      
      const char* stamp (unsigned int i = 0);      
      bool check(const Expected& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
      bool check(const std::string& expected, bool exist = true)      
        /*throw(eh::Exception)*/;      
  };  
    
}
namespace AutoTest
{
  inline  
  TriggerAdmin::Expected&  
  TriggerAdmin::Expected::channel_id(const std::string& val)  
  {  
     values_.insert(std::make_pair(CHANNEL_ID, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TriggerAdmin::channel_id (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::CHANNEL_ID].c_str();  
  }  
  inline  
  TriggerAdmin::Expected&  
  TriggerAdmin::Expected::url(const std::string& val)  
  {  
     values_.insert(std::make_pair(URL, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TriggerAdmin::url (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::URL].c_str();  
  }  
  inline  
  TriggerAdmin::Expected&  
  TriggerAdmin::Expected::url (const ComparableStringList& val)  
  {  
     values_.insert(std::make_pair(URL, val));  
     return *this;  
  }  
  inline  
  TriggerAdmin::Expected&  
  TriggerAdmin::Expected::neg_url(const std::string& val)  
  {  
     values_.insert(std::make_pair(NEG_URL, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TriggerAdmin::neg_url (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::NEG_URL].c_str();  
  }  
  inline  
  TriggerAdmin::Expected&  
  TriggerAdmin::Expected::page_word(const std::string& val)  
  {  
     values_.insert(std::make_pair(PAGE_WORD, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TriggerAdmin::page_word (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::PAGE_WORD].c_str();  
  }  
  inline  
  TriggerAdmin::Expected&  
  TriggerAdmin::Expected::page_word (const ComparableStringList& val)  
  {  
     values_.insert(std::make_pair(PAGE_WORD, val));  
     return *this;  
  }  
  inline  
  TriggerAdmin::Expected&  
  TriggerAdmin::Expected::neg_page_word(const std::string& val)  
  {  
     values_.insert(std::make_pair(NEG_PAGE_WORD, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TriggerAdmin::neg_page_word (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::NEG_PAGE_WORD].c_str();  
  }  
  inline  
  TriggerAdmin::Expected&  
  TriggerAdmin::Expected::search_word(const std::string& val)  
  {  
     values_.insert(std::make_pair(SEARCH_WORD, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TriggerAdmin::search_word (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::SEARCH_WORD].c_str();  
  }  
  inline  
  TriggerAdmin::Expected&  
  TriggerAdmin::Expected::search_word (const ComparableStringList& val)  
  {  
     values_.insert(std::make_pair(SEARCH_WORD, val));  
     return *this;  
  }  
  inline  
  TriggerAdmin::Expected&  
  TriggerAdmin::Expected::neg_search_word(const std::string& val)  
  {  
     values_.insert(std::make_pair(NEG_SEARCH_WORD, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TriggerAdmin::neg_search_word (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::NEG_SEARCH_WORD].c_str();  
  }  
  inline  
  TriggerAdmin::Expected&  
  TriggerAdmin::Expected::url_keyword(const std::string& val)  
  {  
     values_.insert(std::make_pair(URL_KEYWORD, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TriggerAdmin::url_keyword (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::URL_KEYWORD].c_str();  
  }  
  inline  
  TriggerAdmin::Expected&  
  TriggerAdmin::Expected::url_keyword (const ComparableStringList& val)  
  {  
     values_.insert(std::make_pair(URL_KEYWORD, val));  
     return *this;  
  }  
  inline  
  TriggerAdmin::Expected&  
  TriggerAdmin::Expected::neg_url_keyword(const std::string& val)  
  {  
     values_.insert(std::make_pair(NEG_URL_KEYWORD, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TriggerAdmin::neg_url_keyword (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::NEG_URL_KEYWORD].c_str();  
  }  
  inline  
  TriggerAdmin::Expected&  
  TriggerAdmin::Expected::stamp(const std::string& val)  
  {  
     values_.insert(std::make_pair(STAMP, ComparableRegExp(val)));  
     return *this;  
  }  
  inline  
  const char*  
  TriggerAdmin::stamp (unsigned int i)  
  {  
    if (empty() || values_[0].size() != FIELDS_COUNT) return 0;  
    return values_[i][Expected::STAMP].c_str();  
  }  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_TRIGGERADMIN_HPP

