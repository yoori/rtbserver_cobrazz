#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include <HTTP/UrlAddress.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUtils.hpp>
#include <ChannelSvcs/ChannelServer/ChannelContainer.hpp>
#include <ChannelSvcs/ChannelServer/UpdateContainer.hpp>

namespace
{
  namespace fs = std::filesystem;
  namespace CS = AdServer::ChannelSvcs;

  struct Options
  {
    fs::path segments;
    std::string url;
    std::uint64_t iterations = 0;
  };

  std::uint64_t parse_number(const std::string& text)
  {
    std::uint64_t value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc() || result.ptr != text.data() + text.size() || value == 0)
    {
      throw std::runtime_error("Expected a positive integer: " + text);
    }

    return value;
  }

  Options parse_options(int argc, char** argv)
  {
    Options options;
    for (int i = 1; i < argc; ++i)
    {
      const std::string name = argv[i];
      if (i + 1 == argc)
      {
        throw std::runtime_error("Missing value for " + name);
      }

      const std::string value = argv[++i];
      if (name == "--segments")
      {
        options.segments = value;
      }
      else if (name == "--url")
      {
        options.url = value;
      }
      else if (name == "--iterations")
      {
        options.iterations = parse_number(value);
      }
      else
      {
        throw std::runtime_error("Unknown option: " + name);
      }
    }

    if (options.segments.empty() || options.url.empty() || options.iterations == 0)
    {
      throw std::runtime_error("--segments, --url and --iterations are required");
    }

    return options;
  }

  std::uint64_t load_segments(CS::ChannelContainer& container, const fs::path& root)
  {
    std::vector<std::pair<unsigned int, fs::path>> segments;
    for (const auto& entry : fs::directory_iterator(root))
    {
      if (!entry.is_directory())
      {
        continue;
      }

      const auto name = entry.path().filename().string();
      if (name.find_first_not_of("0123456789") != std::string::npos)
      {
        continue;
      }

      const auto id = parse_number(name);
      if (id > std::numeric_limits<unsigned int>::max())
      {
        throw std::runtime_error("Segment ID is too large: " + name);
      }

      segments.emplace_back(id, entry.path());
    }

    std::sort(segments.begin(), segments.end());
    CS::UpdateContainer update(&container, nullptr);
    CS::ChannelIdToMatchInfo_var info = new CS::ChannelIdToMatchInfo;
    const std::set<unsigned short> ports{80, 443};
    std::uint64_t triggers = 0;
    for (const auto& [id, path] : segments)
    {
      if (info->find(id) != info->end())
      {
        throw std::runtime_error("Duplicate segment ID: " + std::to_string(id));
      }

      auto& match_info = (*info)[id];
      match_info.channel = CS::Channel(id);
      match_info.channel.mark_type(CS::Channel::CT_ACTIVE);
      match_info.stamp = Generics::Time::get_time_of_day();
      CS::MergeAtom atom;
      atom.id = id;
      for (const auto& [name, type] : {std::pair{"page", 'P'}, {"urls", 'U'},
        {"url_keywords", 'R'}})
      {
        const auto file = path / name;
        if (!fs::exists(file))
        {
          continue;
        }

        std::ifstream input(file);
        if (!input)
        {
          throw std::runtime_error("Cannot read " + file.string());
        }

        std::string line;
        std::size_t line_number = 0;
        while (std::getline(input, line))
        {
          ++line_number;
          const auto first = line.find_first_not_of(" \t\r");
          if (first == std::string::npos)
          {
            continue;
          }

          line = line.substr(first, line.find_last_not_of(" \t\r") - first + 1);
          if (triggers == std::numeric_limits<unsigned int>::max())
          {
            throw std::runtime_error("Too many triggers");
          }

          const auto trigger_id = static_cast<unsigned int>(triggers + 1);
          const auto previous_size = atom.soft_words.size();
          if (type == 'U')
          {
            CS::TriggerParser::TriggerParser::parse_url(
              id, trigger_id, line, false, ports, atom.soft_words, nullptr);
          }
          else
          {
            CS::TriggerParser::TriggerParser::parse_word(
              id, trigger_id, type, line, false, nullptr, atom.soft_words,
              AdServer::Commons::DEFAULT_MAX_HARD_WORD_SEQ, nullptr);
          }

          if (atom.soft_words.size() == previous_size)
          {
            throw std::runtime_error(
              file.string() + ":" + std::to_string(line_number) + ": invalid trigger");
          }

          ++triggers;
          match_info.channel.mark_type(
            type == 'U' ? CS::CT_URL : type == 'P' ? CS::CT_PAGE : CS::CT_URL_KEYWORDS);
        }

        if (input.bad())
        {
          throw std::runtime_error("Read failed: " + file.string());
        }
      }

      update.add_trigger(atom);
    }

    if (triggers == 0)
    {
      throw std::runtime_error("No triggers found in " + root.string());
    }

    container.merge(update, *info, true);
    std::cout << "segments: " << segments.size() << "\ntriggers: " << triggers << '\n';
    return triggers;
  }

  double cpu_seconds()
  {
    timespec value{};
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &value) != 0)
    {
      throw std::runtime_error("clock_gettime(CLOCK_PROCESS_CPUTIME_ID) failed");
    }

    return value.tv_sec + value.tv_nsec / 1e9;
  }
}

int main(int argc, char** argv)
{
  if (argc == 2 && std::string(argv[1]) == "--help")
  {
    std::cout << "Usage: ChannelContainerPerfTest --segments DIR --url URL --iterations N\n";
    return 0;
  }

  try
  {
    const auto options = parse_options(argc, argv);
    CS::ChannelContainer container(128);
    load_segments(container, options.segments);

    const HTTP::BrowserAddress address{String::SubString(options.url)};
    std::string url_keywords;
    Language::Trigger::normalize_phrase(
      HTTP::keywords_from_http_address(address.url()), url_keywords, nullptr);
    CS::MatchUrls urls;
    const CS::MatchUrls additional_urls;
    CS::MatchWords words[CS::CT_MAX];
    const CS::MatchWords additional_words;
    const CS::StringVector exact_words;
    const Generics::Uuid uid;
    CS::ChannelContainer::match_parse_urls(
      address.url(), url_keywords, {80, 443}, false, urls, words[CS::CT_URL_KEYWORDS],
      nullptr, nullptr);
    if (urls.empty())
    {
      throw std::runtime_error("URL produced no match candidates");
    }

    std::uint64_t matched_channels = 0;
    std::cout << "iterations: " << options.iterations << "\nmatch loop starting\n" << std::flush;
    const auto wall_start = std::chrono::steady_clock::now();
    const auto cpu_start = cpu_seconds();
    for (std::uint64_t i = 0; i < options.iterations; ++i)
    {
      CS::TriggerMatchRes result;
      container.match(
        urls, additional_urls, words, additional_words, exact_words, uid,
        CS::MF_ACTIVE | CS::MF_BLACK_LIST, result);
      matched_channels += result.size();
    }

    const auto cpu = cpu_seconds() - cpu_start;
    const auto wall = std::chrono::duration<double>(
      std::chrono::steady_clock::now() - wall_start).count();
    std::cout << std::fixed << std::setprecision(6)
      << "cpu_seconds: " << cpu << '\n'
      << "wall_seconds: " << wall << '\n'
      << "cpu_us_per_match: " << cpu * 1e6 / options.iterations << '\n'
      << "matched_channels_total: " << matched_channels << '\n';
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
