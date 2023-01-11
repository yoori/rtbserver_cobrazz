// THIS
#include "LogHelper.hpp"
#include "ModelCtrImpl.hpp"

namespace Aspect
{
const char* MODEL_CTR = "MODEL_CTR";
}

namespace PredictorSvcs
{
namespace BidCostPredictor
{

ModelCtrImpl::ModelCtrImpl(Logging::Logger* logger)
  : logger_(ReferenceCounting::add_ref(logger))
{
  collector_.prepare_adding(10000000);
}

ModelCtrImpl::Data ModelCtrImpl::get_ctr(
  const TagId& tag_id,
  const Url& url) const
{
  const LogProcessing::CtrKey key(
    tag_id,
    url);
  if (const auto it = collector_.find(key);
    it != collector_.end())
  {
    const auto& data = it->second;
    return {data.clicks(), data.imps()} ;
  }
  else
  {
    return {0, 0};
  }
}

void ModelCtrImpl::set_ctr(
  const TagId& tag_id,
  const Url_var& url,
  const Clicks& clicks,
  const Imps& imps)
{
  const LogProcessing::CtrKey key(
    tag_id,
    url);
  const LogProcessing::CtrData data(
    imps,
    clicks);

  collector_.add(key, data);
}

void ModelCtrImpl::clear() noexcept
{
  collector_.clear();
}

void ModelCtrImpl::load(const std::string& path)
{
  try
  {
    logger_->info(
      "ModelCtr load started, path=" + path,
      Aspect::MODEL_CTR);
    clear();
    LogHelper<LogProcessing::CtrTraits>::load(path, collector_);
    logger_->info(
      std::string("ModelCtr load is success"),
      Aspect::MODEL_CTR);
  }
  catch (const eh::Exception& exc)
  {
    clear();
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : ModelCtr load is failed. Reason: "
           << exc.what();
    logger_->error(stream.str(), Aspect::MODEL_CTR);
    throw;
  }
}

void ModelCtrImpl::save(const std::string& path) const
{
  try
  {
    logger_->info(
      "ModelCtr save started, file_path=" + path,
      Aspect::MODEL_CTR);
    LogHelper<LogProcessing::CtrTraits>::save(path, collector_);
    logger_->info(
      std::string("ModelCtr save is success"),
      Aspect::MODEL_CTR);
  }
  catch (const eh::Exception& exc)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : ModelCtr save is failed. Reason: "
           << exc.what();
    logger_->error(stream.str(), Aspect::MODEL_CTR);
    throw;
  }
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs