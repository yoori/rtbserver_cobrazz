/// @file RequestInfoManagerStatsAgent.hpp

#ifndef REQUEST_INFO_MANAGER_STATS_AGENT_INCLUDED
#define REQUEST_INFO_MANAGER_STATS_AGENT_INCLUDED

#include <SNMPAgent/SNMPAgentX.hpp>

#include "RequestInfoManagerStats.hpp"


namespace AdServer
{
  namespace RequestInfoSvcs
  {
    class Processor
    {
    public:
      explicit
      Processor(unsigned id)
        noexcept;

      void
      register_ids(SNMPAgentX::GenericSNMPAgent* agent) const
        /*throw(eh::Exception)*/;

      bool
      process_variable(void* variable,
        const SNMPAgentX::GenericSNMPAgent::VariableInfo& info,
        unsigned size, const unsigned* ids,
        const RequestInfoManagerStatsImpl* values) const
        /*throw(eh::Exception)*/;

    protected:
      /// Will be foros_suffix from config
      unsigned id_;
    };

    typedef SNMPAgentX::SNMPStatsGen<RequestInfoManagerStatsImpl, Processor>
      SNMPStatsImpl;
    typedef ReferenceCounting::QualPtr<SNMPStatsImpl> SNMPStatsImpl_var;
  } // namespace RequestInfoSvcs
} // namespace AdServer

//
// INLINES
//

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    inline
    Processor::Processor(unsigned id) noexcept
      : id_(id)
    {
    }

    inline
    void
    Processor::register_ids(SNMPAgentX::GenericSNMPAgent* agent) const
      /*throw(eh::Exception)*/
    {
      if (const SNMPAgentX::GenericSNMPAgent::RootInfo* root =
        agent->get_rootinfo("logStatsTable.logStatsEntry"))
      {
        for (unsigned i = 0; i < StatNames::LOG_COUNT; ++i)
        {
          unsigned ids[] = {i, id_};
          root->register_index(2, ids);
        }
      }
    }

    inline
    bool
    Processor::process_variable(void* variable,
      const SNMPAgentX::GenericSNMPAgent::VariableInfo& info,
      unsigned /*size*/, const unsigned* ids,
      const RequestInfoManagerStatsImpl* values) const
      /*throw(eh::Exception)*/
    {
      return SNMPAgentX::GenericSNMPAgent::set_variable_from_values(
        variable, info, *values->get_log_values(*ids));
    }
  } //  namespace RequestInfoSvcs
} // namespace AdServer

#endif // REQUEST_INFO_MANAGER_STATS_AGENT_INCLUDED
