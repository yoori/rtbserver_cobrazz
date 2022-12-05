#ifndef EXPRESSIONMATCHER_CHANNELOVERLAPREACHPROCESSOR_HPP
#define EXPRESSIONMATCHER_CHANNELOVERLAPREACHPROCESSOR_HPP

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <eh/Exception.hpp>
#include <Generics/Time.hpp>

#include <Commons/Algs.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/Algs.hpp>

namespace AdServer
{
namespace RequestInfoSvcs
{
  /** ChannelOverlapReachProcessor */
  struct ChannelOverlapReachProcessor:
    public virtual ReferenceCounting::Interface
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    struct ChannelOverlapReachInfo
    {
      struct Group
      {
        ChannelIdSet appeared_channels;
        ChannelIdSet overlap_channels;

        bool operator==(const Group& right) const;
      };

      // country_code -> Group map
      typedef std::map<std::string, Group> GroupMap;

      Generics::Time time;
      GroupMap channel_groups;

      bool operator==(const ChannelOverlapReachInfo& right) const;

      void print(std::ostream& ostr, const char*) const noexcept;
    };

  public:
    virtual void process_channel_overlap_reach(
      const ChannelOverlapReachInfo& request_info)
      /*throw(Exception)*/ = 0;
  };

  typedef ReferenceCounting::SmartPtr<ChannelOverlapReachProcessor>
    ChannelOverlapReachProcessor_var;
}
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  inline
  bool
  ChannelOverlapReachProcessor::ChannelOverlapReachInfo::operator==(
    const ChannelOverlapReachInfo& right) const
  {
    return time == right.time && channel_groups == right.channel_groups;
  }

  inline
  bool
  ChannelOverlapReachProcessor::ChannelOverlapReachInfo::Group::operator==(
    const ChannelOverlapReachProcessor::ChannelOverlapReachInfo::Group& right)
    const
  {
    return appeared_channels.size() == right.appeared_channels.size() &&
      overlap_channels.size() == right.overlap_channels.size() &&
      std::equal(appeared_channels.begin(),
        appeared_channels.end(),
        right.appeared_channels.begin()) &&
      std::equal(overlap_channels.begin(),
        overlap_channels.end(),
        right.overlap_channels.begin());
  }

  inline
  void
  ChannelOverlapReachProcessor::ChannelOverlapReachInfo::print(
    std::ostream& ostr, const char* offset) const
    noexcept
  {
    ostr << offset << "time: " <<
      time.get_gm_time() << std::endl;
    ostr << offset << "channel_groups: " << std::endl;
    for(GroupMap::const_iterator group_it = channel_groups.begin();
        group_it != channel_groups.end(); ++group_it)
    {
      ostr << offset << "  " << group_it->first << ": (";
      Algs::print(ostr,
        group_it->second.appeared_channels.begin(),
        group_it->second.appeared_channels.end());
      ostr << "), (";
      Algs::print(ostr,
        group_it->second.overlap_channels.begin(),
        group_it->second.overlap_channels.end());
      ostr << ")" << std::endl;
    }
  }
}
}

#endif /*EXPRESSIONMATCHER_CHANNELOVERLAPREACHPROCESSOR_HPP*/
