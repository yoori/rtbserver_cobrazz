#ifndef BIDCOSTPREDICTOR_CTRCOLLECTOR_HPP
#define BIDCOSTPREDICTOR_CTRCOLLECTOR_HPP

// BOOST
#include <boost/functional/hash.hpp>

// STD
#include <memory>

// UNIXCOMMONS
#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>

// THIS
#include <PredictorSvcs/BidCostPredictor/Types.hpp>

namespace PredictorSvcs::BidCostPredictor
{

class CtrCollector final : private Generics::Uncopyable
{
public:
  using TagId = Types::TagId;
  using Url = Types::Url;
  using UrlView = std::string_view;
  using Urls = std::list<Url>;
  using UrlHashHelper = std::unordered_set<UrlView>;
  using CreativeCategoryId = Types::CreativeCategoryId;
  using Ctr = Types::Ctr;
  using CtrOpt = std::optional<Ctr>;
  using Key = std::tuple<TagId, UrlView, CreativeCategoryId>;

  struct KeyHash final
  {
    std::size_t operator()(const Key& key) const noexcept
    {
      std::size_t seed = 0;
      boost::hash_combine(seed, std::get<0>(key));
      boost::hash_combine(seed, std::get<1>(key));
      boost::hash_combine(seed, std::get<2>(key));
      return seed;
    }
  };
  using Map = std::unordered_map<Key, Ctr, KeyHash>;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  CtrCollector(const std::size_t capacity = 50000000);

  ~CtrCollector() = default;

  void add(
    const TagId tag_id,
    const Url& url,
    const CreativeCategoryId& creative_category_id,
    const Ctr& ctr);

  CtrOpt get(
    const TagId tag_id,
    const Url& url,
    const CreativeCategoryId& creative_category_id) const;

  void save(const std::string& path) const;

  void load(const std::string& path);

  void clear() noexcept;

private:
  Urls urls_;

  UrlHashHelper url_hash_helper_;

  Map map_;
};

inline CtrCollector::CtrOpt CtrCollector::get(
  const TagId tag_id,
  const Url& url,
  const CreativeCategoryId& creative_category_id) const
{
  const auto it = map_.find(
    Key{tag_id, std::string_view(url), creative_category_id});
  if (it != std::end(map_))
  {
    return it->second;
  }

  return {};
}


} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_CTRCOLLECTOR_HPP