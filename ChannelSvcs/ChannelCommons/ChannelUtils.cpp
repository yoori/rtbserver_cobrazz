#include<string>
#include<eh/Exception.hpp>
#include<String/UTF8Case.hpp>
#include<Commons/Constants.hpp>
#include"CommonTypes.hpp"
#include"ChannelUtils.hpp"
#include"Serialization.hpp"

namespace AdServer::ChannelSvcs
{
  void MergeAtom::print(std::ostream& ostr) const /*throw(eh::Exception)*/
  {
    ostr << "{id = " << id << "; lang = " << lang << ";";
    for (SoftTriggerList::const_iterator its(soft_words.begin()); its != soft_words.end(); ++its)
    {
      std::string trigger;
      ostr << " words = ";
      ostr << "( " << its->channel_trigger_id;
      ostr << " '" << Serialization::get_trigger(its->trigger, trigger)
        << "' => ";
      ostr << ");";
    }
    ostr << ";}";
  }

  void SoftTriggerWord::swap(SoftTriggerWord& word) noexcept
  {
    std::swap(channel_trigger_id, word.channel_trigger_id);
    trigger.swap(word.trigger);
  }

  namespace
  {
    template<bool UsePrefixes, typename WordSet>
    void build_combinations_(
      const std::vector<size_t>& positions,
      std::string_view data,
      WordSet& match_words,
      size_t max_seq_length,
      const KeywordPrefixIndex* prefixes) noexcept
    {
      auto pos = positions.front();
      for (auto it = positions.begin() + 1; it != positions.end(); ++it)
      {
        const std::string_view word(data.data() + pos, *it - pos - 1);
        const auto word_hash = Generics::FastHash::hash(word.data(), word.size());
        match_words.insert(Generics::StringViewHashAdapter(word_hash, word));

        auto end = positions.end();
        if constexpr (!UsePrefixes)
        {
          const auto remaining = static_cast<size_t>(end - it);
          if (max_seq_length < remaining)
          {
            end = it + max_seq_length;
          }
        }

        for (auto it2 = it + 1; it2 != end; ++it2)
        {
          const std::string_view phrase(data.data() + pos, *it2 - pos - 1);
          const auto hash = Generics::FastHash::hash(phrase.data(), phrase.size());
          if constexpr (UsePrefixes)
          {
            const auto counts = prefixes->find(hash);
            if (counts.terminal)
            {
              match_words.insert(Generics::StringViewHashAdapter(hash, phrase));
            }

            if (!counts.descendants)
            {
              break;
            }
          }
          else
          {
            match_words.insert(Generics::StringViewHashAdapter(hash, phrase));
          }
        }

        pos = *it;
      }
    }

    template<typename WordSet>
    void build_combination_(
      const std::vector<size_t>& positions, std::string_view data, WordSet& match_words,
      size_t max_seq_length, const KeywordPrefixIndex* prefixes) noexcept
    {
      if (positions.empty())
      {
        return;
      }

      if (prefixes)
      {
        build_combinations_<true>(positions, data, match_words, 0, prefixes);
      }
      else if (max_seq_length)
      {
        build_combinations_<false>(positions, data, match_words, max_seq_length, nullptr);
      }
    }

    std::string_view store_keyword(std::string_view text, Generics::MonoAllocatorArena& arena)
    {
      auto* memory = Generics::MonoAllocator<char>(arena).allocate(text.size());
      std::copy(text.begin(), text.end(), memory);
      return {memory, text.size()};
    }

    class WordCursor
    {
    public:
      explicit WordCursor(std::string_view text) noexcept : text_(text) {}

      std::string_view next(MatchWordVector*& exact_words) noexcept
      {
        while (true)
        {
          while (position_ < text_.size() && (text_[position_] == ' ' || text_[position_] == '\t'))
          {
            ++position_;
          }
          const auto start = position_;
          if (start == text_.size())
          {
            return {};
          }
          while (position_ < text_.size() && text_[position_] != ' ' && text_[position_] != '\t')
          {
            ++position_;
          }
          const auto end = position_;
          const auto word = text_.substr(start, end - start);
          if (word.size() <= TriggerParser::TriggerParser::MAX_TRIGGER_LENGTH)
          {
            return word;
          }

          if (exact_words)
          {
            exact_words->clear();
            exact_words = nullptr;
          }
        }
      }

    private:
      std::string_view text_;
      std::size_t position_ = 0;
    };

    template<bool UsePrefixes>
    void prepare_sequence(
      std::string_view text, PreparedMatchWords& output, const KeywordPrefixIndex* prefixes,
      std::size_t max_length, MatchWordVector*& exact_words, Generics::MonoString& assembled)
    {
      auto& arena = *output.words().get_allocator().arena();
      WordCursor cursor(text);
      while (true)
      {
        const auto word = cursor.next(exact_words);
        if (word.empty())
        {
          break;
        }
        const Generics::StringViewHashAdapter key(word);
        if (exact_words)
        {
          exact_words->push_back(key);
        }

        if constexpr (!UsePrefixes)
        {
          if (!max_length)
          {
            continue;
          }
        }
        output.insert(key);

        WordCursor tail = cursor;
        std::string_view phrase = word;
        bool contiguous = true;
        std::size_t length = 1;
        while (true)
        {
          if constexpr (!UsePrefixes)
          {
            if (length >= max_length)
            {
              break;
            }
            ++length;
          }
          MatchWordVector* ignored = nullptr;
          const auto next = tail.next(ignored);
          if (next.empty())
          {
            break;
          }

          if (contiguous && next.data() == phrase.data() + phrase.size() + 1 &&
            phrase.data()[phrase.size()] == ' ')
          {
            phrase = std::string_view(phrase.data(), next.data() + next.size() - phrase.data());
          }
          else
          {
            if (contiguous)
            {
              assembled.assign(phrase.data(), phrase.size());
              contiguous = false;
            }
            assembled.push_back(' ');
            assembled.append(next.data(), next.size());
            phrase = std::string_view(assembled.data(), assembled.size());
          }
          const auto hash = Generics::FastHash::hash(phrase.data(), phrase.size());
          if constexpr (UsePrefixes)
          {
            const auto counts = prefixes->find(hash);
            if (counts.terminal)
            {
              output.insert(Generics::StringViewHashAdapter(hash,
                contiguous ? phrase : store_keyword(phrase, arena)));
            }

            if (!counts.descendants)
            {
              break;
            }
          }
          else
          {
            output.insert(Generics::StringViewHashAdapter(hash,
              contiguous ? phrase : store_keyword(phrase, arena)));
          }
        }
      }
    }
  }

  void build_combination(
    const std::vector<size_t>& positions, MatchWords& match_words, size_t max_seq_length,
    const KeywordPrefixIndex* prefixes) noexcept
  {
    build_combination_(positions, match_words.data_holder_, match_words, max_seq_length, prefixes);
  }

  void prepare_keywords(
    const MatchWords& input, PreparedMatchWords& output, const KeywordPrefixIndex* prefixes)
  {
    const auto* active_prefixes = input.use_keyword_hooks ? prefixes : nullptr;
    auto capacity = output.size() + input.size();
    for (const auto& sequence : input.deferred_sequences)
    {
      if ((active_prefixes || sequence.max_length) && sequence.positions.size() > 1)
      {
        capacity += sequence.positions.size() - 1;
      }
    }
    output.reserve(capacity);
    output.insert(input.begin(), input.end());
    for (const auto& sequence : input.deferred_sequences)
    {
      build_combination_(
        sequence.positions, input.data_holder_, output, sequence.max_length, active_prefixes);
    }
  }

  void prepare_keywords(
    const KeywordInput& input, PreparedMatchWords& output, const KeywordPrefixIndex* prefixes,
    MatchWordVector* exact_words)
  {
    if (input.text.empty())
    {
      return;
    }

    if (!output.size())
    {
      output.reserve(8);
    }
    auto& arena = *output.words().get_allocator().arena();
    Generics::MonoString assembled(&arena);
    std::string normalized;
    const auto* active_prefixes = input.use_keyword_hooks ? prefixes : nullptr;
    std::size_t position = 0;
    while (position < input.text.size())
    {
      auto end = input.separators.empty() ? input.text.size() :
        input.separators.size() == 1 ? input.text.find(input.separators.front(), position) :
        input.text.find_first_of(input.separators, position);
      if (end == std::string_view::npos)
      {
        end = input.text.size();
      }
      auto sequence = input.text.substr(position, end - position);
      position = end == input.text.size() ? end : end + 1;
      if (sequence.empty())
      {
        continue;
      }

      if (input.mode == PM_SIMPLIFY)
      {
        try
        {
          Language::Trigger::normalize_phrase(
            String::SubString(sequence.data(), sequence.size()), normalized, input.segmentor);
        }
        catch (const eh::Exception&)
        {
          return;
        }

        if (normalized != sequence)
        {
          sequence = store_keyword(normalized, arena);
        }
      }

      if (active_prefixes)
      {
        prepare_sequence<true>(sequence, output, active_prefixes, 0, exact_words, assembled);
      }
      else
      {
        prepare_sequence<false>(
          sequence, output, nullptr, input.max_length, exact_words, assembled);
      }
    }
  }
}
