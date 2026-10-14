#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <ChannelSvcs/ChannelServer/ChannelContainer.hpp>
#include <ChannelSvcs/ChannelServer/UpdateContainer.hpp>

namespace
{
  namespace CS = AdServer::ChannelSvcs;

  std::string anchor(const std::vector<std::string>& words, char type, bool exact, bool negative)
  {
    CS::ChannelContainer container(1, false);
    CS::ChannelIdToMatchInfo_var info = new CS::ChannelIdToMatchInfo;
    auto& metadata = (*info)[1];
    metadata.channel = CS::Channel(1);
    metadata.channel.mark_type(CS::Channel::CT_ACTIVE);
    metadata.channel.mark_type(type == 'P' ? CS::CT_PAGE :
      type == 'R' ? CS::CT_URL_KEYWORDS : CS::CT_SEARCH);
    metadata.stamp = Generics::Time::get_time_of_day();
    CS::UpdateContainer changes(&container, nullptr);
    changes.check_actual(*info);
    CS::TriggerList empty;
    changes.select_parsed_triggers(1, empty, false);
    CS::Parts parts;
    for (const auto& word : words)
    {
      parts.push_back({String::SubString(word), word.find(' ') != std::string::npos});
    }
    CS::MergeAtom atom;
    atom.id = 1;
    atom.soft_words.emplace_back();
    auto& trigger = atom.soft_words.back();
    trigger.channel_trigger_id = 1;
    CS::Serialization::serialize(parts, type, exact, negative, trigger.trigger);
    const auto serialized = trigger.trigger;
    changes.add_trigger(atom);
    container.merge(changes, *info, true);
    String::SubString selected;
    const auto* matcher = container.get_matcher(serialized, 0, selected);
    if (!matcher || matcher->exact() != exact || matcher->negative() != negative)
    {
      throw std::runtime_error("matcher or flags missing");
    }

    if (!negative)
    {
      // Exercise the complete index lookup as well as the selected anchor.
      for (int omitted = -1; omitted < static_cast<int>(words.size()); ++omitted)
      {
        std::string text;
        for (std::size_t i = 0; i < words.size(); ++i)
        {
          if (static_cast<int>(i) != omitted)
          {
            if (!text.empty())
            {
              text += ' ';
            }
            text += words[i];
          }
        }
        CS::KeywordInput inputs[CS::CT_MAX];
        const auto match_type = type == 'P' ? CS::CT_PAGE :
          type == 'R' ? CS::CT_URL_KEYWORDS : CS::CT_SEARCH;
        inputs[match_type] = {text};
        CS::TriggerMatchRes result;
        container.match({}, {}, inputs, {}, Generics::Uuid(), CS::MF_ACTIVE, result);
        if (result.count(1) != static_cast<std::size_t>(omitted == -1))
        {
          throw std::runtime_error("conjunction changed for input '" + text + "'");
        }
      }
    }
    return std::string(selected.data(), selected.size());
  }

  void check(const std::vector<std::string>& words, const char* expected,
    char type = 'P', bool exact = false, bool negative = false)
  {
    const auto actual = anchor(words, type, exact, negative);
    if (actual != expected)
    {
      throw std::runtime_error(
        "expected anchor '" + std::string(expected) + "', got '" + actual + "'");
    }
  }
}

int main()
{
  try
  {
    for (const char type : {'P', 'R', 'S'})
    {
      check({"a", "alphabet"}, "alphabet", type);
      check({"video", "музыка", "метал"}, "музыка", type);
      check({"длинный", "abcdefgh"}, "abcdefgh", type);
      check({"aaa", "bbb"}, "aaa", type);
      check({"rtbiab9x30"}, "rtbiab9x30", type);
      check({"a", "alphabet"}, "alphabet", type, false, true);
      check({"word", "a longer phrase"}, "a longer phrase", type);
    }
    check({"a", "alphabet"}, "a", 'S', true);
    std::cout << "LongestAnchorTest: OK (P/R/S, UTF-8, ties, exact, negative, phrases)\n";
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
