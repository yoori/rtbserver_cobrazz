#ifndef TAGREQUESTPROCESSOR_HPP
#define TAGREQUESTPROCESSOR_HPP

#include <optional>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/Time.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/Containers.hpp>
#include <Commons/StringHolder.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <LogCommons/LogCommons.hpp>

/**
 * TagRequestProcessor
 */
namespace AdServer
{
namespace RequestInfoSvcs
{
  struct TagRequestInfo
  {
    Generics::Time time;
    unsigned long colo_id;
    unsigned long tag_id;
    unsigned long size_id;
    std::string ext_tag_id;
    char user_status;
    std::string referer;
    AdServer::LogProcessing::StringList urls;
    std::optional<unsigned long> referer_hash;

    Generics::Time isp_time;
    std::string country;
    unsigned long site_id;
    Commons::UserId user_id;
    std::optional<unsigned long> page_load_id;
    bool ad_shown;
    bool profile_referer;
    AdServer::CampaignSvcs::RevenueDecimal floor_cost;

    AdServer::Commons::RequestId request_id; // passback request id
    Commons::StringHolder_var user_agent;

    template<typename OStream>
    void print(OStream& out, const char* offset) const noexcept;
  };

  /**
   * TagRequestProcessor
   */
  class TagRequestProcessor:
    public virtual ReferenceCounting::Interface
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    virtual void process_tag_request(const TagRequestInfo&) /*throw(Exception)*/ = 0;

  protected:
    virtual ~TagRequestProcessor() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<TagRequestProcessor>
    TagRequestProcessor_var;
}
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  template<typename OStream>
  void TagRequestInfo::
  print(OStream& out, const char* offset) const noexcept
  {
    out << offset << "user_id = " << user_id.to_string() << std::endl <<
      offset << "time = " << time.get_gm_time() << std::endl <<
      offset << "isp_time = " << isp_time.get_gm_time() << std::endl <<
      offset << "site_id = " << site_id << std::endl <<
      offset << "colo_id = " << colo_id << std::endl <<
      offset << "tag_id = " << tag_id << std::endl <<
      offset << "ext_tag_id = " << ext_tag_id << std::endl <<
      offset << "user_status = '" << user_status << "'" << std::endl <<
      offset << "referer = '" << referer << "'" << std::endl <<
      offset << "page_load_id = ";
    if(page_load_id)
    {
      out << *page_load_id;
    }
    else
    {
      out << "null";
    }
    out  << std::endl <<
      offset << "country = " << country << std::endl <<
      offset << "ad_shown = " << ad_shown << std::endl <<
      offset << "profile_referer = " << profile_referer << std::endl;
  }
}
}

#endif /*TAGREQUESTPROCESSOR_HPP*/
