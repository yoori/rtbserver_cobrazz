#include <fnmatch.h>

#include "../../TestHelpers.hpp"

#include <Generics/TaskRunner.hpp>

#include <Commons/DelegateActiveObject.hpp>
#include <LogCommons/FileReceiver.hpp>

#define EMULATE_FILES

#ifdef EMULATE_FILES
  #include "FileSystemEmulator.hpp"
#else
  #include "FileSystem.hpp"
#endif

using namespace AdServer::LogProcessing;
typedef Sync::Policy::PosixThread SyncPolicy;

#ifdef EMULATE_FILES
class OverridedFileReceiver
  : public FileReceiver
{
public:
  OverridedFileReceiver(
    const char* intermediate_dir,
    size_t max_files_to_store,
    Generics::ActiveObject* interrupter = 0,
    Logging::Logger* logger = 0)
    /*throw(eh::Exception)*/;

public:
  void (*read_internal_hook)(const std::string&);

protected:
  virtual void
  rename_(
    const char* old_name,
    const char* new_name)
    /*throw(eh::Exception)*/;

  virtual void
  unlink_(const char* name)
    /*throw(eh::Exception)*/;

  virtual bool
  access_(const char* name) noexcept;

  virtual void
  dir_select_external_(
    const char* input_dir,
    const char* prefix)
    /*throw(eh::Exception, Interrupted)*/;

  virtual void
  dir_select_internal_(const char* dir) /*throw(eh::Exception, Interrupted)*/;
};

OverridedFileReceiver::OverridedFileReceiver(
  const char* intermediate_dir,
  size_t max_files_to_store,
  Generics::ActiveObject* interrupter,
  Logging::Logger* logger)
  /*throw(eh::Exception)*/
  :  FileReceiver(
      intermediate_dir,
      max_files_to_store,
      interrupter,
      logger),
     read_internal_hook(0)
{}

void
OverridedFileReceiver::rename_(
  const char* old_name, const char* new_name)
  /*throw(eh::Exception)*/
{
  fs::file_system.rename(old_name, new_name);
}

void
OverridedFileReceiver::unlink_(const char* name)
  /*throw(eh::Exception)*/
{
  fs::file_system.remove(name);
}

bool
OverridedFileReceiver::access_(const char* name) noexcept
{
  return fs::file_system.access(name);
}

void
OverridedFileReceiver::dir_select_external_(
  const char* input_dir,
  const char* prefix)
  /*throw(eh::Exception, Interrupted)*/
{
  fs::FileSystem::iterator i = fs::file_system.find(input_dir);

  if (i != fs::file_system.end())
  {
    std::set<std::string> files;

    {
      ::SyncPolicy::WriteGuard lock((*i)->mutex);
      files = (**i);
    }

    for (std::set<std::string>::iterator fi = files.begin();
         fi != files.end(); ++fi)
    {
      const std::string full_path = std::string(input_dir) + "/" + *fi;
      fetch_files_handler_(full_path.c_str(), prefix);
    }
  }
}

void
OverridedFileReceiver::dir_select_internal_(const char* dir)
  /*throw(eh::Exception, Interrupted)*/
{
  fs::FileSystem::iterator i = fs::file_system.find(dir);

  if (i != fs::file_system.end())
  {
    std::set<std::string> files;

    {
      ::SyncPolicy::WriteGuard lock((*i)->mutex);

      for (std::set<std::string>::const_iterator it = (*i)->begin();
           it != (*i)->end(); ++it)
      {
        if (::fnmatch("[A-Z]*", it->c_str(), FNM_PATHNAME) == 0)
        {
          files.insert(*it);
        }
      }
    }

    for (std::set<std::string>::iterator fi = files.begin();
         fi != files.end(); ++fi)
    {
      const std::string full_path = std::string(dir) + "/" + *fi;

      if (read_internal_hook)
      {
        read_internal_hook(full_path);
      }

      fetch_intermediate_dir_handler_(full_path.c_str(), "");
    }
  }
}
#endif // EMULATE_FILES

std::string in_dir;
std::string fe_dir;

FileReceiver_var
make_receiver(
  size_t max_files_to_store) noexcept
{
#ifdef EMULATE_FILES
  return FileReceiver_var(
    new OverridedFileReceiver(
      in_dir.c_str(),
      max_files_to_store,
      0,
      0));
#else
  return FileReceiver_var(
    new FileReceiver(
      in_dir.c_str(),
      max_files_to_store,
      0,
      0));
#endif
}

void
setup()
{
  char pwd[2048];
  fs::file_system.getcwd(pwd, sizeof(pwd));
  in_dir = std::string(pwd) + "/in_dir";
  fe_dir = std::string(pwd) + "/fe_dir";

  fs::file_system.mkdir(in_dir);
  fs::file_system.mkdir(fe_dir);
  fs::file_system.create(fe_dir, "UserOp.20121030.105356.271081.67127731.10.3");
  fs::file_system.create(fe_dir, "UserOp.20121030.105356.271081.67127731.10.5");
}

void
teardown()
{
  fs::file_system.rmrf(in_dir);
  fs::file_system.rmrf(fe_dir);
}


TEST_EX(SimpleTest, setup, teardown)
{
  FileReceiver_var file_receiver = make_receiver(2);
  ASSERT_FALSE (file_receiver->empty());

  fs::file_system.create(
    fe_dir,
    "~UserOp.20121030.105356.271081.67127731.10.2.commit.srv-dev2.ocslab.com");

  fs::file_system.create(
    fe_dir,
    "Click.20121030.105356.271081.67127731.10.2.commit.srv-dev2.ocslab.com");

  std::string str;
  FileReceiver::FileGuard_var file = file_receiver->get_eldest(str);
  ASSERT_TRUE (file.in() == NULL);
  ASSERT_TRUE (file_receiver->empty());

  ASSERT_TRUE (file_receiver->fetch_files(fe_dir.c_str(), "UserOp"));
  ASSERT_FALSE (file_receiver->empty());

  {
    FileReceiver::FileGuard_var file = file_receiver->get_eldest(str);
    ASSERT_TRUE (file.in());
  }

  ASSERT_FALSE (file_receiver->empty());

  {
    FileReceiver::FileGuard_var file = file_receiver->get_eldest(str);
    ASSERT_TRUE (file.in());
  }

  ASSERT_TRUE (file_receiver->empty());
}

TEST_EX(CommitFileModeTest, setup, teardown)
{
  FileReceiver_var file_receiver = make_receiver(10);
  ASSERT_FALSE (file_receiver->empty());

  fs::file_system.create(fe_dir, "UserOp.20121030.105356.271081.67127731.10.4.C");
  fs::file_system.create(fe_dir, "UserOp.20121030.105356.271081.67127731.10.6.C");

  fs::file_system.create(
    fe_dir,
    "~UserOp.20121030.105356.271081.67127731.10.2.commit.srv-dev2.ocslab.com");

  fs::file_system.create(
    fe_dir,
    "~UserOp.20121030.105356.271081.67127731.10.4.commit.srv-dev2.ocslab.com");

  std::string str;
  FileReceiver::FileGuard_var file = file_receiver->get_eldest(str);
  ASSERT_TRUE (file.in() == NULL);
  ASSERT_TRUE (file_receiver->empty());

  ASSERT_TRUE (file_receiver->fetch_files(fe_dir.c_str(), "UserOp"));
  ASSERT_FALSE (file_receiver->empty());

  {
    FileReceiver::FileGuard_var file = file_receiver->get_eldest(str);
    ASSERT_TRUE (file.in() != NULL);
    ASSERT_EQUALS(file->file_name(), std::string("UserOp.20121030.105356.271081.67127731.10.3"))
  }

  {
    FileReceiver::FileGuard_var file = file_receiver->get_eldest(str);
    ASSERT_TRUE (file.in() != NULL);
    ASSERT_EQUALS(file->file_name(), std::string("UserOp.20121030.105356.271081.67127731.10.4"))
  }

  {
    FileReceiver::FileGuard_var file = file_receiver->get_eldest(str);
    ASSERT_TRUE (file.in() != NULL);
    ASSERT_EQUALS(file->file_name(), std::string("UserOp.20121030.105356.271081.67127731.10.5"))
  }

  ASSERT_TRUE (file_receiver->empty());

#ifdef EMULATE_FILES
  // Commit file without regular file must be removed
  fs::FileSystem::iterator it = fs::file_system.find(fe_dir.c_str());
  ASSERT_TRUE (it != fs::file_system.end());
  ASSERT_TRUE ((*it)->size() == 1);
#endif // EMULATE_FILES
}

TEST_EX(CommitFileInIntermediateDir, setup, teardown)
{
  FileReceiver_var file_receiver = make_receiver(2);
  ASSERT_FALSE (file_receiver->empty());

  fs::file_system.create(
    in_dir,
    "~UserOp.20121030.105356.271081.67127731.10.2.commit.srv-dev2.ocslab.com");

  std::string str;
  FileReceiver::FileGuard_var file = file_receiver->get_eldest(str);
  ASSERT_TRUE (file.in() == NULL);
  ASSERT_TRUE (file_receiver->empty());
}

TEST_EX(FetchIntermediateTest, setup, teardown)
{
  FileReceiver_var file_receiver = make_receiver(2);
  ASSERT_TRUE (file_receiver->fetch_files(fe_dir.c_str(), "UserOp"));

  fs::file_system.create(fe_dir, "UserOp.20121030.105356.271081.67127731.10.1");
  fs::file_system.create(fe_dir, "UserOp.20121030.105356.271081.67127731.10.4");

  ASSERT_TRUE (file_receiver->fetch_files(fe_dir.c_str(), "UserOp"));

  std::string str;
  FileReceiver::FileGuard_var file1 = file_receiver->get_eldest(str);
  ASSERT_EQUALS (file1->file_name(), "UserOp.20121030.105356.271081.67127731.10.1");

  FileReceiver::FileGuard_var file2 = file_receiver->get_eldest(str);
  ASSERT_EQUALS (file2->file_name(), "UserOp.20121030.105356.271081.67127731.10.3");

  FileReceiver::FileGuard_var file3 = file_receiver->get_eldest(str);
  ASSERT_EQUALS (file3->file_name(), "UserOp.20121030.105356.271081.67127731.10.4");

  FileReceiver::FileGuard_var file4 = file_receiver->get_eldest(str);
  ASSERT_EQUALS (file4->file_name(), "UserOp.20121030.105356.271081.67127731.10.5");
}

SyncPolicy::Mutex mutex;

#ifdef EMULATE_FILES
const int FETCH_SIZE = 2000;
#else
const int FETCH_SIZE = 300;
#endif

class GetEldestTask
  : public Generics::Task, public ReferenceCounting::AtomicImpl
{
public:
  GetEldestTask(
    FileReceiver_var file_receiver,
    bool revert = false)
    noexcept
    : file_receiver_(file_receiver), revert_(revert)
  {}

  virtual
  ~GetEldestTask() noexcept
  {}

  virtual void
  execute() /*throw(eh::Exception)*/
  {
    std::string str;
    FileReceiver::FileGuard_var file = file_receiver_->get_eldest(str);

    if (file.in())
    {
      if (revert_)
      {
        file->revert();
      }
      else
      {
        assert(fs::file_system.remove(file->full_path()));
      }
    }
  }

private:
  const FileReceiver_var file_receiver_;
  const bool revert_;
};

class FetchFilesTask
  : public Generics::Task, public ReferenceCounting::AtomicImpl
{
public:
  FetchFilesTask(FileReceiver_var file_receiver) noexcept;

  virtual
  ~FetchFilesTask() noexcept;

  virtual void
  execute() /*throw(eh::Exception)*/;

private:
  FileReceiver_var file_receiver_;

private:
  void
  new_file(int number) /*throw(eh::Exception)*/;
};

class ThrowCallback
  : public Generics::ActiveObjectCallback, public ReferenceCounting::AtomicImpl
{
public:
  virtual void
  report_error(Severity severity, const String::SubString& description,
    const char* error_code = 0) noexcept;

protected:
  virtual
  ~ThrowCallback() noexcept
  {}
};

void
ThrowCallback::report_error(Severity, const String::SubString& description,
  const char*) noexcept
{
  std::cerr << description.str() << std::endl;
}


FetchFilesTask::FetchFilesTask(FileReceiver_var file_receiver) noexcept
  : file_receiver_(file_receiver)
{}

FetchFilesTask::~FetchFilesTask() noexcept
{}

void
FetchFilesTask::new_file(int number) /*throw(eh::Exception)*/
{
  std::ostringstream os;
  os << "UserOp.20121030.105356.271081.67127731.10." << number;
  fs::file_system.create(fe_dir, os.str());
}

void
FetchFilesTask::execute() /*throw(eh::Exception)*/
{
  static volatile _Atomic_word file_counter = 0;

  for (int i = 0; i < FETCH_SIZE; ++i)
  {
    new_file(__gnu_cxx::__exchange_and_add(&file_counter, 1));
  }

  file_receiver_->fetch_files(fe_dir.c_str(), "UserOp");
}

TEST_EX(ConcurrentTest, setup, teardown)
{
  Generics::ActiveObjectCallback_var callback = new ThrowCallback();
  Generics::TaskRunner_var get_runner(new Generics::TaskRunner(callback, 15));
  Generics::TaskRunner_var fetch_runner(new Generics::TaskRunner(callback, 1));

  FileReceiver_var file_receiver = make_receiver(100);
  const int FETCH_COUNT = 50;
  const int GET_COUNT = 10000;

  for (int i = 0; i < FETCH_COUNT; ++i)
  {
    fetch_runner->enqueue_task(new FetchFilesTask(file_receiver));
  }

  for (int i = 0; i < GET_COUNT; ++i)
  {
    get_runner->enqueue_task(Generics::Task_var(
      new GetEldestTask(file_receiver, (i % 10 == 0))));
  }

  get_runner->activate_object();
  fetch_runner->activate_object();

  while (fetch_runner->task_count())
  {
    get_runner->enqueue_task(Generics::Task_var(new GetEldestTask(file_receiver)));
  }

  get_runner->wait_for_queue_exhausting();
  get_runner->deactivate_object();
  get_runner->wait_object();

  fetch_runner->wait_for_queue_exhausting();
  fetch_runner->deactivate_object();
  fetch_runner->wait_object();
}

#ifdef EMULATE_FILES

Sync::Conditional cond_var;
bool can_release_file = false;

class ReleaseFileTask
  : public Generics::Task, public ReferenceCounting::AtomicImpl
{
public:
  ReleaseFileTask(FileReceiver::FileGuard_var file_guard) noexcept;

  virtual
  ~ReleaseFileTask() noexcept;

  virtual void
  execute() /*throw(eh::Exception)*/;

private:
  FileReceiver::FileGuard_var file_guard_;
};

ReleaseFileTask::ReleaseFileTask(FileReceiver::FileGuard_var file_guard) noexcept
  : file_guard_(file_guard)
{}

ReleaseFileTask::~ReleaseFileTask() noexcept
{}

void
ReleaseFileTask::execute() /*throw(eh::Exception)*/
{
  {
  Sync::ConditionalGuard guard(cond_var, mutex);

  while (!can_release_file)
  {
    guard.wait();
  }
  }

  {
    FileReceiver::FileGuard_var empty;
    file_guard_.swap(empty);
  }
}

void
read_file_hook(const std::string& file_name) noexcept
{
  const std::string expected = std::string(in_dir) + "/UserOp.20121030.105356.271081.67127731.10.3";
  if (file_name == expected)
  {
    Sync::PosixGuard guard(mutex);
    can_release_file = true;
    cond_var.signal();
  }
}

TEST_EX(FileGuardDestructorDuringInternalFetch, setup, teardown)
{
  OverridedFileReceiver* receiver(new OverridedFileReceiver(in_dir.c_str(), 1));
  FileReceiver_var file_receiver(receiver);
  ASSERT_TRUE (file_receiver->fetch_files(fe_dir.c_str(), "UserOp"));

  std::string res;
  FileReceiver::FileGuard_var fg = file_receiver->get_eldest(res);
  ASSERT_EQUALS (fg->file_name(), "UserOp.20121030.105356.271081.67127731.10.3");

  Generics::ActiveObjectCallback_var callback = new ThrowCallback();
  Generics::TaskRunner_var runner(new Generics::TaskRunner(callback, 1));
  receiver->read_internal_hook = read_file_hook;
  runner->enqueue_task(new ReleaseFileTask(fg));

  {
    FileReceiver::FileGuard_var empty;
    fg.swap(empty);
  }

  runner->activate_object();

  fg = file_receiver->get_eldest(res);

  runner->wait_for_queue_exhausting();
  runner->deactivate_object();
  runner->wait_object();

  ASSERT_EQUALS (fg->file_name(), "UserOp.20121030.105356.271081.67127731.10.5");
}

class TestFileProcessor : public FileProcessor, public ReferenceCounting::AtomicImpl
{
public:
  TestFileProcessor() noexcept
    : files_counter(0)
  {}

  virtual void
  process(FileReceiver::FileGuard* file_ptr)
    noexcept
  {
    if (file_ptr)
    {
      Sync::ConditionalGuard guard(cond);
      ++files_counter;
      cond.signal();
    }
  }

protected:
  virtual
  ~TestFileProcessor() noexcept
  {}

public:
  Sync::Condition cond;
  std::size_t files_counter;
};

TEST_EX(FileReceiverActiveObject, setup, teardown)
{
  FileReceiver_var file_receiver = make_receiver(10);
  std::string res;
  FileReceiver::FileGuard_var file = file_receiver->get_eldest(res);
  ASSERT_TRUE (file_receiver->empty());

  Generics::ActiveObjectCallback_var callback = new ThrowCallback();
  ReferenceCounting::AssertPtr<TestFileProcessor>::Ptr file_processor =
    new TestFileProcessor();

  Generics::ActiveObject_var active_file_receiver =
    new FileThreadProcessor(
      file_processor,
      callback,
      1,
      file_receiver,
      fe_dir.c_str(),
      "UserOp",
      Generics::Time(3));

  active_file_receiver->activate_object();
  sleep(3);

  {
    Sync::ConditionalGuard guard(file_processor->cond);

    while (file_processor->files_counter < 2)
    {
      guard.wait();
    }
  }

  active_file_receiver->deactivate_object();
  active_file_receiver->wait_object();

  ASSERT_TRUE (file_receiver->empty());
}

#endif //EMULATE_FILES

TEST_EX(DuplicateNameTest, setup, teardown)
{
  FileReceiver_var file_receiver = make_receiver(2);
  ASSERT_FALSE (file_receiver->empty());

  ASSERT_TRUE (file_receiver->fetch_files(fe_dir.c_str(), "UserOp"));
  fs::file_system.create(fe_dir, "UserOp.20121030.105356.271081.67127731.10.3");

  try
  {
    file_receiver->fetch_files(fe_dir.c_str(), "UserOp");
    FAIL ("exception for duplicate filename MUST be thrown");
  }
  catch (eh::Exception& ex)
  {
    const std::string msg = ex.what();
    const std::string expected_msg =
        "FileReceiver::fetch_file_(): duplicated file name: 'UserOp.20121030.105356.271081.67127731.10.3'";
    ASSERT_EQUALS (msg, expected_msg);
  }
  catch (...)
  {
    FAIL ("incorrect exception type for duplicate filename");
  }
}

TEST_EX(MoveTest, setup, teardown)
{
  FileReceiver_var file_receiver = make_receiver(2);
  ASSERT_FALSE (file_receiver->empty());

  ASSERT_TRUE (file_receiver->fetch_files(fe_dir.c_str(), "UserOp"));

  const std::string tmp_dir = "./tmp";
  const std::string file1 = "UserOp.20121030.105356.271081.67127731.10.903";
  const std::string file2 = "UserOp.20121030.105356.271081.67127731.10.4";

  fs::file_system.mkdir(tmp_dir);
  fs::file_system.create(tmp_dir, file1);
  fs::file_system.create(tmp_dir, file2);

  file_receiver->move((tmp_dir + '/' + file1).c_str());
  file_receiver->move((tmp_dir + '/' + file2).c_str());

  std::string str;
  auto file_guard = file_receiver->get_eldest(str);
  ASSERT_EQUALS (file_guard->file_name(), "UserOp.20121030.105356.271081.67127731.10.3");
  fs::file_system.remove(file_guard->full_path());

  file_guard = file_receiver->get_eldest(str);
  ASSERT_EQUALS (file_guard->file_name(), file2);
  fs::file_system.remove(file_guard->full_path());

  file_guard = file_receiver->get_eldest(str);
  ASSERT_EQUALS (file_guard->file_name(), "UserOp.20121030.105356.271081.67127731.10.5");
  fs::file_system.remove(file_guard->full_path());

  file_guard = file_receiver->get_eldest(str);
  ASSERT_EQUALS (file_guard->file_name(), file1);
  fs::file_system.remove(file_guard->full_path());

  ASSERT_TRUE (file_receiver->empty());
}

TEST_EX(RevertTest, setup, teardown)
{
  FileReceiver_var file_receiver = make_receiver(2);
  ASSERT_FALSE (file_receiver->empty());

  std::string str;
  ASSERT_TRUE (file_receiver->fetch_files(fe_dir.c_str(), "UserOp"));
  ASSERT_FALSE (file_receiver->empty());

  FileReceiver::FileGuard_var file1 = file_receiver->get_eldest(str);
  ASSERT_TRUE (file1.in() != NULL);
  ASSERT_TRUE (file1->file_name() == "UserOp.20121030.105356.271081.67127731.10.3");

  ASSERT_FALSE (file_receiver->empty());

  FileReceiver::FileGuard_var file2 = file_receiver->get_eldest(str);
  ASSERT_TRUE (file2.in() != NULL);
  ASSERT_TRUE (file2->file_name() == "UserOp.20121030.105356.271081.67127731.10.5");

  ASSERT_TRUE (file_receiver->empty());

  file1->revert();
  ASSERT_FALSE (file_receiver->empty());

  FileReceiver::FileGuard_var file3 = file_receiver->get_eldest(str);
  ASSERT_TRUE (file3.in() != NULL);
  ASSERT_TRUE (file3->file_name() == "UserOp.20121030.105356.271081.67127731.10.3");
  ASSERT_TRUE (file_receiver->empty());
}

TEST(NonExistsIntermediate)
{
  FileReceiver_var file_receiver =
    new FileReceiver(
      "/non_exists_dir",
      10, // max_files_to_store
      nullptr, // interrupter
      nullptr); // logger

  ASSERT_FALSE (file_receiver->empty());
  std::string new_top;
  FileReceiver::FileGuard_var file = file_receiver->get_eldest(new_top);
  ASSERT_FALSE (file);
  ASSERT_TRUE (new_top.empty());
  ASSERT_TRUE (file_receiver->empty());
}

RUN_TESTS
