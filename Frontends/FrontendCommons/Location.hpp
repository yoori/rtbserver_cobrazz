#ifndef FRONTENDS_FRONTENDCOMMONS_LOCATION_HPP_
#define FRONTENDS_FRONTENDCOMMONS_LOCATION_HPP_

#include <string>

#include <String/SubString.hpp>
#include <ReferenceCounting/DefaultImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#include <Commons/DecimalUtils.hpp>

namespace FrontendCommons
{
  typedef const String::AsciiStringManip::Char2Category<',', ' '>
    ListParameterSepCategory;
  typedef const String::AsciiStringManip::Char2Category<'/', '%'>
    CountrySepCategory;

  namespace
  {
    CountrySepCategory COUNTRY_SEP_SYMBOLS;
  }

  struct Location: public ReferenceCounting::DefaultImpl<>
  {
    static ReferenceCounting::SmartPtr<Location>
    parse(const String::SubString& value) noexcept;

    static bool
    extract_country(
      const String::SubString& value,
      std::string& country,
      String::SubString::SizeType& country_end,
      bool can_be_encoded = false)
      noexcept;

    void
    normalize() noexcept;

    std::string country;
    std::string region;
    std::string city;

  protected:
    virtual
    ~Location() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<Location> Location_var;

  struct CoordLocation: public ReferenceCounting::DefaultImpl<>
  {
    static ReferenceCounting::SmartPtr<CoordLocation>
    parse(const String::SubString& value) noexcept;

    AdServer::CampaignSvcs::CoordDecimal longitude;
    AdServer::CampaignSvcs::CoordDecimal latitude;
    AdServer::CampaignSvcs::AccuracyDecimal accuracy;
  };

  typedef ReferenceCounting::SmartPtr<CoordLocation> CoordLocation_var;
}

namespace FrontendCommons
{
  inline
  void
  Location::normalize() noexcept
  {
    String::AsciiStringManip::to_lower(country);
    String::AsciiStringManip::to_lower(region);
    String::AsciiStringManip::to_lower(city);
  }

  inline
  bool
  Location::extract_country(
    const String::SubString& value,
    std::string& country,
    String::SubString::SizeType& country_end,
    bool can_be_encoded)
    noexcept
  {
    if(!can_be_encoded)
    {
      country_end = value.find('/');
    }
    else
    {
      country_end = (COUNTRY_SEP_SYMBOLS.find_owned(
        value.begin(), value.end()) - value.begin());
    }
    country.assign(
      value.data(),
      value.data() + (
        country_end != String::SubString::NPOS ? country_end : value.size()));

    return (country.empty() ||
            (country.size() == 2 &&
              String::AsciiStringManip::ALPHA.is_owned(country[0]) &&
              String::AsciiStringManip::ALPHA.is_owned(country[1])));
  }

  inline
  Location_var
  Location::parse(const String::SubString& value) noexcept
  {
    std::string country;
    String::SubString::SizeType country_end;

    if (extract_country(value, country, country_end))
    {
      std::string region;
      std::string city;
      if(country_end != String::SubString::NPOS)
      {
        String::SubString::SizeType region_end = value.find('/', country_end + 1);
        if(region_end != String::SubString::NPOS)
        {
          region.assign(value.data() + country_end + 1, region_end - (country_end + 1));
          city.assign(value.data() + region_end + 1, value.size() - (region_end + 1));
        }
        else
        {
          region.assign(value.data() + country_end + 1);
        }
      }

      Location_var location = new Location();
      location->country.swap(country);
      location->region.swap(region);
      location->city.swap(city);
      location->normalize();
      return location;
    }

    return Location_var();
  }

  inline
  CoordLocation_var
  CoordLocation::parse(const String::SubString& value) noexcept
  {
    // 50km
    static const AdServer::CampaignSvcs::CoordDecimal DEFAULT_ACCURACY("50000");

    static const AdServer::CampaignSvcs::CoordDecimal MIN_LAT("-90");
    static const AdServer::CampaignSvcs::CoordDecimal MAX_LAT("90");
    static const AdServer::CampaignSvcs::CoordDecimal MIN_LON("-180");
    static const AdServer::CampaignSvcs::CoordDecimal MAX_LON("180");
    // accuracy positive integer
    static const AdServer::CampaignSvcs::CoordDecimal MIN_ACCURACY("0"); 
    // equator length ~  40075 km. maximum accuracy - half of equator length.
    static const AdServer::CampaignSvcs::CoordDecimal MAX_ACCURACY("21000000");

    String::SubString::SizeType latitude_end = value.find('/', 0);
    if(latitude_end != String::SubString::NPOS)
    {
      try
      {
        CoordLocation_var coord_location = new CoordLocation();

        String::SubString::SizeType longitude_end = value.find('/', latitude_end + 1);
        coord_location->latitude = AdServer::Commons::extract_decimal<
          AdServer::CampaignSvcs::CoordDecimal>(
            value.substr(0, latitude_end), Generics::DMR_ROUND, true);
        coord_location->longitude = AdServer::Commons::extract_decimal<
          AdServer::CampaignSvcs::CoordDecimal>(
            value.substr(latitude_end + 1, longitude_end - latitude_end - 1),
            Generics::DMR_ROUND, true);

        if ((coord_location->latitude < MIN_LAT) ||
            (coord_location->latitude > MAX_LAT) ||
            (coord_location->longitude < MIN_LON) ||
            (coord_location->longitude > MAX_LON))
        {
          return CoordLocation_var();
        }

        if(longitude_end != String::SubString::NPOS)
        {
          coord_location->accuracy = AdServer::Commons::extract_decimal<
            AdServer::CampaignSvcs::AccuracyDecimal>(
              value.substr(longitude_end + 1, String::SubString::NPOS));

          if (coord_location->accuracy <= MIN_ACCURACY)
          {
            return CoordLocation_var();
          }

          if (coord_location->accuracy > MAX_ACCURACY)
          {
            coord_location->accuracy = MAX_ACCURACY;
          }
        }
        else
        {
          coord_location->accuracy = DEFAULT_ACCURACY;
        }

        return coord_location;
      }
      catch(const eh::Exception&)
      {//ignory bad coordinates
      }
    }

    return CoordLocation_var();
  }
}

#endif /* FRONTENDS_FRONTENDCOMMONS_LOCATION_HPP_ */
