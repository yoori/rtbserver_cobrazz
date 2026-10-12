#include <array>
#include <cstdlib>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <optional>

#include <Logger/StreamLogger.hpp>
#include <UserInfoSvcs/UserInfoManager/UserInfoContainer.hpp>

namespace AdServer::UserInfoSvcs
{
  struct UserInfoContainerTestAccess
  {
    static void install(
      UserInfoContainer& container,
      const std::array<AdServer::ProfilingCommons::AsyncProfileMap<UserId>*, 6>& maps);
  };

  void UserInfoContainerTestAccess::install(
    UserInfoContainer& container,
    const std::array<AdServer::ProfilingCommons::AsyncProfileMap<UserId>*, 6>& maps)
  {
    using Map = UserInfoContainer::UserProfileMap;
    UserInfoContainer::UserProfileMap_var* targets[] = {
      &container.base_profiles_, &container.temp_profiles_, &container.add_profiles_,
      &container.history_profiles_, &container.temp_history_profiles_, &container.freq_cap_profiles_
    };
    for (std::size_t i = 0; i < maps.size(); ++i)
    {
      Map::ChunkIdToProfileMap chunks;
      chunks.emplace(0, new AdServer::ProfilingCommons::TransactionProfileMap<UserId>(maps[i]));
      *targets[i] = new Map(1, chunks, AdServer::Commons::uuid_distribution_hash);
    }
  }
}

namespace
{
  using namespace AdServer::UserInfoSvcs;
  using AdServer::Commons::StartableAwaitable;
  using AdServer::ProfilingCommons::OP_RUNTIME;
  using Container = UserInfoContainer;
  using AsyncMap = AdServer::ProfilingCommons::AsyncProfileMap<UserId>;
  using Buffer = Generics::ConstSmartMemBuf_var;

  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      std::cerr << "UserInfoContainerTransactionTest: " << message << std::endl;
      std::abort();
    }
  }

  class Storage final: public AsyncMap, public ReferenceCounting::AtomicImpl
  {
  public:
    void check_profile_async(const UserId& key, CheckCallback callback) const override;
    Buffer get_profile_async(const UserId& key, GetCallback callback, std::optional<Generics::Time>)
      override;
    void save_profile_async(
      const UserId& key, const Generics::ConstSmartMemBuf* profile, const Generics::Time&,
      SaveCallback callback) override;
    void remove_profile_async(
      const UserId& key, AdServer::ProfilingCommons::OperationPriority, RemoveCallback callback)
      override;
    void clear_expired_async(const Generics::Time&, CompleteCallback callback) override;
    unsigned long size() const noexcept override;

    bool defer_reads = false;
    bool defer_writes = false;
    std::optional<std::string> read_error;
    std::optional<std::string> write_error;
    unsigned reads_started = 0;
    unsigned removes_started = 0;
    std::map<UserId, Buffer> profiles;
    std::deque<std::function<void()>> reads;
    std::deque<std::function<void()>> writes;
  };

  void Storage::check_profile_async(const UserId& key, CheckCallback callback) const
  {
    callback(profiles.count(key) != 0, {});
  }

  Buffer
  Storage::get_profile_async(const UserId& key, GetCallback callback, std::optional<Generics::Time>)
  {
    ++reads_started;
    auto complete = [this, key, callback = std::move(callback)]
    {
      const auto it = profiles.find(key);
      callback(it == profiles.end() ? Buffer() : it->second, read_error);
    };
    if (defer_reads)
    {
      reads.emplace_back(std::move(complete));
    }
    else
    {
      complete();
    }

    return {};
  }

  void Storage::save_profile_async(
    const UserId& key, const Generics::ConstSmartMemBuf* profile, const Generics::Time&,
    SaveCallback callback)
  {
    // Delay completion, while retaining the storage's read-after-write ordering guarantee.
    if (!write_error)
    {
      profiles[key] = ReferenceCounting::add_ref(profile);
    }

    auto complete = [this, callback = std::move(callback)]
    {
      if (callback)
      {
        callback(write_error);
      }
    };
    if (defer_writes)
    {
      writes.emplace_back(std::move(complete));
    }
    else
    {
      complete();
    }
  }

  void Storage::remove_profile_async(
    const UserId& key, AdServer::ProfilingCommons::OperationPriority, RemoveCallback callback)
  {
    ++removes_started;
    callback(profiles.erase(key) != 0, {});
  }

  void Storage::clear_expired_async(const Generics::Time&, CompleteCallback callback)
  {
    if (callback)
    {
      callback();
    }
  }

  unsigned long Storage::size() const noexcept
  {
    return profiles.size();
  }

  void complete_one(std::deque<std::function<void()>>& callbacks)
  {
    require(!callbacks.empty(), "expected a pending storage operation");
    auto callback = std::move(callbacks.front());
    callbacks.pop_front();
    callback();
  }

  struct Operation
  {
    explicit Operation(StartableAwaitable<bool> operation);
    ~Operation();
    bool result();

    StartableAwaitable<bool> operation;
    bool done = false;
    std::optional<std::exception_ptr> error;
  };

  Operation::Operation(StartableAwaitable<bool> value) : operation(std::move(value))
  {
    operation.start([this](std::optional<std::exception_ptr> exception)
    {
      error = std::move(exception);
      done = true;
    });
  }

  Operation::~Operation()
  {
    require(done, "an operation remained suspended");
  }

  bool Operation::result()
  {
    require(done, "operation did not finish");
    if (error)
    {
      std::rethrow_exception(*error);
    }

    return operation.await_resume();
  }

  void expect_waiter_limit(Operation& operation)
  {
    require(operation.done && operation.error, "waiter limit was ignored");
    try
    {
      std::rethrow_exception(*operation.error);
    }
    catch (const Container::ResourceExhausted&)
    {
      return;
    }
    catch (...)
    {}

    require(false, "waiter rejection changed its exception type");
  }

  struct Fixture
  {
    explicit
    Fixture(bool use_add = true, unsigned long base_limit = 0, unsigned long temp_limit = 0);
    void drain();

    Logging::Logger_var logger;
    std::array<ReferenceCounting::SmartPtr<Storage>, 6> stores;
    UserInfoContainer_var container;
  };

  Fixture::Fixture(bool use_add, unsigned long base_limit, unsigned long temp_limit)
  {
    logger = new Logging::OStream::Logger(Logging::OStream::Config(std::cerr, 0));
    using Traits = AdServer::ProfilingCommons::LevelMapTraits;
    const Traits traits(Traits::BLOCK_RUNTIME, 0, 0, 0, 0, Generics::Time(86400));
    container = new Container(
      logger, 1, {}, traits, traits, traits, traits, traits, 1, Generics::Time(3600),
      Generics::Time(3600), false, Generics::Time(1800), use_add, base_limit, temp_limit,
      0, 1, 0, 1, 1, 1, 2);
    std::array<AsyncMap*, 6> maps;
    for (std::size_t i = 0; i < stores.size(); ++i)
    {
      stores[i] = new Storage();
      maps[i] = stores[i].in();
    }

    UserInfoContainerTestAccess::install(*container, maps);
    ChannelDictionary_var channels = new ChannelDictionary();
    FreqCapConfig_var freq_caps = new FreqCapConfig();
    container->config(Generics::Time::ZERO, Generics::Time::ZERO, channels, freq_caps);
  }

  void Fixture::drain()
  {
    for (unsigned pass = 0; pass < 100; ++pass)
    {
      bool pending = false;
      for (auto& store : stores)
      {
        if (!store->reads.empty())
        {
          pending = true;
          complete_one(store->reads);
        }

        if (!store->writes.empty())
        {
          pending = true;
          complete_one(store->writes);
        }
      }

      if (!pending)
      {
        return;
      }
    }

    require(false, "storage operations did not drain");
  }

  StartableAwaitable<bool>
  match(Container& container, UserId user, bool temporary = false, bool write = false)
  {
    const Container::RequestMatchParams params(
      user, Generics::Time(100000), String::SubString(), String::SubString(), !write, 1,
      Generics::Time::ZERO, false, temporary, !write);
    ColoUserId colo;
    ChannelIdPack channels;
    Generics::MonoAllocatorArena arena;
    ChannelMatchMap result(&arena);
    Container::UserAppearance appearance;
    ProfileProperties properties;
    co_return co_await container.co_match(
      params, 1, 1, colo, channels, result, appearance, properties, OP_RUNTIME, nullptr);
  }

  StartableAwaitable<bool> merge(Container& container, UserId user)
  {
    const Container::RequestMatchParams params(
      user, Generics::Time(100000), String::SubString(), String::SubString(), false, 1,
      Generics::Time::ZERO, false, false);
    Generics::MemBuf empty;
    Container::UserAppearance appearance;
    co_return co_await container.co_merge(
      params, empty, empty, empty, empty, appearance, 1, 1, OP_RUNTIME);
  }

  Buffer make_history()
  {
    HistoryUserProfileWriter writer;
    writer.major_version() = CURRENT_HISTORY_MAJOR_PROFILE_VERSION;
    writer.minor_version() = CURRENT_HISTORY_MINOR_PROFILE_VERSION;
    Generics::SmartMemBuf_var history = new Generics::SmartMemBuf();
    history->membuf().alloc(writer.size());
    writer.save(history->membuf().data(), writer.size());
    return Generics::transfer_membuf(history);
  }

  StartableAwaitable<bool>
  exchange(Container& container, UserId user, Buffer base = {}, Buffer history = {})
  {
    Generics::MemBuf empty;
    co_return co_await container.co_exchange_merge(
      user, base ? base->membuf() : empty, history ? history->membuf() : empty, nullptr);
  }

  StartableAwaitable<bool> read(Container& container, UserId user)
  {
    Generics::SmartMemBuf_var profile;
    co_return co_await container.co_get_user_profile(user, false, &profile, nullptr, nullptr);
  }

  void test_parallel_reads_and_user_isolation()
  {
    Fixture fixture;
    auto& base = *fixture.stores[0];
    auto& add = *fixture.stores[2];
    base.defer_reads = add.defer_reads = true;
    const auto user = UserId::create_random_based();
    Operation first(match(*fixture.container, user));
    require(base.reads.size() == 1 && add.reads.size() == 1, "base/add reads were serialized");
    Operation merging(merge(*fixture.container, user));
    require(!merging.done && add.reads.size() == 1, "merge bypassed the common lock");

    auto other = UserId::create_random_based();
    while (other == user || other.hash() % 1024 != user.hash() % 1024)
    {
      other = UserId::create_random_based();
    }

    Operation independent(match(*fixture.container, other));
    require(base.reads.size() == 2 && add.reads.size() == 2, "a different user was blocked");
    Operation standalone(read(*fixture.container, user));
    require(base.reads.size() == 3, "standalone read acquired the user lock");
    fixture.drain();
    require(first.result() && merging.result() && independent.result() && standalone.result(),
      "match/merge failed after releasing reads");
  }

  void test_pending_writes(bool fail_write, bool history_first)
  {
    Fixture fixture;
    const auto user = UserId::create_random_based();
    Operation seed(match(*fixture.container, user, false, true));
    require(seed.result(), "seed match failed");
    auto& base = *fixture.stores[0];
    auto& add = *fixture.stores[2];
    auto& history = *fixture.stores[3];
    add.profiles[user] = base.profiles.at(user);
    base.defer_writes = history.defer_writes = true;
    if (fail_write)
    {
      history.write_error = "injected write failure";
    }

    Operation writing(exchange(*fixture.container, user, base.profiles.at(user), make_history()));
    require(writing.result(), "writing exchange failed");
    require(base.writes.size() == 1 && history.writes.size() == 1,
      "writes were not submitted together");
    const auto base_reads = base.reads_started;
    const auto add_reads = add.reads_started;
    Operation fraud(fixture.container->co_fraud_user(user, Generics::Time(100001)));
    Operation exchanging(exchange(*fixture.container, user));
    Operation removing(fixture.container->co_remove_user_profile(user));
    require(!fraud.done && !exchanging.done && !removing.done, "pending writes released the user");
    complete_one(history_first ? history.writes : base.writes);
    require(base.reads_started == base_reads && add.reads_started == add_reads,
      "the first write completion released the common lock");
    require(!fraud.done && !exchanging.done && !removing.done, "another write is still pending");
    fixture.drain();
    require(fraud.result() && exchanging.result() && removing.result(), "queued operation failed");
    require(base.profiles.empty() && add.profiles.empty(), "remove overtook a pending write");
  }

  void test_waiter_limits_and_temporary_users()
  {
    Fixture fixture(false, 1, 2);
    fixture.stores[0]->defer_reads = true;
    const auto user = UserId::create_random_based();
    Operation first(match(*fixture.container, user));
    Operation rejected_base(match(*fixture.container, user));
    expect_waiter_limit(rejected_base);
    Operation temporary(match(*fixture.container, user, true));
    require(!temporary.done && fixture.stores[1]->reads_started == 0,
      "temporary and persistent profiles did not share the user lock");
    Operation rejected_temp(match(*fixture.container, user, true));
    expect_waiter_limit(rejected_temp);
    Operation removing(fixture.container->co_remove_user_profile(user));
    require(!removing.done, "remove bypassed pending operations");
    fixture.drain();
    require(first.result() && temporary.result() && removing.result(),
      "waiter queue did not drain");
    Operation retry(match(*fixture.container, user));
    fixture.drain();
    require(retry.result(), "rejected waiters leaked their registration");
  }

  void test_read_failure()
  {
    Fixture fixture;
    auto& base = *fixture.stores[0];
    auto& add = *fixture.stores[2];
    base.defer_reads = add.defer_reads = true;
    base.read_error = "injected read failure";
    const auto user = UserId::create_random_based();
    Operation first(match(*fixture.container, user));
    Operation next(exchange(*fixture.container, user));
    complete_one(base.reads);
    require(!first.done && !next.done, "read failure abandoned another pending read");
    complete_one(add.reads);
    require(first.done && first.error, "read error was lost");
    fixture.drain();
    require(next.result(), "read failure leaked the user lock");
  }
}

int main()
{
  try
  {
    test_parallel_reads_and_user_isolation();
    test_pending_writes(false, false);
    test_pending_writes(false, true);
    test_pending_writes(true, false);
    test_waiter_limits_and_temporary_users();
    test_read_failure();
    std::cout << "UserInfoContainerTransactionTest: PASS" << std::endl;
  }
  catch (const std::exception& ex)
  {
    std::cerr << "UserInfoContainerTransactionTest: " << ex.what() << std::endl;
    return 1;
  }
}
