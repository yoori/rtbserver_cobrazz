// STD
#include <regex>

// THIS
#include "DataModelProviderImpl.hpp"
#include "Utils.hpp"

namespace Aspect
{
const char* DATA_PROVIDER = "DATA_PROVIDER";
}

namespace PredictorSvcs
{
namespace BidCostPredictor
{

DataModelProviderImpl::DataModelProviderImpl(
  const std::string& input_dir,
  Logging::Logger* logger)
  : input_dir_(input_dir),
    logger_(ReferenceCounting::add_ref(logger)),
    observer_(new ActiveObjectObserver(this)),
    prefix_(LogTraits::B::log_base_name()),
    persantage_(logger_, Aspect::DATA_PROVIDER, 5)
{
  for (std::uint8_t i = 1; i <= COUNT_THREADS; ++i)
  {
    task_runners_.emplace_back(new Generics::TaskRunner(observer_, 1));
  }

  help_collector_.prepare_adding(50000000);
}

DataModelProviderImpl::~DataModelProviderImpl()
{
  shutdown_manager_.stop();
  observer_->clear_delegate();
  wait();
}

bool DataModelProviderImpl::load(
  HelpCollector& help_collector) noexcept
{
  if (is_success_)
  {
    logger_->info(
      std::string("DataModelProvider already is loaded"),
      Aspect::DATA_PROVIDER);
    help_collector = help_collector_;
    return true;
  }
  else if (shutdown_manager_.is_stoped())
  {
    if (is_interrupted_.load())
    {
      logger_->critical(
        std::string("DataModelProvider is interrupted"),
        Aspect::DATA_PROVIDER);
    }
    else
    {
      logger_->critical(
        std::string("DataModelProvider is failed"),
        Aspect::DATA_PROVIDER);
    }
    return false;
  }

  logger_->info(
    std::string("DataModelProvider started"),
    Aspect::DATA_PROVIDER);
  try
  {
    start();
  }
  catch (const Exception& exc)
  {
    std::stringstream stream;
    stream << "DataModelProvider is failed. Reason: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::DATA_PROVIDER);
    shutdown_manager_.stop();
    return false;
  }

  wait();

  if (is_success_)
  {
    help_collector = help_collector_;
    logger_->info(
      std::string("DataModelProvider is finish success"),
      Aspect::DATA_PROVIDER);
    return true;
  }
  else
  {
    logger_->critical(
      std::string("DataModelProvider is failed"),
      Aspect::DATA_PROVIDER);
    return false;
  }
}

void DataModelProviderImpl::start()
{
  if (!Utils::exist_directory(input_dir_))
  {
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << ": Not existing input directory="
         << input_dir_;
    throw Exception(ostr);
  }

  if (task_runners_.empty())
    throw Exception("task_runners is empty");

  aggregated_files_ =
    Utils::get_directory_files(
      input_dir_,
      prefix_);
  std::reverse(
    std::begin(aggregated_files_),
    std::end(aggregated_files_));

  persantage_.set_total_number(aggregated_files_.size());

  if (aggregated_files_.empty())
  {
    logger_->info(
      std::string("Aggregated files is empty"),
      Aspect::DATA_PROVIDER);
    is_success_ = true;
    return;
  }

  is_running_ = true;

  try
  {
    for (auto& task_runner : task_runners_)
    {
      task_runner->activate_object();
    }
  }
  catch(const eh::Exception& exc)
  {
    shutdown_manager_.stop();
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << ": Can't init task runner : "
         << exc.what();
    throw Exception(ostr);
  }

  for (int i = 1; i <= 2; ++i)
  {
    if (!post_task(
      ThreadID::Read,
      &DataModelProviderImpl::do_read))
    {
      Stream::Error ostr;
      ostr << __PRETTY_FUNCTION__
           << " Fatal error: initial read is failed";
      throw Exception(ostr);
    }
  }
}

void DataModelProviderImpl::stop() noexcept
{
  logger_->info(
    std::string("DataModelProvider was interrupted"),
    Aspect::DATA_PROVIDER);
  shutdown_manager_.stop();
}

void DataModelProviderImpl::wait() noexcept
{
  if (!is_running_)
    return;

  is_running_ = false;

  shutdown_manager_.wait();

  for (auto& task_runner : task_runners_)
  {
    try
    {
      task_runner->wait_for_queue_exhausting();
    }
    catch (...)
    {}

    try
    {
      task_runner->deactivate_object();
      task_runner->wait_object();
    }
    catch (...)
    {}
  }

  task_runners_.clear();
}

void DataModelProviderImpl::do_read() noexcept
{
  if (shutdown_manager_.is_stoped())
    return;

  if (aggregated_files_.empty())
  {
    if (!is_read_stoped_)
    {
      post_task(
        ThreadID::Calculate,
        &DataModelProviderImpl::do_stop);
    }

    is_read_stoped_ = true;
    return;
  }

  persantage_.increase();

  const auto it = aggregated_files_.begin();
  const auto file_path = *it;
  aggregated_files_.erase(it);

  auto temp_collector = pool_collector_.get_collector();
  try
  {
    LogHelper<LogTraits>::load(file_path, temp_collector);
  }
  catch (const Exception& exc)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << ": Can't add file="
           << file_path
           << " to model collector. Reason: "
           << exc.what();
    logger_->error(stream.str(), Aspect::DATA_PROVIDER);

    post_task(
      ThreadID::Read,
      &DataModelProviderImpl::do_read);
    post_task(
      ThreadID::Clean,
      &DataModelProviderImpl::do_clean,
      std::move(temp_collector));
    return;
  }

  post_task(
    ThreadID::Calculate,
    &DataModelProviderImpl::do_calculate,
    std::move(temp_collector));
}

void DataModelProviderImpl::do_calculate(
  Collector& temp_collector) noexcept
{
  if (shutdown_manager_.is_stoped())
    return;

  post_task(
    ThreadID::Read,
    &DataModelProviderImpl::do_read);

  try
  {
    for (const auto& [key_temp, data_temp] : temp_collector)
    {
      const auto& tag_id = key_temp.tag_id();
      const auto& url = key_temp.url();

      auto it_url_hash = url_hash_.find(url);
      if (it_url_hash == url_hash_.end())
      {
        auto url_var = std::make_shared<std::string>(url);
        it_url_hash = url_hash_.try_emplace(*url_var, url_var).first;
      }

      const HelpKey key(tag_id, it_url_hash->second);
      auto& help_collector_inner = help_collector_.find_or_insert(key);

      const auto& cost = key_temp.cost();
      const HelpInnerKey key_inner(cost);
      auto& data_inner = help_collector_inner.find_or_insert(key_inner);
      data_inner += data_temp;

      if (data_inner.is_null())
      {
        const auto it = help_collector_inner.find(key_inner);
        if (it != help_collector_inner.end())
          help_collector_inner.erase(it);
      }
    }
  }
  catch (const eh::Exception& exc)
  {
    shutdown_manager_.stop();
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << ": Reason : "
           << exc.what();
    logger_->critical(stream.str(), Aspect::DATA_PROVIDER);
    return;
  }

  post_task(
    ThreadID::Clean,
    &DataModelProviderImpl::do_clean,
    std::move(temp_collector));
}

void DataModelProviderImpl::do_clean(Collector& collector) noexcept
{
  if (shutdown_manager_.is_stoped())
    return;

  pool_collector_.add_collector(std::move(collector));
}

void DataModelProviderImpl::do_stop() noexcept
{
  if (shutdown_manager_.is_stoped())
    return;

  is_interrupted_.store(true);
  is_success_ = true;
  shutdown_manager_.stop();
}

void DataModelProviderImpl::report_error(
  Severity severity,
  const String::SubString& description,
  const char* error_code) noexcept
{
  if (severity == Severity::CRITICAL_ERROR || severity == Severity::ERROR)
  {
    shutdown_manager_.stop();
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : DataModelProvider stopped due to incorrect operation of queues."
           << " Reason: "
           << description;
    logger_->critical(
      stream.str(),
      Aspect::DATA_PROVIDER,
      error_code);
  }
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs