#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/utilities/db_ttl.h>

#include <Stream/MemoryStream.hpp>

#include "RocksDBProfileMap.hpp"

namespace AdServer
{
namespace ProfilingCommons
{
  RocksDBProfileMapImpl::RocksDBProfileMapImpl(
    const String::SubString& path,
    const Generics::Time& expire_time)
    : path_(path.str())
  {
    static const char* FUN = "RocksDBProfileMapImpl::RocksDBProfileMapImpl()";

    rocksdb::Options options;
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    options.create_if_missing = true;

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

  Generics::ConstSmartMemBuf_var
  RocksDBProfileMapImpl::get_profile(
    const std::string& key,
    Generics::Time* /*last_access_time*/)
  {
    static const char* FUN = "RocksDBProfileMapImpl::get_profile()";

    std::string value;
    rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), key.c_str(), &value);

    if(status.IsNotFound())
    {
      return Generics::ConstSmartMemBuf_var();
    }

    if(!status.ok())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't read record from DB: " << path_;
      throw Exception(ostr.str());
    }

    return Generics::ConstSmartMemBuf_var(
      new Generics::ConstSmartMemBuf(value.data(), value.size()));
  }

  bool
  RocksDBProfileMapImpl::remove_profile(
    const std::string& key,
    OperationPriority)
  {
    rocksdb::Status status = db_->Delete(rocksdb::WriteOptions(), key.c_str());

    if(status.IsNotFound() || !status.ok())
    {
      return false;
    }

    /*
    if(!status.ok())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't read record from DB: " << path_;
      throw Exception(ostr.str());
    }
    */

    return true;
  }

  void
  RocksDBProfileMapImpl::save_profile(
    const std::string& key,
    const Generics::ConstSmartMemBuf* profile,
    const Generics::Time&,
    OperationPriority)
  {
    static const char* FUN = "RocksDBProfileMapImpl::save_profile()";

    rocksdb::Status status = db_->Put(
      rocksdb::WriteOptions(),
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
}
}
