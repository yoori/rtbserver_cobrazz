#include <atomic>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include <Generics/AppUtils.hpp>
#include <Generics/Time.hpp>

#include <CORBACommons/CorbaAdapters.hpp>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/UserInfoManip.hpp>

#include <UserInfoSvcs/UserBindServer/UserBindServer.hpp>

namespace
{
  std::string
  random_external_id(std::mt19937& gen)
  {
    static constexpr std::size_t id_size = 10;
    static constexpr char first = 'a';
    static constexpr char last = 'z';
    std::uniform_int_distribution<int> dist(first, last);

    std::string id;
    id.reserve(id_size);
    for(std::size_t i = 0; i < id_size; ++i)
    {
      id.push_back(static_cast<char>(dist(gen)));
    }
    return id;
  }

  void
  print_usage()
  {
    std::cerr
      << "Usage: UserBindCorbaPerf [OPTIONS]\n"
      << "Options:\n"
      << "  --reference <corba-ref> corba object reference\n"
      << "  --count <N>             number of requests (default: 1000)\n"
      << "  --threads <N>           sender threads count (default: 1)\n"
      << "  --user-id <id>          fixed id for GetUserRequestInfo::id\n";
  }
}

int
main(int argc, char** argv)
{
  try
  {
    using namespace Generics::AppUtils;

    StringOption opt_user_bind_mapper_ref;
    Option<unsigned long> opt_count(1000);
    Option<unsigned int> opt_threads(1);
    StringOption opt_user_id;

    Args args(-1);
    args.add(equal_name("reference") || short_name("r"), opt_user_bind_mapper_ref);
    args.add(equal_name("count") || short_name("c"), opt_count);
    args.add(equal_name("threads") || short_name("t"), opt_threads);
    args.add(equal_name("user-id"), opt_user_id);

    args.parse(argc - 1, argv + 1);

    if(!opt_user_bind_mapper_ref.installed())
    {
      std::cerr << "--reference parameter is required" << std::endl;
      print_usage();
      return 1;
    }

    if(*opt_threads == 0)
    {
      std::cerr << "--threads must be > 0" << std::endl;
      return 1;
    }

    CORBACommons::CorbaClientAdapter_var corba_client_adapter(
      new CORBACommons::CorbaClientAdapter());

    std::vector<AdServer::UserInfoSvcs::UserBindMapper_var> user_bind_mappers;
    user_bind_mappers.reserve(10);

    for(int i = 0; i < 10; ++i)
    {
      CORBA::Object_var user_user_bind_mapper_obj =
        corba_client_adapter->resolve_object(
          CORBACommons::CorbaObjectRef(opt_user_bind_mapper_ref->c_str()));

      AdServer::UserInfoSvcs::UserBindMapper_var user_bind_mapper =
        AdServer::UserInfoSvcs::UserBindServer::_narrow(user_user_bind_mapper_obj);

      if(CORBA::is_nil(user_bind_mapper.in()))
      {
        std::cerr << "Can't narrow UserBindMapper from --reference" << std::endl;
        return 1;
      }

      user_bind_mappers.emplace_back(std::move(user_bind_mapper));
    }

    std::atomic<std::uint64_t> sent_count{0};
    std::atomic<std::uint64_t> done_count{0};
    std::atomic<std::uint64_t> error_count{0};

    std::thread reporter_thread([&]() {
      std::uint64_t prev = 0;
      while(done_count.load(std::memory_order_relaxed) < *opt_count)
      {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        const auto now = std::chrono::system_clock::now();
        const std::time_t now_tt = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_r(&now_tt, &tm);

        const auto current = done_count.load(std::memory_order_relaxed);
        const auto per_second = current - prev;
        prev = current;

        std::cout << std::put_time(&tm, "%T") << ": " << per_second << std::endl;
      }
    });

    std::vector<std::thread> sender_threads;
    sender_threads.reserve(*opt_threads);

    for(unsigned long i = 0; i < *opt_threads; ++i)
    {
      sender_threads.emplace_back([&]() {
        std::mt19937 gen(std::random_device{}());

        while(true)
        {
          const auto req_index = sent_count.fetch_add(1, std::memory_order_relaxed);
          if(req_index >= *opt_count)
          {
            return;
          }

          AdServer::UserInfoSvcs::UserBindMapper::GetUserRequestInfo request;

          if(opt_user_id.installed())
          {
            request.id << *opt_user_id;
          }
          else
          {
            request.id << random_external_id(gen);
          }

          const auto now = Generics::Time::get_time_of_day();
          request.timestamp = CorbaAlgs::pack_time(now);
          request.silent = true;
          request.generate_user_id = false;
          request.for_set_cookie = false;
          request.create_timestamp = CorbaAlgs::pack_time(now);
          request.current_user_id = CorbaAlgs::pack_user_id(AdServer::Commons::UserId());

          try
          {
            auto& mapper = user_bind_mappers[rand() % user_bind_mappers.size()];
            mapper->get_user_id(request);
          }
          catch(const AdServer::UserInfoSvcs::UserBindMapper::NotReady&)
          {
            error_count.fetch_add(1, std::memory_order_relaxed);
          }
          catch(const AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound&)
          {
            error_count.fetch_add(1, std::memory_order_relaxed);
          }
          catch(const AdServer::UserInfoSvcs::UserBindMapper::ImplementationException&)
          {
            error_count.fetch_add(1, std::memory_order_relaxed);
          }
          catch(const CORBA::SystemException&)
          {
            error_count.fetch_add(1, std::memory_order_relaxed);
          }

          done_count.fetch_add(1, std::memory_order_relaxed);
        }
      });
    }

    for(auto& sender : sender_threads)
    {
      sender.join();
    }

    reporter_thread.join();

    std::cout << "completed: " << done_count.load(std::memory_order_relaxed)
      << ", errors: " << error_count.load(std::memory_order_relaxed)
      << std::endl;

    return 0;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
  catch(const CORBA::SystemException& ex)
  {
    std::cerr << "Caught CORBA::SystemException: " << ex << std::endl;
  }

  return 1;
}
