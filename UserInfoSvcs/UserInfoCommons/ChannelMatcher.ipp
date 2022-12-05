#ifndef _USER_INFO_SVCS_CHANNEL_MATCHER_IPP_
#define _USER_INFO_SVCS_CHANNEL_MATCHER_IPP_

namespace AdServer
{
namespace UserInfoSvcs
{
  inline
  void ChannelsMatcher::delete_excess_timestamps_(
    SessionMatchesWriter::timestamps_Container& wr,
    const ChannelIntervalList& cil) noexcept
  {
    unsigned long max_vis = cil.max_visits();
    unsigned long min_window_size = cil.min_window_size();
      
    if(max_vis == 0)
    {
      wr.clear();
      return;
    }

    SessionMatchesWriter::timestamps_Container::
      iterator window_end_it = wr.begin();
    SessionMatchesWriter::timestamps_Container::
      iterator window_begin_it = window_end_it;
    SessionMatchesWriter::timestamps_Container::
      iterator erase_candidate_it = window_end_it;
    unsigned long window_imps = 0;
  
    for(; window_end_it != wr.end(); ++window_end_it, ++window_imps)
    {
      while(static_cast<unsigned long>(*window_end_it - *window_begin_it) >
            min_window_size)
      {
        // offset window begin
        --window_imps;
        ++window_begin_it;
        erase_candidate_it = window_begin_it;
      }

      if(window_imps > 2*max_vis) // > max_vis without partly match
      {
        ++erase_candidate_it;
      }
    
      if(window_imps >= 4*max_vis) // >= 2*max_vis without partly match
      {
        // clear middle element
        wr.erase(erase_candidate_it--);
        --window_imps;
      }
    }
  }
}
}

#endif /*_USER_INFO_SVCS_CHANNEL_MATCHER_IPP_*/
