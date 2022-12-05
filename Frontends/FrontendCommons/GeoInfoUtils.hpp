#ifndef GEOINFOUTILS_HPP__
#define GEOINFOUTILS_HPP__

#include <Commons/CorbaAlgs.hpp>
#include <CampaignSvcs/CampaignManager/CampaignManager.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>
#include "Location.hpp"

namespace FrontendCommons
{
  bool
  fill_geo_location_info(
    AdServer::CampaignSvcs::CampaignManager::GeoInfoSeq& location_seq,
    const Location* location);

  bool
  fill_geo_coord_location_info(
    AdServer::CampaignSvcs::CampaignManager::GeoCoordInfoSeq& coord_location_seq,
    const CoordLocation* coord_location);
}

namespace FrontendCommons
{
  inline
  bool
  fill_geo_location_info(
    AdServer::CampaignSvcs::CampaignManager::GeoInfoSeq& location_seq,
    const Location* location)
  {
    if (location)
    {
      location_seq.length(1);
      location_seq[0].country << location->country;
      location_seq[0].region << location->region;
      location_seq[0].city << location->city;

      return true;
    }

    return false;
  }

  inline
  bool
  fill_geo_coord_location_info(
    AdServer::CampaignSvcs::CampaignManager::GeoCoordInfoSeq& coord_location_seq,
    const CoordLocation* coord_location)
  {
    if (coord_location)
    {
      CORBA::ULong length = coord_location_seq.length();
      coord_location_seq.length(length + 1);

      coord_location_seq[length].longitude =
        CorbaAlgs::pack_decimal(coord_location->longitude);
      coord_location_seq[length].latitude =
        CorbaAlgs::pack_decimal(coord_location->latitude);
      coord_location_seq[length].accuracy =
        CorbaAlgs::pack_decimal(coord_location->accuracy);

      return true;
    }

    return false;
  }
}

#endif /*GEOINFOUTILS_HPP__*/

