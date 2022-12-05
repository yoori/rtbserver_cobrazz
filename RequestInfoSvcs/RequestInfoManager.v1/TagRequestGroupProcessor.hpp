#ifndef REQUESTINFOSVCS_REQUESTINFOMANAGER_TAGREQUESTGROUPPROCESSOR_HPP
#define REQUESTINFOSVCS_REQUESTINFOMANAGER_TAGREQUESTGROUPPROCESSOR_HPP

#include <string>
#include <set>

#include <eh/Exception.hpp>
#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/Time.hpp>
#include <Commons/Algs.hpp>

namespace AdServer
{
namespace RequestInfoSvcs
{
  struct TagRequestGroupProcessor: public virtual ReferenceCounting::Interface
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    static const Generics::Time DEFAULT_EXPIRE_TIME; // 1 day

    struct TagRequestGroupInfo
    {
      typedef std::multiset<unsigned long> TagIdSet;

      // verified passback info
      TagRequestGroupInfo(): colo_id(0), site_id(0) {}

      TagRequestGroupInfo(
        const Generics::Time& time_val,
        unsigned long colo_id_val,
        unsigned long site_id_val,
        const char* country_val,
        const TagIdSet& tags_val,
        bool ad_shown_val,
        bool rollback_val)
        : time(time_val),
          colo_id(colo_id_val),
          site_id(site_id_val),
          country(country_val),
          tags(tags_val),
          ad_shown(ad_shown_val),
          rollback(rollback_val)
      {}

      bool operator==(const TagRequestGroupInfo& right) const
      {
        return time == right.time &&
          colo_id == right.colo_id &&
          site_id == right.site_id &&
          country == right.country &&
          ad_shown == right.ad_shown &&
          rollback == right.rollback &&
          tags.size() == right.tags.size() &&
          std::equal(tags.begin(), tags.end(), right.tags.begin());
      }

      std::ostream& print(std::ostream& ostr, const char* prefix) const
      {
        ostr << prefix << "time = " << time.get_gm_time() << std::endl <<
          prefix << "country = " << country << std::endl <<
          prefix << "colo_id = " << colo_id << std::endl <<
          prefix << "site_id = " << site_id << std::endl <<
          prefix << "ad_shown = " << ad_shown << std::endl <<
          prefix << "rollback = " << rollback << std::endl <<
          prefix << "tags = ";
        Algs::print(ostr, tags.begin(), tags.end());
        ostr << std::endl;
        return ostr;
      }

      Generics::Time time;
      unsigned long colo_id;
      unsigned long site_id;
      std::string country;
      TagIdSet tags;
      bool ad_shown;
      bool rollback;
    };

    typedef std::list<TagRequestGroupProcessor::TagRequestGroupInfo>
      TagRequestGroupInfoList;

    virtual void
    process_tag_request_group(const TagRequestGroupInfo& tag_request_group_info)
      /*throw(Exception)*/ = 0;

  protected:
    virtual ~TagRequestGroupProcessor() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<TagRequestGroupProcessor>
    TagRequestGroupProcessor_var;
}
}

#endif /*REQUESTINFOSVCS_REQUESTINFOMANAGER_TAGREQUESTGROUPPROCESSOR_HPP*/
