#include <atomic>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>

#include <HTTP/UrlAddress.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUtils.hpp>
#include <ChannelSvcs/ChannelCommons/Serialization.hpp>
#include <ChannelSvcs/ChannelServer/ChannelContainer.hpp>
#include <ChannelSvcs/ChannelServer/UpdateContainer.hpp>

namespace
{
  namespace CS = AdServer::ChannelSvcs;

  void require(bool value, const char* message)
  {
    if (!value)
    {
      throw std::runtime_error(message);
    }
  }

  struct CollidingHash
  {
    static std::size_t hash(const void*, std::size_t) noexcept
    {
      return 7;
    }
  };

  void index_test()
  {
    CS::BasicUrlPrefixIndex<CollidingHash> index;
    index.add("a.com/x/y");
    index.add("b.com/x/y/z");
    auto counts = index.find(7);
    require(counts.terminal == 2 && counts.descendants == 5, "collision counts");
    auto snapshot = index;
    index.remove("a.com/x/y");
    counts = index.find(7);
    require(counts.terminal == 1 && counts.descendants == 3, "collision removal");
    require(snapshot.find(7).terminal == 2, "old snapshot changed");
    index.remove("b.com/x/y/z");
    require(!index.find(7).terminal && !index.find(7).descendants, "last key removal");

    CS::UrlPrefixIndex prefixes;
    prefixes.add("a.com/x/y");
    prefixes.add("a.com/x/z");
    const auto hash = Generics::FastHash::hash("a.com/x", 7);
    require(prefixes.find(hash).descendants == 2, "shared ancestor");
    auto old = prefixes;
    prefixes.remove("a.com/x/y");
    require(prefixes.find(hash).descendants == 1, "shared ancestor removed too early");
    require(old.find(hash).descendants == 2, "copy-on-write ancestor");
  }

  using Channels = std::map<unsigned int, std::vector<std::string>>;

  void update(CS::ChannelContainer& container, const Channels& channels, char type = 'U')
  {
    static unsigned int trigger_id = 1;
    CS::ChannelIdToMatchInfo_var info = new CS::ChannelIdToMatchInfo;
    for (const auto& [id, urls] : channels)
    {
      auto& item = (*info)[id];
      item.channel = CS::Channel(id);
      item.channel.mark_type(CS::Channel::CT_ACTIVE);
      item.channel.mark_type(type == 'U' ? CS::CT_URL : type == 'P' ? CS::CT_PAGE :
        type == 'R' ? CS::CT_URL_KEYWORDS : CS::CT_SEARCH);
      item.stamp = Generics::Time::get_time_of_day();
    }

    CS::UpdateContainer changes(&container, nullptr);
    changes.check_actual(*info);
    for (const auto& [id, urls] : channels)
    {
      CS::TriggerList empty;
      changes.select_parsed_triggers(id, empty, false);
      CS::MergeAtom atom;
      atom.id = id;
      for (const auto& url : urls)
      {
        if (type == 'U')
        {
          CS::TriggerParser::TriggerParser::parse_url(
            id, trigger_id++, url, false, {80, 443}, atom.soft_words, nullptr);
        }
        else
        {
          CS::TriggerParser::TriggerParser::parse_word(
            id, trigger_id++, type, url, false, nullptr, atom.soft_words,
            AdServer::Commons::DEFAULT_MAX_HARD_WORD_SEQ, nullptr);
        }
      }
      changes.add_trigger(atom);
    }
    container.merge(changes, *info, true);
  }

  std::set<unsigned long> match(CS::ChannelContainer& container, const std::string& url,
    unsigned int extra_flags = 0)
  {
    HTTP::BrowserAddress address{String::SubString(url)};
    const auto keywords = HTTP::keywords_from_http_address(address.url());
    CS::MatchUrls urls;
    CS::MatchWords words[CS::CT_MAX];
    std::string parsed_keywords;
    Language::Trigger::normalize_phrase(keywords, parsed_keywords, nullptr);
    CS::ChannelContainer::match_parse_urls(
      address.url(), parsed_keywords, {80, 443}, false, urls, words[CS::CT_URL_KEYWORDS],
      nullptr, nullptr, nullptr, true);
    CS::TriggerMatchRes result;
    container.match(urls, {}, words, {}, {}, Generics::Uuid(), CS::MF_ACTIVE | extra_flags, result);
    std::set<unsigned long> ids;
    for (const auto& item : result)
    {
      ids.insert(item.first);
    }
    return ids;
  }

  void keyword_prefix_test()
  {
    CS::BasicKeywordPrefixIndex<CollidingHash> collisions;
    collisions.add("one");
    require(!collisions.find(7).terminal, "single word has no hook");
    collisions.add("one two three");
    collisions.add("four five");
    auto old = collisions;
    require(collisions.find(7).terminal == 2 && collisions.find(7).descendants == 1,
      "keyword collision counts");
    collisions.remove("one two three");
    require(collisions.find(7).terminal == 1 && !collisions.find(7).descendants,
      "keyword collision removal");
    require(old.find(7).terminal == 2 && old.find(7).descendants == 1,
      "keyword snapshot mutated");
    collisions.remove("four five");
    require(!collisions.find(7).terminal, "last keyword collision removal");

    CS::KeywordPrefixIndex prefixes;
    prefixes.add("alpha beta gamma");
    auto parse = [&](const std::string& text, const CS::KeywordPrefixIndex* hints,
      std::size_t max_length = AdServer::Commons::DEFAULT_MAX_HARD_WORD_SEQ)
    {
      CS::MatchWords words;
      CS::MatchBreakSeparators separators;
      CS::parse_keywords(String::SubString(text), words, CS::PM_NO_SIMPLIFY, &separators,
        max_length, nullptr, nullptr, hints);
      std::set<std::string> result;
      for (const auto& word : words)
      {
        result.emplace(word.text());
      }
      return result;
    };
    require(parse("alpha beta", nullptr, 0).empty(), "zero unfiltered keyword length limit");
    require(parse("alpha alpha beta", nullptr, 1) == std::set<std::string>{"alpha", "beta"},
      "single words only, with deduplication");
    require(parse("alpha beta gamma", nullptr, 2).count("alpha beta gamma") == 0,
      "unfiltered keyword length limit exceeded");
    for (const std::size_t limit : {0, 1, 2})
    {
      const std::set<std::string> expected{"alpha", "beta", "gamma", "alpha beta gamma"};
      require(parse("alpha beta gamma", &prefixes, limit) == expected,
        "hooks incorrectly constrained by unfiltered length limit");
      CS::MatchWords deferred;
      CS::MatchBreakSeparators separators;
      CS::parse_keywords(String::SubString("alpha beta gamma"), deferred, CS::PM_NO_SIMPLIFY,
        &separators, limit, nullptr, nullptr, nullptr, true);
      Generics::MonoAllocatorFixedArena<4096> arena;
      CS::PreparedMatchWords prepared(&arena.arena());
      CS::prepare_keywords(deferred, prepared, &prefixes);
      std::set<std::string> actual;
      for (const auto& word : prepared.candidates())
      {
        actual.emplace(word.text());
      }
      require(actual == expected, "deferred hooks incorrectly constrained by length limit");
    }
    for (const bool filtered : {false, true})
    {
      const auto* hints = filtered ? &prefixes : nullptr;
      require(parse("alpha", hints) == std::set<std::string>{"alpha"}, "one word input");
      require(parse("", hints).empty(), "empty word input");
    }
    require(parse("alpha beta gamma delta", &prefixes) ==
      std::set<std::string>{"alpha", "beta", "gamma", "delta", "alpha beta gamma"},
      "prune hard phrases from two words");
    require(parse("alpha beta\ngamma delta", &prefixes) ==
      std::set<std::string>{"alpha", "beta", "gamma", "delta"}, "preserve phrase boundaries");
    prefixes.add("alpha beta");
    require(parse("alpha beta gamma", &prefixes).count("alpha beta") == 1,
      "terminal with descendants");
    prefixes.remove("alpha beta gamma");
    require(parse("alpha beta gamma", &prefixes) ==
      std::set<std::string>{"alpha", "beta", "gamma", "alpha beta"}, "remove descendants");
    require(parse("alpha beta gamma", nullptr).count("beta gamma") == 1,
      "unfiltered parser compatibility");
  }

  void raw_keyword_test()
  {
    CS::KeywordPrefixIndex prefixes;
    for (const auto* phrase : {"alpha beta", "alpha beta gamma", "beta gamma", "one two"})
    {
      prefixes.add(phrase);
    }
    const std::string oversized(CS::TriggerParser::TriggerParser::MAX_TRIGGER_LENGTH + 1, 'x');
    const std::vector<std::string> texts{"", " ,\n\t", "alpha beta gamma", "alpha\tbeta  gamma",
      "alpha alpha beta", "Alpha, BETA\nGamma", "one\t two,three\nfour", "ЁЖ\tABC",
      "alpha " + oversized + " beta\ngamma", "alpha beta " + oversized,
      std::string("alpha ") + static_cast<char>(0xff) + " beta"};
    for (const auto& text : texts)
    {
      for (const auto mode : {CS::PM_NO_SIMPLIFY, CS::PM_SIMPLIFY})
      {
        for (const std::string_view separators : {"", "\n", ",\n"})
        {
          for (const bool filtered : {false, true})
          {
            for (const std::size_t limit : {0, 1, 2, 20})
            {
              const auto* hints = filtered ? &prefixes : nullptr;
              CS::MatchWords legacy;
              CS::StringVector legacy_exact;
              const auto parse = [&](const auto* delimiter)
              {
                CS::parse_keywords(String::SubString(text), legacy, mode, delimiter, limit,
                  &legacy_exact, nullptr, hints);
              };
              CS::MatchBreakSeparators breaks;
              String::AsciiStringManip::SepNL newlines;
              if (separators.empty())
              {
                parse(static_cast<const CS::MatchBreakSeparators*>(nullptr));
              }
              else if (separators == "\n")
              {
                parse(&newlines);
              }
              else
              {
                parse(&breaks);
              }
              Generics::MonoAllocatorFixedArena<4096> arena;
              CS::PreparedMatchWords prepared(&arena.arena());
              CS::MatchWordVector exact(&arena.arena());
              CS::prepare_keywords({text, mode, separators, limit}, prepared, hints, &exact);
              std::set<std::string> expected, actual;
              for (const auto& word : legacy)
              {
                expected.emplace(word.text());
              }
              for (const auto& word : prepared.candidates())
              {
                actual.emplace(word.text());
                const auto hash = Generics::FastHash::hash(word.text().data(), word.text().size());
                require(word.hash() == hash, "raw keyword hash differs from stored text");
              }
              if (actual != expected)
              {
                throw std::runtime_error("raw keyword candidates differ: " + text);
              }
              std::vector<std::string> actual_exact;
              for (const auto& word : exact)
              {
                actual_exact.emplace_back(word.text());
              }
              require(actual_exact == legacy_exact, "raw exact-search word order differs");
            }
          }
        }
      }
    }

    const std::string borrowed = "alpha beta gamma";
    Generics::MonoAllocatorFixedArena<4096> arena;
    CS::PreparedMatchWords prepared(&arena.arena());
    CS::prepare_keywords({borrowed}, prepared, &prefixes);
    for (const auto& word : prepared.candidates())
    {
      require(word.text().data() == borrowed.data() + borrowed.find(word.text()),
        "canonical input copied unnecessarily");
    }

    CS::ChannelContainer container(128);
    CS::KeywordInput inputs[CS::CT_MAX];
    const std::string source = "alpha\t beta";
    inputs[CS::CT_SEARCH] = {source};
    update(container, {{1, {"[alpha beta]"}}}, 'S');
    CS::TriggerMatchRes result;
    container.match({}, {}, inputs, {}, Generics::Uuid(), CS::MF_ACTIVE, result);
    require(result.count(1) == 1, "raw exact search failed");
    update(container, {{2, {"alpha beta"}}}, 'R');
    CS::TriggerMatchRes url_result;
    container.match({}, {}, inputs, {}, Generics::Uuid(), CS::MF_ACTIVE, url_result);
    require(url_result.count(2) == 1, "raw search words missing from URL keyword input");
    update(container, {}, 'R');
    CS::TriggerMatchRes removed;
    container.match({}, {}, inputs, {}, Generics::Uuid(), CS::MF_ACTIVE, removed);
    require(removed.empty(), "raw input retained old hooks across updates");
  }

  void hard_keyword_test()
  {
    for (const char type : {'P', 'R', 'S'})
    {
      CS::ChannelContainer container(128);
      const auto match_type = type == 'P' ? CS::CT_PAGE :
        type == 'R' ? CS::CT_URL_KEYWORDS : CS::CT_SEARCH;
      auto lookup = [&](const std::string& text, const CS::KeywordPrefixIndex* hints,
        unsigned int flags = 0, bool deferred = false, bool raw = false)
      {
        CS::TriggerMatchRes result;
        if (raw)
        {
          CS::KeywordInput inputs[CS::CT_MAX];
          inputs[match_type] = {text};
          container.match({}, {}, inputs, {}, Generics::Uuid(), CS::MF_ACTIVE | flags, result);
        }
        else
        {
          CS::MatchWords words[CS::CT_MAX];
          CS::StringVector exact;
          CS::MatchBreakSeparators separators;
          CS::parse_keywords(String::SubString(text), words[match_type], CS::PM_NO_SIMPLIFY,
            &separators, AdServer::Commons::DEFAULT_MAX_HARD_WORD_SEQ, &exact, nullptr, hints,
            deferred);
          container.match(
            {}, {}, words, {}, exact, Generics::Uuid(), CS::MF_ACTIVE | flags, result);
        }
        std::set<unsigned long> ids;
        for (const auto& item : result)
        {
          ids.insert(item.first);
        }
        return ids;
      };
      const Channels channels{{1, {"anchor \"alpha beta\""}}, {2, {"\"alpha beta gamma\""}},
        {3, {"\"alpha beta\""}}};
      update(container, channels, type);
      std::string large_input = "anchor alpha beta gamma";
      for (unsigned int i = 0; i < 1000; ++i)
      {
        large_input += " filler" + std::to_string(i);
      }
      for (unsigned int i = 0; i < 3; ++i)
      {
        require(lookup(large_input, nullptr, 0, true) == std::set<unsigned long>{1, 2, 3},
          "request arena overflow changed matching");
        require(lookup(large_input, nullptr, 0, false, true) == std::set<unsigned long>{1, 2, 3},
          "raw request arena overflow changed matching");
        require(lookup("alpha gamma", nullptr, 0, true).empty(),
          "request arena retained previous candidates");
      }
      CS::MatchWords repeated[CS::CT_MAX];
      CS::MatchBreakSeparators repeated_separators;
      const std::string repeated_input = large_input + " " + large_input;
      CS::parse_keywords(String::SubString(repeated_input), repeated[match_type],
        CS::PM_NO_SIMPLIFY, &repeated_separators, AdServer::Commons::DEFAULT_MAX_HARD_WORD_SEQ,
        nullptr, nullptr, nullptr, true);
      repeated[match_type].insert(Generics::StringViewHashAdapter(std::string_view("anchor")));
      CS::TriggerMatchRes repeated_result;
      container.match({}, {}, repeated, {}, {}, Generics::Uuid(), CS::MF_ACTIVE, repeated_result);
      require(repeated_result.size() == 3, "duplicate candidates lost a matching channel");
      for (const auto& item : repeated_result)
      {
        require(item.second.trigger_ids[match_type].size() == 1,
          "duplicate candidates matched a trigger more than once");
      }
      auto snapshot = container.keyword_prefixes();
      for (const std::string text : {"anchor alpha beta gamma", "alpha beta", "alpha gamma",
        "anchor alpha beta\ngamma", "gamma alpha beta anchor"})
      {
        require(lookup(text, &(*snapshot)[match_type]) == lookup(text, nullptr),
          "filtered hard matching differs from full matching");
        require(lookup(text, nullptr, 0, true) == lookup(text, nullptr),
          "deferred hard matching differs from full matching");
        require(lookup(text, nullptr, 0, false, true) == lookup(text, nullptr),
          "raw hard matching differs from full matching");
      }
      require(lookup("anchor alpha beta gamma", &(*snapshot)[match_type]) ==
        std::set<unsigned long>{1, 2, 3}, "non-key hard part missing");
      update(container, {{1, {"anchor \"alpha beta\""}}}, type);
      auto current = container.keyword_prefixes();
      require(lookup("anchor alpha beta gamma", &(*current)[match_type]) ==
        std::set<unsigned long>{1}, "shared hard part removal");
      require(lookup("anchor alpha beta gamma", &(*snapshot)[match_type]) ==
        std::set<unsigned long>{1}, "old hooks with new matcher snapshot");
      const auto hash = Generics::FastHash::hash("alpha beta gamma", 16);
      require((*snapshot)[match_type].find(hash).terminal != 0, "old hooks lost terminal");
      require(!(*current)[match_type].find(hash).terminal, "removed hard hook retained");
      update(container, {}, type);
      require(lookup("anchor alpha beta gamma", &(*snapshot)[match_type]).empty(),
        "retained hooks after final removal");
      require(!(*container.keyword_prefixes())[match_type].find(
        Generics::FastHash::hash("alpha beta", 10)).terminal, "last hard hook removal");
      CS::ChannelContainer nonstrict(128, true);
      update(nonstrict, {{11, {"anchor \"alpha beta\""}}}, type);
      CS::MatchWords partial[CS::CT_MAX];
      partial[match_type].insert(Generics::StringViewHashAdapter(std::string_view("beta")));
      CS::TriggerMatchRes partial_result;
      nonstrict.match({}, {}, partial, {}, {}, Generics::Uuid(),
        CS::MF_ACTIVE | CS::MF_NONSTRICTKW, partial_result);
      require(partial_result.size() == 1 && partial_result.begin()->first == 11,
        "non-strict keyword expansion lost matcher");
      CS::KeywordInput raw_partial[CS::CT_MAX];
      raw_partial[match_type] = {"Beta", CS::PM_SIMPLIFY, "", 1};
      CS::TriggerMatchRes raw_partial_result;
      nonstrict.match({}, {}, raw_partial, {}, Generics::Uuid(),
        CS::MF_ACTIVE | CS::MF_NONSTRICTKW, raw_partial_result);
      require(raw_partial_result.size() == 1 && raw_partial_result.count(11),
        "raw non-strict keyword expansion lost matcher");

      CS::MatchWords reusable[CS::CT_MAX];
      CS::MatchBreakSeparators separators;
      CS::parse_keywords(String::SubString("new hard phrase"), reusable[match_type],
        CS::PM_NO_SIMPLIFY, &repeated_separators, AdServer::Commons::DEFAULT_MAX_HARD_WORD_SEQ,
        nullptr, nullptr, nullptr, true);
      require(reusable[match_type].empty(), "deferred parser expanded candidates early");
      update(container, {{9, {"\"new hard phrase\""}}}, type);
      CS::TriggerMatchRes added_result;
      container.match({}, {}, reusable, {}, {}, Generics::Uuid(), CS::MF_ACTIVE, added_result);
      require(added_result.size() == 1 && added_result.begin()->first == 9,
        "deferred request did not use newly published hooks");
      update(container, {}, type);
      CS::TriggerMatchRes removed_result;
      container.match({}, {}, reusable, {}, {}, Generics::Uuid(), CS::MF_ACTIVE, removed_result);
      require(removed_result.empty(), "reused deferred request retained old candidates");

      update(container, {{1, {"\"stable hard\""}}}, type);
      std::atomic<bool> stop{false};
      std::atomic<bool> failed{false};
      std::atomic<unsigned long> reads{0};
      std::thread reader([&]
      {
        while (!stop.load())
        {
          if (lookup("stable hard", nullptr, 0, true) != std::set<unsigned long>{1})
          {
            failed = true;
          }
          ++reads;
        }
      });
      while (!reads.load())
      {
        std::this_thread::yield();
      }
      for (unsigned int i = 0; i < 10; ++i)
      {
        update(container, {{1, {"\"stable hard\""}},
          {2, {i % 2 ? "\"changing hard one\"" : "\"changing hard two\""}}}, type);
      }
      stop = true;
      reader.join();
      require(!failed, "unchanged hard phrase lost during concurrent updates");

      if (type == 'S')
      {
        update(container, {{1, {"[alpha beta]"}}}, type);
        snapshot = container.keyword_prefixes();
        require(lookup("alpha beta", &(*snapshot)[match_type]) == std::set<unsigned long>{1},
          "exact search words preserved");
        require(lookup("beta alpha", &(*snapshot)[match_type]).empty(), "exact search order");
        require(lookup("alpha beta", nullptr, 0, true) == std::set<unsigned long>{1},
          "deferred exact search");
      }
    }
  }

  void keyword_update_test()
  {
    for (const char type : {'P', 'R', 'S'})
    {
      CS::ChannelContainer container(128);
      const auto match_type = type == 'P' ? CS::CT_PAGE :
        type == 'R' ? CS::CT_URL_KEYWORDS : CS::CT_SEARCH;
      auto lookup = [&](const std::string& word)
      {
        CS::MatchWords words[CS::CT_MAX];
        words[match_type].insert(Generics::StringViewHashAdapter(word));
        CS::TriggerMatchRes result;
        container.match({}, {}, words, {}, {}, Generics::Uuid(), CS::MF_ACTIVE, result);
        std::set<unsigned long> ids;
        for (const auto& item : result)
        {
          ids.insert(item.first);
        }
        return ids;
      };
      for (const std::string& word : {std::string("sharedkeyword"), std::string(80, 'x')})
      {
        update(container, {{1, {word}}, {2, {word}}}, type);
        require(lookup(word) == std::set<unsigned long>{1, 2}, "shared keyword key");
        require(lookup(word + "z").empty(), "different keyword");
        update(container, {{2, {word}}}, type);
        require(lookup(word) == std::set<unsigned long>{2}, "keyword key owner removal");
        update(container, {{2, {"replacement"}}}, type);
        require(lookup(word).empty(), "last keyword removal");
        require(lookup("replacement") == std::set<unsigned long>{2}, "keyword replacement");
        update(container, {}, type);
        require(lookup("replacement").empty(), "empty keyword index");
      }
    }
  }

  void keyword_snapshot_test()
  {
    for (const char type : {'P', 'R', 'S'})
    {
      CS::ChannelContainer container(128);
      const auto match_type = type == 'P' ? CS::CT_PAGE :
        type == 'R' ? CS::CT_URL_KEYWORDS : CS::CT_SEARCH;
      update(container, {{1, {"\"alpha beta\""}}}, type);
      std::atomic<bool> stop{false}, failed{false};
      std::atomic<unsigned long> reads{0};
      std::thread reader([&]
      {
        CS::KeywordInput inputs[CS::CT_MAX];
        inputs[match_type] = {"alpha beta gamma delta"};
        while (!stop.load())
        {
          CS::TriggerMatchRes result;
          container.match({}, {}, inputs, {}, Generics::Uuid(), CS::MF_ACTIVE, result);
          if (result.size() != 1 || !result.count(1))
          {
            failed = true;
          }
          ++reads;
        }
      });
      while (!reads.load())
      {
        std::this_thread::yield();
      }
      for (unsigned int pass = 0; pass < 30; ++pass)
      {
        update(container, {{1, {pass % 2 ? "\"alpha beta\"" : "\"gamma delta\""}}}, type);
      }
      stop = true;
      reader.join();
      require(!failed, "keyword hooks and chunks belong to different snapshots");
    }
  }

  void lexeme_hooks_test()
  {
    const auto lexeme = [](const std::string& text)
    {
      CS::Lexeme_var result = new CS::Lexeme;
      result->data = text;
      result->forms.emplace_back(String::SubString(result->data));
      return result;
    };
    for (const bool exact : {false, true})
    {
      CS::SoftTriggerList parsed;
      CS::TriggerParser::TriggerParser::parse_word(1, 1, 'S',
        exact ? "[alpha beta]" : "alpha beta", false, nullptr, parsed, 20, nullptr);
      CS::SoftMatcher_var matcher = new CS::SoftMatcher(0, parsed.front().trigger);
      CS::SubStringVector parts;
      CS::Serialization::get_parts(matcher->get_trigger(), parts);
      CS::LexemesPtrVector lexemes{lexeme("alpha alias"), lexeme("beta alias")};
      matcher->configure_matching_info(parts, lexemes, 0);
      std::vector<std::string_view> hints;
      matcher->get_keyword_parts(hints);
      CS::KeywordPrefixIndex prefixes;
      for (const auto part : hints)
      {
        prefixes.add(part);
      }
      Generics::MonoAllocatorFixedArena<4096> arena;
      CS::PreparedMatchWords words(&arena.arena());
      CS::prepare_keywords({"alpha alias beta alias"}, words, &prefixes);
      const Generics::StringViewHashAdapter main_form(std::string_view("alpha alias"));
      const Generics::StringViewHashAdapter other_form(std::string_view("beta alias"));
      require(words.words().count(main_form) == 1, "main lexeme form missing from hooks");
      require(words.words().count(other_form) == 1, "non-key lexeme form missing from hooks");
      if (exact)
      {
        CS::MatchWordVector ordered(&arena.arena());
        ordered.emplace_back(std::string_view("alpha alias"));
        ordered.emplace_back(std::string_view("beta alias"));
        require(matcher->match_exact(ordered), "exact lexeme matching failed");
        std::reverse(ordered.begin(), ordered.end());
        require(!matcher->match_exact(ordered), "exact lexeme order ignored");
      }
      else
      {
        require(matcher->match(words.words(), false), "soft lexeme matching failed");
      }
      for (const auto part : hints)
      {
        prefixes.remove(part);
      }
      require(!prefixes.find(main_form.hash()).terminal,
        "lexeme hook remains after removal");
    }
  }

  void url_branch_test()
  {
    CS::ChannelContainer container(128);
    std::string path;
    for (unsigned int i = 0; i < 8; ++i)
    {
      path += "/part" + std::to_string(i);
    }
    path += "/end";
    const std::string url = "http://test.me" + path + "?next=/other/path";
    CS::MatchUrls parsed;
    CS::ChannelContainer::match_parse_refer(String::SubString(url), {80, 443}, false,
      parsed, nullptr);
    require(parsed.size() == 3, "parser expanded URL path before matching");
    auto lookup = [&]()
    {
      CS::MatchWords words[CS::CT_MAX];
      CS::TriggerMatchRes result;
      container.match(parsed, {}, words, {}, {}, Generics::Uuid(), CS::MF_ACTIVE, result);
      std::set<unsigned long> ids;
      for (const auto& item : result)
      {
        ids.insert(item.first);
      }
      return ids;
    };
    require(lookup().empty(), "empty URL index");
    update(container, {{1, {"\"" + url + "\""}}, {2, {"www.test.me" + path}}, {3, {"me"}}});
    require(lookup() == std::set<unsigned long>{1, 2, 3},
      "deep URL, query slashes or reused request lost a match");
    require(match(container, url + "x") == std::set<unsigned long>{2, 3},
      "exact URL query mismatch");
    CS::ChannelContainer::match_parse_refer(String::SubString("http://other.me/live/path"),
      {80, 443}, false, parsed, nullptr);
    update(container, {{1, {"\"" + url + "\""}}, {4, {"other.me/live/path"}}});
    require(lookup() == std::set<unsigned long>{1, 4}, "independent URLs in one request");
    update(container, {{4, {"other.me/live/path"}}});
    require(lookup() == std::set<unsigned long>{4}, "pruned URL hid the next URL");

    std::string deep_path;
    for (unsigned int i = 0; i < 16; ++i)
    {
      deep_path += "/part" + std::to_string(i);
    }
    update(container, {{1, {"test.me/part0/part1/"}}, {2, {"test.me" + deep_path + "/end"}}});
    require(match(container, "http://test.me" + deep_path + "/end") ==
      std::set<unsigned long>{1}, "URL path depth limit changed");

    update(container, {{1, {"test.me/a/"}}, {2, {"www.test.me/a/b/c/end"}}});
    require(match(container, "http://test.me/a/b/c/end") == std::set<unsigned long>{1, 2},
      "terminal-only host pruned the www branch");
    update(container, {{1, {"\"http://test.me/a/b\""}},
      {2, {"\"http://www.test.me/a/b\""}}});
    require(match(container, "http://test.me/a/b") == std::set<unsigned long>{1},
      "synthetic www branch matched an exact URL");
    require(match(container, "http://www.test.me/a/b") == std::set<unsigned long>{2},
      "stripped www branch matched an exact URL");
    update(container, {{1, {"test.me/a//b/end"}}});
    require(match(container, "http://test.me/a//b/end") == std::set<unsigned long>{1},
      "empty path component");

    CS::ChannelContainer nonstrict(128, true);
    update(nonstrict, {{1, {"first.me/one/end"}}, {2, {"second.me/two/end"}}});
    CS::MatchUrls soft_urls;
    for (const char* source : {"http://first.me/one/end", "http://second.me/two/end"})
    {
      CS::ChannelContainer::match_parse_refer(String::SubString(source), {80, 443}, true,
        soft_urls, nullptr);
    }
    CS::MatchWords words[CS::CT_MAX];
    CS::TriggerMatchRes soft_result;
    nonstrict.match(soft_urls, {}, words, {}, {}, Generics::Uuid(),
      CS::MF_ACTIVE | CS::MF_NONSTRICTURL, soft_result);
    require(soft_result.count(1) && soft_result.count(2), "multiple nonstrict URLs lost a match");
  }

  void container_test()
  {
    CS::ChannelContainer container(128);
    const std::string exact = "\"http://test.me/test1/test2/" + std::string(130, 'x') + "\"";
    Channels channels{{1, {exact}}, {2, {"test.me/test1/test2/other"}},
      {3, {"me"}}, {4, {"www.test.me/test1/test2/alias"}}};
    update(container, channels);
    require(match(container, "http://test.me/test1/test2/") == std::set<unsigned long>{3},
      "exact shorter URL");
    require(match(container, exact.substr(1, exact.size() - 2)) == std::set<unsigned long>{1, 3},
      "exact full URL");
    require(match(container, "http://test.me/test1/test2/other") == std::set<unsigned long>{2, 3},
      "shared prefix");
    require(match(container, "http://test.me/test1/test2/alias") == std::set<unsigned long>{3, 4},
      "www branch");
    channels.erase(1);
    update(container, channels);
    require(match(container, "http://test.me/test1/test2/other") == std::set<unsigned long>{2, 3},
      "remove one trigger sharing key");
    channels[2] = {"test.me/new/path/end"};
    update(container, channels);
    require(match(container, "http://test.me/test1/test2/other") == std::set<unsigned long>{3},
      "remove last trigger of key");
    require(match(container, "http://test.me/new/path/end") == std::set<unsigned long>{2, 3},
      "new branch");
    update(container, {});
    require(match(container, "http://test.me/new/path/end").empty(), "remove all");
    update(container, {{1, {exact}}});
    require(match(container, exact.substr(1, exact.size() - 2)) == std::set<unsigned long>{1},
      "reinsert after empty");

    update(container, {{10, {"dense.me/"}}, {11, {"dense.me/a/"}},
      {12, {"dense.me/a/b/"}}, {13, {"dense.me/a/b/c/"}}});
    require(match(container, "http://dense.me/a/b/c/end") ==
      std::set<unsigned long>{10, 11, 12, 13}, "dense path");

    const std::string long_prefix = "http://test.me/" + std::string(130, 'x');
    update(container, {{1, {long_prefix + "/end"}}});
    require(match(container, long_prefix + "/end") == std::set<unsigned long>{1},
      "XXH3 prefix lookup");
    require(match(container, "http://test.me/other/end").empty(), "different long branch");
    update(container, {});
    require(match(container, long_prefix + "/end").empty(), "XXH3 prefix removal");

    CS::ChannelContainer nonstrict(128, true);
    update(nonstrict, {{1, {"test.me/a/b/end"}}});
    require(match(nonstrict, "http://test.me/a/b/end", CS::MF_NONSTRICTURL) ==
      std::set<unsigned long>{1}, "non-strict URL lookup");
    update(nonstrict, {});
    require(match(nonstrict, "http://test.me/a/b/end", CS::MF_NONSTRICTURL).empty(),
      "non-strict URL removal");

    update(container, {{1, {"test.me/a/"}}});
    const std::string concurrent_url = "http://test.me/a/b/c/end";
    // Move a matching key between chunks while the same query keeps matching channel 1.
    require(Generics::FastHash::hash("test.me/a", 9) % 128 !=
      Generics::FastHash::hash("test.me/a/b/c", 13) % 128, "distinct update chunks");
    std::atomic<bool> stop{false};
    std::atomic<bool> failed{false};
    std::atomic<unsigned long> reads{0};
    std::thread reader([&]
    {
      while (!stop.load())
      {
        if (match(container, concurrent_url) != std::set<unsigned long>{1})
        {
          failed = true;
        }
        ++reads;
      }
    });
    while (reads.load() == 0)
    {
      std::this_thread::yield();
    }
    for (unsigned int i = 0; i < 20; ++i)
    {
      Channels next{{1, {i % 2 ? "test.me/a/" : "test.me/a/b/c/"}}};
      if (i % 2)
      {
        next[2] = {"unrelated.me/a/b/c"};
      }
      update(container, next);
    }
    stop = true;
    reader.join();
    require(!failed, "inconsistent published snapshot");
  }
}

int main()
{
  try
  {
    CS::SoftTriggerWord empty;
    require(empty.channel_trigger_id == 0, "uninitialized trigger ID");
    CS::SoftTriggerWord moved(std::move(empty));
    require(moved.channel_trigger_id == 0, "moving an empty trigger changed its ID");
    moved.channel_trigger_id = 17;
    moved.trigger = "alpha";
    empty.swap(moved);
    require(empty.channel_trigger_id == 17 && empty.trigger == "alpha",
      "trigger transfer lost content");
    index_test();
    container_test();
    url_branch_test();
    keyword_update_test();
    keyword_snapshot_test();
    lexeme_hooks_test();
    keyword_prefix_test();
    hard_keyword_test();
    raw_keyword_test();
    std::cout << "URL prefix index tests passed\n";
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
