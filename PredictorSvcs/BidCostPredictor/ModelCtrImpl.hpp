#ifndef BIDCOSTPREDICTOR_MODELCTRIMPL_HPP
#define BIDCOSTPREDICTOR_MODELCTRIMPL_HPP

// THIS
#include "ReferenceCounting/AtomicImpl.hpp"
#include "CtrCollector.hpp"
#include "ModelCtr.hpp"

namespace PredictorSvcs
{
namespace BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class ModelCtrImpl :
  public ModelCtr,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  using TagId = typename ModelCtr::TagId;
  using Url = typename ModelCtr::Url;
  using Url_var = typename ModelCtr::Url_var;
  using Ctr = typename ModelCtr::Ctr;

  ModelCtrImpl(Logging::Logger* logger);

  ~ModelCtrImpl() override = default;

  Ctr get_ctr(
    const TagId& tag_id,
    const Url& url) const override;

  void set_ctr(
    const TagId& tag_id,
    const Url_var& url,
    const Ctr& ctr) override;

  void clear() noexcept;

  void load(const std::string& path);

  void save(const std::string& path) const;

private:
  Logging::Logger_var logger_;

  LogProcessing::CtrCollector collector_;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_MODELCTRIMPL_HPP
