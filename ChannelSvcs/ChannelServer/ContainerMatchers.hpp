
#ifndef AD_SERVER_CONTAINER_MATCHERS_HPP
#define AD_SERVER_CONTAINER_MATCHERS_HPP

#include <vector>
#include <ostream>
#include <set>
#include <eh/Exception.hpp>
#include <ChannelSvcs/ChannelCommons/CommonTypes.hpp>

namespace AdServer
{
namespace ChannelSvcs
{
  typedef std::set<unsigned int> ExcludeContainerType;

  class PositiveAtom
  {
  public:
    PositiveAtom(unsigned int c_id = 0, unsigned int t_id = 0) noexcept
      : channel_id(c_id), channel_trigger_id(t_id) {};
    bool operator<(const PositiveAtom& cp) const noexcept
    { 
      if(channel_id != cp.channel_id)
      {
        return channel_id < cp.channel_id;
      }
      return channel_trigger_id < cp.channel_trigger_id;
    }
    bool operator==(unsigned int id) const noexcept { return channel_id == id; }
    bool operator==(const PositiveAtom& id) const noexcept;
  public:
    unsigned int channel_id;
    unsigned int channel_trigger_id;
  };

  typedef std::vector<PositiveAtom> PositiveContainerType;

}// namespace ChannelSvcs
}// namespace AdServer

namespace AdServer
{
  namespace ChannelSvcs
  {

  inline
  bool PositiveAtom::operator==(const PositiveAtom& id) const noexcept
  {
    return channel_id == id.channel_id &&
      channel_trigger_id == id.channel_trigger_id;
  }

  inline
  bool operator<(const PositiveAtom& cp1, unsigned int cp2) noexcept
  {
    return cp1.channel_id < cp2;
  }

  inline
  bool operator<(unsigned int cp1, const PositiveAtom& cp2) noexcept
  {
    return cp1 < cp2.channel_id;
  }

  }// namespace ChannelSvcs
}// namespace AdServer

inline
std::ostream& operator<<(std::ostream& os, const AdServer::ChannelSvcs::PositiveAtom& atom) noexcept
{
  os << atom.channel_id << '(' << atom.channel_trigger_id << ")";
  return os;
}

#endif //AD_SERVER_CONTAINER_MATCHERS_HPP
