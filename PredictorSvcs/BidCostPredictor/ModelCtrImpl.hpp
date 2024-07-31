#ifndef BIDCOSTPREDICTOR_MODELCTRIMPL_HPP
#define BIDCOSTPREDICTOR_MODELCTRIMPL_HPP

// UNIXCOMMONS
#include "ReferenceCounting/AtomicImpl.hpp"

// THIS
#include "CtrCollector.hpp"
#include "ModelCtr.hpp"

namespace PredictorSvcs::BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class ModelCtrImpl final :
  public ModelCtr,
  public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using CtrCollector = LogProcessing::CtrCollector;
  using CtrKey = LogProcessing::CtrKey;
  using CtrData = LogProcessing::CtrData;
  using CtrTraits = LogProcessing::CtrTraits;
  using TagId = typename ModelCtr::TagId;
  using Url = typename ModelCtr::Url;
  using UrlPtr = typename ModelCtr::UrlPtr;
  using Ctr = typename ModelCtr::Ctr;

public:
  explicit ModelCtrImpl(Logger* logger);

  Ctr get_ctr(
    const TagId& tag_id,
    const Url& url) const override;

  void set_ctr(
    const TagId& tag_id,
    const UrlPtr& url,
    const Ctr& ctr) override;

  void clear() noexcept;

  void load(const std::string& path);

  void save(const std::string& path) const;

protected:
  ~ModelCtrImpl() override = default;

private:
  const Logger_var logger_;

  CtrCollector collector_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_MODELCTRIMPL_HPP
