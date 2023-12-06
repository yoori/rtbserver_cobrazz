#include <cstring>
#include <istream>
#include <String/Tokenizer.hpp>

#include "DictionaryContainer.hpp"

namespace AdServer
{

  namespace ChannelSvcs
  {

    const char Dictionary::LEXEME_SEPARATOR = '\n';
    const char Dictionary::FORM_SEPARATOR = '\0';

    Dictionary::Dictionary() /*throw(eh::Exception)*/
      : lexems_count_(0)
    {
      buffer_.reserve(1024 * 1024);
      buffer_.push_back(LEXEME_SEPARATOR);
    }

    void
    Dictionary::add_lexeme(const Forms& forms) /*throw(eh::Exception)*/
    {
      const size_t prev_size = buffer_.size();

      for (Forms::const_iterator ci = forms.begin();
          ci != forms.end(); ++ci)
      {
        if (!ci->empty())
        {
          buffer_.insert(buffer_.end(), ci->data(), ci->data() + ci->size());
          buffer_.push_back(FORM_SEPARATOR);
        }
      }

      if (prev_size != buffer_.size())
      {
        buffer_.push_back(LEXEME_SEPARATOR);
        ++lexems_count_;
      }
    }

    bool
    index_compare(char const* const a, char const* const b) noexcept
    {
      return ::strcmp(a, b) < 0;
    }

    void
    Dictionary::get_forms(const char* word, Forms& forms) const /*throw(eh::Exception)*/
    {
      Indexes::const_iterator ci = std::lower_bound(word_indexes_.begin(),
          word_indexes_.end(), word, index_compare);

      while (ci != word_indexes_.end() && ::strcmp(word, *ci) == 0)
      {
        const char* begin = *ci;
        while (*begin != '\n')
          --begin;

        make_lexeme_(begin + 1, forms);
        ++ci;
      }
    }

    void
    Dictionary::get_statistic(std::ostream& ostr) const /*throw(eh::Exception)*/
    {
      ostr << "Lexems: " << get_lexems_count() << ", words: "
          << get_words_count() << ", buffer size: " << get_buffer_size();
    }

    void
    Dictionary::append(const Dictionary& dict) /*throw(eh::Exception)*/
    {
      buffer_.insert(buffer_.end(), dict.buffer_.begin() + 1,
          dict.buffer_.end());
      lexems_count_ += dict.lexems_count_;
    }

    void
    Dictionary::clear() noexcept
    {
      buffer_.clear();
      word_indexes_.clear();
      lexems_count_ = 0;
    }

    void
    Dictionary::finalize() /*throw(eh::Exception)*/
    {
      Buffer(buffer_).swap(buffer_); // reduce memory usage
      build_index_();
      Indexes(word_indexes_).swap(word_indexes_); // reduce memory usage
    }

    size_t
    Dictionary::get_words_count() const noexcept
    {
      return word_indexes_.size();
    }

    size_t
    Dictionary::get_lexems_count() const noexcept
    {
      return lexems_count_;
    }

    size_t
    Dictionary::get_buffer_size() const noexcept
    {
      return buffer_.size();
    }

    void
    Dictionary::make_lexeme_(const char* begin, Forms& forms) const /*throw(eh::Exception)*/
    {
      const char* word_begin = begin;
      const char* word_end = begin;

      while (*word_end != '\n')
      {
        if (*word_end == '\0')
        {
          forms.insert(
              String::SubString(word_begin, word_end - word_begin));
          word_begin = word_end + 1;
        }
        ++word_end;
      }
    }

    void
    Dictionary::build_index_() /*throw(eh::Exception)*/
    {
      word_indexes_.clear();
      const char* index = &buffer_[0];

      for (size_t i = 0; i < buffer_.size(); ++i)
      {
        if (buffer_[i] == FORM_SEPARATOR)
        {
          word_indexes_.push_back(index);
          index = &buffer_[i + 1];
        }
        else if (buffer_[i] == LEXEME_SEPARATOR)
        {
          index = &buffer_[i + 1];
        }
      }

      std::sort(word_indexes_.begin(), word_indexes_.end(), index_compare);
    }

    void
    DictionaryContainer::add_dictionary_(const char* lang) /*throw(eh::Exception)*/
    {
      WriteGuard_ guard(lock_);
      if (dictionaries_.find(lang) == dictionaries_.end())
      {
        Dictionary_var dict(new Dictionary());
        dictionaries_.insert(Dictionaries::value_type(lang, dict));
      }
    }

    void
    DictionaryContainer::load_dictionary(const char* lang,
        std::istream& is) /*throw(eh::Exception)*/
    {
      add_dictionary_(lang);

      std::string line;

      const size_t LOAD_PACKAGE_SIZE = 1024 * 1024;
      Dictionary_var package_for_load(new Dictionary);

      while (is.good())
      {
        std::getline(is, line);
        if (is.fail())
        {
          break;
        }

        typedef const String::AsciiStringManip::Char2Category<' ', '\t'>
          LexSeparators;
        String::StringManip::Splitter<LexSeparators> splitter(line);
        std::set<String::SubString> tokens;
        String::SubString token;

        while (splitter.get_token(token))
        {
          tokens.insert(token);
        }

        package_for_load->add_lexeme(tokens);

        if (package_for_load->get_buffer_size() > LOAD_PACKAGE_SIZE)
        {
          WriteGuard_ guard(lock_);
          dictionaries_[lang]->append(*package_for_load);
          package_for_load->clear();
        }
      }

      {
        WriteGuard_ guard(lock_);
        dictionaries_[lang]->append(*package_for_load);
        dictionaries_[lang]->finalize();
      }
    }

    void
    DictionaryContainer::find_lexemes(const char* lang,
        LexemeItemVector& res) const /*throw(eh::Exception)*/
    {
      if (res.empty())
      {
        return;
      }
      //Don't lock it, because data shouldn't change in time of executing this method
      std::string lang_name(lang);
      if (lang_name.empty())
      {
        for (Dictionaries::const_iterator dict_it = dictionaries_.begin();
            dict_it != dictionaries_.end(); ++dict_it)
        {
          const Dictionary& dict = *(dict_it->second);
          fill_lexemes_(dict, res);
        }
      }
      else
      {
        const Dictionaries::const_iterator dict_it = dictionaries_.find(
            lang_name);
        if (dict_it == dictionaries_.end())
        {
          return;
        }
        const Dictionary& dict = *(dict_it->second);
        fill_lexemes_(dict, res);
      }
    }

    void
    DictionaryContainer::get_statistic(std::ostream& ostr) const /*throw(eh::Exception)*/
    {
      WriteGuard_ guard(lock_);
      size_t length = 0, lexems_counts = 0, words_count = 0;

      for (Dictionaries::const_iterator it = dictionaries_.begin();
          it != dictionaries_.end(); ++it)
      {
        const Dictionary& dict = *(it->second);

        ostr << it->first << ": " << dict.get_words_count() << " words, "
            << dict.get_lexems_count() << " lexemes, " << dict.get_buffer_size()
            << " length; ";

        length += dict.get_buffer_size();
        lexems_counts += dict.get_lexems_count();
        words_count += dict.get_words_count();
      }

      ostr << "ALL : " << words_count << " words, " << lexems_counts
          << " lexemes, " << length << " length.";
    }

    void
    DictionaryContainer::fill_lexemes_(const Dictionary& dict,
        LexemeItemVector& res) const /*throw(eh::Exception)*/
    {
      for (LexemeItemVector::iterator it = res.begin(); it != res.end(); ++it)
      {
        dict.get_forms(it->word, it->forms);
      }
    }

  }

}
