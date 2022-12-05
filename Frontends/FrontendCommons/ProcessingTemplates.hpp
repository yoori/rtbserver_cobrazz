
#ifndef _FRONTENDCOMMONS_PROCCESSING_TEMPLATES_HPP_
#define _FRONTENDCOMMONS_PROCCESSING_TEMPLATES_HPP_

#include <string>
#include <map>

#include <eh/Exception.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <String/TextTemplate.hpp>
#include <Generics/Hash.hpp>
#include <HTTP/Http.hpp>
#include <HTTP/HTTPCookie.hpp>

#include <Frontends/FrontendCommons/HTTPUtils.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#include "FCGI.hpp"

namespace FrontendCommons
{
  typedef std::map<std::string, std::string> TokenValueMap;

  class PassbackTemplateKey
  {
  public:
    PassbackTemplateKey(
      const char* request_app_format,
      const char* passback_app_format)
      noexcept
      : request_app_format_(request_app_format),
        passback_app_format_(passback_app_format),
        hash_(0)
    {
      Generics::Murmur64Hash hasher(hash_);
      hash_add(hasher, request_app_format_);
      hash_add(hasher, passback_app_format_);
    }

    bool operator==(const PassbackTemplateKey& right) const
    {
      return request_app_format_ == right.request_app_format_ &&
        passback_app_format_ == right.passback_app_format_;
    }

    size_t hash() const
    {
      return hash_;
    }

  protected:
    const std::string request_app_format_;
    const std::string passback_app_format_;
    size_t hash_;
  };

  struct PassbackProcessingTemplate: public ReferenceCounting::AtomicImpl
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    PassbackProcessingTemplate(
      const char* file,
      const char* content_type_val)
      /*throw(Exception)*/;

    int
    instantiate(
      FCGI::HttpResponse& response,
      const char* url,
      AdServer::CampaignSvcs::UserStatus user_status,
      bool use_pub_pixels,
      unsigned long random,
      const TokenValueMap* args)
      /*throw(eh::Exception)*/;

    String::TextTemplate::IStream text_template;
    const std::string content_type;

  protected:
    virtual ~PassbackProcessingTemplate() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<PassbackProcessingTemplate>
    PassbackProcessingTemplate_var;

  typedef Generics::GnuHashTable<
    FrontendCommons::PassbackTemplateKey,
    FrontendCommons::PassbackProcessingTemplate_var>
    PassbackProcessingTemplateMap;
}


#endif //_FRONTENDCOMMONS_PROCCESSING_TEMPLATES_HPP_ 

