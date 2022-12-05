#include <ReferenceCounting/DefaultImpl.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <CORBACommons/Stats_s.hpp>
#include "ProcessStatsControl.hpp"
#include "ChannelServerImpl.hpp"

namespace AdServer
{
namespace ChannelSvcs
{
  ChannelServerStatsImpl::ChannelServerStatsImpl(
    ChannelServerCustomImpl* delegate) /*throw(eh::Exception)*/ :
    delegate_(ReferenceCounting::add_ref(delegate))
  {
  }

  CORBACommons::StatsValueSeq* ChannelServerStatsImpl::get_stats()
    /*throw(CORBACommons::ProcessStatsControl::ImplementationException)*/
  {
    const char* FUN = "ChannelServerStatsImpl::get_stats";
    CORBACommons::StatsValueSeq_var stats_seq =
      new CORBACommons::StatsValueSeq;
    if(delegate_.in() != 0)
    {
      AdServer::ChannelSvcs::ChannelServerStats stats;
      delegate_->get_stats(stats);
      try
      {
        size_t i;
        stats_seq->length(AdServer::ChannelSvcs::ChannelServerStats::PARAMS_MAX + 2);
        for(i = 0; i < AdServer::ChannelSvcs::ChannelServerStats::PARAMS_MAX;
            i++)
        {
          (*stats_seq)[i].key =
            AdServer::ChannelSvcs::ChannelServerStats::param_name[i];
          (*stats_seq)[i].value <<= (CORBA::ULong)stats.params[i];
        }
        (*stats_seq)[i].key = "Configuration";
        (*stats_seq)[i++].value <<= stats.configuration;
        (*stats_seq)[i].key = "ConfigurationDate";
        (*stats_seq)[i].value <<= stats.configuration_date.gm_ft();
      }
      catch(const eh::Exception &e)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught eh::Exception."
          " : " << e.what();
        CORBACommons::throw_desc<
          CORBACommons::ProcessStatsControl::ImplementationException>(
            ostr.str());
      }
      catch(const CORBA::SystemException& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught CORBA::SystemException."
         " : " << e;
        CORBACommons::throw_desc<
          CORBACommons::ProcessStatsControl::ImplementationException>(
           ostr.str());
      }
    }
    return stats_seq._retn();
  }

}
}
