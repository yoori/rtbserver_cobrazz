#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>

#include <eh/Exception.hpp>
#include <Generics/GnuHashTable.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

namespace AdServer::CampaignSvcs
{
  class GeoChannelIndex: public ReferenceCounting::AtomicCopyImpl
  {
  public:
    struct Key
    {
    public:
      Key(std::string_view country, std::string_view region, std::string_view city)
        noexcept;

      bool operator==(const Key& right) const noexcept;

      unsigned long hash() const noexcept;

      std::string_view country() const noexcept;

      std::string_view region() const noexcept;

      std::string_view city() const noexcept;

    private:
      std::string_view country_;
      std::string_view region_;
      std::string_view city_;
      unsigned long hash_;
    };

    using GeoChannelIdSet = std::unordered_set<unsigned long>;

    typedef Generics::GnuHashTable<Key, unsigned long> GeoChannelMap;

    struct NameHash
    {
      size_t operator()(const std::unique_ptr<std::string>& value) const noexcept;
    };

    struct NameEqual
    {
      bool operator()(
        const std::unique_ptr<std::string>& left,
        const std::unique_ptr<std::string>& right) const noexcept;
    };

    using NameSet = std::unordered_set<
      std::unique_ptr<std::string>,
      NameHash,
      NameEqual>;

    // all_names_: holds strings for Key string_view references and decreases memory usage.
    NameSet all_names_;

  public:
    GeoChannelIndex() noexcept;

    GeoChannelIndex(const GeoChannelIndex& init);

    void add(const String::SubString& country,
      const String::SubString& region,
      const String::SubString& city,
      unsigned long channel_id)
      noexcept;

    void close() noexcept;

    /* return channels in priority decreasing order : most accurate at begin */
    void match(
      ChannelIdArray& result_channels,
      const String::SubString& country,
      const String::SubString& region,
      const String::SubString& city)
      noexcept;

    const GeoChannelMap& channels() const noexcept;

    const GeoChannelIdSet& channel_ids() const noexcept;

  protected:
    virtual
    ~GeoChannelIndex() noexcept;

  private:
    std::string_view
    resolve_name_(const String::SubString& name) noexcept;

    static std::string_view
    to_string_view_(const String::SubString& name) noexcept;

  private:
    GeoChannelMap channels_;
    GeoChannelIdSet channel_ids_;
  };

  typedef ReferenceCounting::SmartPtr<GeoChannelIndex>
    GeoChannelIndex_var;

  class GeoCoordChannelIndex: public ReferenceCounting::AtomicImpl
  {
  public:
    struct Key
    {
      Key(const CoordDecimal& longitude_val,
        const CoordDecimal& latitude_val,
        const AccuracyDecimal& accuracy_val)
        : longitude(longitude_val),
          latitude(latitude_val),
          accuracy(accuracy_val),
          hash_(0)
      {}

      CoordDecimal longitude;
      CoordDecimal latitude;
      AccuracyDecimal accuracy;

      bool operator==(const Key& right) const
      {
        return longitude == right.longitude &&
          latitude == right.latitude && accuracy == right.accuracy;
      }

      size_t hash() const
      {
        return hash_;
      }

    protected:
      void calc_hash_()
      {
        Generics::Murmur64Hash hasher(hash_);
        hash_add(hasher, longitude);
        hash_add(hasher, latitude);
        hash_add(hasher, accuracy);
      }

    protected:
      size_t hash_;
    };

    struct Value: public ReferenceCounting::AtomicImpl
    {
      ChannelIdList channels;

    protected:
      virtual ~Value() noexcept
      {}
    };

    typedef ReferenceCounting::SmartPtr<Value> Value_var;

    typedef Generics::GnuHashTable<Key, Value_var> ChannelMap;

  public:
    GeoCoordChannelIndex() noexcept;

    void
    add(const Key& key, unsigned long channel_id)
      noexcept;

    void
    match(
      ChannelIdSet& result_channels,
      const CoordDecimal& longitude,
      const CoordDecimal& latitude,
      const AccuracyDecimal& accuracy) const
      noexcept;

    const ChannelMap&
    channels() const noexcept;

    const ChannelIdSet&
    channel_ids() const noexcept;

  protected:
    class CoordIndexImpl;

  protected:
    virtual
    ~GeoCoordChannelIndex() noexcept;

  private:
    ChannelIdSet channel_ids_;
    ChannelMap channels_;
    std::unique_ptr<CoordIndexImpl> coord_index_impl_;
  };

  typedef ReferenceCounting::SmartPtr<GeoCoordChannelIndex>
    GeoCoordChannelIndex_var;
}

namespace AdServer::CampaignSvcs
{
  inline
  GeoChannelIndex::Key::Key(
    std::string_view country,
    std::string_view region,
    std::string_view city)
    noexcept
    : country_(country),
      region_(region),
      city_(city),
      hash_(0)
  {
    hash_ = Generics::CRC::quick(0, country_.data(), country_.length());
    hash_ = Generics::CRC::quick(hash_, region_.data(), region_.length());
    hash_ = Generics::CRC::quick(hash_, city_.data(), city_.length());
  }

  inline
  bool
  GeoChannelIndex::Key::operator==(const Key& right) const noexcept
  {
    return country_ == right.country_ && region_ == right.region_ && city_ == right.city_;
  }

  inline
  unsigned long
  GeoChannelIndex::Key::hash() const noexcept
  {
    return hash_;
  }

  inline
  std::string_view
  GeoChannelIndex::Key::country() const noexcept
  {
    return country_;
  }

  inline
  std::string_view
  GeoChannelIndex::Key::region() const noexcept
  {
    return region_;
  }

  inline
  std::string_view
  GeoChannelIndex::Key::city() const noexcept
  {
    return city_;
  }

  inline
  size_t
  GeoChannelIndex::NameHash::operator()(const std::unique_ptr<std::string>& value) const noexcept
  {
    return std::hash<std::string_view>()(std::string_view(*value));
  }

  inline
  bool
  GeoChannelIndex::NameEqual::operator()(
    const std::unique_ptr<std::string>& left,
    const std::unique_ptr<std::string>& right) const noexcept
  {
    return *left == *right;
  }

  inline
  std::string_view
  GeoChannelIndex::to_string_view_(const String::SubString& name) noexcept
  {
    return name.empty() ?
      std::string_view() :
      std::string_view(name.data(), name.size());
  }

  inline
  std::string_view
  GeoChannelIndex::resolve_name_(const String::SubString& name)
    noexcept
  {
    std::unique_ptr<std::string> name_string;
    if (name.empty())
    {
      name_string = std::make_unique<std::string>();
    }
    else
    {
      name_string = std::make_unique<std::string>(name.data(), name.length());
    }

    const auto result = all_names_.emplace(std::move(name_string));
    return std::string_view(**result.first);
  }

  inline
  void
  GeoChannelIndex::add(
    const String::SubString& country,
    const String::SubString& region,
    const String::SubString& city,
    unsigned long channel_id)
    noexcept
  {
    const std::string_view packed_country = resolve_name_(country);
    const std::string_view packed_region = resolve_name_(region);
    const std::string_view packed_city = resolve_name_(city);
    channels_[Key(packed_country, packed_region, packed_city)] = channel_id;
    channel_ids_.insert(channel_id);
  }

  inline
  void
  GeoChannelIndex::close() noexcept
  {
  }

  inline
  void
  GeoChannelIndex::match(
    ChannelIdArray& result_channels,
    const String::SubString& country_val,
    const String::SubString& region_val,
    const String::SubString& city_val)
    noexcept
  {
    const std::string_view country = to_string_view_(country_val);
    const std::string_view region = to_string_view_(region_val);
    const std::string_view city = to_string_view_(city_val);
    const std::string_view empty;
    GeoChannelMap::const_iterator ind_it;

    if (region_val[0] || city_val[0])
    {
      if (city_val[0])
      {
        ind_it = channels_.find(Key(country, region, city));

        if (ind_it != channels_.end())
        {
          result_channels.push_back(ind_it->second);
        }
      }

      if (region_val[0])
      {
        ind_it = channels_.find(Key(country, region, empty));

        if (ind_it != channels_.end())
        {
          result_channels.push_back(ind_it->second);
        }
      }
    }

    ind_it = channels_.find(Key(country, empty, empty));
    if (ind_it != channels_.end())
    {
      result_channels.push_back(ind_it->second);
    }
  }

  inline
  const GeoChannelIndex::GeoChannelMap&
  GeoChannelIndex::channels() const noexcept
  {
    return channels_;
  }

  inline
  const GeoChannelIndex::GeoChannelIdSet&
  GeoChannelIndex::channel_ids() const noexcept
  {
    return channel_ids_;
  }

  // GeoCoordChannelIndex
  inline
  const GeoCoordChannelIndex::ChannelMap&
  GeoCoordChannelIndex::channels() const noexcept
  {
    return channels_;
  }

  inline
  const ChannelIdSet&
  GeoCoordChannelIndex::channel_ids() const noexcept
  {
    return channel_ids_;
  }
}
