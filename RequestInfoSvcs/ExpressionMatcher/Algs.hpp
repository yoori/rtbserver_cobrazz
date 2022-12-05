#ifndef EXPRESSIONMATCHER__ALGS_HPP
#define EXPRESSIONMATCHER__ALGS_HPP

#include <Generics/Time.hpp>
#include "ColoReachProcessor.hpp"

namespace AdServer
{
namespace RequestInfoSvcs
{
  typedef std::list<ColoReachProcessor::ColoReachInfo> ColoReachInfoList;

  template <typename ReaderIteratorType>
  void
  split_colo_reach_info_by_id(
    ColoReachInfoList& isp_colo_reach_info_list,
    const ColoReachProcessor::ColoReachInfo& collected_data,
    const unsigned long colo_id,
    bool household,
    ReaderIteratorType begin,
    ReaderIteratorType end,
    bool update_only_passed_id)
  {
    std::map<unsigned long, Generics::Time> isp_create_time_map;
    for(ReaderIteratorType it = begin; it != end; ++it)
    {
      if (update_only_passed_id && ((*it).id() != colo_id))
      {
        continue;
      }
      isp_create_time_map[(*it).id()] =
        Generics::Time((*it).create_time());
    }

    std::map<unsigned long, ColoReachProcessor::ColoReachInfo*>
       colo_reach_info_map;

    // Suppose no possible colo_id == ULONG_MAX
    unsigned long prev_id = ULONG_MAX;
    typedef AdServer::RequestInfoSvcs::IdAppearanceList IdAppearanceList;
    for(IdAppearanceList::const_iterator it =
          collected_data.colocations.begin();
          it != collected_data.colocations.end(); ++it)
    {
      if (update_only_passed_id && (it->id != colo_id))
      {
        continue;
      }

      if (it->id != prev_id)
      {
        prev_id = it->id;
        isp_colo_reach_info_list.push_back(
          ColoReachProcessor::ColoReachInfo());

        colo_reach_info_map[prev_id] = &(isp_colo_reach_info_list.back());

        colo_reach_info_map[prev_id]->household = household;

        if (isp_create_time_map.count(prev_id))
        {
          colo_reach_info_map[prev_id]->create_time =
            Algs::round_to_day(isp_create_time_map[prev_id]);
        }
        else
        {
          // Smth goes wrong... throw Exception?
        }
      }

      colo_reach_info_map[prev_id]->colocations.push_back(*it);
    }

    // Suppose ad_colocations and merge_colocations subset of colocations,
    //   so all id possible in ad_colocations and merge_colocations
    //   already present in colo_reach_info_map.
    typedef AdServer::RequestInfoSvcs::IdAppearanceList IdAppearanceList;
    for(IdAppearanceList::const_iterator it =
          collected_data.ad_colocations.begin();
          it != collected_data.ad_colocations.end(); ++it)
    {
      if (update_only_passed_id && (it->id != colo_id))
      {
        continue;
      }

      if (colo_reach_info_map.count(it->id))
      {
        colo_reach_info_map[it->id]->ad_colocations.push_back(*it);
      }
      else
      {
        // found id which not presented. throw Exception?
      }
    }

    for(IdAppearanceList::const_iterator it =
          collected_data.merge_colocations.begin();
          it != collected_data.merge_colocations.end(); ++it)
    {
      if (update_only_passed_id && (it->id != colo_id))
      {
        continue;
      }

      if (colo_reach_info_map.count(it->id))
      {
        colo_reach_info_map[it->id]->merge_colocations.push_back(*it);
      }
      else
      {
        // found id which not presented. throw Exception?
      }
    }
  }
}
}
#endif

