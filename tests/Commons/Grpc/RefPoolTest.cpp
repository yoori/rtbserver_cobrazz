#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include <Commons/Grpc/RefPool.hpp>

namespace
{
  class NullActiveObjectCallback final:
    public virtual Generics::ActiveObjectCallback,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    void
    report_error(Severity, const String::SubString&, const char* = nullptr) throw () override
    {}

  protected:
    ~NullActiveObjectCallback() throw () override = default;
  };

  struct MockClient
  {
    explicit MockClient(int id_val)
      : id(id_val)
    {}

    int id;
  };

  using Pool = AdServer::Grpc::RefPool<MockClient>;

  std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
  make_scheduler()
  {
    auto scheduler =
      std::make_shared<AdServer::Commons::BoostAsioContextRunActiveObject>(
        Generics::ActiveObjectCallback_var(new NullActiveObjectCallback()),
        std::make_shared<boost::asio::io_service>(),
        1);
    scheduler->activate_object();
    return scheduler;
  }

  Generics::Time
  now_plus_msec(const suseconds_t msec)
  {
    return Generics::Time::get_time_of_day() + Generics::Time(0, msec * 1000);
  }

  std::optional<Pool::Ref>
  wait_for_ref(const std::shared_ptr<Pool>& pool, const std::chrono::milliseconds timeout)
  {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
      auto ref = pool->get_object();
      if (ref)
      {
        return ref;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    return std::nullopt;
  }

  void
  assert_no_ref(const std::shared_ptr<Pool>& pool)
  {
    assert(!pool->get_object().has_value());
  }

  void
  test_bad_ref_is_issued_as_single_probe()
  {
    auto scheduler = make_scheduler();
    auto pool = std::make_shared<Pool>(
      std::vector<std::shared_ptr<MockClient>>{
        std::make_shared<MockClient>(1)},
      scheduler);
    pool->activate_object();

    {
      auto ref = pool->get_object();
      assert(ref.has_value());
      assert((*ref)->id == 1);
      ref->mark_as_bad(now_plus_msec(50));
    }

    assert_no_ref(pool);

    auto probe_ref = wait_for_ref(pool, std::chrono::milliseconds(500));
    assert(probe_ref.has_value());
    assert((*probe_ref)->id == 1);

    assert_no_ref(pool);

    probe_ref.reset();

    auto available_ref = wait_for_ref(pool, std::chrono::milliseconds(500));
    assert(available_ref.has_value());
    assert((*available_ref)->id == 1);

    pool->deactivate_object();
    pool->wait_object();
    scheduler->deactivate_object();
    scheduler->wait_object();
  }

  void
  test_failed_probe_returns_to_bad_list()
  {
    auto scheduler = make_scheduler();
    auto pool = std::make_shared<Pool>(
      std::vector<std::shared_ptr<MockClient>>{
        std::make_shared<MockClient>(2)},
      scheduler);
    pool->activate_object();

    {
      auto ref = pool->get_object();
      assert(ref.has_value());
      assert((*ref)->id == 2);
      ref->mark_as_bad(now_plus_msec(30));
    }

    auto probe_ref = wait_for_ref(pool, std::chrono::milliseconds(500));
    assert(probe_ref.has_value());
    assert((*probe_ref)->id == 2);
    probe_ref->mark_as_bad(now_plus_msec(80));
    probe_ref.reset();

    assert_no_ref(pool);

    auto retry_ref = wait_for_ref(pool, std::chrono::milliseconds(500));
    assert(retry_ref.has_value());
    assert((*retry_ref)->id == 2);

    pool->deactivate_object();
    pool->wait_object();
    scheduler->deactivate_object();
    scheduler->wait_object();
  }
}

int
main()
{
  test_bad_ref_is_issued_as_single_probe();
  test_failed_probe_returns_to_bad_list();

  std::cout << "RefPoolTest passed" << std::endl;
  return 0;
}
