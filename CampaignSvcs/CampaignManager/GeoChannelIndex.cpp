#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#include <boost/geometry/index/predicates.hpp>
#include <boost/geometry/index/rtree.hpp>

#include "GeoChannelIndex.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  namespace bg = boost::geometry;
  namespace bgi = boost::geometry::index;
  namespace bgm = bg::model;

  /*
  template<typename BaseType>
  class DecimalAdapter: public BaseType
  {
  public:
    DecimalAdapter()
    {}

    DecimalAdapter(double val)
      : BaseType(val)
    {}

    DecimalAdapter(const BaseType& init)
      : BaseType(init)
    {}
  };
  */

  class GeoCoordChannelIndex::CoordIndexImpl
  {
  public:
    //typedef DecimalAdapter<CoordDecimal> CoordDecimalAdapter;

    void
    add(const Key& key, Value* value)
      noexcept;

    void
    match(ChannelIdSet& result_channels,
      const CoordDecimal& longitude,
      const CoordDecimal& latitude,
      const AccuracyDecimal& accuracy)
      const
      noexcept;

  protected:
    typedef bg::model::point<
      double, 2, bg::cs::spherical_equatorial<bg::degree> > GeoPoint;
    typedef bg::model::box<GeoPoint> GeoBox;

    struct GeoCoordChannelHolder
    {
      GeoCoordChannelHolder()
      {}

      GeoCoordChannelHolder(
        double longitude_val,
        double latitude_val,
        double accuracy_val,
        Value* value_val)
        : point(longitude_val, latitude_val),
          accuracy(accuracy_val),
          value(ReferenceCounting::add_ref(value_val))
      {}

      GeoPoint point;
      double accuracy;
      Value_var value;
    };

    typedef std::pair<GeoBox, GeoCoordChannelHolder> ValuePair;
    typedef bgi::rtree<ValuePair, bgi::rstar<16> > GeoMap;

    class MatchCollector:
      public std::iterator<std::output_iterator_tag, void, void, void, void>
    {
    public:
      MatchCollector(
        ChannelIdSet& result_channels,
        const GeoPoint& point,
        double accuracy)
        : result_channels_(result_channels),
          point_(point),
          accuracy_(accuracy)
      {}

      MatchCollector&
      operator++()
      {
        return *this;
      }

      MatchCollector&
      operator++(int)
      {
        return *this;
      }

      MatchCollector& operator*()
      {
        return *this;
      }

      MatchCollector&
      operator=(const ValuePair& value)
      {
        // final check of point
        if(CoordIndexImpl::channel_check_(
             value,
             point_,
             accuracy_))
        {
          std::copy(
            value.second.value->channels.begin(),
            value.second.value->channels.end(),
            std::inserter(result_channels_, result_channels_.begin()));
        }

        return *this;
      }

    protected:
      ChannelIdSet& result_channels_;
      const GeoPoint point_;
      const double accuracy_;
    };

  protected:
    static bool
    channel_check_(
      const ValuePair& value_pair,
      const GeoPoint& point,
      double accuracy)
      noexcept;

    static GeoCoordChannelIndex::CoordIndexImpl::GeoBox
    cover_box_(
      const CoordDecimal& longitude,
      const CoordDecimal& latitude,
      const AccuracyDecimal& accuracy)
      noexcept;

    static double
    coord_to_double_(const CoordDecimal& coord_decimal)
      noexcept;

  protected:
    static const double EARTH_RADIUS_M_DOUBLE;
    static const AccuracyDecimal EARTH_RADIUS_M;
    static const AccuracyDecimal EARTH_M_TO_ANGLE_MULTIPLYER;
    static const CoordDecimal ANGLE_MULTIPLYER;
    static const double TRUST_PROBABILITY;
    GeoMap geo_map_;
  };

  const double
  GeoCoordChannelIndex::CoordIndexImpl::TRUST_PROBABILITY = 0.8;

  const double
  GeoCoordChannelIndex::CoordIndexImpl::EARTH_RADIUS_M_DOUBLE = 6368500.0;

  const AccuracyDecimal
  GeoCoordChannelIndex::CoordIndexImpl::EARTH_RADIUS_M("6368500");

  const AccuracyDecimal
  GeoCoordChannelIndex::CoordIndexImpl::ANGLE_MULTIPLYER(
    360 / (2 * boost::math::constants::pi<double>()));

  const AccuracyDecimal
  GeoCoordChannelIndex::CoordIndexImpl::EARTH_M_TO_ANGLE_MULTIPLYER =
    AccuracyDecimal::div(
      ANGLE_MULTIPLYER, EARTH_RADIUS_M, Generics::DDR_CEIL);

  void
  GeoCoordChannelIndex::CoordIndexImpl::add(
    const Key& key,
    Value* value)
    noexcept
  {
    // fill covering box, convert accuracy to angle
    geo_map_.insert(std::make_pair(
      cover_box_(key.longitude, key.latitude, key.accuracy),
      GeoCoordChannelHolder(
        coord_to_double_(key.longitude),
        coord_to_double_(key.latitude),
        coord_to_double_(key.accuracy),
        value)));
  }

  void
  GeoCoordChannelIndex::CoordIndexImpl::match(
    ChannelIdSet& result_channels,
    const CoordDecimal& longitude,
    const CoordDecimal& latitude,
    const AccuracyDecimal& accuracy) const
    noexcept
  {
    GeoPoint point(coord_to_double_(longitude), coord_to_double_(latitude));
    GeoBox cover_box = cover_box_(longitude, latitude, accuracy);
    geo_map_.query(
      bgi::intersects(cover_box),
      MatchCollector(
        result_channels,
        point,
        coord_to_double_(accuracy)));
  }

  GeoCoordChannelIndex::CoordIndexImpl::GeoBox
  GeoCoordChannelIndex::CoordIndexImpl::cover_box_(
    const CoordDecimal& longitude,
    const CoordDecimal& latitude,
    const AccuracyDecimal& accuracy)
    noexcept
  {
    CoordDecimal angle_delta = AccuracyDecimal::div(
      AccuracyDecimal::mul(accuracy, ANGLE_MULTIPLYER, Generics::DMR_CEIL),
      EARTH_RADIUS_M,
      Generics::DDR_CEIL);

    return GeoBox(
      GeoPoint(
        coord_to_double_(longitude - angle_delta),
        coord_to_double_(latitude - angle_delta)),
      GeoPoint(
        coord_to_double_(longitude + angle_delta),
        coord_to_double_(latitude + angle_delta)));
  }

  double
  GeoCoordChannelIndex::CoordIndexImpl::coord_to_double_(
    const CoordDecimal& coord)
    noexcept
  {
    double res = coord.floating<double>();
    return res;
  }

  bool
  GeoCoordChannelIndex::CoordIndexImpl::channel_check_(
    const ValuePair& value_pair,
    const GeoPoint& point,
    double accuracy)
    noexcept
  {
    double distance = boost::geometry::distance(
      value_pair.second.point, point) *
      EARTH_RADIUS_M_DOUBLE;
    double distance_sqr = distance * distance;

    if(distance < accuracy + value_pair.second.accuracy)
    {
      double cross_s;
      double accuracy_sqr = accuracy * accuracy;
      double right_accuracy_sqr = value_pair.second.accuracy * value_pair.second.accuracy;

      if(distance <= std::numeric_limits<double>::epsilon())
      {
        cross_s = std::min(accuracy_sqr, right_accuracy_sqr) *
          boost::math::constants::pi<double>();
      }
      else if(distance + accuracy <= value_pair.second.accuracy)
      {
        cross_s = accuracy_sqr * boost::math::constants::pi<double>();
      }
      else if(distance + value_pair.second.accuracy <= accuracy)
      {
        cross_s = right_accuracy_sqr * boost::math::constants::pi<double>();
      }
      else
      {
        double f1 = std::acos(
          (accuracy_sqr - right_accuracy_sqr + distance_sqr) / (2 * accuracy * distance));
        double f2 = std::acos(
          (right_accuracy_sqr - accuracy_sqr + distance_sqr) / (2 * value_pair.second.accuracy * distance));
        double s1 = accuracy_sqr * f1;
        double s2 = right_accuracy_sqr * f2;
        cross_s = s1 + s2 - std::sqrt(
          (accuracy + value_pair.second.accuracy - distance) *
          (distance + accuracy - value_pair.second.accuracy) *
          (distance - accuracy + value_pair.second.accuracy) *
          (distance + accuracy + value_pair.second.accuracy)) / 2;
      }

      if(cross_s / (accuracy_sqr * boost::math::constants::pi<double>()) >= TRUST_PROBABILITY)
      {
        return true;
      }
    }

    return false;
  }

  // GeoChannelIndex impl
  GeoChannelIndex::GeoChannelIndex() noexcept
  {
    empty_string_ = new Commons::StringHolder("");
    all_names_.insert(Commons::StringHolderHashAdapter(empty_string_));
  }

  GeoChannelIndex::~GeoChannelIndex() noexcept
  {}

  // GeoCoordChannelIndex impl
  GeoCoordChannelIndex::GeoCoordChannelIndex() noexcept
    : coord_index_impl_(new GeoCoordChannelIndex::CoordIndexImpl())
  {}

  GeoCoordChannelIndex::~GeoCoordChannelIndex() noexcept
  {}

  void
  GeoCoordChannelIndex::add(
    const Key& key,
    unsigned long channel_id)
    noexcept
  {
    channel_ids_.insert(channel_id);

    ChannelMap::iterator ch_it = channels_.find(key);
    if(ch_it != channels_.end())
    {
      ch_it->second->channels.push_back(channel_id);
    }
    else
    {
      Value_var value = new Value();
      value->channels.push_back(channel_id);
      channels_.insert(std::make_pair(key, value));
      coord_index_impl_->add(key, value);
    }
  }

  void
  GeoCoordChannelIndex::match(
    ChannelIdSet& result_channels,
    const CoordDecimal& longitude,
    const CoordDecimal& latitude,
    const AccuracyDecimal& accuracy) const
    noexcept
  {
    coord_index_impl_->match(
      result_channels, longitude, latitude, accuracy);
  }
}
}
