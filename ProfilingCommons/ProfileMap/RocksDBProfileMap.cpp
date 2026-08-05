#include <rocksdb/db.h>
#include <rocksdb/iterator.h>
#include <rocksdb/utilities/db_ttl.h>

#include <exception>

#include <Stream/MemoryStream.hpp>

#include "RocksDBProfileMap.hpp"
#include "RocksDBOptions.hpp"

namespace AdServer::ProfilingCommons
{
  RocksDBProfileMapImpl::RocksDBProfileMapImpl(
    const String::SubString& path,
    const Generics::Time& expire_time,
    bool disable_wal)
    : path_(path.str()),
      db_(nullptr),
      disable_wal_(disable_wal)
  {
    static const char* FUN = "RocksDBProfileMapImpl::RocksDBProfileMapImpl()";

    rocksdb::Options options;
    configure_rocksdb_profile_map_options(options);

    rocksdb::Status status = rocksdb::DBWithTTL::Open(
      options,
      path.str().c_str(),
      &db_,
      expire_time.tv_sec);

    if(!status.ok())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't open DB: " << path;
      throw Exception(ostr.str());
    }
  }

  RocksDBProfileMapImpl::~RocksDBProfileMapImpl() noexcept
  {
    db_->Close();
    delete db_;
  }

  bool
  RocksDBProfileMapImpl::check_profile(const std::string& key) const
  {
    static const char* FUN = "RocksDBProfileMapImpl::check_profile()";

    logical_read_operations_.fetch_add(1, std::memory_order_relaxed);
    physical_read_operations_.fetch_add(1, std::memory_order_relaxed);

    std::string value;
    rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), key.c_str(), &value);

    if(status.IsNotFound())
    {
      return false;
    }

    if(!status.ok())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't read record from DB: " << path_;
      throw Exception(ostr.str());
    }

    return true;
  }

  void
  RocksDBProfileMapImpl::check_profile_async(const std::string& key, CheckCallback callback) const
  {
    if(!callback)
    {
      return;
    }

    bool exists = false;
    std::optional<std::string> error;
    try
    {
      exists = check_profile(key);
    }
    catch(const std::exception& ex)
    {
      error = ex.what();
    }
    catch(...)
    {
      error = "unknown check error";
    }

    callback(exists, std::move(error));
  }

  Generics::ConstSmartMemBuf_var
  RocksDBProfileMapImpl::get_profile(const std::string& key, Generics::Time* last_access_time)
  {
    Generics::SmartMemBuf_var profile = get_own_profile(
      key,
      last_access_time);

    return profile.in() ?
      Generics::transfer_membuf(profile) :
      Generics::ConstSmartMemBuf_var();
  }

  Generics::SmartMemBuf_var
  RocksDBProfileMapImpl::get_own_profile(
    const std::string& key,
    Generics::Time* /*last_access_time*/)
  {
    static const char* FUN = "RocksDBProfileMapImpl::get_own_profile()";

    logical_read_operations_.fetch_add(1, std::memory_order_relaxed);
    physical_read_operations_.fetch_add(1, std::memory_order_relaxed);

    std::string value;
    rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), key.c_str(), &value);

    if(status.IsNotFound())
    {
      return Generics::SmartMemBuf_var();
    }

    if(!status.ok())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't read record from DB: " << path_;
      throw Exception(ostr.str());
    }

    return Generics::SmartMemBuf_var(
      new Generics::SmartMemBuf(value.data(), value.size()));
  }

  Generics::ConstSmartMemBuf_var
  RocksDBProfileMapImpl::get_profile_async(
    const std::string& key,
    GetCallback callback,
    std::optional<Generics::Time> last_access_time)
  {
    if(!callback)
    {
      return Generics::ConstSmartMemBuf_var();
    }

    Generics::ConstSmartMemBuf_var profile;
    std::optional<std::string> error;
    try
    {
      Generics::Time access_time;
      profile = get_profile(
        key,
        last_access_time ? &access_time : nullptr);
    }
    catch(const std::exception& ex)
    {
      error = ex.what();
    }
    catch(...)
    {
      error = "unknown get error";
    }

    callback(std::move(profile), std::move(error));

    return Generics::ConstSmartMemBuf_var();
  }

  Generics::SmartMemBuf_var
  RocksDBProfileMapImpl::get_own_profile_async(
    const std::string& key,
    GetOwnCallback callback,
    std::optional<Generics::Time> last_access_time)
  {
    if(!callback)
    {
      return Generics::SmartMemBuf_var();
    }

    Generics::SmartMemBuf_var profile;
    std::optional<std::string> error;
    try
    {
      Generics::Time access_time;
      profile = get_own_profile(
        key,
        last_access_time ? &access_time : nullptr);
    }
    catch(const std::exception& ex)
    {
      error = ex.what();
    }
    catch(...)
    {
      error = "unknown get error";
    }

    callback(std::move(profile), std::move(error));

    return Generics::SmartMemBuf_var();
  }

  bool
  RocksDBProfileMapImpl::remove_profile(const std::string& key, OperationPriority)
  {
    logical_write_operations_.fetch_add(1, std::memory_order_relaxed);
    physical_write_operations_.fetch_add(1, std::memory_order_relaxed);

    rocksdb::WriteOptions write_options;
    write_options.disableWAL = disable_wal_;

    rocksdb::Status status = db_->Delete(write_options, key.c_str());

    if(status.IsNotFound() || !status.ok())
    {
      return false;
    }

    return true;
  }

  void
  RocksDBProfileMapImpl::remove_profile_async(
    const std::string& key,
    OperationPriority op_priority,
    RemoveCallback callback)
  {
    bool result = false;
    std::optional<std::string> error;
    try
    {
      result = remove_profile(key, op_priority);
    }
    catch(const std::exception& ex)
    {
      error = ex.what();
    }
    catch(...)
    {
      error = "unknown remove error";
    }

    if(callback)
    {
      callback(result, std::move(error));
    }
  }

  void
  RocksDBProfileMapImpl::clear_expired_async(const Generics::Time&, CompleteCallback complete)
  {
    if(complete)
    {
      complete();
    }
  }

  void
  RocksDBProfileMapImpl::process_keys(
    std::function<void(const std::string&)> process_key,
    std::function<void(void)> process_complete)
    /*throw(Exception)*/
  {
    static const char* FUN = "RocksDBProfileMapImpl::process_keys()";

    std::unique_ptr<rocksdb::Iterator> it(
      db_->NewIterator(rocksdb::ReadOptions()));
    for(it->SeekToFirst(); it->Valid(); it->Next())
    {
      process_key(it->key().ToString());
    }

    const auto status = it->status();
    if(!status.ok())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't iterate DB '" << path_ << "': " <<
        status.ToString();
      throw Exception(ostr.str());
    }

    if(process_complete)
    {
      process_complete();
    }
  }

  void
  RocksDBProfileMapImpl::save_profile(
    const std::string& key,
    const Generics::ConstSmartMemBuf* profile,
    const Generics::Time&,
    OperationPriority)
  {
    static const char* FUN = "RocksDBProfileMapImpl::save_profile()";

    logical_write_operations_.fetch_add(1, std::memory_order_relaxed);
    physical_write_operations_.fetch_add(1, std::memory_order_relaxed);

    rocksdb::WriteOptions write_options;
    write_options.disableWAL = disable_wal_;

    rocksdb::Status status = db_->Put(
      write_options,
      key.c_str(),
      rocksdb::Slice(
        static_cast<const char*>(profile->membuf().data()),
        profile->membuf().size()));
    if(!status.ok())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't save record to DB '" << path_ << "': " << status.ToString();
      throw Exception(ostr.str());
    }
  }

  void
  RocksDBProfileMapImpl::save_profile_async(
    const std::string& key,
    const Generics::ConstSmartMemBuf* profile,
    const Generics::Time& now,
    SaveCallback callback)
  {
    std::optional<std::string> error;
    try
    {
      save_profile(key, profile, now, OP_RUNTIME);
    }
    catch(const std::exception& ex)
    {
      error = ex.what();
    }
    catch(...)
    {
      error = "unknown save error";
    }

    if(callback)
    {
      callback(std::move(error));
    }
  }

  unsigned long
  RocksDBProfileMapImpl::size() const noexcept
  {
    return 1;
  }

  unsigned long
  RocksDBProfileMapImpl::area_size() const noexcept
  {
    return 1;
  }

  ProfileMap<std::string>::Stats
  RocksDBProfileMapImpl::stats() const noexcept
  {
    return {
      logical_read_operations_.load(std::memory_order_relaxed),
      logical_write_operations_.load(std::memory_order_relaxed),
      physical_read_operations_.load(std::memory_order_relaxed),
      physical_write_operations_.load(std::memory_order_relaxed)
    };
  }

  void
  RocksDBProfileMapImpl::flush()
  {
    static const char* FUN = "RocksDBProfileMapImpl::flush()";

    rocksdb::FlushOptions options;
    options.wait = true;

    const auto status = db_->Flush(options);
    if(!status.ok())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't flush DB '" << path_ << "': " <<
        status.ToString();
      throw Exception(ostr.str());
    }
  }
}
