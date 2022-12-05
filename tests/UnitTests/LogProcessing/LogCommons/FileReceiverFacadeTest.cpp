#include "../../TestHelpers.hpp"

#include <Generics/TaskRunner.hpp>

#include <LogCommons/FileReceiverFacade.hpp>

#include "FileSystem.hpp"

enum class LogType
{
  Action,
  Click,
  Impression
};

class CerrCallback
  : public Generics::ActiveObjectCallback, public ReferenceCounting::AtomicImpl
{
public:
  virtual void
  report_error(
    Severity,
    const String::SubString& description,
    const char* = 0)
    noexcept
  {
    std::cerr << description.str() << std::endl;
  }

protected:
  virtual
  ~CerrCallback() noexcept
  {}
};

std::ostream&
operator<< (
  std::ostream& os,
  LogType log_type)
{
  os << "LogType(";

  switch (log_type)
  {
    case LogType::Action:
      os << "Action";
      break;
    case LogType::Click:
      os << "Click";
      break;
    case LogType::Impression:
      os << "Impression";
      break;
  }

  os << ")";
  return os;
}

typedef AdServer::LogProcessing::FileReceiverFacade<
  AdServer::LogProcessing::DefaultOrderStrategy<LogType> > FileReceiverFacade;
typedef ReferenceCounting::QualPtr<FileReceiverFacade> FileReceiverFacade_var;

std::string fetch_dir;
std::string action_intermediate_dir;
std::string click_intermediate_dir;
std::string impression_intermediate_dir;

FileReceiverFacade_var file_receiver_facade;

AdServer::LogProcessing::FileReceiver_var action_file_receiver;
AdServer::LogProcessing::FileReceiver_var click_file_receiver;
AdServer::LogProcessing::FileReceiver_var impression_file_receiver;

Generics::ActiveObjectCallback_var callback;
Generics::TaskRunner_var runner;

void
setup()
{
  char pwd[2048];
  fs::file_system.getcwd(pwd, sizeof(pwd));
  fetch_dir = std::string(pwd) + "/In";
  action_intermediate_dir = fetch_dir + "/Action";
  click_intermediate_dir = fetch_dir + "/Click";
  impression_intermediate_dir = fetch_dir + "/Impression";

  fs::file_system.mkdir(fetch_dir);
  fs::file_system.mkdir(action_intermediate_dir);
  fs::file_system.mkdir(click_intermediate_dir);
  fs::file_system.mkdir(impression_intermediate_dir);

  const std::size_t max_files_to_store = 10;
  FileReceiverFacade::FileReceiversInitializer file_receivers;

  file_receivers.emplace_back(
    LogType::Action,
    action_file_receiver = AdServer::LogProcessing::FileReceiver_var(
      new AdServer::LogProcessing::FileReceiver(
        action_intermediate_dir.c_str(),
        max_files_to_store)));

  file_receivers.emplace_back(
    LogType::Click,
    click_file_receiver = AdServer::LogProcessing::FileReceiver_var(
      new AdServer::LogProcessing::FileReceiver(
        click_intermediate_dir.c_str(),
        max_files_to_store)));

  file_receivers.emplace_back(
    LogType::Impression,
    impression_file_receiver = AdServer::LogProcessing::FileReceiver_var(
      new AdServer::LogProcessing::FileReceiver(
        impression_intermediate_dir.c_str(),
        max_files_to_store)));

  file_receiver_facade = new FileReceiverFacade(file_receivers);
  file_receiver_facade->activate_object();

  callback = new CerrCallback();
  runner = new Generics::TaskRunner(callback, 1);
}

void
teardown()
{
  runner->deactivate_object();
  file_receiver_facade->deactivate_object();
  file_receiver_facade->wait_object();
  runner->wait_object();

  file_receiver_facade.reset();
  action_file_receiver.reset();
  click_file_receiver.reset();
  impression_file_receiver.reset();

  callback.reset();
  runner.reset();

  fs::file_system.rmrf(fetch_dir);
  fs::file_system.rmrf(action_intermediate_dir);
  fs::file_system.rmrf(click_intermediate_dir);
  fs::file_system.rmrf(impression_intermediate_dir);
}

struct FileEntityContainer
{
  Sync::Policy::PosixThread::Mutex mutex;
  std::list<FileReceiverFacade::FileEntity> file_entities;
};

class GetEldestTask
  : public Generics::Task, public ReferenceCounting::AtomicImpl
{
public:
  GetEldestTask(
    FileReceiverFacade* file_receiver_facade,
    FileEntityContainer& container)
    noexcept
    : file_receiver_facade_(ReferenceCounting::add_ref(file_receiver_facade)),
      container_(container)
  {}

  virtual void
  execute() /*throw(eh::Exception)*/
  {
    while (file_receiver_facade_->active())
    {
      FileReceiverFacade::FileEntity file = file_receiver_facade_->get_eldest();

      if (file.file_guard)
      {
        Sync::Policy::PosixThread::WriteGuard lock(container_.mutex);
        container_.file_entities.push_back(file);
      }
    }
  }

protected:
  virtual
  ~GetEldestTask() noexcept
  {}

private:
  FileReceiverFacade_var file_receiver_facade_;
  FileEntityContainer& container_;
};

TEST_EX(Simple, setup, teardown)
{
  FileEntityContainer container;

  runner->enqueue_task(
    Generics::Task_var(new GetEldestTask(file_receiver_facade, container)));
  runner->enqueue_task(
    Generics::Task_var(new GetEldestTask(file_receiver_facade, container)));

  fs::file_system.create(fetch_dir, "Action.20121030.105456.271081.67127731.10.0");
  fs::file_system.create(fetch_dir, "Action.20121030.105756.271081.67127731.10.3");
  fs::file_system.create(fetch_dir, "Click.20121030.105656.271081.67127731.10.1");
  fs::file_system.create(fetch_dir, "Click.20121030.105556.271081.67127731.10.4");
  fs::file_system.create(fetch_dir, "Impression.20121030.105356.271081.67127731.10.2");

  action_file_receiver->fetch_files(fetch_dir.c_str(), "Action");
  click_file_receiver->fetch_files(fetch_dir.c_str(), "Click");
  impression_file_receiver->fetch_files(fetch_dir.c_str(), "Impression");

  runner->activate_object();
  ::sleep(2);

  {
    Sync::Policy::PosixThread::WriteGuard lock(container.mutex);
    ASSERT_EQUALS (container.file_entities.size(), 5U);
    auto it = container.file_entities.begin();
    ASSERT_EQUALS (it->type, LogType::Action);
    ++it;
    ASSERT_EQUALS (it->type, LogType::Click);
    ++it;
    ASSERT_EQUALS (it->type, LogType::Impression);
    ++it;
    ASSERT_EQUALS (it->type, LogType::Click);
    ++it;
    ASSERT_EQUALS (it->type, LogType::Action);
  }

  fs::file_system.create(fetch_dir, "Impression.20121030.105356.271081.67127731.10.6");
  fs::file_system.create(fetch_dir, "Impression.20121030.105356.271081.67127731.10.5");

  action_file_receiver->fetch_files(fetch_dir.c_str(), "Action");
  click_file_receiver->fetch_files(fetch_dir.c_str(), "Click");
  impression_file_receiver->fetch_files(fetch_dir.c_str(), "Impression");

  ::sleep(2);

  {
    Sync::Policy::PosixThread::WriteGuard lock(container.mutex);
    ASSERT_EQUALS (container.file_entities.size(), 7U);

    auto it = container.file_entities.begin();
    std::advance(it, 5);
    ASSERT_EQUALS (it->type, LogType::Impression);
    ++it;
    ASSERT_EQUALS (it->type, LogType::Impression);
  }
}

TEST_EX(Timeout, setup, teardown)
{
  FileReceiverFacade::FileEntity fe;
  const Generics::Time curr_ts = Generics::Time::get_time_of_day();
  fe = file_receiver_facade->get_eldest(2);
  ASSERT_TRUE (!fe.file_guard);

  ASSERT_TRUE( Generics::Time::get_time_of_day() > curr_ts + 2 );
}

RUN_TESTS
