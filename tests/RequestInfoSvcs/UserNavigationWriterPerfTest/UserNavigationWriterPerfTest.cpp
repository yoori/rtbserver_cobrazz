#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <sys/resource.h>

#include <Commons/PlainMonoAllocatorTypes.hpp>
#include <Generics/AppUtils.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserNavigationProfile.hpp>

namespace
{
  using AdServer::RequestInfoSvcs::UserNavigationProfileWriter;
  using AdServer::RequestInfoSvcs::UserNavigationProfileCustomWriter;
  using MonoTestTypes = AdServer::Commons::PlainMonoAllocatorTypes;
  using StdTestTypes = PlainTypes::DefaultWritingTypes;

  using StdWriter = UserNavigationProfileCustomWriter<StdTestTypes>;
  using MonoWriter = UserNavigationProfileCustomWriter<MonoTestTypes>;

  struct GeneratedWriter: public UserNavigationProfileWriter
  {
    using Context = StdTestTypes::Context;

    explicit GeneratedWriter(Context& context);
  };

  GeneratedWriter::GeneratedWriter(Context&)
  {}

  constexpr std::uint32_t PROFILE_VERSION = 3;
  constexpr std::uint32_t FIRST_DATE = 20000 * 86400;
  constexpr unsigned long DEFAULT_COUNT = 1000000;
  constexpr unsigned long DEFAULT_DAYS = 10;
  constexpr unsigned long DEFAULT_URLS = 10;
  constexpr unsigned long DEFAULT_KEYWORDS = 100;

  struct Options
  {
    unsigned long count;
    unsigned long days;
    unsigned long urls;
    unsigned long keywords;
    std::string writer;
    std::string scenario;
    bool verify_only;
  };

  struct ProfileData
  {
    unsigned long days;
    std::vector<std::string> urls;
    std::vector<std::string> keywords;
  };

  enum class Scenario
  {
    Materialize,
    Fill,
    Update
  };

  Options parse_options(int argc, char** argv)
  {
    using namespace Generics::AppUtils;

    Option<unsigned long> count(DEFAULT_COUNT);
    Option<unsigned long> days(DEFAULT_DAYS);
    Option<unsigned long> urls(DEFAULT_URLS);
    Option<unsigned long> keywords(DEFAULT_KEYWORDS);
    Option<std::string> writer("all");
    Option<std::string> scenario("materialize");
    CheckOption verify_only;
    CheckOption help;
    Args args(-1);
    args.add(equal_name("count"), count);
    args.add(equal_name("days"), days);
    args.add(equal_name("urls"), urls);
    args.add(equal_name("keywords"), keywords);
    args.add(equal_name("writer"), writer);
    args.add(equal_name("scenario"), scenario);
    args.add(equal_name("verify-only"), verify_only);
    args.add(equal_name("help") || short_name("h"), help);
    args.parse(argc - 1, argv + 1);

    if (help.enabled())
    {
      std::cout << "Usage: UserNavigationWriterPerfTest [OPTIONS]\n"
        << "  --count N     Iterations per writer/scenario (default: " << DEFAULT_COUNT << ")\n"
        << "  --days N      Profile days (default: " << DEFAULT_DAYS << ")\n"
        << "  --urls N      URLs per day (default: " << DEFAULT_URLS << ")\n"
        << "  --keywords N  Keywords per day (default: " << DEFAULT_KEYWORDS << ")\n"
        << "  --writer generated|std|mono|all (default: all)\n"
        << "  --scenario materialize|fill|update|all (default: materialize)\n"
        << "  --verify-only Check serialization, filling, updates and arena propagation\n";
      std::exit(0);
    }

    if (*count == 0 || *days == 0 ||
      *days > (std::numeric_limits<std::uint32_t>::max() - FIRST_DATE) / 86400)
    {
      throw std::runtime_error("count and days must be positive; dates must fit uint32_t");
    }

    if (*writer != "all" && *writer != "generated" && *writer != "std" && *writer != "mono")
    {
      throw std::runtime_error("Unknown writer: " + *writer);
    }

    if (*scenario != "all" && *scenario != "materialize" &&
      *scenario != "fill" && *scenario != "update")
    {
      throw std::runtime_error("Unknown scenario: " + *scenario);
    }

    return {*count, *days, *urls, *keywords, *writer, *scenario, verify_only.enabled()};
  }

  template<typename Writer>
  std::uint64_t profile_checksum(const Writer& profile)
  {
    const auto& day = profile.days().back();
    std::uint64_t result = profile.version() + profile.days().size() + day.date() +
      day.navigations().size() + day.page_keywords().size();
    if (!day.navigations().empty())
    {
      result += day.navigations().back().count() + day.navigations().back().url().size();
    }

    if (!day.page_keywords().empty())
    {
      result += day.page_keywords().back().count() + day.page_keywords().back().keyword().size();
    }

    return result;
  }

  ProfileData make_profile_data(const Options& options)
  {
    ProfileData data{options.days, {}, {}};
    for (unsigned long i = 0; i < options.urls; ++i)
    {
      data.urls.push_back("https://example.test/navigation/" + std::to_string(1000000 + i));
    }

    for (unsigned long i = 0; i < options.keywords; ++i)
    {
      data.keywords.push_back("navigation-materialization-keyword-" + std::to_string(1000000 + i));
    }
    return data;
  }

  template<typename Writer>
  void fill_profile(Writer& profile, const ProfileData& data)
  {
    profile.version() = PROFILE_VERSION;
    profile.days().resize(data.days);
    for (unsigned long day_index = 0; day_index < data.days; ++day_index)
    {
      auto& day = profile.days()[day_index];
      day.date() = FIRST_DATE + day_index * 86400;
      day.navigations().resize(data.urls.size());
      day.page_keywords().resize(data.keywords.size());
      for (unsigned long i = 0; i < data.urls.size(); ++i)
      {
        auto& navigation = day.navigations()[i];
        navigation.url().assign(data.urls[i].data(), data.urls[i].size());
        navigation.count() = day_index + i + 1;
      }

      for (unsigned long i = 0; i < data.keywords.size(); ++i)
      {
        auto& keyword = day.page_keywords()[i];
        keyword.keyword().assign(data.keywords[i].data(), data.keywords[i].size());
        keyword.count() = day_index + i + 1;
      }
    }
  }

  template<typename Writer>
  std::vector<char> serialize(const Writer& profile)
  {
    std::vector<char> buffer(profile.size());
    profile.save(buffer.data(), buffer.size());
    return buffer;
  }

  template<typename Writer>
  void update_profile(Writer& profile)
  {
    auto& days = profile.days();
    const auto next_date = days.empty() ? FIRST_DATE : days.back().date() + 86400;
    if (!days.empty())
    {
      auto& day = days.back();
      if (!day.navigations().empty())
      {
        day.navigations().front().url().append("/updated-navigation-with-a-long-suffix");
        ++day.navigations().front().count();
      }

      if (!day.page_keywords().empty())
      {
        day.page_keywords().front().keyword().append("-updated-keyword-with-a-long-suffix");
        ++day.page_keywords().front().count();
      }

      auto& navigation = day.navigations().emplace_back();
      navigation.url() = "https://example.test/additional-navigation";
      navigation.count() = 17;
      auto& keyword = day.page_keywords().emplace_back();
      keyword.keyword() = "additional-navigation-keyword";
      keyword.count() = 23;
    }

    auto& day = days.emplace_back();
    day.date() = next_date;
    auto& navigation = day.navigations().emplace_back();
    navigation.url() = "https://example.test/new-day-navigation";
    navigation.count() = 31;
    auto& keyword = day.page_keywords().emplace_back();
    keyword.keyword() = "new-day-navigation-keyword";
    keyword.count() = 37;
  }

  void check(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  template<typename String>
  std::string_view view(const String& value)
  {
    return {value.data(), value.size()};
  }

  template<typename Writer>
  void verify_fields(const Writer& actual, const UserNavigationProfileWriter& expected)
  {
    check(actual.version() == expected.version(), "Version mismatch");
    check(actual.days().size() == expected.days().size(), "Day count mismatch");
    for (std::size_t i = 0; i < expected.days().size(); ++i)
    {
      const auto& left = actual.days()[i];
      const auto& right = expected.days()[i];
      check(left.date() == right.date(), "Date mismatch");
      check(left.navigations().size() == right.navigations().size(), "URL count mismatch");
      check(left.page_keywords().size() == right.page_keywords().size(), "Keyword count mismatch");
      for (std::size_t j = 0; j < right.navigations().size(); ++j)
      {
        const auto& entry = left.navigations()[j];
        const auto& reference = right.navigations()[j];
        check(view(entry.url()) == view(reference.url()), "URL mismatch");
        check(entry.count() == reference.count(), "Navigation count mismatch");
      }

      for (std::size_t j = 0; j < right.page_keywords().size(); ++j)
      {
        const auto& entry = left.page_keywords()[j];
        const auto& reference = right.page_keywords()[j];
        check(view(entry.keyword()) == view(reference.keyword()), "Keyword mismatch");
        check(entry.count() == reference.count(), "Keyword frequency mismatch");
      }
    }
  }

  template<typename Writer>
  void verify_context(const Writer& profile, typename Writer::Context& context)
  {
    if constexpr (std::is_same_v<typename Writer::Context, MonoTestTypes::Context>)
    {
      check(profile.days().get_allocator().arena() == &context, "Wrong days arena");
      for (const auto& day : profile.days())
      {
        check(day.navigations().get_allocator().arena() == &context, "Wrong navigations arena");
        check(day.page_keywords().get_allocator().arena() == &context, "Wrong keywords arena");
        for (const auto& entry : day.navigations())
        {
          check(&entry.url().context() == &context, "Wrong URL arena");
        }

        for (const auto& entry : day.page_keywords())
        {
          check(&entry.keyword().context() == &context, "Wrong keyword arena");
        }
      }
    }
  }

  template<typename Writer>
  void verify_borrowed_strings(const Writer& profile, const std::vector<char>& buffer)
  {
    if constexpr (std::is_same_v<typename Writer::Context, MonoTestTypes::Context>)
    {
      const auto begin = reinterpret_cast<std::uintptr_t>(buffer.data());
      const auto end = begin + buffer.size();
      for (const auto& day : profile.days())
      {
        for (const auto& entry : day.navigations())
        {
          const auto address = reinterpret_cast<std::uintptr_t>(entry.url().data());
          check(address >= begin && address < end, "Deserialized URL does not borrow the profile");
        }

        for (const auto& entry : day.page_keywords())
        {
          const auto address = reinterpret_cast<std::uintptr_t>(entry.keyword().data());
          check(address >= begin && address < end,
            "Deserialized keyword does not borrow the profile");
        }
      }
    }
  }

  template<typename Writer>
  void verify_writer(const ProfileData& data)
  {
    UserNavigationProfileWriter expected;
    fill_profile(expected, data);
    const auto buffer = serialize(expected);
    typename Writer::Context context;
    Writer actual(context);
    actual.init(buffer.data(), buffer.size());
    verify_borrowed_strings(actual, buffer);
    verify_fields(actual, expected);
    verify_context(actual, context);
    check(serialize(actual) == buffer, "Deserialization round trip mismatch");

    update_profile(expected);
    update_profile(actual);
    verify_fields(actual, expected);
    verify_context(actual, context);
    const auto updated = serialize(actual);
    check(updated == serialize(expected), "Updated serialization mismatch");
    UserNavigationProfileWriter decoded;
    decoded.init(updated.data(), updated.size());
    verify_fields(actual, decoded);

    actual.init(buffer.data(), buffer.size());
    check(serialize(actual) == buffer, "Repeated init mismatch");
    verify_context(actual, context);

    typename Writer::Context filled_context;
    Writer filled(filled_context);
    fill_profile(filled, data);
    expected.init(buffer.data(), buffer.size());
    verify_fields(filled, expected);
    verify_context(filled, filled_context);
    check(serialize(filled) == buffer, "Filled serialization mismatch");
  }

  template<typename Writer>
  void verify_invalid_buffers()
  {
    UserNavigationProfileWriter expected;
    fill_profile(expected, ProfileData{1, {"url"}, {"keyword"}});
    const auto buffer = serialize(expected);
    for (const auto size : {std::size_t(0), std::size_t(4),
      static_cast<std::size_t>(Writer::FIXED_SIZE - 1)})
    {
      typename Writer::Context context;
      Writer actual(context);
      bool rejected = false;
      try
      {
        actual.init(buffer.data(), size);
        actual.size();
      }
      catch (const PlainTypes::CorruptedStruct&)
      {
        rejected = true;
      }
      check(rejected, "Truncated buffer accepted");
    }

    typename Writer::Context context;
    Writer actual(context);
    actual.init(buffer.data(), buffer.size());
    std::vector<char> small_buffer(actual.size() - 1);
    bool rejected = false;
    try
    {
      actual.save(small_buffer.data(), small_buffer.size());
    }
    catch (const PlainTypes::CorruptedStruct&)
    {
      rejected = true;
    }
    check(rejected, "Undersized output buffer accepted");
  }

  void verify(const ProfileData& data)
  {
    const ProfileData cases[] = {
      data,
      {0, {}, {}},
      {1, {}, {}},
      {2, {"", "u"}, {"", "k"}},
      {3, {std::string(4096, 'u')}, {std::string(8192, 'k')}}
    };
    for (const auto& item : cases)
    {
      verify_writer<StdWriter>(item);
      verify_writer<MonoWriter>(item);
    }
    verify_invalid_buffers<StdWriter>();
    verify_invalid_buffers<MonoWriter>();
    std::cout << "verification=passed\n";
  }

  double cpu_seconds()
  {
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0)
    {
      throw std::runtime_error("getrusage failed");
    }

    return usage.ru_utime.tv_sec + usage.ru_stime.tv_sec +
      (usage.ru_utime.tv_usec + usage.ru_stime.tv_usec) / 1000000.0;
  }

  template<typename Writer>
  [[gnu::noinline]] std::uint64_t exercise(
    const ProfileData& data,
    const std::vector<char>& buffer,
    std::vector<char>& output,
    Scenario scenario,
    unsigned long count)
  {
    std::uint64_t checksum = 0;
    for (unsigned long i = 0; i < count; ++i)
    {
      typename Writer::Context context;
      Writer profile(context);
      if (scenario == Scenario::Fill)
      {
        fill_profile(profile, data);
      }
      else
      {
        profile.init(buffer.data(), buffer.size());
      }

      if (scenario == Scenario::Update)
      {
        update_profile(profile);
        profile.save(output.data(), output.size());
        asm volatile("" : : "g"(output.data()) : "memory");
      }
      // Keep the entire materialized object observable to the optimizer without runtime work.
      asm volatile("" : : "g"(&profile) : "memory");
      checksum += profile_checksum(profile);
    }

    return checksum;
  }

  template<typename Writer>
  void measure(
    const char* name,
    const ProfileData& data,
    const std::vector<char>& buffer,
    Scenario scenario,
    unsigned long count)
  {
    UserNavigationProfileWriter expected;
    expected.init(buffer.data(), buffer.size());
    if (scenario == Scenario::Update)
    {
      update_profile(expected);
    }

    const auto expected_checksum = profile_checksum(expected) * count;
    std::vector<char> output(scenario == Scenario::Update ? expected.size() : 0);
    const double cpu_start = cpu_seconds();
    const auto start = std::chrono::steady_clock::now();
    const auto checksum = exercise<Writer>(data, buffer, output, scenario, count);
    const auto finish = std::chrono::steady_clock::now();
    const double cpu_elapsed = cpu_seconds() - cpu_start;
    const double elapsed_us = std::chrono::duration<double, std::micro>(finish - start).count();
    check(checksum == expected_checksum, "Benchmark checksum mismatch");
    if (scenario == Scenario::Update)
    {
      check(output == serialize(expected), "Benchmark output mismatch");
    }

    const char* scenario_name = scenario == Scenario::Materialize ? "materialize" :
      (scenario == Scenario::Fill ? "fill" : "update");
    std::cout << "writer=" << name << " scenario=" << scenario_name
      << " elapsed_ms=" << elapsed_us / 1000
      << " us_per_iteration=" << elapsed_us / count
      << " cpu_ms=" << cpu_elapsed * 1000
      << " cpu_us_per_iteration=" << cpu_elapsed * 1000000 / count
      << " checksum=" << checksum << '\n';
  }
}

int main(int argc, char** argv)
{
  try
  {
    const auto options = parse_options(argc, argv);
    const auto data = make_profile_data(options);
    verify(data);
    if (options.verify_only)
    {
      return 0;
    }

    UserNavigationProfileWriter profile;
    fill_profile(profile, data);
    const auto buffer = serialize(profile);
    std::cout << std::fixed << std::setprecision(3)
      << "iterations=" << options.count
      << " days=" << options.days
      << " urls_per_day=" << options.urls
      << " keywords_per_day=" << options.keywords
      << " profile_bytes=" << buffer.size() << '\n';
    std::vector<Scenario> scenarios;
    if (options.scenario == "materialize" || options.scenario == "all")
    {
      scenarios.push_back(Scenario::Materialize);
    }

    if (options.scenario == "fill" || options.scenario == "all")
    {
      scenarios.push_back(Scenario::Fill);
    }

    if (options.scenario == "update" || options.scenario == "all")
    {
      scenarios.push_back(Scenario::Update);
    }

    for (const auto scenario : scenarios)
    {
      if (options.writer == "generated" || options.writer == "all")
      {
        measure<GeneratedWriter>("generated", data, buffer, scenario, options.count);
      }

      if (options.writer == "std" || options.writer == "all")
      {
        measure<StdWriter>("std", data, buffer, scenario, options.count);
      }

      if (options.writer == "mono" || options.writer == "all")
      {
        measure<MonoWriter>("mono", data, buffer, scenario, options.count);
      }
    }
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << "UserNavigationWriterPerfTest: " << ex.what() << '\n';
    return 1;
  }
}
