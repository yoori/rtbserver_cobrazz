#include <utime.h>

#include <eh/Errno.hpp>
#include <Generics/DirSelector.hpp>

#include <Commons/PathManip.hpp>
#include <ProfilingCommons/PlainStorage3/FileReader.hpp>
#include <ProfilingCommons/PlainStorage3/FileWriter.hpp>

#include "UidChannelSnapshot.hpp"

namespace AdServer
{
namespace UserInfoSvcs
{
  namespace
  {
    const std::string CURR_TMP_PREFIX = ".$tmp_curr_";
    const std::string NEW_TMP_PREFIX = ".$tmp_new_";

    const std::string CHANNEL_FILE_NAME_PREFIX = "channel";

    const std::size_t PADDING_LENGTH = 24;

    class LoadGuard
    {
    public:
      LoadGuard(UidChannelFile* channel) /*throw(eh::Exception)*/
        : channel_(ReferenceCounting::add_ref(channel))
      {
        channel_->load();
      }

      ~LoadGuard() noexcept
      {
        channel_->clear();
      }

    private:
      UidChannelFile_var channel_;
    };

    inline
    bool
    is_active(Generics::ActiveObject* interruptor) noexcept
    {
      return (!interruptor || interruptor->active());
    }
  }

  /*
   * UidChannelFile
   */
  const std::size_t UidChannelFile::PORTION_COUNT = 1000;

  UidChannelFile::UidChannelFile(
    const char* full_path,
    const Generics::Time& timestamp)
    /*throw(eh::Exception)*/
    : full_path_(full_path), timestamp_(timestamp)
  {
    static const char* FUN = "UidChannelFile::UidChannelFile";

    PathManip::split_path(
      full_path_.c_str(),
      nullptr,
      &name_);

    if (name_.compare(0, CHANNEL_FILE_NAME_PREFIX.length(), CHANNEL_FILE_NAME_PREFIX) ||
        !(channel_id_ = std::atol(name_.c_str() + CHANNEL_FILE_NAME_PREFIX.length())))
    {
      Stream::Error ostr;
      ostr << FUN << ": can't parse channel id from '" << full_path_ << "'";
      throw Exception(ostr.str());
    }
  }

  void
  UidChannelFile::load(Generics::ActiveObject* interruptor) /*throw(eh::Exception)*/
  {
    static const char* FUN = "UidChannelFile::load";

    try
    {
      uuids_.clear();
      uuids_.resize(PORTION_COUNT);
      std::ifstream ifs(full_path_.c_str());

      if (!ifs) {
        eh::throw_errno_exception<Exception>(
          "can't open file '", full_path_.c_str(), "'");
      }

      std::string line;

      while (std::getline(ifs, line) && is_active(interruptor))
      {
        const Generics::Uuid uuid(line, line.length() == PADDING_LENGTH);
        uuids_[uuid.hash() % PORTION_COUNT].emplace_back(uuid);
      }

      for (auto it = uuids_.begin(); it != uuids_.end() && is_active(interruptor); ++it)
      {
        std::sort(it->begin(), it->end());
      }
    }
    catch (eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": name='" << name_ << "' got eh::Exception: " << ex.what();
      throw Exception(ostr.str());
    }
  }

  unsigned long
  UidChannelFile::channel_id() const noexcept
  {
    return channel_id_;
  }

  const std::string&
  UidChannelFile::name() const noexcept
  {
    return name_;
  }

  const std::string&
  UidChannelFile::full_path() const noexcept
  {
    return full_path_;
  }

  const Generics::Time&
  UidChannelFile::timestamp() const noexcept
  {
    return timestamp_;
  }

  std::size_t
  UidChannelFile::count() const noexcept
  {
    std::size_t c = 0;

    for (auto it = uuids_.begin(); it != uuids_.end(); ++it)
    {
      c += it->size();
    }

    return c;
  }

  void
  UidChannelFile::clear() noexcept
  {
    uuids_.clear();
  }

  void
  UidChannelFile::check(std::size_t size_limit) /*throw(Exception)*/
  {
    const std::size_t c = count();

    if (c < size_limit)
    {
      Stream::Error ostr;
      ostr << "UidChannelFile::check FAILED: name='" << name_ << "' count() = " << c <<
        " less than " << size_limit;
      throw Exception(ostr.str());
    }
  }

  void
  UidChannelFile::update_timestamp(const Generics::Time& timestamp)
    /*throw(eh::Exception)*/
  {
    const struct timeval times[2] = {timestamp, timestamp};

    if (utimes(full_path_.c_str(), times))
    {
      eh::throw_errno_exception<Exception>(
        "can't update modification time for file '", full_path_.c_str(), "'");
    }

    timestamp_ = timestamp;
  }

  void
  UidChannelFile::add(
    UserOperationProcessor& processor,
    const Generics::Time& timestamp,
    Generics::ActiveObject* interruptor) const
    /*throw(eh::Exception)*/
  {
    for (auto it = uuids_.begin(); it != uuids_.end(); ++it)
    {
      for (auto ui = it->begin(); ui != it->end() && is_active(interruptor); ++ui)
      {
        match_(*ui, timestamp, processor);
      }
    }
  }

  void
  UidChannelFile::remove(
    UserOperationProcessor& processor,
    Generics::ActiveObject* interruptor) const
    /*throw(eh::Exception)*/
  {
    for (auto it = uuids_.begin(); it != uuids_.end(); ++it)
    {
      for (auto ui = it->begin(); ui != it->end() && is_active(interruptor); ++ui)
      {
        remove_match_(*ui, timestamp_, processor);
      }
    }
  }

  void
  UidChannelFile::update(
    const UidChannelFile& new_file,
    UserOperationProcessor& processor,
    Generics::ActiveObject* interruptor) const
    /*throw(eh::Exception)*/
  {
    for (std::size_t i = 0; i < PORTION_COUNT; ++i)
    {
      const UuidsPortion& uuids = uuids_[i];
      const UuidsPortion& new_file_uuids = new_file.uuids_[i];

      UuidsPortion::const_iterator curr_it = uuids.begin();
      UuidsPortion::const_iterator new_it = new_file_uuids.begin();

      while (curr_it != uuids.end() &&
             new_it != new_file_uuids.end() &&
             is_active(interruptor))
      {
        if (*curr_it == *new_it)
        {
          match_(*new_it, new_file.timestamp_, processor);
          ++curr_it;
          ++new_it;
        }
        else if (*curr_it < *new_it)
        {
          remove_match_(*curr_it, timestamp_, processor);
          ++curr_it;
        }
        else // *curr_it > *new_it
        {
          match_(*new_it, new_file.timestamp_, processor);
          ++new_it;
        }
      }

      while (curr_it != uuids.end() && is_active(interruptor))
      {
        remove_match_(*curr_it, timestamp_, processor);
        ++curr_it;
      }

      while (new_it != new_file_uuids.end() && is_active(interruptor))
      {
        match_(*new_it, new_file.timestamp_, processor);
        ++new_it;
      }
    }
  }

  void
  UidChannelFile::match_(
    const Generics::Uuid& uuid,
    const Generics::Time& timestamp,
    UserOperationProcessor& processor) const
    /*throw(eh::Exception)*/
  {
    processor.add_audience_channels(
      uuid, AudienceChannelSet{ {channel_id(), timestamp} });
  }

  void
  UidChannelFile::remove_match_(
    const Generics::Uuid& uuid,
    const Generics::Time& timestamp,
    UserOperationProcessor& processor) const
    /*throw(eh::Exception)*/
  {
    processor.remove_audience_channels(
      uuid, AudienceChannelSet{ {channel_id(), timestamp} });
  }

  /*
   * UidChannelSnapshot
   */
  UidChannelSnapshot::UidChannelSnapshot(const char* path)
    /*throw(eh::Exception)*/
    : path_(path)
  {}

  void
  UidChannelSnapshot::load()
    /*throw(Exception)*/
  {
    static const char* FUN = "UidChannelSnapshot::load";

    try
    {
      Sync::PosixGuard guard(files_lock_);
      files_.clear();
      std::function<bool(const char*, const struct stat&)> functior =
        std::bind(&UidChannelSnapshot::fetch_file_, this, std::placeholders::_1, std::placeholders::_2);
      Generics::DirSelect::directory_selector(path_.c_str(), functior, "*");
    }
    catch (eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": got eh::Exception: " << ex.what();
      throw Exception(ostr.str());
    }
  }

  std::vector<UidChannelSnapshot::Operation>
  UidChannelSnapshot::sync(
    const UidChannelSnapshot& temp_snapshot,
    unsigned long refresh_period,
    Generics::ActiveObject* interruptor) const
    /*throw(eh::Exception)*/
  {
    Sync::PosixGuard guard(files_lock_);
    std::vector<Operation> operations;

    Files::const_iterator it = files_.begin();
    Files::const_iterator temp_it = temp_snapshot.files_.begin();

    while (it != files_.end() && temp_it != temp_snapshot.files_.end() &&
           is_active(interruptor))
    {
      const UidChannelFile_var& file = *it;
      const UidChannelFile_var& temp_file = *temp_it;

      if (file->channel_id() == temp_file->channel_id())
      {
        if (file->timestamp() < temp_file->timestamp())
        {
          operations.emplace_back(OT_UPDATE, file, temp_file);
        }
        else
        {
          const Generics::Time now = Generics::Time::get_time_of_day();
          Generics::Time timestamp = file->timestamp();
          timestamp += refresh_period;

          if (timestamp < now)
          {
            operations.emplace_back(OT_REFRESH, file,
              UidChannelFile_var(new UidChannelFile(file->full_path().c_str(), now)));
          }
        }

        ++it;
        ++temp_it;
      }
      else if (file->channel_id() < temp_file->channel_id())
      {
        operations.emplace_back(OT_DELETE, file);
        ++it;
      }
      else // file->channel_id() > temp_file->channel_id()
      {
        operations.emplace_back(OT_ADD, nullptr, temp_file);
        ++temp_it;
      }
    }

    while (it != files_.end() && is_active(interruptor))
    {
      operations.emplace_back(OT_DELETE, *it);
      ++it;
    }

    while (temp_it != temp_snapshot.files_.end() && is_active(interruptor))
    {
      operations.emplace_back(OT_ADD,  nullptr, *temp_it);
      ++temp_it;
    }

    return operations;
  }

  void
  UidChannelSnapshot::execute(
    const Operation& op,
    UserOperationProcessor& processor,
    Generics::ActiveObject* interruptor)
    /*throw(eh::Exception)*/
  {
    switch (op.type)
    {
    case UidChannelSnapshot::OT_ADD:
      add_file(op.new_file, processor, interruptor);
      break;
    case UidChannelSnapshot::OT_DELETE:
      remove_file(op.current_file, processor, interruptor);
      break;
    case UidChannelSnapshot::OT_REFRESH:
      refresh_file(op.current_file, op.new_file->timestamp(), processor, interruptor);
      break;
    case UidChannelSnapshot::OT_UPDATE:
      update_file(op.current_file, op.new_file, processor, interruptor);
      break;
    }
  }

  void
  UidChannelSnapshot::add_file(
    const UidChannelFile_var& new_file,
    UserOperationProcessor& processor,
    Generics::ActiveObject* interruptor)
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "UidChannelSnapshot::add_file";

    try
    {
      LoadGuard load_guard(new_file);
      new_file->check();
      new_file->add(processor, new_file->timestamp(), interruptor);

      const std::string file_path = path_ + '/' + new_file->name();
      const std::string tmp_file_path = path_ + '/' + NEW_TMP_PREFIX + new_file->name();
      copy_file_(new_file->full_path().c_str(), tmp_file_path.c_str(), interruptor);

      if (is_active(interruptor))
      {
        rename_file_(tmp_file_path.c_str(), file_path.c_str());

        UidChannelFile_var file = new UidChannelFile(file_path.c_str(), Generics::Time());
        file->update_timestamp(new_file->timestamp());

        Sync::PosixGuard guard(files_lock_);
        files_.insert(file);
      }
    }
    catch (eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": got eh::Exception: " << ex.what();
      throw Exception(ostr.str());
    }
  }

  void
  UidChannelSnapshot::remove_file(
    const UidChannelFile_var& file,
    UserOperationProcessor& processor,
    Generics::ActiveObject* interruptor)
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "UidChannelSnapshot::remove_file";

    try
    {
      LoadGuard load_guard(file);
      file->remove(processor, interruptor);

      if (is_active(interruptor))
      {
        Sync::PosixGuard guard(files_lock_);
        files_.erase(file);

        if (std::remove(file->full_path().c_str()))
        {
          eh::throw_errno_exception<Exception>(
            "can't remove file '", file->full_path().c_str(), "' : ");
        }
      }
    }
    catch (eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": got eh::Exception: " << ex.what();
      throw Exception(ostr.str());
    }
  }

  void
  UidChannelSnapshot::refresh_file(
    const UidChannelFile_var& file,
    const Generics::Time& timestamp,
    UserOperationProcessor& processor,
    Generics::ActiveObject* interruptor)
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "UidChannelSnapshot::refresh_file";

    try
    {
      LoadGuard load_guard(file);
      file->add(processor, timestamp, interruptor);

      if (is_active(interruptor))
      {
        file->update_timestamp(timestamp);
      }
    }
    catch (eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": got eh::Exception: " << ex.what();
      throw Exception(ostr.str());
    }
  }

  void
  UidChannelSnapshot::update_file(
    const UidChannelFile_var& curr_file,
    const UidChannelFile_var& new_file,
    UserOperationProcessor& processor,
    Generics::ActiveObject* interruptor)
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "UidChannelSnapshot::update_file";

    try
    {
      LoadGuard curr_load_guard(curr_file);
      LoadGuard new_load_guard(new_file);
      new_file->check();
      curr_file->update(*new_file, processor, interruptor);

      const std::string tmp_curr_path = path_ + '/' + CURR_TMP_PREFIX + curr_file->name();
      const std::string tmp_new_path = path_ + '/' + NEW_TMP_PREFIX + new_file->name();

      copy_file_(new_file->full_path().c_str(), tmp_new_path.c_str(), interruptor);

      if (is_active(interruptor))
      {
        rename_file_(curr_file->full_path().c_str(), tmp_curr_path.c_str());
        rename_file_(tmp_new_path.c_str(), curr_file->full_path().c_str());

        if (std::remove(tmp_curr_path.c_str()))
        {
          eh::throw_errno_exception<Exception>(
            "can't remove file '", tmp_curr_path.c_str(), "' : ");
        }

        curr_file->update_timestamp(new_file->timestamp());
      }
    }
    catch (eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": got eh::Exception: " << ex.what();
      throw Exception(ostr.str());
    }
  }

  bool
  UidChannelSnapshot::fetch_file_(
    const char* full_path,
    const struct stat& st)
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "UidChannelSnapshot::fetch_file_";

    try
    {
      files_.insert(UidChannelFile_var(
        new UidChannelFile(full_path, Generics::Time(st.st_mtim.tv_sec))));
    }
    catch (eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": got eh::Exception: " << ex.what();
      throw Exception(ostr.str());
    }

    return true;
  }

  void
  UidChannelSnapshot::copy_file_(
    const char* src_path,
    const char* dst_path,
    Generics::ActiveObject* interruptor)
    /*throw(eh::Exception)*/
  {
    AdServer::ProfilingCommons::FileReader file_reader(src_path, BUFSIZ);
    AdServer::ProfilingCommons::FileWriter file_writer(dst_path, BUFSIZ);

    char buf[BUFSIZ];
    unsigned long size = 0;

    while (!file_reader.eof() &&
           (size = file_reader.read(buf, BUFSIZ)) &&
           (!interruptor || interruptor->active()))
    {
      file_writer.write(buf, size);
    }
  }

  void
  UidChannelSnapshot::rename_file_(
    const char* src_path,
    const char* dst_path)
    /*throw(eh::Exception)*/
  {
    if (::rename(src_path, dst_path))
    {
      eh::throw_errno_exception<Exception>(
        "failed to rename file '", src_path, "' to '", dst_path, "'");
    }
  }

  std::size_t
  UidChannelSnapshot::size() const noexcept
  {
    return files_.size();
  }
}
}
