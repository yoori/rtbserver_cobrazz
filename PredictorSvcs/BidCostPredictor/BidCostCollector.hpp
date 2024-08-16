#ifndef BIDCOSTPREDICTOR_BIDCOSTCOLLECTOR_HPP
#define BIDCOSTPREDICTOR_BIDCOSTCOLLECTOR_HPP

// THIS
#include <PredictorSvcs/BidCostPredictor/Types.hpp>

namespace PredictorSvcs::BidCostPredictor
{

class BidCostCollector final : private Generics::Uncopyable
{
public:
  using TagId = Types::TagId;
  using Url = Types::Url;
  using UrlView = std::string_view;
  using Urls = std::list<Url>;
  using UrlHashHelper = std::unordered_set<UrlView>;
  using WinRate = Types::WinRate;
  using Cost = Types::FixedNumber;

  struct Data final
  {
    Data(
      const TagId tag_id,
      const UrlView& url,
      const WinRate& win_rate,
      const Cost& cost,
      const Cost& max_cost)
      : tag_id(tag_id),
        url(url),
        win_rate(win_rate),
        cost(cost),
        max_cost(max_cost)
    {
    }

    bool operator==(const Data&) const = default;

    TagId tag_id = 0;
    UrlView url;
    WinRate win_rate = WinRate::ZERO;
    Cost cost = WinRate::ZERO;
    Cost max_cost = WinRate::ZERO;
  };
  using Container = std::deque<Data>;
  using ConstIterator = typename Container::const_iterator;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  BidCostCollector();

  ~BidCostCollector() = default;

  const Data& add(
    const TagId tag_id,
    const Url& url,
    const WinRate& win_rate,
    const Cost& cost,
    const Cost& max_cost);

  std::size_t size() const noexcept;

  ConstIterator begin() const noexcept;

  ConstIterator end() const noexcept;

  void save(const std::string& path) const;

  void load(const std::string& path);

  void clear() noexcept;

private:
  Urls urls_;

  UrlHashHelper url_hash_helper_;

  Container container_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_BIDCOSTCOLLECTOR_HPP
