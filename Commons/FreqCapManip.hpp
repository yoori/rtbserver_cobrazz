#ifndef _FREQCAPMANIP_HPP_
#define _FREQCAPMANIP_HPP_

#include "FreqCap.hpp"
#include <Commons/FreqCapInfo.hpp>
#include <Commons/CorbaAlgs.hpp>

namespace AdServer
{
  namespace Commons
  {
    inline
    void pack_freq_cap(
      AdServer::Commons::FreqCapInfo& res, const FreqCap& fc)
    {
      res.fc_id = fc.fc_id;
      res.lifelimit = fc.lifelimit;
      res.period = CorbaAlgs::pack_time(fc.period);
      res.window_limit = fc.window_limit;
      res.window_time = CorbaAlgs::pack_time(fc.window_time);
    }

    inline
    void unpack_freq_cap(
      FreqCap& res, const AdServer::Commons::FreqCapInfo& fc)
    {
      res.fc_id = fc.fc_id;
      res.lifelimit = fc.lifelimit;
      res.period = CorbaAlgs::unpack_time(fc.period);
      res.window_limit = fc.window_limit;
      res.window_time = CorbaAlgs::unpack_time(fc.window_time);
    }
  }
}

#endif
