#ifndef USERINFOSVCS_USEROPERATIONGENERATOR_UIDCHANNELSNAPSHOT_HPP_
#define USERINFOSVCS_USEROPERATIONGENERATOR_UIDCHANNELSNAPSHOT_HPP_

#include <deque>
#include <ostream>
#include <string>
#include <set>
#include <vector>

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/Uuid.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Sync/PosixLock.hpp>

#include <UserInfoSvcs/UserInfoManager/UserOperationProcessor.hpp>

namespace AdServer
{
namespace UserInfoSvcs
{
  class UidChannelFile : public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    typedef std::deque<Generics::Uuid> UuidsPortion;
    typedef std::vector<UuidsPortion> Uuids;

  public:
    UidChannelFile(
      const char* full_path,
      const Generics::Time& timestamp)
      /*throw(eh::Exception)*/;

    void
    load(Generics::ActiveObject* interruptor = nullptr)
      /*throw(eh::Exception)*/;

    void
    clear() noexcept;

    void
    check(std::size_t size_limit = 100) /*throw(Exception)*/;

    void
    update_timestamp(const Generics::Time& timestamp) /*throw(eh::Exception)*/;

    void
    add(
      UserOperationProcessor& processor,
      const Generics::Time& timestamp,
      Generics::ActiveObject* interruptor) const
      /*throw(eh::Exception)*/;

    void
    remove(
      UserOperationProcessor& processor,
      Generics::ActiveObject* interruptor) const
      /*throw(eh::Exception)*/;

    void
    update(
      const UidChannelFile& new_file,
      UserOperationProcessor& processor,
      Generics::ActiveObject* interruptor) const
      /*throw(eh::Exception)*/;

    unsigned long
    channel_id() const noexcept;

    const std::string&
    name() const noexcept;

    const std::string&
    full_path() const noexcept;

    const Generics::Time&
    timestamp() const noexcept;

    std::size_t
    count() const noexcept;

  private:
    static const std::size_t PORTION_COUNT;

    std::string full_path_;
    std::string name_;
    unsigned long channel_id_;
    Generics::Time timestamp_;
    Uuids uuids_;

  private:
    virtual
    ~UidChannelFile() noexcept
    {}

    void
    match_(
      const Generics::Uuid& uuid,
      const Generics::Time& timestamp,
      UserOperationProcessor& processor) const
      /*throw(eh::Exception)*/;

    void
    remove_match_(
      const Generics::Uuid& uuid,
      const Generics::Time& timestamp,
      UserOperationProcessor& processor) const
      /*throw(eh::Exception)*/;
  };

  typedef ReferenceCounting::AssertPtr<UidChannelFile>::Ptr UidChannelFile_var;

  class UidChannelSnapshot : public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    class UidChannelFilePtrComparator
    {
    public:
      bool
      operator() (
        const UidChannelFile_var& arg1,
        const UidChannelFile_var& arg2) const
        noexcept;
    };

    typedef std::set<UidChannelFile_var, UidChannelFilePtrComparator> Files;

    enum OperationType
    {
      OT_ADD,
      OT_DELETE,
      OT_UPDATE,
      OT_REFRESH
    };

    struct Operation
    {
      OperationType type;
      UidChannelFile_var current_file;
      UidChannelFile_var new_file;

      Operation(
        OperationType t,
        UidChannelFile* curr_f,
        UidChannelFile* new_f = nullptr)
        noexcept;
    };

  public:
    UidChannelSnapshot(const char* path) /*throw(eh::Exception)*/;

    void
    load() /*throw(Exception)*/;

    std::vector<Operation>
    sync(
      const UidChannelSnapshot& temp_snapshot,
      unsigned long refresh_period,
      Generics::ActiveObject* interruptor = nullptr) const
      /*throw(eh::Exception)*/;

    void
    execute(
      const Operation& op,
      UserOperationProcessor& processor,
      Generics::ActiveObject* interruptor = nullptr)
      /*throw(eh::Exception)*/;

    void
    add_file(
      const UidChannelFile_var& new_file,
      UserOperationProcessor& processor,
      Generics::ActiveObject* interruptor = nullptr)
      /*throw(eh::Exception)*/;

    void
    remove_file(
      const UidChannelFile_var& file,
      UserOperationProcessor& processor,
      Generics::ActiveObject* interruptor = nullptr)
      /*throw(eh::Exception)*/;

    void
    refresh_file(
      const UidChannelFile_var& file,
      const Generics::Time& timestamp,
      UserOperationProcessor& processor,
      Generics::ActiveObject* interruptor = nullptr)
      /*throw(eh::Exception)*/;

    void
    update_file(
      const UidChannelFile_var& curr_file,
      const UidChannelFile_var& new_file,
      UserOperationProcessor& processor,
      Generics::ActiveObject* interruptor = nullptr)
      /*throw(eh::Exception)*/;

    std::size_t
    size() const noexcept;

  protected:
    virtual
    ~UidChannelSnapshot() noexcept
    {}

    bool
    fetch_file_(
      const char* full_path,
      const struct stat& st)
      /*throw(eh::Exception)*/;

    void
    rename_file_(
      const char* src_path,
      const char* dst_path)
      /*throw(eh::Exception)*/;

    void
    copy_file_(
      const char* src_path,
      const char* dst_path,
      Generics::ActiveObject* interruptor)
      /*throw(eh::Exception)*/;

  public:
    std::string path_;

    mutable Sync::PosixMutex files_lock_;
    Files files_;
  };

  typedef ReferenceCounting::AssertPtr<UidChannelSnapshot>::Ptr
    UidChannelSnapshot_var;
}
}

namespace AdServer
{
namespace UserInfoSvcs
{
  inline
  bool
  UidChannelSnapshot::UidChannelFilePtrComparator::operator() (
    const UidChannelFile_var& arg1,
    const UidChannelFile_var& arg2) const
    noexcept
  {
    return (arg1->channel_id() < arg2->channel_id());
  }

  inline
  UidChannelSnapshot::Operation::Operation(
    OperationType t,
    UidChannelFile* curr_f,
    UidChannelFile* new_f)
    noexcept
    : type(t),
      current_file(ReferenceCounting::add_ref(curr_f)),
      new_file(ReferenceCounting::add_ref(new_f))
  {}

  inline
  std::ostream&
  operator<< (
    std::ostream& os,
    const UidChannelSnapshot::Operation& op)
    /*throw(eh::Exception)*/
  {
    switch (op.type)
    {
    case UidChannelSnapshot::OT_ADD:
      os << "ADD: " << op.new_file->full_path();
      break;
    case UidChannelSnapshot::OT_DELETE:
      os << "DELETE: " << op.current_file->full_path();
      break;
    case UidChannelSnapshot::OT_REFRESH:
      os << "REFRESH: " << op.current_file->full_path();
      break;
    case UidChannelSnapshot::OT_UPDATE:
      os << "UPDATE: " << op.current_file->full_path() <<
        ", " << op.new_file->full_path();
      break;
    }

    return os;
  }
}
}

#endif /* USERINFOSVCS_USEROPERATIONGENERATOR_UIDCHANNELSNAPSHOT_HPP_ */
