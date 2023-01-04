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
  using Clicks = typename ModelCtr::Clicks;
  using Imps = typename ModelCtr::Imps;
  using Data = typename ModelCtr::Data;

  ModelCtrImpl(Logging::Logger_var& logger);

  ~ModelCtrImpl() override = default;

  Data getCtr(
          const TagId& tag_id,
          const Url& url) const;

  void setCtr(
          const TagId& tag_id,
          const Url_var& url,
          const Clicks& clicks,
          const Imps& imps);

  void clear() noexcept;

  void load(const std::string& path);

  void save(const std::string& path) const;

private:
  Logging::Logger_var& logger_;

  LogProcessing::CtrCollector collector_;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_MODELCTRIMPL_HPP
