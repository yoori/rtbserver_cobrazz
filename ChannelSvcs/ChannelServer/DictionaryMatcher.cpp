#include "DictionaryMatcher.hpp"

namespace AdServer::ChannelSvcs
{
  const char* DictionaryMatcher::ASPECT = "DictionaryMatcher";

  DictionaryMatcher::DictionaryMatcher(
    const std::vector<std::string>& endpoints, Logging::Logger* logger)
    : dictionary_client_(endpoints), logger_(ReferenceCounting::add_ref(logger))
  {
  }

  DictionaryMatcher::~DictionaryMatcher() noexcept = default;

  void DictionaryMatcher::get_lexemes(const char* lang, LexemeCache& lexemes)
  {
    if (dictionary_client_.empty())
    {
      return;
    }

    try
    {
      std::vector<std::string> words;
      words.reserve(lexemes.size());
      for (const auto& entry : lexemes)
      {
        words.emplace_back(entry.first.text());
      }

      const auto response = dictionary_client_.get_lexemes(lang, words);
      for (std::size_t i = 0; i < words.size(); ++i)
      {
        const auto& lexeme = response.lexemes(i);
        if (lexeme.forms_size() == 0)
        {
          continue;
        }

        Lexeme_var data = new Lexeme;
        std::size_t total_size = 0;
        for (const auto& form : lexeme.forms())
        {
          total_size += form.size();
        }

        data->data.reserve(total_size);
        data->forms.reserve(lexeme.forms_size());
        for (const auto& form : lexeme.forms())
        {
          const auto offset = data->data.size();
          data->data.append(form);
          data->forms.emplace_back(String::SubString(data->data.data() + offset, form.size()));
        }

        lexemes[String::SubString(words[i])] = std::move(data);
      }

      trace_result_(lang, lexemes);
    }
    catch (const eh::Exception& error)
    {
      throw Exception(error.what());
    }
  }

  void DictionaryMatcher::trace_result_(const char* lang, const LexemeCache& cache) noexcept
  {
    if (logger_->log_level() >= Logging::Logger::TRACE)
    {
      for (const auto& entry : cache)
      {
        Stream::Error trace;
        trace << lang << ": '" << entry.first << "' :";
        if (entry.second)
        {
          for (const auto& form : entry.second->forms)
          {
            trace << " '" << form.text() << "'";
          }
        }

        logger_->log(trace.str(), Logging::Logger::TRACE, ASPECT);
      }
    }
  }

  bool DictionaryMatcher::ready() const noexcept
  {
    if (dictionary_client_.empty())
    {
      return true;
    }

    try
    {
      dictionary_client_.get_lexemes("", {});
      return true;
    }
    catch (const std::exception&)
    {
      return false;
    }
  }
}
