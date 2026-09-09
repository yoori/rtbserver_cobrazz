#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <Generics/TaskRunner.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/StreamLogger.hpp>

#include <LogCommons/LogHolder.hpp>

namespace
{
  struct InnerKey
  {
    std::size_t hash() const noexcept
    {
      return 0;
    }
  };

  class InnerCollector
  {
  public:
    using Container = std::vector<std::pair<InnerKey, unsigned long>>;
    using const_iterator = Container::const_iterator;

    void add(const InnerKey& key, unsigned long value)
    {
      values_.emplace_back(key, value);
    }

    const_iterator begin() const noexcept
    {
      return values_.begin();
    }

    const_iterator end() const noexcept
    {
      return values_.end();
    }

  private:
    Container values_;
  };

  class Collector
  {
  public:
    using KeyT = unsigned long;
    using DataT = InnerCollector;

    bool empty() const noexcept
    {
      return records_ == 0;
    }

    void add(KeyT, DataT&&)
    {
      ++records_;
    }

    void swap(Collector& other) noexcept
    {
      std::swap(records_, other.records_);
    }

  private:
    std::size_t records_ = 0;
  };

  struct LogTraits
  {
    using CollectorType = Collector;

    static const char* log_base_name() noexcept
    {
      return "LogHolderPortionedTest";
    }
  };

  struct ThrowingSavePolicy
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    void save(Collector&, const char*)
    {
      throw Exception("expected save error");
    }
  };

  class LogHolder final:
    public AdServer::LogProcessing::LogHolderPortioned<LogTraits, ThrowingSavePolicy>
  {
  public:
    using Base = AdServer::LogProcessing::LogHolderPortioned<LogTraits, ThrowingSavePolicy>;

    explicit LogHolder(Generics::TaskRunner* task_runner)
      : Base(
          AdServer::LogProcessing::LogFlushTraits(Generics::Time::ZERO, "/unused"),
          ThrowingSavePolicy(),
          task_runner,
          1)
    {}

    ~LogHolder() noexcept override = default;
  };
}

int main()
{
  Logging::Logger_var logger =
    new Logging::OStream::Logger(Logging::OStream::Config(std::cerr));
  Logging::ActiveObjectCallbackImpl_var callback =
    new Logging::ActiveObjectCallbackImpl(logger, "LogHolderPortionedTest");
  Generics::TaskRunner_var task_runner = new Generics::TaskRunner(callback, 1);
  task_runner->activate_object();

  int result = 1;
  {
    LogHolder holder(task_runner);
    InnerCollector data;
    data.add(InnerKey(), 1);
    holder.add_record(1, data);

    try
    {
      holder.flush_if_required(Generics::Time::get_time_of_day());
      std::cerr << "save exception was not propagated" << std::endl;
    }
    catch (const eh::Exception& ex)
    {
      if (std::string(ex.what()).find("expected save error") != std::string::npos)
      {
        result = 0;
      }
      else
      {
        std::cerr << "unexpected exception: " << ex.what() << std::endl;
      }
    }
  }

  task_runner->deactivate_object();
  task_runner->wait_object();
  return result;
}
