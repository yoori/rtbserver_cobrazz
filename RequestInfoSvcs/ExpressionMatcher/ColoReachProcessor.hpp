#ifndef EXPRESSIONMATCHER_COLOREACHPROCESSOR_HPP
#define EXPRESSIONMATCHER_COLOREACHPROCESSOR_HPP

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <eh/Exception.hpp>
#include <Generics/Time.hpp>

#include <Commons/Algs.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/Algs.hpp>

namespace AdServer
{
namespace RequestInfoSvcs
{
  /** ColoReachProcessor */
  class ColoReachProcessor:
    public virtual ReferenceCounting::Interface
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    struct ColoReachInfo
    {
      Generics::Time create_time;
      bool household;
      AdServer::RequestInfoSvcs::IdAppearanceList colocations;
      AdServer::RequestInfoSvcs::IdAppearanceList ad_colocations;
      AdServer::RequestInfoSvcs::IdAppearanceList merge_colocations;

      bool operator==(const ColoReachInfo& right) const;

      void print(std::ostream& ostr, const char*) const noexcept;
    };

  public:
    /*
      Both function do the same..
        process_gmt_colo_reach - GlobalColoUserStat
        process_isp_request - ColoUserStat
    */
    virtual void process_gmt_colo_reach(
      const ColoReachInfo& request_info)
      /*throw(Exception)*/ = 0;

    virtual void process_isp_colo_reach(
      const ColoReachInfo& request_info)
      /*throw(Exception)*/ = 0;

  protected:
    virtual
    ~ColoReachProcessor() noexcept = default;
  };

  typedef ReferenceCounting::SmartPtr<ColoReachProcessor>
    ColoReachProcessor_var;
}
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  inline
  bool
  ColoReachProcessor::ColoReachInfo::operator==(
    const ColoReachInfo& right) const
  {
    return create_time == right.create_time &&
      colocations.size() == right.colocations.size() &&
      ad_colocations.size() == right.ad_colocations.size() &&
      merge_colocations.size() == right.merge_colocations.size() &&
      std::equal(colocations.begin(),
        colocations.end(),
        right.colocations.begin()) &&
      std::equal(ad_colocations.begin(),
        ad_colocations.end(),
        right.ad_colocations.begin()) &&
      std::equal(merge_colocations.begin(),
        merge_colocations.end(),
        right.merge_colocations.begin());
  }

  inline
  void
  ColoReachProcessor::ColoReachInfo::print(
    std::ostream& ostr, const char* offset) const
    noexcept
  {
    ostr << offset << "create_time: " <<
      create_time.get_gm_time() << std::endl <<
      offset << "colocations: ";
    Algs::print(ostr, colocations.begin(), colocations.end());
    ostr << std::endl;
    ostr << offset << "ad_colocations: ";
    Algs::print(ostr, ad_colocations.begin(), ad_colocations.end());
    ostr << std::endl;
    ostr << offset << "merge_colocations: ";
    Algs::print(ostr, merge_colocations.begin(), merge_colocations.end());
    ostr << std::endl;
  }
}
}

#endif /*EXPRESSIONMATCHER_COLOREACHPROCESSOR_HPP*/
