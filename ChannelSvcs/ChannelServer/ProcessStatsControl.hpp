#ifndef CHANNEL_SVCS_PROCESS_STAT_CONTROL
#define CHANNEL_SVCS_PROCESS_STAT_CONTROL

#include <ReferenceCounting/DefaultImpl.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <eh/Exception.hpp>
#include <CORBACommons/Stats_s.hpp>

#include "ChannelServerImpl.hpp"

namespace AdServer
{
namespace ChannelSvcs
{
  class ChannelServerStatsImpl:
    public virtual CORBACommons::ReferenceCounting::ServantImpl
      <POA_CORBACommons::ProcessStatsControl>
  {
  public:
    ChannelServerStatsImpl(ChannelServerCustomImpl* delegate)
      /*throw(eh::Exception)*/;

    virtual ~ChannelServerStatsImpl() noexcept {};

    virtual CORBACommons::StatsValueSeq* get_stats()
      /*throw(CORBACommons::ProcessStatsControl::ImplementationException)*/;

  private:

    ChannelServerCustomImpl_var delegate_;
  };

  typedef ReferenceCounting::SmartPtr<ChannelServerStatsImpl>
    ChannelServerStatsImpl_var;
}
}

#endif //CHANNEL_SVCS_PROCESS_STAT_CONTROL

