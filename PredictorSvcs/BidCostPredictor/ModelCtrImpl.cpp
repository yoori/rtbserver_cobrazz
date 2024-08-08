// THIS
#include "LogHelper.hpp"
#include "ModelCtrImpl.hpp"

namespace Aspect
{

inline constexpr char MODEL_CTR[] = "MODEL_CTR";

} // namespace Aspect

namespace PredictorSvcs::BidCostPredictor
{

ModelCtrImpl::ModelCtrImpl(Logging::Logger* logger)
  : logger_(ReferenceCounting::add_ref(logger))
{
  collector_.prepare_adding(10000000);
}

ModelCtrImpl::Ctr ModelCtrImpl::get_ctr(
  const TagId& tag_id,
  const Url& url,
  const CreativeCategoryId& creative_category_id) const
{
  const CtrKey key(tag_id, url, creative_category_id);
  const auto it = collector_.find(key);
  if (it != std::end(collector_))
  {
    return it->second.ctr() ;
  }
  else
  {
    return Ctr::ZERO;
  }
}

void ModelCtrImpl::set_ctr(
  const TagId& tag_id,
  const UrlPtr& url,
  const CreativeCategoryId& creative_category_id,
  const Ctr& ctr)
{
  const CtrKey key(tag_id, url, creative_category_id);
  const CtrData data(ctr);
  collector_.add(key, data);
}

void ModelCtrImpl::clear() noexcept
{
  try
  {
    collector_.clear();
  }
  catch (...)
  {
  }
}

void ModelCtrImpl::load(const std::string& path)
{
  try
  {
    {
      std::ostringstream stream;
      stream << FNS
             << "ModelCtr load started, path="
             << path;
      logger_->info(stream.str(), Aspect::MODEL_CTR);
    }

    clear();
    LogHelper<LogProcessing::CtrTraits>::load(path, collector_);

    {
      std::ostringstream stream;
      stream << FNS
             << "ModelCtr load is success";
      logger_->info(stream.str(), Aspect::MODEL_CTR);
    }
  }
  catch (const eh::Exception& exc)
  {
    clear();

    Stream::Error stream;
    stream << FNS
           << "ModelCtr load is failed. Reason: "
           << exc.what();
    logger_->error(stream.str(), Aspect::MODEL_CTR);
    throw;
  }
  catch (...)
  {
    clear();

    Stream::Error stream;
    stream << FNS
           << "ModelCtr load is failed. Reason: Unknown error";
    logger_->error(stream.str(), Aspect::MODEL_CTR);
    throw;
  }
}

void ModelCtrImpl::save(const std::string& path) const
{
  try
  {
    {
      std::ostringstream stream;
      stream << FNS
             << "ModelCtr save started, file_path="
             << path;
      logger_->info(stream.str(), Aspect::MODEL_CTR);
    }

    LogHelper<CtrTraits>::save(path, collector_);

    {
      std::ostringstream stream;
      stream << FNS
             << "ModelCtr save is success";
      logger_->info(stream.str(), Aspect::MODEL_CTR);
    }
  }
  catch (const eh::Exception& exc)
  {
    std::remove(path.c_str());

    Stream::Error stream;
    stream << FNS
           << "ModelCtr save is failed. Reason: "
           << exc.what();
    logger_->error(stream.str(), Aspect::MODEL_CTR);
    throw;
  }
  catch (...)
  {
    std::remove(path.c_str());

    Stream::Error stream;
    stream << FNS
           << "ModelCtr save is failed. Reason: Unknown error";
    logger_->error(stream.str(), Aspect::MODEL_CTR);
    throw;
  }
}

} // namespace PredictorSvcs::BidCostPredictor