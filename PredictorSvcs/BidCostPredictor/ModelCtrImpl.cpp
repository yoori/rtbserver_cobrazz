// THIS
#include "LogHelper.hpp"
#include "ModelCtrImpl.hpp"

namespace Aspect
{

inline constexpr char MODEL_CTR[] = "MODEL_CTR";

} // namespace Aspect

namespace PredictorSvcs::BidCostPredictor
{

ModelCtrImpl::ModelCtrImpl(Logger* logger)
  : logger_(ReferenceCounting::add_ref(logger))
{
}

ModelCtrImpl::Ctr ModelCtrImpl::get_ctr(
  const TagId& tag_id,
  const Url& url,
  const CreativeCategoryIds& creative_category_ids) const
{
  static const std::string any_domain = "?";

  Ctr ctr = Ctr::MAXIMUM;
  {
    for (const auto& creative_category_id : creative_category_ids)
    {
      const auto category_ctr = collector_.get(tag_id, url, creative_category_id);
      if (category_ctr)
      {
        ctr = std::min(ctr, *category_ctr);
      }
    }

    if (ctr != Ctr::MAXIMUM)
    {
      return ctr;
    }
  }

  {
    const auto empty_category_ctr = collector_.get(tag_id, url, 0);
    if (empty_category_ctr)
    {
      ctr = std::min(ctr, *empty_category_ctr);
    }

    for (const auto& creative_category_id : creative_category_ids)
    {
      const auto category_ctr = collector_.get(tag_id, any_domain, creative_category_id);
      if (category_ctr)
      {
        ctr = std::min(ctr, *category_ctr);
      }
    }

    if (ctr != Ctr::MAXIMUM)
    {
      return ctr;
    }
  }

  {
    const auto empty_category_ctr = collector_.get(tag_id, any_domain, 0);
    if (empty_category_ctr)
    {
      return *empty_category_ctr;
    }
  }

  return Ctr::ZERO;
}

void ModelCtrImpl::set_ctr(
  const TagId& tag_id,
  const Url& url,
  const CreativeCategoryId& creative_category_id,
  const Ctr& ctr)
{
  collector_.add(
    tag_id,
    url,
    creative_category_id,
    ctr);
}

void ModelCtrImpl::clear() noexcept
{
  collector_.clear();
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
    collector_.load(path);

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

    collector_.save(path);

    {
      std::ostringstream stream;
      stream << FNS
             << "ModelCtr save is success";
      logger_->info(stream.str(), Aspect::MODEL_CTR);
    }
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "ModelCtr save is failed. Reason: "
           << exc.what();
    logger_->error(stream.str(), Aspect::MODEL_CTR);
    throw;
  }
  catch (...)
  {
    Stream::Error stream;
    stream << FNS
           << "ModelCtr save is failed. Reason: Unknown error";
    logger_->error(stream.str(), Aspect::MODEL_CTR);
    throw;
  }
}

} // namespace PredictorSvcs::BidCostPredictor