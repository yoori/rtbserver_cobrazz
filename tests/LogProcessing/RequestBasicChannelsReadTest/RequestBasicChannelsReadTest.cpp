#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <LogCommons/RequestBasicChannels.hpp>

namespace
{
  namespace LP = AdServer::LogProcessing;

  struct Options
  {
    std::size_t iterations = 1000;
    std::size_t triggers = 1000;
    std::vector<std::string> files;
  };

  void
  print_usage(std::ostream& out, const char* program)
  {
    out
      << "Usage: " << program
      << " [--iterations N] [--triggers N] [RBC_FILE ...]\n\n"
      << "Without RBC_FILE, repeatedly parses one synthetic RBC record.\n"
      << "With RBC_FILE, repeatedly loads complete RequestBasicChannels files.\n";
  }

  std::size_t
  parse_size(std::string_view value, std::string_view option)
  {
    std::size_t result = 0;
    const auto [ptr, error] = std::from_chars(
      value.data(),
      value.data() + value.size(),
      result);
    if (error != std::errc() || ptr != value.data() + value.size() || result == 0)
    {
      throw std::invalid_argument(
        std::string(option) + " requires a positive integer");
    }
    return result;
  }

  Options
  parse_options(int argc, char** argv)
  {
    Options options;
    for (int index = 1; index < argc; ++index)
    {
      const std::string_view argument(argv[index]);
      if (argument == "--help")
      {
        print_usage(std::cout, argv[0]);
        std::exit(0);
      }

      if (argument == "--iterations" || argument == "--triggers")
      {
        if (++index == argc)
        {
          throw std::invalid_argument(std::string(argument) + " requires a value");
        }

        const std::size_t value = parse_size(argv[index], argument);
        if (argument == "--iterations")
        {
          options.iterations = value;
        }
        else
        {
          options.triggers = value;
        }
        continue;
      }

      if (argument.starts_with("--"))
      {
        throw std::invalid_argument("unknown option: " + std::string(argument));
      }

      options.files.emplace_back(argument);
    }
    return options;
  }

  LP::RequestBasicChannelsInnerData
  make_synthetic_record(std::size_t trigger_count)
  {
    LP::NumberArray history_channels;
    history_channels.reserve(trigger_count);

    LP::RequestBasicChannelsInnerData::TriggerMatchArray trigger_matches;
    trigger_matches.reserve(trigger_count);
    for (std::size_t index = 0; index < trigger_count; ++index)
    {
      const auto channel_id = static_cast<std::uint32_t>(1'000'000 + index);
      history_channels.push_back(channel_id);
      trigger_matches.emplace_back(channel_id, channel_id + 10'000'000);
    }

    auto search_trigger_matches = trigger_matches;
    auto url_trigger_matches = trigger_matches;
    auto url_keyword_trigger_matches = trigger_matches;
    LP::RequestBasicChannelsInnerData::Match match(
      std::move(history_channels),
      std::move(trigger_matches),
      std::move(search_trigger_matches),
      std::move(url_trigger_matches),
      std::move(url_keyword_trigger_matches));

    return LP::RequestBasicChannelsInnerData(
      'H',
      LP::UserId("hSUsEk05T-m8PafRng8v6w.."),
      LP::UserId(),
      LP::RequestBasicChannelsInnerData::MatchOptional(std::move(match)),
      LP::RequestBasicChannelsInnerData::AdRequestPropsOptional());
  }

  LP::RequestBasicChannelsInnerData
  make_ad_request_record()
  {
    using Record = LP::RequestBasicChannelsInnerData;

    LP::NumberArray impression_channels = {300, 301, 302};
    Record::AdSlotImpression display_ad(
      Record::FixedNum("100.25"),
      impression_channels);
    Record::AdSlotImpressionOptional display_ad_optional(display_ad);

    Record::AdBidSlotImpressionList text_ads;
    text_ads.emplace_back(
      Record::FixedNum("200.25"),
      Record::FixedNum("300.25"),
      impression_channels);
    text_ads.emplace_back(
      Record::FixedNum("400.25"),
      Record::FixedNum("500.25"),
      impression_channels);

    LP::NumberArray full_freq_caps = {1, 2, 3};
    Record::AdSelectProps ad_select(
      12'345'678,
      "SIZE 0",
      "format !@#$%,^$&*:.",
      true,
      true,
      full_freq_caps);
    Record::AdSelectPropsOptional ad_select_optional(ad_select);

    LP::StringList sizes = {"SIZE 1", "SIZE 2", "SIZE 3"};
    Record::AdRequestProps ad_request(
      sizes,
      "RU",
      20,
      Record::FixedNum("30.125"),
      display_ad_optional,
      text_ads,
      ad_select_optional,
      AdServer::CampaignSvcs::AT_MAX_ECPM);
    Record::AdRequestPropsOptional ad_request_optional(ad_request);

    return Record(
      'P',
      LP::UserId("hSUsEk05T-m8PafRng8v6w.."),
      LP::UserId("PPPPPPPPPPPPPPPPPPPPPA.."),
      Record::MatchOptional(),
      Record::AdRequestPropsOptional(ad_request_optional));
  }

  std::size_t
  trigger_count(const LP::RequestBasicChannelsInnerData& data)
  {
    if (!data.match_request().present())
    {
      return 0;
    }

    const auto& match = data.match_request().get();
    return match.page_trigger_channels().size() +
      match.search_trigger_channels().size() +
      match.url_trigger_channels().size() +
      match.url_keyword_trigger_channels().size();
  }

  std::string
  replace_field(
    std::string record,
    std::size_t field_index,
    std::string_view value)
  {
    std::size_t begin = 0;
    for (std::size_t index = 0; index < field_index; ++index)
    {
      begin = record.find('\t', begin);
      if (begin == std::string::npos)
      {
        throw std::runtime_error("not enough fields in synthetic RBC record");
      }
      ++begin;
    }

    std::size_t end = record.find('\t', begin);
    if (end == std::string::npos)
    {
      end = record.find('\n', begin);
    }
    record.replace(begin, end - begin, value);
    return record;
  }

  bool
  parse_history_channels(
    const std::string& record,
    std::string_view value,
    LP::NumberArray* channels = nullptr)
  {
    constexpr std::size_t HISTORY_CHANNELS_FIELD = 3;

    std::istringstream input(
      replace_field(record, HISTORY_CHANNELS_FIELD, value));
    LP::RequestBasicChannelsInnerData data;
    if (!(input >> data))
    {
      return false;
    }

    if (channels)
    {
      *channels = data.match_request().get().history_channels();
    }
    return true;
  }

  void
  verify_history_channel_parser(const std::string& record)
  {
    const auto max_value = std::numeric_limits<std::uint32_t>::max();
    const std::string max_string = std::to_string(max_value);

    LP::NumberArray channels;
    if (!parse_history_channels(record, "+1," + max_string, &channels) ||
      channels != LP::NumberArray({1, max_value}))
    {
      throw std::runtime_error("history channel boundary parsing failed");
    }

    if (parse_history_channels(record, max_string + '0') ||
      parse_history_channels(record, "-1"))
    {
      throw std::runtime_error("invalid history channel value was accepted");
    }
  }

  bool
  parse_page_trigger_matches(
    const std::string& record,
    std::string_view value,
    LP::RequestBasicChannelsInnerData::TriggerMatchArray* matches = nullptr)
  {
    // user_type, user_id, temporary_user_id, history_channels,
    // page_trigger_channels
    constexpr std::size_t PAGE_TRIGGER_CHANNELS_FIELD = 4;

    std::istringstream input(
      replace_field(record, PAGE_TRIGGER_CHANNELS_FIELD, value));
    LP::RequestBasicChannelsInnerData data;
    if (!(input >> data))
    {
      return false;
    }

    if (matches)
    {
      *matches = data.match_request().get().page_trigger_channels();
    }
    return true;
  }

  void
  verify_trigger_match_parser(const std::string& record)
  {
    const auto max_value = std::numeric_limits<std::uint32_t>::max();
    const std::string max_string = std::to_string(max_value);

    LP::RequestBasicChannelsInnerData::TriggerMatchArray matches;
    if (!parse_page_trigger_matches(
        record,
        "+1:+2," + max_string + ':' + max_string,
        &matches) ||
      matches.size() != 2 ||
      matches[0] != LP::RequestBasicChannelsInnerData::TriggerMatch(1, 2) ||
      matches[1] !=
        LP::RequestBasicChannelsInnerData::TriggerMatch(max_value, max_value))
    {
      throw std::runtime_error("TriggerMatch boundary parsing failed");
    }

    const std::vector<std::string> invalid_values =
    {
      "",
      "+:1",
      "1",
      ":1",
      "1:",
      "1::2",
      "1:2,",
      "1:2,,3:4",
      "-1:2",
      "1:-2",
      "1:2x",
      max_string + "0:1",
      "1:" + max_string + '0'
    };
    for (const auto& value : invalid_values)
    {
      if (parse_page_trigger_matches(record, value))
      {
        throw std::runtime_error(
          "invalid TriggerMatch value was accepted: " + value);
      }
    }
  }

  bool
  parse_record(const std::string& record)
  {
    std::istringstream input(record);
    LP::RequestBasicChannelsInnerData data;
    return static_cast<bool>(input >> data);
  }

  void
  verify_ad_request_parser()
  {
    const auto expected = make_ad_request_record();
    std::ostringstream output;
    output << expected << '\n';
    const std::string serialized = output.str();

    LP::RequestBasicChannelsInnerData restored;
    std::istringstream input(serialized);
    if (!(input >> restored) || !(restored == expected))
    {
      throw std::runtime_error("RBC ad-request round-trip failed");
    }

    constexpr std::size_t DISPLAY_AD_FIELD = 8;
    constexpr std::size_t TEXT_ADS_FIELD = 9;
    constexpr std::size_t AD_SELECT_FIELD = 10;
    if (parse_record(replace_field(
        serialized, DISPLAY_AD_FIELD, "@100:")) ||
      parse_record(replace_field(
        serialized, TEXT_ADS_FIELD, "100:200:300/")) ||
      parse_record(replace_field(
        serialized, AD_SELECT_FIELD, "@1:size:format:1:1:")))
    {
      throw std::runtime_error("invalid RBC compound field was accepted");
    }
  }

  void
  run_synthetic(const Options& options)
  {
    const auto expected = make_synthetic_record(options.triggers);
    std::ostringstream output;
    output << expected << '\n';
    const std::string serialized = output.str();

    verify_history_channel_parser(serialized);
    verify_trigger_match_parser(serialized);
    verify_ad_request_parser();

    {
      LP::RequestBasicChannelsInnerData restored;
      std::istringstream input(serialized);
      if (!(input >> restored) || !(restored == expected))
      {
        throw std::runtime_error("synthetic RBC round-trip failed");
      }
    }

    std::size_t checksum = 0;
    std::istringstream input(serialized);
    const auto start = std::chrono::steady_clock::now();
    for (std::size_t iteration = 0; iteration < options.iterations; ++iteration)
    {
      input.clear();
      input.seekg(0);

      LP::RequestBasicChannelsInnerData data;
      if (!(input >> data))
      {
        throw std::runtime_error("synthetic RBC read failed");
      }
      checksum += trigger_count(data);
    }
    const auto elapsed = std::chrono::steady_clock::now() - start;
    const double seconds = std::chrono::duration<double>(elapsed).count();
    const double mib = static_cast<double>(serialized.size()) *
      static_cast<double>(options.iterations) / (1024.0 * 1024.0);

    std::cout
      << "mode: synthetic\n"
      << "record bytes: " << serialized.size() << '\n'
      << "trigger matches per record: " << options.triggers * 4 << '\n'
      << "iterations: " << options.iterations << '\n'
      << "elapsed: " << std::fixed << std::setprecision(6) << seconds << " sec\n"
      << "records/sec: " << options.iterations / seconds << '\n'
      << "MiB/sec: " << mib / seconds << '\n'
      << "checksum: " << checksum << '\n';
  }

  std::size_t
  record_count(const LP::RequestBasicChannelsCollector& collector)
  {
    std::size_t result = 0;
    for (const auto& [key, records] : collector)
    {
      static_cast<void>(key);
      result += records.size();
    }
    return result;
  }

  void
  run_files(const Options& options)
  {
    std::uintmax_t bytes_per_iteration = 0;
    for (const auto& file : options.files)
    {
      bytes_per_iteration += std::filesystem::file_size(file);
    }

    std::size_t records = 0;
    const auto start = std::chrono::steady_clock::now();
    for (std::size_t iteration = 0; iteration < options.iterations; ++iteration)
    {
      for (const auto& file : options.files)
      {
        std::ifstream input(file);
        if (!input)
        {
          throw std::runtime_error("can't open RBC file: " + file);
        }

        LP::RequestBasicChannelsCollector collector;
        LP::RequestBasicChannelsTraits::IoHelperType io_helper(collector);
        io_helper.load(input);
        records += record_count(collector);
      }
    }
    const auto elapsed = std::chrono::steady_clock::now() - start;
    const double seconds = std::chrono::duration<double>(elapsed).count();
    const double total_bytes = static_cast<double>(bytes_per_iteration) *
      static_cast<double>(options.iterations);

    std::cout
      << "mode: files\n"
      << "files: " << options.files.size() << '\n'
      << "iterations: " << options.iterations << '\n'
      << "records: " << records << '\n'
      << "elapsed: " << std::fixed << std::setprecision(6) << seconds << " sec\n"
      << "records/sec: " << records / seconds << '\n'
      << "MiB/sec: " << total_bytes / (1024.0 * 1024.0) / seconds << '\n';
  }
}

int
main(int argc, char** argv)
{
  try
  {
    const Options options = parse_options(argc, argv);
    if (options.files.empty())
    {
      run_synthetic(options);
    }
    else
    {
      run_files(options);
    }
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << "RequestBasicChannelsReadTest: " << error.what() << '\n';
  }
  return 1;
}
