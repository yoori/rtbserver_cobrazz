#ifndef USERINFOSVCS_USERBINDCHUNKTYPES_HPP
#define USERINFOSVCS_USERBINDCHUNKTYPES_HPP

// STD
#include <chrono>
#include <filesystem>
#include <fstream>
#include <shared_mutex>
#include <sstream>

// UNIXCOMMONS
#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/RocksDB/DataBase.hpp>

// THIS
#include <Commons/UserInfoManip.hpp>
#include <UserInfoSvcs/UserBindServer/ExternalIdHashAdapter.hpp>
#include <UserInfoSvcs/UserBindServer/FetchableHashTable.hpp>
#include <UserInfoSvcs/UserBindServer/Utils.hpp>

namespace AdServer::UserInfoSvcs
{

extern std::uint16_t get_current_day_from_2000();

class SeenUserInfoHolder final
{
private:
  using TimeOffset = std::int32_t;

public:
  explicit SeenUserInfoHolder() noexcept;

  std::uint16_t init_day() const noexcept;

  void reset_init_day() noexcept;

  bool need_save() const noexcept;

  Generics::Time get_time(const Generics::Time& base_time) const noexcept;

  void set_time(
    const Generics::Time& time,
    const Generics::Time& base_time) noexcept;

  void adjust_time(
    const Generics::Time& old_base_time,
    const Generics::Time& new_base_time) noexcept;

  bool is_init() const noexcept;

  auto operator<=>(const SeenUserInfoHolder&) const = default;

private:
  friend std::ostream& operator<<(
    std::ostream& stream,
    const SeenUserInfoHolder& holder);
  friend std::istream& operator>>(
    std::istream& stream,
    SeenUserInfoHolder& holder);

  static const TimeOffset TIME_OFFSET_NOT_INIT_;
  static const Generics::Time TIME_OFFSET_MIN_;
  static const Generics::Time TIME_OFFSET_MAX_;

  TimeOffset first_seen_time_offset_;
  std::uint16_t initial_day_ = 0;
};

struct BoundUserInfoHolder final
{
  enum BoundFlags
  {
    BF_SETCOOKIE = 1
  };

  explicit BoundUserInfoHolder() noexcept;

  std::uint16_t init_day() const noexcept;

  void reset_init_day() noexcept;

  bool need_save() const noexcept;

  auto operator<=>(const BoundUserInfoHolder&) const = default;

  Commons::UserId user_id;
  unsigned char flags = 0;
  std::uint8_t bad_event_count = 0;
  std::uint16_t last_bad_event_day = 0;
  std::uint16_t initial_day = 0;
};

extern std::ostream& operator<<(
  std::ostream& stream,
  const BoundUserInfoHolder& holder);

extern std::istream& operator>>(
  std::istream& stream,
  BoundUserInfoHolder& holder);

class StringDefHashAdapter final
{
public:
  StringDefHashAdapter() noexcept = default;

  StringDefHashAdapter(
    const std::string_view& text,
    const std::size_t hash);

  StringDefHashAdapter(
    const String::SubString& text,
    const std::size_t hash);

  StringDefHashAdapter(
    const std::string& text,
    const std::size_t hash);

  StringDefHashAdapter(
    const char* text,
    const std::size_t hash);

  StringDefHashAdapter(StringDefHashAdapter&& init) noexcept;

  StringDefHashAdapter(const StringDefHashAdapter& init);

  StringDefHashAdapter& operator=(
    const StringDefHashAdapter& init);

  StringDefHashAdapter& operator=(
    StringDefHashAdapter&& init) noexcept;

  ~StringDefHashAdapter();

  bool operator==(
    const StringDefHashAdapter& right) const noexcept;

  std::size_t hash() const noexcept;

  std::string_view str() const noexcept;

private:
  friend std::ostream& operator<<(
    std::ostream& stream,
    const StringDefHashAdapter& adapter);
  friend std::istream& operator>>(
    std::istream& stream,
    StringDefHashAdapter& adapter);

private:
  void* data_ = nullptr;
};

template<typename T>
concept HashAdapterConcept = requires(T t)
{
  { t.hash() } -> std::same_as<size_t>;
};

class HashHashAdapter final
{
public:
  HashHashAdapter(const std::size_t hash_val = 0) noexcept;

  template<HashAdapterConcept HashAdapterType>
  HashHashAdapter(const HashAdapterType& init);

  bool operator==(const HashHashAdapter& right) const noexcept;

  std::size_t hash() const noexcept;

  std::string str() const;

private:
  friend std::istream& operator>>(
    std::istream& stream,
    HashHashAdapter& adapter);

  std::size_t hash_;
};

class RocksdbColumnFamily final : private Generics::Uncopyable
{
public:
  using DB = UServerUtils::Grpc::RocksDB::DataBase;
  using DBPtr = std::shared_ptr<DB>;
  using Handle = rocksdb::ColumnFamilyHandle;

public:
  explicit RocksdbColumnFamily(
    const DBPtr& db,
    const std::string& name);

  ~RocksdbColumnFamily() = default;

  rocksdb::Status get(
    const rocksdb::ReadOptions& options,
    const rocksdb::Slice& key,
    std::string* value);

  rocksdb::Status put(
    const rocksdb::WriteOptions& options,
    const rocksdb::Slice& key,
    const rocksdb::Slice& value);

  rocksdb::Status erase(
    const rocksdb::WriteOptions& options,
    const rocksdb::Slice& key);

private:
  DBPtr db_;

  Handle& handle_;
};

using RocksdbColumnFamilyPtr = std::shared_ptr<RocksdbColumnFamily>;

extern std::vector<RocksdbColumnFamilyPtr> create_rocksdb(
  Logging::Logger* logger,
  const std::string& db_path,
  const std::vector<std::string>& column_family_names,
  const std::size_t number_threads,
  const rocksdb::CompactionStyle compaction_style,
  const std::uint32_t block_сache_size_mb,
  const std::uint32_t ttl);

template<class Key, class Value>
class Container : private Generics::Uncopyable
{
public:
  Container() = default;

  virtual ~Container() = default;

  virtual bool get(Value& value, const Key& key) = 0;

  virtual bool get(Value& value, Key&& key) = 0;

  virtual void set(const Key& key, const Value& value) = 0;

  virtual void set(Key&& key, Value&& value) = 0;

  virtual void set(const Key& key, Value&& value) = 0;

  virtual void set(Key&& key, const Value& value) = 0;

  virtual bool erase(const Key& key) = 0;

  virtual bool erase(Key&& key) = 0;
};

template<class Key, class Value>
using ContainerPtr = std::shared_ptr<Container<Key, Value>>;

template<class Key, class Value>
class LoaderDelegate : private Generics::Uncopyable
{
public:
  LoaderDelegate() = default;

  virtual ~LoaderDelegate() = default;

  virtual void on_load(Key&& key, Value&& value) = 0;
};

template<class Key, class Value>
using LoaderDelegatePtr = std::shared_ptr<LoaderDelegate<Key, Value>>;

template<class Key, class Value>
class Loader final : private Generics::Uncopyable
{
public:
  using Delegate = LoaderDelegate<Key, Value>;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit Loader(
    const std::string& file_path,
    Delegate& delegate);

  void load();

private:
  const std::string file_path_;

  Delegate& delegate_;
};

class Saver : private Generics::Uncopyable
{
public:
  Saver() = default;

  virtual ~Saver() = default;

  virtual void save(std::ostream& stream) = 0;
};

using SaverPtr = std::shared_ptr<Saver>;

class FilterDate final
{
public:
  FilterDate(const std::uint16_t number_days)
   : number_days_(number_days)
  {
  }

  template<class Value>
  bool operator()(const Value& value) noexcept
  {
    return current_day < value.init_day() + number_days_;
  }

private:
  const std::uint16_t number_days_ = 10;

  const std::uint16_t current_day = get_current_day_from_2000();
};

class FilterAlwaysTrue final
{
public:
  FilterAlwaysTrue() = default;

  template<class Value>
  bool operator()(const Value& value) noexcept
  {
    return true;
  }
};

template<
  class Key,
  class Value,
  class Filter = FilterAlwaysTrue,
  template<typename, typename> class HashTable = USFetchableHashTable>
class MemoryContainer final
  : public Container<Key, Value>,
    private LoaderDelegate<Key, Value>,
    public Saver
{
public:
  using Fetcher = typename HashTable<Key, Value>::Fetcher<Filter>;
  using FetchArray = typename HashTable<Key, Value>::FetchArray;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  
public:
  MemoryContainer(
    const std::size_t actual_fetch_size = 100,
    const std::size_t max_fetch_size = 1000,
    const Filter& filter = Filter{},
    const FetcherDelegatePtr<Key, Value>& delegate = {});

  ~MemoryContainer() override = default;

  bool get(Value& value, const Key& key) override;

  bool get(Value& value, Key&& key) override;

  void set(const Key& key, const Value& value) override;

  void set(Key&& key, Value&& value) override;

  void set(const Key& key, Value&& value) override;

  void set(Key&& key, const Value& value) override;

  bool erase(const Key& key) override;

  bool erase(Key&& key) override;

  void save(std::ostream& stream) override;

private:
  void on_load(Key&& key, Value&& value) override;

private:
  const std::size_t actual_fetch_size_;

  const std::size_t max_fetch_size_;

  const Filter filter_;

  const FetcherDelegatePtr<Key, Value> delegate_;

  HashTable<Key, Value> hash_table_;
};

template<
  class Key,
  class Value,
  class Filter = FilterAlwaysTrue,
  template<typename, typename> class HashTable = USFetchableHashTable>
using MemoryContainerPtr = std::shared_ptr<MemoryContainer<Key, Value, Filter, HashTable>>;

struct RocksdbParams final : Generics::Uncopyable
{
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using ReadOptions = rocksdb::ReadOptions;
  using ReadOptionsPtr = std::shared_ptr<ReadOptions>;
  using WriteOptions = rocksdb::WriteOptions;
  using WriteOptionsPtr = std::shared_ptr<WriteOptions>;

  RocksdbParams(
    Logger* logger,
    const RocksdbColumnFamilyPtr& column_family,
    const ReadOptionsPtr& read_options,
    const WriteOptionsPtr& write_options)
    : logger(ReferenceCounting::add_ref(logger)),
      column_family(column_family),
      read_options(read_options),
      write_options(write_options)
  {
  }

  const Logger_var logger;
  const RocksdbColumnFamilyPtr column_family;
  const ReadOptionsPtr read_options;
  const WriteOptionsPtr write_options;
};

using RocksdbParamsPtr = std::shared_ptr<RocksdbParams>;

template<class Key, class Value>
class RocksdbContainer final
  : public Container<Key, Value>,
    public FetcherDelegate<Key, Value>
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit RocksdbContainer(
    const RocksdbParamsPtr& rocksdb_params,
    const std::string& prefix_key = {});

  ~RocksdbContainer() override = default;

  bool get(Value& value, const Key& key) override;

  bool get(Value& value, Key&& key) override;

  void set(const Key& key, const Value& value) override;

  void set(Key&& key, Value&& value) override;

  void set(const Key& key, Value&& value) override;

  void set(Key&& key, const Value& value) override;

  bool erase(const Key& key) override;

  bool erase(Key&& key) override;

private:
  void on_hashtable_erase(Key&& key, Value&& value) noexcept override;

private:
  const RocksdbParamsPtr rocksdb_params_;

  const std::string prefix_key_;
};

template<class PrefixKey, class SuffixKey, class Value>
class RocksdbContainer<std::pair<PrefixKey, SuffixKey>, Value> final
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit RocksdbContainer(const RocksdbParamsPtr& rocksdb_params);

  ~RocksdbContainer() = default;

  bool get(
    Value& value,
    const PrefixKey& prefix_key,
    const SuffixKey& suffix_key);

  void set(
    const PrefixKey& prefix_key,
    const SuffixKey& suffix_key,
    const Value& value);

  bool erase(
    const PrefixKey& prefix_key,
    const SuffixKey& suffix_key);

private:
  const RocksdbParamsPtr rocksdb_params_;
};

template<class Key, class Value>
using RocksdbContainerPtr = std::shared_ptr<RocksdbContainer<Key, Value>>;

template<class Key, class Value>
class RocksdbContainerFactory : private Generics::Uncopyable
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using ReadOptions = rocksdb::ReadOptions;
  using ReadOptionsPtr = std::shared_ptr<ReadOptions>;
  using WriteOptions = rocksdb::WriteOptions;
  using WriteOptionsPtr = std::shared_ptr<WriteOptions>;

public:
  RocksdbContainerFactory(const RocksdbParamsPtr& rocksdb_params);

  ~RocksdbContainerFactory() = default;

  RocksdbContainerPtr<Key, Value> create(const std::string& key_prefix);

private:
  const RocksdbParamsPtr rocksdb_params_;
};

template<class Key, class Value>
using RocksdbContainerFactoryPtr = std::shared_ptr<RocksdbContainerFactory<Key, Value>>;

template<class PrefixKey, class SuffixKey, class Value>
class Loader<std::pair<PrefixKey, SuffixKey>, Value> final : private Generics::Uncopyable
{
public:
  using Key = std::pair<PrefixKey, SuffixKey>;
  using Delegate = LoaderDelegate<Key, Value>;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit Loader(
    const std::string& file_path,
    Delegate& delegate);

  void load();

private:
  const std::string file_path_;

  Delegate& delegate_;
};

template<class T, class E>
concept LRReferenceConcept = std::is_same_v<std::decay_t<T>, E>;

template<
  class PrefixKey,
  class SuffixKey,
  class Value,
  class Filter = FilterAlwaysTrue,
  template<typename, typename> class HashTable = USFetchableHashTable>
class BoundContainers :
  public Saver,
  public LoaderDelegate<std::pair<PrefixKey, SuffixKey>, Value>,
  private Generics::Uncopyable
{
private:
  static constexpr std::size_t kSize = 2;

  using MemoryContainerT = MemoryContainer<SuffixKey, Value, Filter, HashTable>;
  using MemoryContainerPtrT = MemoryContainerPtr<SuffixKey, Value, Filter, HashTable>;
  using ContainerPtrT = ContainerPtr<SuffixKey, Value>;
  using ContainersHashTable = Generics::GnuHashTable<PrefixKey, MemoryContainerPtrT>;
  using RocksdbContainerFactoryPtrT = RocksdbContainerFactoryPtr<SuffixKey, Value>;
  using RocksdbContainerPtrT = RocksdbContainerPtr<std::pair<PrefixKey, SuffixKey>, Value>;

public:
  BoundContainers(
    const std::size_t actual_fetch_size,
    const std::size_t max_fetch_size,
    const Filter& filter,
    const RocksdbParamsPtr& rocksdb_params);

  ~BoundContainers() override = default;

  ContainerPtrT get(
    Value& value,
    const PrefixKey& prefix_key,
    const SuffixKey& suffix_key);

  template<
    LRReferenceConcept<PrefixKey> PKey,
    LRReferenceConcept<SuffixKey> SKey,
    LRReferenceConcept<Value> V>
  void set(
    PKey&& prefix_key,
    SKey&& suffix_key,
    V&& value);

  bool erase(
    const PrefixKey& prefix_key,
    const SuffixKey& suffix_key);

  void save(std::ostream& stream) override;

private:
  void on_load(std::pair<PrefixKey, SuffixKey>&& key, Value&& value);

private:
  const std::size_t actual_fetch_size_;

  const std::size_t max_fetch_size_;

  const Filter filter_;

  const RocksdbContainerFactoryPtrT rocksdb_factory_;

  const RocksdbContainerPtrT rocksdb_container_;

  ContainersHashTable hash_table_;

  mutable std::shared_mutex mutex_;
};

template<
  class PrefixKey,
  class SuffixKey,
  class Value,
  class Filter = FilterAlwaysTrue,
  template<typename, typename> class HashTable = USFetchableHashTable>
using BoundContainersPtr = std::shared_ptr<BoundContainers<PrefixKey, SuffixKey, Value, Filter, HashTable>>;

template<
  class Key,
  class Value,
  class Filter = FilterAlwaysTrue,
  template<typename, typename> class HashTable = USFetchableHashTable>
class SeenContainers :
  public Saver,
  public LoaderDelegate<Key, Value>,
  private Generics::Uncopyable
{
private:
  static constexpr std::size_t kSize = 2;

  using MemoryContainerT = MemoryContainer<Key, Value, Filter, HashTable>;
  using MemoryContainerPtrT = MemoryContainerPtr<Key, Value, Filter, HashTable>;
  using RocksdbContainerT = RocksdbContainer<Key, Value>;
  using RocksdbContainerPtrT = RocksdbContainerPtr<Key, Value>;
  using ContainerPtrT = ContainerPtr<Key, Value>;
  using Containers = std::array<ContainerPtrT, 2>;

public:
  SeenContainers(
    const std::size_t actual_fetch_size,
    const std::size_t max_fetch_size,
    const Filter& filter,
    const RocksdbParamsPtr& rocksdb_params);

  ~SeenContainers() override = default;

  ContainerPtrT get(Value& value, const Key& key);

  template<
    LRReferenceConcept<Key> K,
    LRReferenceConcept<Value> V>
  void set(K&& key, V&& value);

  bool erase(const Key& key);

  void save(std::ostream& stream) override;

private:
  void on_load(Key&& key, Value&& value);

private:
  Containers containers_;
};

template<
  class Key,
  class Value,
  class Filter = FilterAlwaysTrue,
  template<typename, typename> class HashTable = USFetchableHashTable>
using SeenContainersPtr = std::shared_ptr<SeenContainers<Key, Value, Filter, HashTable>>;

class Portion final
  : private SeenContainers<
             HashHashAdapter,
             SeenUserInfoHolder,
             FilterDate,
             USFetchableHashTable>,
    private BoundContainers<
             StringDefHashAdapter,
             ExternalIdHashAdapter,
             BoundUserInfoHolder,
             FilterDate,
             SparseFetchableHashTable>,
    private Generics::Uncopyable
{
public:
  using SeenRocksdbContainerFactory = RocksdbContainerFactory<
    HashHashAdapter,
    SeenUserInfoHolder>;
  using SeenRocksdbContainerFactoryPtr = std::shared_ptr<SeenRocksdbContainerFactory>;
  using BoundRocksdbContainerFactory = RocksdbContainerFactory<
    ExternalIdHashAdapter,
    BoundUserInfoHolder>;
  using BoundRocksdbContainerFactoryPtr = std::shared_ptr<BoundRocksdbContainerFactory>;
  using SeenContainersT = SeenContainers<
    HashHashAdapter,
    SeenUserInfoHolder,
    FilterDate,
    USFetchableHashTable>;
  using BoundContainersT = BoundContainers<
    StringDefHashAdapter,
    ExternalIdHashAdapter,
    BoundUserInfoHolder,
    FilterDate,
    SparseFetchableHashTable>;

public:
  Portion(
    const std::size_t actual_fetch_size,
    const std::size_t max_fetch_size,
    const FilterDate& filter,
    const RocksdbParamsPtr& seen_rocksdb_params,
    const RocksdbParamsPtr& bound_rocksdb_params);

  ~Portion() override = default;

  template<
    LRReferenceConcept<StringDefHashAdapter> PKey,
    LRReferenceConcept<ExternalIdHashAdapter> SKey,
    LRReferenceConcept<BoundUserInfoHolder> V>
  void set_bound(
    PKey&& prefix_key,
    SKey&& suffix_key,
    V&& value);

  auto get_bound(
    BoundUserInfoHolder& value,
    const StringDefHashAdapter& prefix_key,
    const ExternalIdHashAdapter& suffix_key);

  bool erase_bound(
    const StringDefHashAdapter& prefix_key,
    const ExternalIdHashAdapter& suffix_key);

  template<
    LRReferenceConcept<HashHashAdapter> K,
    LRReferenceConcept<SeenUserInfoHolder> V>
  void set_seen(K&& key, V&& value);

  auto get_seen(
    SeenUserInfoHolder& value,
    const HashHashAdapter& key);

  bool erase_seen(const HashHashAdapter& key);

private:
  friend class Portions;
};

using PortionPtr = std::shared_ptr<Portion>;

class Portions final :
  private Generics::Uncopyable,
  private LoaderDelegate<HashHashAdapter, SeenUserInfoHolder>,
  private LoaderDelegate<std::pair<StringDefHashAdapter, ExternalIdHashAdapter>, BoundUserInfoHolder>
{
public:
  using Logger = Logging::Logger;
  using SeenRocksdbContainerFactoryPtr = Portion::SeenRocksdbContainerFactoryPtr;
  using BoundRocksdbContainerFactoryPtr = Portion::BoundRocksdbContainerFactoryPtr;
  using LoadFilter = std::function<bool(const std::size_t&)>;

private:
  using SeenContainersT = Portion::SeenContainersT;
  using BoundContainersT = Portion::BoundContainersT;
  using Array = std::vector<PortionPtr>;

public:
  explicit Portions(
    Logging::Logger* logger,
    const std::size_t portions_number,
    const std::string& directory,
    const std::string& seen_name,
    const std::string& bound_name,
    const std::string& rocksdb_path,
    const std::size_t rocksdb_number_threads,
    const rocksdb::CompactionStyle rocksdb_compaction_style,
    const std::uint32_t rocksdb_block_сache_size_mb,
    const std::uint32_t rocksdb_ttl,
    const std::size_t actual_fetch_size,
    const std::size_t max_fetch_size,
    const FilterDate& filter,
    const LoadFilter& load_filter = [] (const std::size_t) {return true;});

  ~Portions() = default;

  std::size_t hash(const String::SubString& id) const noexcept;

  PortionPtr portion(const std::size_t id_hash) const noexcept;

  void save();

private:
  void on_load(
    HashHashAdapter&& key,
    SeenUserInfoHolder&& value) override;

  void on_load(
    std::pair<StringDefHashAdapter, ExternalIdHashAdapter>&& key,
    BoundUserInfoHolder&& value) override;

private:
  std::size_t portion_index(const std::size_t id_hash) const noexcept;

private:
  void load();

private:
  const std::string directory_;

  const std::string seen_name_;

  const std::string bound_name_;

  const LoadFilter load_filter_;

  Array array_;
};

using PortionsPtr = std::shared_ptr<Portions>;

} // namespace AdServer::UserInfoSvcs

namespace AdServer::UserInfoSvcs
{

inline std::ostream& operator<<(
  std::ostream& stream,
  const SeenUserInfoHolder& holder)
{
  stream << holder.first_seen_time_offset_
         << ' '
         << holder.initial_day_;
  return stream;
}

inline std::istream& operator>>(
  std::istream& stream,
  SeenUserInfoHolder& holder)
{
  stream >> holder.first_seen_time_offset_
         >> holder.initial_day_;
  return stream;
}

inline std::ostream& operator<<(
  std::ostream& stream,
  const BoundUserInfoHolder& holder)
{
  stream << holder.user_id
         << ' '
         << holder.flags
         << ' '
         << static_cast<std::uint16_t>(holder.bad_event_count)
         << ' '
         << holder.last_bad_event_day
         << ' '
         << holder.initial_day;
  return stream;
}

inline std::istream& operator>>(
  std::istream& stream,
  BoundUserInfoHolder& holder)
{
  stream >> holder.user_id
         >> holder.flags;

  std::uint16_t bad_event_count_helper;
  stream >> bad_event_count_helper;
  holder.bad_event_count = static_cast<uint8_t>(bad_event_count_helper);

  stream >> holder.last_bad_event_day
         >> holder.initial_day;
  return stream;
}

inline std::ostream& operator<<(
  std::ostream& stream,
  const StringDefHashAdapter& adapter)
{
  const auto& str = adapter.str();
  stream << (str.empty() ? "###" : str);
  return stream;
}

inline std::istream& operator>>(
  std::istream& stream,
  StringDefHashAdapter& adapter)
{
  static std::string empty("###");

  std::string text;
  stream >> text;
  if (text.size() == 3 && text == empty)
  {
    text = std::string();
  }

  const auto hash = Utils::hash(text);
  StringDefHashAdapter other(text, hash);
  adapter = std::move(other);
  return stream;
}

inline std::ostream& operator<<(
  std::ostream& stream,
  const HashHashAdapter& adapter)
{
  stream << adapter.hash();
}

inline std::istream& operator>>(
  std::istream& stream,
  HashHashAdapter& adapter)
{
  stream >> adapter.hash_;
}

inline bool SeenUserInfoHolder::need_save() const noexcept
{
  return true;
}

inline std::uint16_t SeenUserInfoHolder::init_day() const noexcept
{
  return initial_day_;
}

inline void SeenUserInfoHolder::reset_init_day() noexcept
{
  initial_day_ = get_current_day_from_2000();
}

inline bool BoundUserInfoHolder::need_save() const noexcept
{
  return !user_id.is_null();
}

inline std::uint16_t BoundUserInfoHolder::init_day() const noexcept
{
  return initial_day;
}

inline void BoundUserInfoHolder::reset_init_day() noexcept
{
  initial_day = get_current_day_from_2000();
}

inline StringDefHashAdapter::StringDefHashAdapter(
  const std::string_view& text,
  const std::size_t hash)
{
  data_ = ::malloc(text.size() + sizeof(std::size_t) + 1);
  if (!data_)
  {
    throw std::bad_alloc();
  }

  *static_cast<std::size_t*>(data_) = hash;
  ::memcpy(
    static_cast<unsigned char*>(data_) + sizeof(std::size_t),
    text.data(),
    text.size());
  static_cast<char*>(data_)[sizeof(std::size_t) + text.size()] = 0;
}

inline StringDefHashAdapter::StringDefHashAdapter(
  const String::SubString& text,
  const std::size_t hash)
  : StringDefHashAdapter(
      std::string_view(text.data(), text.size()),
      hash)
{
}

inline StringDefHashAdapter::StringDefHashAdapter(
  const std::string& text,
  const std::size_t hash)
  : StringDefHashAdapter(
      std::string_view(text.data(), text.size()),
      hash)
{
}

inline StringDefHashAdapter::StringDefHashAdapter(
  const char* text,
  const std::size_t hash)
  : StringDefHashAdapter(
      std::string_view(text),
      hash)
{
}

inline StringDefHashAdapter::StringDefHashAdapter(
  StringDefHashAdapter&& other) noexcept
{
  data_ = other.data_;
  other.data_ = nullptr;
}

inline StringDefHashAdapter::StringDefHashAdapter(
  const StringDefHashAdapter& other)
{
  if(other.data_)
  {
    const auto size = ::strlen(
      static_cast<char*>(other.data_) + sizeof(std::size_t)) + sizeof(std::size_t) + 1;
    data_ = ::malloc(size);
    if (!data_)
    {
      throw std::bad_alloc();
    }

    ::memcpy(data_, other.data_, size);
  }
}

inline StringDefHashAdapter& StringDefHashAdapter::operator=(
  const StringDefHashAdapter& other)
{
  if (this != &other)
  {
    if(data_)
    {
      ::free(data_);
    }

    data_ = nullptr;
    if (other.data_)
    {
      const auto size = ::strlen(
        static_cast<char*>(other.data_) + sizeof(std::size_t)) + sizeof(std::size_t) + 1;
      data_ = ::malloc(size);
      if (!data_)
      {
        throw std::bad_alloc();
      }

      ::memcpy(data_, other.data_, size);
    }
  }

  return *this;
}

inline StringDefHashAdapter& StringDefHashAdapter::operator=(
  StringDefHashAdapter&& other) noexcept
{
  if (this != &other)
  {
    if(data_)
    {
      ::free(data_);
    }

    data_ = other.data_;
    other.data_ = nullptr;
  }

  return *this;
}

inline StringDefHashAdapter::~StringDefHashAdapter() noexcept
{
  if (data_)
  {
    ::free(data_);
  }
}

inline bool StringDefHashAdapter::operator==(
  const StringDefHashAdapter& right) const noexcept
{
  if (data_ && right.data_)
  {
    return ::strcmp(
      static_cast<char*>(data_) + sizeof(std::size_t),
      static_cast<char*>(right.data_) + sizeof(std::size_t)) == 0;
  }
  else if (!data_ && !right.data_)
  {
    return true;
  }
  else
  {
    return false;
  }
}

inline std::size_t StringDefHashAdapter::hash() const noexcept
{
  return data_ ? *static_cast<std::size_t*>(data_) : 0;
}

inline std::string_view StringDefHashAdapter::str() const noexcept
{
  return data_ ? std::string_view(static_cast<char*>(data_) + sizeof(std::size_t)) :
                 std::string_view();
}

inline HashHashAdapter::HashHashAdapter(const std::size_t hash_val) noexcept
  : hash_(hash_val)
{
}

template<HashAdapterConcept HashAdapterType>
HashHashAdapter::HashHashAdapter(const HashAdapterType& init)
  : hash_(init.hash())
{
}

inline bool HashHashAdapter::operator==(const HashHashAdapter& right) const noexcept
{
  return hash_ == right.hash_;
}

inline std::size_t HashHashAdapter::hash() const noexcept
{
  return hash_;
}

inline std::string HashHashAdapter::str() const
{
  return std::to_string(hash_);
}

template<class Key, class Value>
Loader<Key, Value>::Loader(
  const std::string& file_path,
  Delegate& delegate)
  : file_path_(file_path),
    delegate_(delegate)
{
}

template<class Key, class Value>
void Loader<Key, Value>::load()
{
  std::ifstream stream(file_path_, std::ios::in);
  if (!stream.is_open())
  {
    return;
  }
  if (stream.eof() || stream.peek() == std::char_traits<char>::eof())
  {
    return;
  }

  Key key;
  Value value;
  while (!stream.eof())
  {
    stream >> key >> value;
    if (stream.fail() || stream.bad())
    {
      Stream::Error stream;
      stream << FNS
             << "file="
             << file_path_
             << " is broken";
      throw Exception(stream);
    }

    try
    {
      delegate_.on_load(std::move(key), std::move(value));
    }
    catch (...)
    {
    }

    if (stream.eof() || stream.peek() == '\n')
    {
      break;
    }
  }
}

template<
  class Key,
  class Value,
  class Filter,
  template<typename, typename> class HashTable>
MemoryContainer<Key, Value, Filter, HashTable>::MemoryContainer(
  const std::size_t actual_fetch_size,
  const std::size_t max_fetch_size,
  const Filter& filter,
  const FetcherDelegatePtr<Key, Value>& delegate)
  : actual_fetch_size_(actual_fetch_size),
    max_fetch_size_(max_fetch_size),
    filter_(filter),
    delegate_(delegate)
{
}

template<
  class Key,
  class Value,
  class Filter,
  template<typename, typename> class HashTable>
bool MemoryContainer<Key, Value, Filter, HashTable>::get(Value& value, const Key& key)
{
  return hash_table_.get(value, key);
}

template<
  class Key,
  class Value,
  class Filter,
  template<typename, typename> class HashTable>
bool MemoryContainer<Key, Value, Filter, HashTable>::get(Value& value, Key&& key)
{
  return hash_table_.get(value, std::move(key));
}

template<
  class Key,
  class Value,
  class Filter,
  template<typename, typename> class HashTable>
void MemoryContainer<Key, Value, Filter, HashTable>::set(const Key& key, const Value& value)
{
  return hash_table_.set(key, value);
}

template<
  class Key,
  class Value,
  class Filter,
  template<typename, typename> class HashTable>
void MemoryContainer<Key, Value, Filter, HashTable>::set(Key&& key, Value&& value)
{
  return hash_table_.set(std::move(key), std::move(value));
}

template<
  class Key,
  class Value,
  class Filter,
  template<typename, typename> class HashTable>
void MemoryContainer<Key, Value, Filter, HashTable>::set(const Key& key, Value&& value)
{
  return hash_table_.set(key, std::move(value));
}

template<
  class Key,
  class Value,
  class Filter,
  template<typename, typename> class HashTable>
void MemoryContainer<Key, Value, Filter, HashTable>::set(Key&& key, const Value& value)
{
  return hash_table_.set(std::move(key), value);
}

template<
  class Key,
  class Value,
  class Filter,
  template<typename, typename> class HashTable>
bool MemoryContainer<Key, Value, Filter, HashTable>::erase(const Key& key)
{
  return hash_table_.erase(key);
}

template<
  class Key,
  class Value,
  class Filter,
  template<typename, typename> class HashTable>
bool MemoryContainer<Key, Value, Filter, HashTable>::erase(Key&& key)
{
  return hash_table_.erase(std::move(key));
}

template<
  class Key,
  class Value,
  class Filter,
  template<class, class> class HashTable>
void MemoryContainer<Key, Value, Filter, HashTable>::on_load(
  Key&& key,
  Value&& value)
{
  hash_table_.set(std::move(key), std::move(value));
}

template<
  class Key,
  class Value,
  class Filter,
  template<class, class> class HashTable>
void MemoryContainer<Key, Value, Filter, HashTable>::save(std::ostream& stream)
{
  auto fetcher = hash_table_.fetcher(filter_, delegate_);
  FetchArray values;
  bool is_end = false;
  bool need_white_space = false;
  while (!is_end)
  {
    is_end = !fetcher.get(
      values,
      actual_fetch_size_,
      max_fetch_size_);
    for (const auto& [key, value] : values)
    {
      if (value.need_save())
      {
        if (need_white_space)
        {
          stream << ' ';
        }
        stream << key << ' ' << value;
        need_white_space = true;
      }
    }
  }
}

inline RocksdbColumnFamily::RocksdbColumnFamily(
  const DBPtr& db,
  const std::string& name)
  : db_(db),
    handle_(db->column_family(name))
{
}

inline rocksdb::Status RocksdbColumnFamily::get(
  const rocksdb::ReadOptions& options,
  const rocksdb::Slice& key,
  std::string* value)
{
  return db_->get().Get(options, &handle_, key, value);
}

inline rocksdb::Status RocksdbColumnFamily::put(
  const rocksdb::WriteOptions& options,
  const rocksdb::Slice& key,
  const rocksdb::Slice& value)
{
  return db_->get().Put(options, &handle_, key, value);
}

inline rocksdb::Status RocksdbColumnFamily::erase(
  const rocksdb::WriteOptions& options,
  const rocksdb::Slice& key)
{
  return db_->get().Delete(options, &handle_, key);
}

template<class Key, class Value>
RocksdbContainer<Key, Value>::RocksdbContainer(
  const RocksdbParamsPtr& rocksdb_params,
  const std::string& prefix_key)
  : rocksdb_params_(rocksdb_params),
    prefix_key_(prefix_key)
{
}

template<class Key, class Value>
bool RocksdbContainer<Key, Value>::get(Value& value, const Key& key)
{
  try
  {
    std::string key_str(prefix_key_);
    key_str += key.str();

    const auto& column_family = rocksdb_params_->column_family;
    const auto& read_options = rocksdb_params_->read_options;

    std::string result;
    const auto status = column_family->get(
      *read_options,
      rocksdb::Slice(key_str.data(), key_str.size()),
      &result);
    if (status.ok())
    {
      std::istringstream stream(std::move(result));
      stream >> value;
      if (stream.fail() || stream.bad())
      {
        Stream::Error stream;
        stream << FNS
               << "stream is failed";
        throw Exception(stream);
      }

      return true;
    }
    else if (!status.IsNotFound())
    {
      Stream::Error stream;
      stream << FNS
             << status.ToString();
      throw Exception(stream);
    }
  }
  catch (const eh::Exception& exc)
  {
    const auto& logger = rocksdb_params_->logger;
    Stream::Error stream;
    stream << FNS
           << exc.what();
    logger->error(stream.str());
  }
  catch (...)
  {
    const auto& logger = rocksdb_params_->logger;
    Stream::Error stream;
    stream << FNS
           << "Unknown error";
    logger->error(stream.str());
  }

  return false;
}

template<class Key, class Value>
bool RocksdbContainer<Key, Value>::get(Value& value, Key&& key)
{
  return get(value, key);
}

template<class Key, class Value>
void RocksdbContainer<Key, Value>::set(const Key& key, const Value& value)
{
  try
  {
    std::string key_str(prefix_key_);
    key_str += key.str();

    std::string string_buffer;
    string_buffer.reserve(50);
    std::ostringstream stream(std::move(string_buffer));
    stream << value;
    if (stream.fail() || stream.bad())
    {
      Stream::Error stream;
      stream << FNS
             << "stream is failed";
      throw Exception(stream);
    }
    const auto value_str = stream.str();

    const auto& column_family = rocksdb_params_->column_family;
    const auto& write_options = rocksdb_params_->write_options;

    const auto status = column_family->put(
      *write_options,
      rocksdb::Slice(key_str.data(), key_str.size()),
      rocksdb::Slice(value_str.data(), value_str.size()));
    if (!status.ok())
    {
      Stream::Error stream;
      stream << FNS
             << status.ToString();
      throw Exception(stream);
    }
  }
  catch (const eh::Exception& exc)
  {
    const auto& logger = rocksdb_params_->logger;
    Stream::Error stream;
    stream << FNS
           << exc.what();
    logger->error(stream.str());
  }
  catch (...)
  {
    const auto& logger = rocksdb_params_->logger;
    Stream::Error stream;
    stream << FNS
           << "Unknown error";
    logger->error(stream.str());
  }
}

template<class Key, class Value>
void RocksdbContainer<Key, Value>::set(Key&& key, Value&& value)
{
  set(key, value);
}

template<class Key, class Value>
void RocksdbContainer<Key, Value>::set(const Key& key, Value&& value)
{
  set(key, value);
}

template<class Key, class Value>
void RocksdbContainer<Key, Value>::set(Key&& key, const Value& value)
{
  set(key, value);
}

template<class Key, class Value>
bool RocksdbContainer<Key, Value>::erase(const Key& key)
{
  try
  {
    std::string key_str(prefix_key_);
    key_str += key.str();

    const auto& column_family = rocksdb_params_->column_family;
    const auto& write_options = rocksdb_params_->write_options;

    const auto status = column_family->erase(
      *write_options,
      rocksdb::Slice(key_str.data(), key_str.size()));
    if (status.ok())
    {
      return true;
    }
    else if (!status.IsNotFound())
    {
      Stream::Error stream;
      stream << FNS
             << status.ToString();
      throw Exception(stream);
    }
  }
  catch (const eh::Exception& exc)
  {
    const auto& logger = rocksdb_params_->logger;
    Stream::Error stream;
    stream << FNS
           << exc.what();
    logger->error(stream.str());
  }
  catch (...)
  {
    const auto& logger = rocksdb_params_->logger;
    Stream::Error stream;
    stream << FNS
           << "Unknown error";
    logger->error(stream.str());
  }

  return false;
}

template<class Key, class Value>
bool RocksdbContainer<Key, Value>::erase(Key&& key)
{
  return erase(key);
}

template<class Key, class Value>
void RocksdbContainer<Key, Value>::on_hashtable_erase(
  Key&& key,
  Value&& value) noexcept
{
  try
  {
    set(std::move(key), std::move(value));
  }
  catch (...)
  {
  }
}

template<class PrefixKey, class SuffixKey, class Value>
RocksdbContainer<std::pair<PrefixKey, SuffixKey>, Value>::RocksdbContainer(
  const RocksdbParamsPtr& rocksdb_params)
  : rocksdb_params_(rocksdb_params)
{
}

template<class PrefixKey, class SuffixKey, class Value>
bool RocksdbContainer<std::pair<PrefixKey, SuffixKey>, Value>::get(
  Value& value,
  const PrefixKey& prefix_key,
  const SuffixKey& suffix_key)
{
   try
  {
    std::string key_str(prefix_key.str());
    key_str += '/';
    key_str += suffix_key.str();

    const auto& column_family = rocksdb_params_->column_family;
    const auto& read_options = rocksdb_params_->read_options;

    std::string result;
    const auto status = column_family->get(
      *read_options,
      rocksdb::Slice(key_str.data(), key_str.size()),
      &result);
    if (status.ok())
    {
      std::istringstream stream(std::move(result));
      stream >> value;
      if (stream.fail() || stream.bad())
      {
        Stream::Error stream;
        stream << FNS
               << "stream is failed";
        throw Exception(stream);
      }

      return true;
    }
    else if (!status.IsNotFound())
    {
      Stream::Error stream;
      stream << FNS
             << status.ToString();
      throw Exception(stream);
    }
  }
  catch (const eh::Exception& exc)
  {
    const auto& logger = rocksdb_params_->logger;
    Stream::Error stream;
    stream << FNS
           << exc.what();
    logger->error(stream.str());
  }
  catch (...)
  {
    const auto& logger = rocksdb_params_->logger;
    Stream::Error stream;
    stream << FNS
           << "Unknown error";
    logger->error(stream.str());
  }

  return false;
}

template<class PrefixKey, class SuffixKey, class Value>
void RocksdbContainer<std::pair<PrefixKey, SuffixKey>, Value>::set(
  const PrefixKey& prefix_key,
  const SuffixKey& suffix_key,
  const Value& value)
{
  try
  {
    std::string key_str(prefix_key.str());
    key_str += '/';
    key_str += suffix_key.str();

    std::string string_buffer;
    string_buffer.reserve(50);
    std::ostringstream stream(std::move(string_buffer));
    stream << value;
    if (stream.fail() || stream.bad())
    {
      Stream::Error stream;
      stream << FNS
             << "stream is failed";
      throw Exception(stream);
    }
    const auto value_str = stream.str();

    const auto& column_family = rocksdb_params_->column_family;
    const auto& write_options = rocksdb_params_->write_options;

    const auto status = column_family->put(
      *write_options,
      rocksdb::Slice(key_str.data(), key_str.size()),
      rocksdb::Slice(value_str.data(), value_str.size()));
    if (!status.ok())
    {
      Stream::Error stream;
      stream << FNS
             << status.ToString();
      throw Exception(stream);
    }
  }
  catch (const eh::Exception& exc)
  {
    const auto& logger = rocksdb_params_->logger;
    Stream::Error stream;
    stream << FNS
           << exc.what();
    logger->error(stream.str());
  }
  catch (...)
  {
    const auto& logger = rocksdb_params_->logger;
    Stream::Error stream;
    stream << FNS
           << "Unknown error";
    logger->error(stream.str());
  }
}

template<class PrefixKey, class SuffixKey, class Value>
bool RocksdbContainer<std::pair<PrefixKey, SuffixKey>, Value>::erase(
  const PrefixKey& prefix_key,
  const SuffixKey& suffix_key)
{
  try
  {
    std::string key_str(prefix_key.str());
    key_str += '/';
    key_str += suffix_key.str();

    const auto& column_family = rocksdb_params_->column_family;
    const auto& write_options = rocksdb_params_->write_options;

    const auto status = column_family->erase(
      *write_options,
      rocksdb::Slice(key_str.data(), key_str.size()));
    if (status.ok())
    {
      return true;
    }
    else if (!status.IsNotFound())
    {
      Stream::Error stream;
      stream << FNS
             << status.ToString();
      throw Exception(stream);
    }
  }
  catch (const eh::Exception& exc)
  {
    const auto& logger = rocksdb_params_->logger;
    Stream::Error stream;
    stream << FNS
           << exc.what();
    logger->error(stream.str());
  }
  catch (...)
  {
    const auto& logger = rocksdb_params_->logger;
    Stream::Error stream;
    stream << FNS
           << "Unknown error";
    logger->error(stream.str());
  }

  return false;
}

template<class Key, class Value>
RocksdbContainerFactory<Key, Value>::RocksdbContainerFactory(
  const RocksdbParamsPtr& rocksdb_params)
  : rocksdb_params_(rocksdb_params)
{
}

template<class Key, class Value>
RocksdbContainerPtr<Key, Value>
RocksdbContainerFactory<Key, Value>::create(const std::string& key_prefix)
{
  return std::make_shared<RocksdbContainer<Key, Value>>(
    rocksdb_params_,
    key_prefix);
}

template<class PrefixKey, class SuffixKey, class Value>
Loader<std::pair<PrefixKey, SuffixKey>, Value>::Loader(
  const std::string& file_path,
  Delegate& delegate)
  : file_path_(file_path),
    delegate_(delegate)
{
}

template<class PrefixKey, class SuffixKey, class Value>
void Loader<std::pair<PrefixKey, SuffixKey>, Value>::load()
{
  std::ifstream stream(file_path_, std::ios::in);
  if (!stream.is_open())
  {
    return;
  }
  if (stream.eof() || stream.peek() == std::char_traits<char>::eof())
  {
    return;
  }

  PrefixKey prefix_key;
  SuffixKey suffix_key;
  Value value;
  bool is_prefix_key = true;
  while (!stream.eof())
  {
    if (is_prefix_key)
    {
      stream >> prefix_key;
    }
    else
    {
      stream >> suffix_key >> value;
    }

    if (stream.fail() || stream.bad())
    {
      Stream::Error stream;
      stream << FNS
             << "file="
             << file_path_
             << " is broken";
      throw Exception(stream);
    }

    if (is_prefix_key)
    {
      is_prefix_key = false;
    }
    else
    {
      delegate_.on_load(
        std::make_pair(prefix_key, std::move(suffix_key)),
        std::move(value));
    }

    if (stream.peek() == '\n')
    {
      is_prefix_key = true;
      stream.get();
      if (stream.peek() == std::char_traits<char>::eof())
      {
        break;
      }
    }
  }
}

template<
  class PrefixKey,
  class SuffixKey,
  class Value,
  class Filter,
  template<typename, typename> class HashTable>
BoundContainers<PrefixKey, SuffixKey, Value, Filter, HashTable>::BoundContainers(
  const std::size_t actual_fetch_size,
  const std::size_t max_fetch_size,
  const Filter& filter,
  const RocksdbParamsPtr& rocksdb_params)
  : actual_fetch_size_(actual_fetch_size),
    max_fetch_size_(max_fetch_size),
    filter_(filter),
    rocksdb_factory_(std::make_shared<RocksdbContainerFactory<
      SuffixKey,
      Value>>(rocksdb_params)),
    rocksdb_container_(std::make_shared<RocksdbContainer<
      std::pair<PrefixKey, SuffixKey>,
      Value>>(rocksdb_params))
{
}

template<
  class PrefixKey,
  class SuffixKey,
  class Value,
  class Filter,
  template<typename, typename> class HashTable>
BoundContainers<PrefixKey, SuffixKey, Value, Filter, HashTable>::ContainerPtrT
BoundContainers<PrefixKey, SuffixKey, Value, Filter, HashTable>::get(
  Value& value,
  const PrefixKey& prefix_key,
  const SuffixKey& suffix_key)
{
  MemoryContainerPtrT memory_container;
  {
    std::shared_lock lock(mutex_);
    const auto it = hash_table_.find(prefix_key);
    if (it != hash_table_.end())
    {
      memory_container = it->second;
    }
  }

  if (memory_container && memory_container->get(value, suffix_key))
  {
    return memory_container;
  }
  else if (rocksdb_container_->get(value, prefix_key, suffix_key))
  {
    std::string prefix(prefix_key.str());
    prefix += '/';
    return rocksdb_factory_->create(prefix);
  }

  return {};
}

template<
  class PrefixKey,
  class SuffixKey,
  class Value,
  class Filter,
  template<typename, typename> class HashTable>
template<
  LRReferenceConcept<PrefixKey> PKey,
  LRReferenceConcept<SuffixKey> SKey,
  LRReferenceConcept<Value> V>
void BoundContainers<PrefixKey, SuffixKey, Value, Filter, HashTable>::set(
  PKey&& prefix_key,
  SKey&& suffix_key,
  V&& value)
{
  MemoryContainerPtrT memory_container;
  {
    std::shared_lock lock(mutex_);
    const auto it = hash_table_.find(prefix_key);
    if (it != hash_table_.end())
    {
      memory_container = it->second;
    }
  }

  if (!memory_container)
  {
    std::string prefix_str(prefix_key.str());
    prefix_str += '/';

    auto new_memory_container = std::make_shared<MemoryContainerT>(
      actual_fetch_size_,
      max_fetch_size_,
      filter_,
      rocksdb_factory_->create(prefix_str));

    std::unique_lock lock(mutex_);
    auto it = hash_table_.find(prefix_key);
    if (it == hash_table_.end())
    {
      it = hash_table_.emplace(
        std::forward<PKey>(prefix_key),
        std::move(new_memory_container)).first;
    }
    memory_container = it->second;
  }

  memory_container->set(
    std::forward<SKey>(suffix_key),
    std::forward<V>(value));
}

template<
  class PrefixKey,
  class SuffixKey,
  class Value,
  class Filter,
  template<typename, typename> class HashTable>
bool BoundContainers<PrefixKey, SuffixKey, Value, Filter, HashTable>::erase(
  const PrefixKey& prefix_key,
  const SuffixKey& suffix_key)
{
  MemoryContainerPtrT memory_container;
  {
    std::shared_lock lock(mutex_);
    const auto it = hash_table_.find(prefix_key);
    if (it != hash_table_.end())
    {
      memory_container = it->second;
    }
  }

  if (memory_container && memory_container->erase(suffix_key))
  {
    return true;
  }
  else if (rocksdb_container_->erase(prefix_key, suffix_key))
  {
    return true;
  }

  return false;
}

template<
  class PrefixKey,
  class SuffixKey,
  class Value,
  class Filter,
  template<typename, typename> class HashTable>
void BoundContainers<PrefixKey, SuffixKey, Value, Filter, HashTable>::on_load(
  std::pair<PrefixKey, SuffixKey>&& key,
  Value&& value)
{
  set(std::move(key.first), std::move(key.second), std::move(value));
}

template<
  class PrefixKey,
  class SuffixKey,
  class Value,
  class Filter,
  template<typename, typename> class HashTable>
void BoundContainers<PrefixKey, SuffixKey, Value, Filter, HashTable>::save(std::ostream& stream)
{
  using Savers = std::deque<std::pair<PrefixKey, SaverPtr>>;
  Savers savers;

  {
    std::shared_lock lock(mutex_);
    for (const auto& [key, container] : hash_table_)
    {
      savers.emplace_back(key, container);
    }
  }

  bool need_new_line = false;
  for (auto& [key, saver] : savers)
  {
    if (need_new_line)
    {
      stream << '\n';
    }
    stream << key << ' ';

    const auto start_position = stream.tellp();
    saver->save(stream);
    const auto end_position = stream.tellp();

    need_new_line = true;
    if (start_position == end_position)
    {
      stream.seekp(start_position - 1);
    }
  }
}

template<
  class Key,
  class Value,
  class Filter,
  template<typename, typename> class HashTable>
SeenContainers<Key, Value, Filter, HashTable>::SeenContainers(
  const std::size_t actual_fetch_size,
  const std::size_t max_fetch_size,
  const Filter& filter,
  const RocksdbParamsPtr& rocksdb_params)
{
  auto rocksdb_container = std::make_shared<RocksdbContainer<Key, Value>>(
    rocksdb_params,
    "");
  auto memory_container = std::make_shared<MemoryContainerT>(
    actual_fetch_size,
    max_fetch_size,
    filter,
    rocksdb_container);
  containers_[0] = memory_container;
  containers_[1] = rocksdb_container;
}

template<
  class Key,
  class Value,
  class Filter,
  template<typename, typename> class HashTable>
SeenContainers<Key, Value, Filter, HashTable>::ContainerPtrT
SeenContainers<Key, Value, Filter, HashTable>::get(Value& value, const Key& key)
{
  for (std::size_t i = 0; i < kSize; ++i)
  {
    if (containers_[i]->get(value, key))
    {
      return containers_[i];
    }
  }

  return {};
}

template<
  class Key,
  class Value,
  class Filter,
  template<typename, typename> class HashTable>
template<
  LRReferenceConcept<Key> K,
  LRReferenceConcept<Value> V>
void SeenContainers<Key, Value, Filter, HashTable>::set(K&& key, V&& value)
{
  containers_[0]->set(std::forward<K>(key), std::forward<V>(value));
}

template<
  class Key,
  class Value,
  class Filter,
  template<typename, typename> class HashTable>
bool SeenContainers<Key, Value, Filter, HashTable>::erase(const Key& key)
{
  for (std::size_t i = 0; i < kSize; ++i)
  {
    if (containers_[i]->erase(key))
    {
      return true;
    }
  }

  return false;
}

template<
  class Key,
  class Value,
  class Filter,
  template<typename, typename> class HashTable>
void SeenContainers<Key, Value, Filter, HashTable>::save(std::ostream& stream)
{
  auto saver = std::dynamic_pointer_cast<Saver>(containers_[0]);
  saver->save(stream);
}

template<
  class Key,
  class Value,
  class Filter,
  template<typename, typename> class HashTable>
void SeenContainers<Key, Value, Filter, HashTable>::on_load(
  Key&& key,
  Value&& value)
{
  set(std::move(key), std::move(value));
}

template<
  LRReferenceConcept<StringDefHashAdapter> PKey,
  LRReferenceConcept<ExternalIdHashAdapter> SKey,
  LRReferenceConcept<BoundUserInfoHolder> V>
inline void Portion::set_bound(
  PKey&& prefix_key,
  SKey&& suffix_key,
  V&& value)
{
  BoundContainersT::set(
    std::forward<PKey>(prefix_key),
    std::forward<SKey>(suffix_key),
    std::forward<V>(value));
}

inline auto Portion::get_bound(
  BoundUserInfoHolder& value,
  const StringDefHashAdapter& prefix_key,
  const ExternalIdHashAdapter& suffix_key)
{
  return BoundContainersT::get(value, prefix_key, suffix_key);
}

inline bool Portion::erase_bound(
  const StringDefHashAdapter& prefix_key,
  const ExternalIdHashAdapter& suffix_key)
{
  return BoundContainersT::erase(prefix_key, suffix_key);
}

template<
  LRReferenceConcept<HashHashAdapter> K,
  LRReferenceConcept<SeenUserInfoHolder> V>
inline void Portion::set_seen(K&& key, V&& value)
{
  SeenContainersT::set(
    std::forward<K>(key),
    std::forward<V>(value));
}

inline auto Portion::get_seen(
  SeenUserInfoHolder& value,
  const HashHashAdapter& key)
{
  return SeenContainersT::get(value, key);
}

inline bool Portion::erase_seen(const HashHashAdapter& key)
{
  return SeenContainersT::erase(key);
}

} // namespace AdServer::UserInfoSvcs

#endif // USERINFOSVCS_USERBINDCHUNKTYPES_HPP