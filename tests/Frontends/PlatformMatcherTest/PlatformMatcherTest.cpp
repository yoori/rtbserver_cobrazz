#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <Frontends/FrontendCommons/RequestMatchers.hpp>
#include <String/StringManip.hpp>

#include "../JsonParserPerfUtils.hpp"

namespace
{
  const char PLATFORM_MATCHER_RULES_ENV[] =
    "ADSERVER_PLATFORM_MATCHER_RULES";

  constexpr char DEFAULT_PLATFORM_RULES[] = R"PLATFORM_RULES(# platform_id	name	type	use_name	marker	match_regexp	output_regexp	priority
1156225	blackberry			BlackBerry			8
1156226	iphone			iPhone		\\s+os\\s+([_.\\d]*)	8
1156227	ipad			iPad		\\s+os\\s+([_.\\d]*)	8
1156228	android			Android		android\\s+([_.\\d]*)	8
1156228	android			Silk			7
1156228	android			HTC_Sensation_			6
1156229	windows phone os			Windows Phone OS		windows phone os\\s+([_.\\d]*)	8
1156230	palmos			PalmOS			8
1156230	palmos			PalmSource			8
1156231	symbos			Series			8
1156231	symbos			Symbian			8
1156231	symbos			SymbOS			8
1156232	windows mobile			Windows CE			8
1156232	windows mobile			Windows Mobile			8
1156232	windows mobile			HTC_HD2_			6
1156232	windows mobile			HTC-S			6
1156232	windows mobile			HTC_Touch_			6
1156232	windows mobile			XV6850			6
1156233	mac os x			Mac OS X		mac os x\\s+([\\d._]+)	2
1156234	linux			Linux		linux\\s+([\\d._]+)	2
1156235	windows			Windows		windows\\s+([.0-9a-z ]+)	3
1156235	windows			Win		win([a-z]*)([.0-9]*)	2
1156236	windows me			Win 9x 4.90			3
1156237	openbsd			OpenBSD		openbsd\\s+([\\d._]+)	2
1156238	freebsd			FreeBSD		freebsd\\s+([\\d._]+)	2
1156239	netbsd			NetBSD		netbsd\\s+([\\d._]+)	2
1156240	sunos			SunOS		sunos\\s+([\\d._]+)	2
1596265	maemo			Maemo			8
1596266	e-ink			Kindle			6
1596267	mobile os			Fennec			5
1596267	mobile os			midori			5
1596267	mobile os			Minimo			5
1596267	mobile os			Mobile			5
1596267	mobile os			tear			5
1596268	tablet	DEVICE-TABLET		HTC_Flyer			2
1596268	tablet	DEVICE-TABLET		A101IT			1
1596268	tablet	DEVICE-TABLET		A70BHT			1
1596268	tablet	DEVICE-TABLET		Advent Vega			1
1596268	tablet	DEVICE-TABLET		Dell Streak 7			1
1596268	tablet	DEVICE-TABLET		GT-P10			1
1596268	tablet	DEVICE-TABLET		Ideos S7			1
1596268	tablet	DEVICE-TABLET		iPad			1
1596268	tablet	DEVICE-TABLET		LePad			1
1596268	tablet	DEVICE-TABLET		MID7015			1
1596268	tablet	DEVICE-TABLET		pandigitalnova			1
1596268	tablet	DEVICE-TABLET		pandigitalsprnova			1
1596268	tablet	DEVICE-TABLET		SC-01C			1
1596268	tablet	DEVICE-TABLET		SCH-I800			1
1596268	tablet	DEVICE-TABLET		SGH-I987			1
1596268	tablet	DEVICE-TABLET		SGH-T849			1
1596268	tablet	DEVICE-TABLET		SHW-M180L			1
1596268	tablet	DEVICE-TABLET		SHW-M180S			1
1596268	tablet	DEVICE-TABLET		SPH-P100			1
1596268	tablet	DEVICE-TABLET		Sprint ATP51			1
1596268	tablet	DEVICE-TABLET		Tablet			1
1596268	tablet	DEVICE-TABLET		ViewPad7			1
1596268	tablet	DEVICE-TABLET		zt180			1
1596269	smartphone	DEVICE-SMARTPHONE		CLDC			1
1596269	smartphone	DEVICE-SMARTPHONE		fone 945			1
1596269	smartphone	DEVICE-SMARTPHONE		HTC			1
1596269	smartphone	DEVICE-SMARTPHONE		HTC Magic			1
1596269	smartphone	DEVICE-SMARTPHONE		HTCX06HT			1
1596269	smartphone	DEVICE-SMARTPHONE		MIDP			1
1596269	smartphone	DEVICE-SMARTPHONE		Mobile			1
1596269	smartphone	DEVICE-SMARTPHONE		Nexus One			1
1596269	smartphone	DEVICE-SMARTPHONE		SC-02B			1
1596276	webos			webOS		\\s+webos/([_.\\d]*)	8
1596277	meego			MeeGo			8
1596278	blackberry tablet os			PlayBook			6
1739679	opera mini	WEBBROWSER		Opera Mini			1
1739680	opera mobile	WEBBROWSER		Opera Mobi			1
1739681	bada			Bada			1
1739682	mtkos			MTK			1
1891016	chrome 7-15	BROWSER		Chrome	^mozilla.+chrome/([7-9]|1[0-5])		1
1891017	chrome 16+	BROWSER		Chrome	^mozilla.+chrome/([1-9]\\d{2,}|[2-9]\\d|1[6-9])		2
1891018	firefox 3.6	BROWSER		Firefox/3.6	^mozilla.+firefox/3\\.6		1
1891019	firefox 8+	BROWSER		Firefox	^mozilla.+firefox/([1-9]\\d+|[89])		1
1891020	yandex search	BROWSER		MSIE6	yandexsearch		1
1891020	yandex search	BROWSER		Yandex Search	yandexsearch		1
1891021	ie 7	BROWSER		MSIE 7	^mozilla.+msie[ ]?7\\.0		1
1891021	ie 7	BROWSER		MSIE7	^mozilla.+msie[ ]?7\\.0		1
1891022	ie 8	BROWSER		MSIE 8	^mozilla.+msie[ ]?8\\.0		1
1891022	ie 8	BROWSER		MSIE8	^mozilla.+msie[ ]?8\\.0		1
1891023	ie 9	BROWSER		MSIE 9	^mozilla.+msie[ ]?9\\.0		1
1891023	ie 9	BROWSER		MSIE9	^mozilla.+msie[ ]?9\\.0		1
1891024	ie 10	BROWSER		MSIE 10	^mozilla.+msie[ ]?10\\.0		1
1891024	ie 10	BROWSER		MSIE10	^mozilla.+msie[ ]?10\\.0		1
1891025	safari 6+	BROWSER		Safari	version/([1-9]\\d+|[6-9])		1
1891036	ie 11+	BROWSER		Trident	^mozilla.+edg([.\\d]*)		1
2681249	opera desktop	BROWSER		OPR	^mozilla.+opr/([.\\d]*)		3
2681250	yandexbrowser	BROWSER		YaBrowser	^mozilla.+yabrowser([.\\d]*)		3
3673755	application	APPLICATION					0
3818152	smarttv	DEVICE-SMARTTV		SmartTV			1
3818152	smarttv	DEVICE-SMARTTV		SMART-TV			1
3818152	smarttv	DEVICE-SMARTTV		tv	mozilla.+tv		1
)PLATFORM_RULES";

  constexpr std::string_view USER_AGENTS[] = {
    "BlackBerry9000/5.0.0.93 Profile/MIDP-2.0 Configuration/CLDC-1.1 VendorID/179",
    "Mozilla/5.0 (BlackBerry; U; BlackBerry 9900; en-US) AppleWebKit/534.11+ (KHTML, like Gecko) Version/7.0.0.261 Mobile Safari/534.11+",
    "Mozilla/5.0 (PlayBook; U; RIM Tablet OS 1.0.0; en-US) AppleWebKit/534.8+ (KHTML, like Gecko) Version/0.0.1 Safari/534.8+",
    "Mozilla/5.0 (Linux; U; Android 1.6; en-us; eeepc Build/Donut) AppleWebKit/528.5+ (KHTML, like Gecko) Version/3.1.2 Mobile Safari/525.20.1",
    "Mozilla/5.0 (Linux; U; Android 2.1-update1; ru-ru; GT-I9000 Build/ECLAIR) AppleWebKit/530.17 (KHTML, like Gecko) Version/4.0 Mobile Safari/530.17",
    "Mozilla/5.0 (Linux; U; Android 2.2; ru-ru; GT-I9000 Build/FROYO) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1",
    "Mozilla/5.0 (Linux; U; Android 3.1; en-us; GT-P7510 Build/HMJ37) AppleWebKit/534.13 (KHTML, like Gecko) Version/4.0 Safari/534.13",
    "Mozilla/5.0 (Linux; U; Android 2.1-update1 (7hero-astar9.3); ru-ru; HTC Legend Build/ERE27) AppleWebKit/530.17 (KHTML, like Gecko) Version/4.0 Mobile Safari/530.17",
    "Mozilla/5.0 (X11; U; Linux i686 (x86_64); en-US; rv:1.8.1.6) Gecko/20070817 IceWeasel/2.0.0.6-g2",
    "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0)",
    "Mozilla/4.0 (compatible; MSIE 6.0; Windows CE; IEMobile m.n)",
    "Mozilla/5.0 (Windows; U; Windows CE 4.21; rv:1.8b4) Gecko/20050720 Minimo/0.007",
    "Mozilla/5.0 (Windows; I; Windows NT 5.1; ru; rv:1.9.2.13) Gecko/20100101 Firefox/4.0",
    "Opera/9.80 (Windows NT 6.1; U; ru) Presto/2.8.131 Version/11.10",
    "Opera/9.80 (Macintosh; Intel Mac OS X 10.6.7; U; ru) Presto/2.8.131 Version/11.10",
    "Opera/9.60 (J2ME/MIDP; Opera Mini/4.2.14912/812; U; ru) Presto/2.4.15",
    "Mozilla/5.0 (SymbianOS/9.1; U; en-us) AppleWebKit/413 (KHTML, like Gecko) Safari/413",
    "Mozilla/4.0 (PSP (PlayStation Portable); 2.00)"
  };

  struct Options
  {
    std::uint64_t count = 100000;
    std::uint64_t threads = 1;
    std::string platform_rules_path;
  };

  struct PlatformMatcherRule
  {
    unsigned long platform_id = 0;
    std::string name;
    std::string type;
    std::string use_name;
    std::string marker;
    std::string match_regexp;
    std::string output_regexp;
    unsigned long priority = 0;
  };

  using PlatformMatcherRuleArray = std::vector<PlatformMatcherRule>;

  struct PlatformMatcherRuleStats
  {
    std::size_t rules = 0;
    std::size_t match_regexps = 0;
    std::size_t output_regexps = 0;
  };

  struct MatcherConfig
  {
    FrontendCommons::PlatformMatcher_var platform_matcher;
    std::string rules_source;
    PlatformMatcherRuleStats rule_stats;
  };

  struct WorkerResult
  {
    std::uint64_t matched = 0;
    std::uint64_t checksum = 0;
  };

  void
  print_usage()
  {
    std::cerr
      << "Usage: PlatformMatcherTest [OPTIONS]\n"
      << "Options:\n"
      << "  --count <N>    total PlatformMatcher::match calls (default: 100000)\n"
      << "  --threads <N>  worker threads count (default: 1)\n"
      << "  --platform-rules <PATH>\n"
      << "                 platform matcher rules TSV override\n"
      << "  --help, -h     print this help\n";
  }

  Options
  parse_options(int argc, char** argv)
  {
    Options options;

    for(int i = 1; i < argc; ++i)
    {
      const std::string arg(argv[i]);
      if(arg == "--help" || arg == "-h")
      {
        print_usage();
        std::exit(0);
      }
      else if(arg == "--count")
      {
        if(++i == argc)
        {
          throw std::runtime_error("--count requires value");
        }
        options.count = AdServer::Tests::Frontends::parse_uint64(
          argv[i],
          "--count");
      }
      else if(arg == "--threads")
      {
        if(++i == argc)
        {
          throw std::runtime_error("--threads requires value");
        }
        options.threads = AdServer::Tests::Frontends::parse_uint64(
          argv[i],
          "--threads");
      }
      else if(arg == "--platform-rules")
      {
        if(++i == argc)
        {
          throw std::runtime_error("--platform-rules requires value");
        }
        options.platform_rules_path = argv[i];
      }
      else
      {
        throw std::runtime_error("unexpected option '" + arg + "'");
      }
    }

    return options;
  }

  std::string
  unescape_value(std::string_view value)
  {
    std::string result;
    result.reserve(value.size());

    for(std::size_t i = 0; i < value.size(); ++i)
    {
      const char ch = value[i];
      if(ch != '\\' || i + 1 == value.size())
      {
        result += ch;
        continue;
      }

      const char escaped = value[++i];
      switch(escaped)
      {
      case '\\':
        result += '\\';
        break;
      case 't':
        result += '\t';
        break;
      case 'r':
        result += '\r';
        break;
      case 'n':
        result += '\n';
        break;
      default:
        result += escaped;
        break;
      }
    }

    return result;
  }

  std::vector<std::string>
  split_fields(std::string_view line)
  {
    std::vector<std::string> result;
    std::size_t field_begin = 0;

    for(std::size_t i = 0; i <= line.size(); ++i)
    {
      if(i == line.size() || line[i] == '\t')
      {
        result.emplace_back(
          unescape_value(line.substr(field_begin, i - field_begin)));
        field_begin = i + 1;
      }
    }

    return result;
  }

  unsigned long
  parse_unsigned_long(
    const std::string& value,
    const char* field_name,
    unsigned long line_number)
  {
    unsigned long result = 0;
    if(!String::StringManip::str_to_int(String::SubString(value), result))
    {
      std::ostringstream error;
      error << "invalid " << field_name << " at line " << line_number <<
        ": '" << value << "'";
      throw std::runtime_error(error.str());
    }

    return result;
  }

  PlatformMatcherRuleArray
  load_platform_matcher_rules(std::istream& input)
  {
    PlatformMatcherRuleArray result;
    std::string line;
    unsigned long line_number = 0;

    while(std::getline(input, line))
    {
      ++line_number;
      if(!line.empty() && line.back() == '\r')
      {
        line.pop_back();
      }

      if(line.empty() || line[0] == '#')
      {
        continue;
      }

      std::vector<std::string> fields = split_fields(line);
      if(fields.size() != 8)
      {
        std::ostringstream error;
        error << "invalid platform matcher rule at line " << line_number <<
          ": expected 8 fields, got " << fields.size();
        throw std::runtime_error(error.str());
      }

      PlatformMatcherRule rule;
      rule.platform_id = parse_unsigned_long(
        fields[0],
        "platform_id",
        line_number);
      rule.name = std::move(fields[1]);
      rule.type = std::move(fields[2]);
      rule.use_name = std::move(fields[3]);
      rule.marker = std::move(fields[4]);
      rule.match_regexp = std::move(fields[5]);
      rule.output_regexp = std::move(fields[6]);
      rule.priority = parse_unsigned_long(
        fields[7],
        "priority",
        line_number);
      result.emplace_back(std::move(rule));
    }

    return result;
  }

  void
  add_platform_matcher_rules(
    FrontendCommons::PlatformMatcher& platform_matcher,
    const PlatformMatcherRuleArray& rules)
  {
    for(const auto& rule : rules)
    {
      platform_matcher.add_rule(
        rule.platform_id,
        String::SubString(rule.name),
        rule.type.c_str(),
        rule.use_name.c_str(),
        rule.marker.c_str(),
        rule.match_regexp.c_str(),
        rule.output_regexp.c_str(),
        rule.priority);
    }
  }

  PlatformMatcherRuleStats
  collect_platform_matcher_rule_stats(
    const PlatformMatcherRuleArray& rules) noexcept
  {
    PlatformMatcherRuleStats stats;
    stats.rules = rules.size();

    for(const auto& rule : rules)
    {
      if(!rule.match_regexp.empty())
      {
        ++stats.match_regexps;
      }

      if(!rule.output_regexp.empty())
      {
        ++stats.output_regexps;
      }
    }

    return stats;
  }

  MatcherConfig
  make_platform_matcher(
    PlatformMatcherRuleArray rules,
    std::string rules_source)
  {
    if(rules.empty())
    {
      throw std::runtime_error(
        "platform rules source '" + rules_source + "' is empty");
    }

    FrontendCommons::PlatformMatcher_var platform_matcher(
      new FrontendCommons::PlatformMatcher());
    add_platform_matcher_rules(*platform_matcher, rules);

    return {
      platform_matcher,
      std::move(rules_source),
      collect_platform_matcher_rule_stats(rules)
    };
  }

  MatcherConfig
  load_platform_matcher(const std::string& path)
  {
    std::ifstream input(path);
    if(!input)
    {
      throw std::runtime_error(
        "can't open platform rules file '" + path + "'");
    }

    return make_platform_matcher(
      load_platform_matcher_rules(input),
      path);
  }

  MatcherConfig
  load_default_platform_matcher()
  {
    std::istringstream input(DEFAULT_PLATFORM_RULES);
    return make_platform_matcher(
      load_platform_matcher_rules(input),
      "compiled-in platform rules");
  }

  MatcherConfig
  make_platform_matcher(const Options& options)
  {
    if(!options.platform_rules_path.empty())
    {
      return load_platform_matcher(options.platform_rules_path);
    }

    if(const char* env_path = std::getenv(PLATFORM_MATCHER_RULES_ENV))
    {
      if(env_path[0] != 0)
      {
        return load_platform_matcher(env_path);
      }
    }

    return load_default_platform_matcher();
  }

  std::uint64_t
  checksum_string(const std::string& value)
  {
    std::uint64_t result = value.size();
    if(!value.empty())
    {
      result += static_cast<unsigned char>(value.front());
      result += static_cast<unsigned char>(value.back());
    }
    return result;
  }

  __attribute__((noinline))
  WorkerResult
  run_match_benchmark(
    const FrontendCommons::PlatformMatcher& platform_matcher,
    std::uint64_t count,
    std::uint64_t offset)
  {
    WorkerResult result;
    constexpr std::uint64_t examples_size =
      sizeof(USER_AGENTS) / sizeof(USER_AGENTS[0]);

    for(std::uint64_t i = 0; i < count; ++i)
    {
      const std::string_view user_agent =
        USER_AGENTS[(i + offset) % examples_size];
      std::string platform;
      std::string full_platform;
      FrontendCommons::PlatformMatcher::PlatformIdSet platform_ids;

      if(platform_matcher.match(
          &platform_ids,
          platform,
          full_platform,
          user_agent))
      {
        result.checksum += platform_ids.size();
        result.checksum += checksum_string(platform);
        result.checksum += checksum_string(full_platform);
        ++result.matched;
      }
    }

    return result;
  }
}

int
main(int argc, char** argv)
{
  try
  {
    const Options options = parse_options(argc, argv);
    const MatcherConfig matcher_config = make_platform_matcher(options);
    std::vector<std::thread> threads;
    std::vector<WorkerResult> results(options.threads);
    threads.reserve(options.threads);

    const auto started_at = std::chrono::steady_clock::now();
    const auto cpu_started = AdServer::Tests::Frontends::current_cpu_times();

    const std::uint64_t count_per_thread = options.count / options.threads;
    const std::uint64_t tail_count = options.count % options.threads;

    for(std::uint64_t thread_i = 0; thread_i < options.threads; ++thread_i)
    {
      const std::uint64_t thread_count =
        count_per_thread + (thread_i < tail_count ? 1 : 0);
      const std::uint64_t offset = thread_i * count_per_thread + std::min(
        thread_i,
        tail_count);

      threads.emplace_back(
        [
          matcher = matcher_config.platform_matcher.in(),
          &results,
          thread_i,
          thread_count,
          offset
        ]()
        {
          results[thread_i] = run_match_benchmark(
            *matcher,
            thread_count,
            offset);
        });
    }

    for(auto& thread : threads)
    {
      thread.join();
    }

    const auto cpu_finished = AdServer::Tests::Frontends::current_cpu_times();
    const auto finished_at = std::chrono::steady_clock::now();

    WorkerResult total;
    for(const auto& result : results)
    {
      total.matched += result.matched;
      total.checksum += result.checksum;
    }

    if(total.matched == 0)
    {
      throw std::runtime_error("PlatformMatcher didn't match any user agent");
    }

    const double elapsed_sec = std::chrono::duration<double>(
      finished_at - started_at).count();
    const double user_cpu_sec = cpu_finished.user - cpu_started.user;
    const double sys_cpu_sec = cpu_finished.sys - cpu_started.sys;
    const double cpu_sec = user_cpu_sec + sys_cpu_sec;
    const double rate = static_cast<double>(options.count) / elapsed_sec;
    const double cpu_ns_per_call =
      cpu_sec * 1000000000.0 / static_cast<double>(options.count);

    std::cout
      << "count=" << options.count << '\n'
      << "threads=" << options.threads << '\n'
      << "rules_source=" << matcher_config.rules_source << '\n'
      << "platform_rules=" << matcher_config.rule_stats.rules << '\n'
      << "platform_match_regexps=" <<
        matcher_config.rule_stats.match_regexps << '\n'
      << "platform_output_regexps=" <<
        matcher_config.rule_stats.output_regexps << '\n'
      << "matched=" << total.matched << '\n'
      << "checksum=" << total.checksum << '\n'
      << "elapsed_sec=" << AdServer::Tests::Frontends::format_float(
        elapsed_sec) << '\n'
      << "rate_per_sec=" << AdServer::Tests::Frontends::format_float(
        rate) << '\n'
      << "cpu_sec=" << AdServer::Tests::Frontends::format_float(
        cpu_sec) << '\n'
      << "user_cpu_sec=" << AdServer::Tests::Frontends::format_float(
        user_cpu_sec) << '\n'
      << "sys_cpu_sec=" << AdServer::Tests::Frontends::format_float(
        sys_cpu_sec) << '\n'
      << "cpu_ns_per_call=" << AdServer::Tests::Frontends::format_float(
        cpu_ns_per_call) << '\n';

    return 0;
  }
  catch(const std::exception& ex)
  {
    std::cerr << "std::exception: " << ex.what() << std::endl;
  }

  return 1;
}
