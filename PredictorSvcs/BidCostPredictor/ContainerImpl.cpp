// THIS
#include "ContainerImpl.h"
#include "ModelBidCostImpl.hpp"
#include "ModelCtrImpl.hpp"
#include "Utils.hpp"

// STD
#include <regex>

namespace Aspect
{
const char CONTAINER[] = "CONTAINER";
}

namespace PredictorSvcs
{
namespace BidCostPredictor
{

ContainerImpl::ContainerImpl(
        const Logging::Logger_var& logger,
        const std::string& model_dir,
        const std::string& ctr_model_dir)
        : logger_(logger),
          model_dir_(model_dir),
          ctr_model_dir_(ctr_model_dir),
          model_bid_cost_(new ModelBidCostImpl(logger_)),
          model_ctr_(new ModelCtrImpl(logger_))
{
  initialize();
}

void ContainerImpl::initialize()
{
  try
  {
    logger_->info(
            std::string("Start initialize container"),
            Aspect::CONTAINER);

    const auto path_model_file = getLastFile(model_dir_);
    model_bid_cost_->load(path_model_file);

    const auto path_ctr_model_file = getLastFile(ctr_model_dir_);
    model_ctr_->load(path_ctr_model_file);

    logger_->info(
            std::string("Initialization container is success finished"),
            Aspect::CONTAINER);
  }
  catch(const Exception& exc)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : Initialization container is failed : Reason: "
           << exc.what();
    logger_->error(stream.str(), Aspect::CONTAINER);
    throw;
  }
}

std::string ContainerImpl::getLastFile(const std::string& path_dir)
{
  if (!Utils::ExistDirectory(path_dir))
  {
    Stream::Error stream;
    stream << __PRETTY_FUNCTION__
           << "Not existing directory="
           << path_dir;
    throw Exception(stream);
  }

  auto directories = Utils::GetDirectoryFiles(
          path_dir,
          "",
          Utils::DirInfo::Directory);
  if (directories.empty())
  {
    Stream::Error stream;
    stream << __PRETTY_FUNCTION__
           << "Directory not contain model directory";
    throw Exception(stream);
  }

  const std::regex time_regex("\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}");

  auto it_result = std::end(directories);
  auto it = std::begin(directories);
  auto it_end = std::end(directories);
  for (; it != it_end; ++it)
  {
    if (std::regex_search(*it, time_regex))
    {
      it_result = it;
    }
  }

  if (it_result == std::end(directories))
  {
    Stream::Error stream;
    stream << __PRETTY_FUNCTION__
           << "Directory not contain model directory "
           << "with format yyyy:mm:dd hh:mm:ss";
    throw Exception(stream);
  }

  const auto& result_path_directory = *it_result;
  const auto files = Utils::GetDirectoryFiles(
          result_path_directory,
          "",
          Utils::DirInfo::RegularFile);
  if (files.empty())
  {
    Stream::Error stream;
    stream << __PRETTY_FUNCTION__
           << "Not exist file in directory="
           << result_path_directory;
    throw Exception(stream);
  }

  if (files.size() > 1)
  {
    Stream::Error stream;
    stream << __PRETTY_FUNCTION__
           <<  "Logic error. Number files in the "
           <<  "directory must be equal one";
    throw Exception(stream);
  }

  return files.front();
}

ContainerImpl::Cost ContainerImpl::getCost(
        const TagId& tag_id,
        const Url& url,
        const WinRate& win_rate,
        const Cost& current_cost) const
{
  return model_bid_cost_->getCost(tag_id, url, win_rate, current_cost);
}

ContainerImpl::Data ContainerImpl::getCtr(
        const TagId& tag_id,
        const Url& url) const
{
  return model_ctr_->getCtr(tag_id, url);
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs