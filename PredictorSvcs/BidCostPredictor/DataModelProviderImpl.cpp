// STD
#include <filesystem>
#include <regex>

// THIS
#include "DataModelProviderImpl.hpp"
#include "Utils.hpp"

namespace Aspect
{

inline constexpr char DATA_PROVIDER[] = "DATA_PROVIDER";

} // namespace Aspect

namespace PredictorSvcs::BidCostPredictor
{

DataModelProviderImpl::DataModelProviderImpl(
  const Imps max_imps,
  const std::string& input_dir,
  Logger* logger,
  CreativeProvider* creative_provider)
  : input_dir_(input_dir),
    logger_(ReferenceCounting::add_ref(logger)),
    creative_provider_(ReferenceCounting::add_ref(creative_provider)),
    prefix_(LogTraits::B::log_base_name()),
    persantage_(logger_, Aspect::DATA_PROVIDER, 5),
    help_collector_(max_imps, 1000000, 1)
{
  using Severity = Generics::ActiveObjectCallback::Severity;

  if (!std::filesystem::is_directory(input_dir_))
  {
    Stream::Error stream;
    stream << FNS
           << "Not existing input directory="
           << input_dir_;
    throw Exception(stream);
  }

  observer_ = new ActiveObjectObserver(
    [this] (
      const Severity severity,
      const String::SubString& description,
      const char* error_code) {
      if (severity == Severity::CRITICAL_ERROR || severity == Severity::ERROR)
      {
        shutdown_manager_.stop();

        Stream::Error stream;
        stream << FNS
               << "DataModelProvider stopped due to incorrect operation of queues."
               << " Reason: "
               << description;
        logger_->critical(
          stream.str(),
          Aspect::DATA_PROVIDER,
          error_code);
      }
    });

  const auto size = static_cast<std::uint8_t>(ThreadID::MAX_NUMBER);
  for (std::uint8_t i = 0; i < size; i += 1)
  {
    task_runners_.emplace_back(
      new Generics::TaskRunner(observer_, 1));
    task_runners_.back()->activate_object();
  }

  if (creative_provider_)
  {
    creative_provider_->load(cc_id_to_categories_);
  }
}

DataModelProviderImpl::~DataModelProviderImpl()
{
  shutdown_manager_.stop();
  clear();
}

void DataModelProviderImpl::wait() noexcept
{
  shutdown_manager_.wait();
}

void DataModelProviderImpl::clear() noexcept
{
  for (auto& task_runner : task_runners_)
  {
    try
    {
      task_runner->wait_for_queue_exhausting();
    }
    catch (...)
    {
    }

    try
    {
      task_runner->deactivate_object();
      task_runner->wait_object();
    }
    catch (...)
    {
    }
  }
  task_runners_.clear();
}

template<ConceptMemberPtr MemPtr, class ...Args>
bool DataModelProviderImpl::post_task(
  const ThreadID id,
  MemPtr mem_ptr,
  Args&& ...args) noexcept
{
  try
  {
    if (task_runners_.size() <= static_cast<std::size_t>(id))
    {
      Stream::Error stream;
      stream << "Thread id="
             << static_cast<std::size_t>(id)
             << " greater than number of threads="
             << task_runners_.size();
      throw Exception(stream);
    }

    task_runners_[static_cast<std::size_t>(id)]->enqueue_task(
      AdServer::Commons::make_delegate_task(
        std::bind(
          mem_ptr,
          this,
          std::forward<Args>(args)...)));
    return true;
  }
  catch (const eh::Exception& exc)
  {
    shutdown_manager_.stop();

    Stream::Error stream;
    stream << FNS
           << "Can't enqueue_task. Reason: "
           << exc.what();
    logger_->critical(
      stream.str(),
      Aspect::DATA_PROVIDER);
  }
  catch (...)
  {
    shutdown_manager_.stop();

    Stream::Error stream;
    stream << FNS
           << "Can't enqueue_task. Reason: Unknown error";
    logger_->critical(
      stream.str(),
      Aspect::DATA_PROVIDER);
  }

  return false;
}

bool DataModelProviderImpl::load(
  HelpCollector& help_collector) noexcept
{
  if (is_load_success_.load())
  {
    std::ostringstream stream;
    stream << FNS
           << "DataModelProvider already is loaded";
    logger_->info(stream.str(), Aspect::DATA_PROVIDER);
    help_collector = help_collector_;

    return true;
  }
  else if (shutdown_manager_.is_stoped())
  {
    if (is_interrupted_.load())
    {
      Stream::Error stream;
      stream << FNS
             << "DataModelProvider is interrupted";
      logger_->critical(stream.str(), Aspect::DATA_PROVIDER);
    }
    else
    {
      Stream::Error stream;
      stream << FNS
             << "DataModelProvider is failed";
      logger_->critical(stream.str(),  Aspect::DATA_PROVIDER);
    }

    return false;
  }

  if (!is_idle_.load())
  {
    Stream::Error stream;
    stream << FNS
           << "Already in the process of loading";
    logger_->critical(stream.str(),  Aspect::DATA_PROVIDER);

    return false;
  }
  is_idle_.store(false);

  {
    std::ostringstream stream;
    stream << FNS
           << "DataModelProvider started";
    logger_->info(stream.str(), Aspect::DATA_PROVIDER);
  }

  try
  {
    run();
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << "DataModelProvider is failed. Reason: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::DATA_PROVIDER);
    shutdown_manager_.stop();

    return false;
  }
  catch (...)
  {
    Stream::Error stream;
    stream << "DataModelProvider is failed. Reason: Unknown error";
    logger_->critical(stream.str(), Aspect::DATA_PROVIDER);
    shutdown_manager_.stop();

    return false;
  }

  wait();

  if (is_load_success_.load())
  {
    help_collector = help_collector_;

    std::ostringstream stream;
    stream << FNS
           << "DataModelProvider loading successful";
    logger_->info(stream.str(), Aspect::DATA_PROVIDER);

    return true;
  }
  else
  {
    Stream::Error stream;
    stream << FNS
           << "DataModelProvider is failed";
    logger_->critical(stream.str(), Aspect::DATA_PROVIDER);

    return false;
  }
}

void DataModelProviderImpl::run()
{
  aggregated_files_ = Utils::get_directory_files(
    input_dir_,
    prefix_);
  std::reverse(
    std::begin(aggregated_files_),
    std::end(aggregated_files_));

  persantage_.set_total_number(aggregated_files_.size());

  if (aggregated_files_.empty())
  {
    is_load_success_.store(true);
    std::ostringstream stream;
    stream << FNS
           << "Aggregated files is empty";
    logger_->info(stream.str(), Aspect::DATA_PROVIDER);
    return;
  }

  for (int i = 1; i <= 2; ++i)
  {
    if (!post_task(ThreadID::Read, &DataModelProviderImpl::do_read))
    {
      Stream::Error stream;
      stream << FNS
             << " Fatal error: initial read is failed";
      throw Exception(stream);
    }
  }
}

void DataModelProviderImpl::stop() noexcept
{
  std::ostringstream stream;
  stream << FNS
         << "DataModelProvider was interrupted";
  logger_->info(stream.str(), Aspect::DATA_PROVIDER);

  is_interrupted_.store(true);
  shutdown_manager_.stop();
}

void DataModelProviderImpl::do_read() noexcept
{
  if (shutdown_manager_.is_stoped())
  {
    return;
  }

  if (aggregated_files_.empty())
  {
    static bool is_read_stoped = false;
    if (!is_read_stoped)
    {
      post_task(
        ThreadID::Calculate,
        &DataModelProviderImpl::do_stop);
    }

    is_read_stoped = true;
    return;
  }

  persantage_.increase();

  const auto it = aggregated_files_.begin();
  const auto file_path = *it;
  aggregated_files_.erase(it);

  //auto temp_collector = pool_collector_.get_collector();
  auto temp_collector = Collector();
  temp_collector.prepare_adding(1000000);
  try
  {
    LogHelper<LogTraits>::load(file_path, temp_collector);
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
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
  {
    return;
  }

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
        const auto url_ptr = std::make_shared<std::string>(url);
        it_url_hash = url_hash_.try_emplace(*url_ptr, url_ptr).first;
      }

      const HelpCollector::Key key(tag_id, it_url_hash->second);
      auto& help_collector_inner = help_collector_.find_or_insert(key);

      const auto& cost = key_temp.cost();
      if (!data_temp.is_null())
      {
        help_collector_inner.add(cost, data_temp);
      }
    }
  }
  catch (const eh::Exception& exc)
  {
    shutdown_manager_.stop();
    Stream::Error stream;
    stream << FNS
           << "Reason : "
           << exc.what();
    logger_->critical(stream.str(), Aspect::DATA_PROVIDER);
    return;
  }
  catch (...)
  {
    shutdown_manager_.stop();
    Stream::Error stream;
    stream << FNS
           << "Reason : Unknown error";
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
  {
    return;
  }

  collector.clear();
  //pool_collector_.add_collector(std::move(collector));
}

void DataModelProviderImpl::do_stop() noexcept
{
  if (shutdown_manager_.is_stoped())
  {
    return;
  }

  is_load_success_.store(true);
  shutdown_manager_.stop();
}

} // namespace PredictorSvcs::BidCostPredictor