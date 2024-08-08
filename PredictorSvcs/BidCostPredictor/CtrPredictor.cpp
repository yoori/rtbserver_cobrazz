// STD
#include <filesystem>
#include <regex>

// THIS
#include <Commons/DelegateTaskGoal.hpp>
#include "CtrPredictor.hpp"
#include "ModelCtrImpl.hpp"
#include "Utils.hpp"

namespace Aspect
{

inline constexpr char CTR_PREDICTOR[] = "CTR_PREDICTOR";

} // namespace Aspect

namespace PredictorSvcs::BidCostPredictor
{

CtrPredictor::CtrPredictor(Logger* logger)
  : logger_(ReferenceCounting::add_ref(logger)),
    model_ctr_(new ModelCtrImpl(logger_))
{
}

CtrPredictor::Ctr CtrPredictor::predict(
  const TagId& tag_id,
  const Url& url,
  const CreativeCategoryIds& creative_category_ids)
{
  return model_ctr_->get_ctr(tag_id, url, creative_category_ids);
}

void CtrPredictor::load(const std::string& file_path)
{
  try
  {
    {
      std::ostringstream stream;
      stream << FNS
             << "Start load ctr model";
      logger_->info(stream.str(), Aspect::CTR_PREDICTOR);
    }

    model_ctr_->clear();
    model_ctr_->load(file_path);

    {
      std::ostringstream stream;
      stream << FNS
             << "Ctr model is success loaded";
      logger_->info(stream.str(), Aspect::CTR_PREDICTOR);
    }
  }
  catch (const Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "Can't load ctr model reason : "
           << exc.what();
    logger_->error(stream.str(), Aspect::CTR_PREDICTOR);
    throw;
  }
  catch (...)
  {
    Stream::Error stream;
    stream << FNS
           << "Can't load ctr model reason : "
           << "Unknown error";
    logger_->error(stream.str(), Aspect::CTR_PREDICTOR);
    throw;
  }
}

} // namespace PredictorSvcs::BidCostPredictor