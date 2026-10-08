#include <Commons/Constants.hpp>
#include <ChannelSvcs/ChannelCommons/Serialization.hpp>
#include "SoftMatcher.hpp"

namespace AdServer::ChannelSvcs
{
  std::ostream& SoftMatcher::print(std::ostream& out) const /*throw(eh::Exception)*/
  {
    out  << lang_ << " : '" << trigger_;
    out << "' : ";
    size_t i;
    for (i = 0; i < simple_words_.size(); i++)
    {
      if (i)
      {
        out << ", ";
      }
      out << "'" << simple_words_[i].text() << "'";
    }

    if (main_lexem_)
    {
      if (i)
      {
        out << ", ";
      }
      out << "'" << main_lexem_->forms[0].text().str() << "'";
    }
    for (size_t j = 0; j < words_.size(); j++)
    {
      if (i)
      {
        out << ", ";
      }
      out << "'" << words_[j]->forms[0].text().str() << "'";
    }
    out << std::endl;
    return out;
  }

  bool SoftMatcher::match_exact(const MatchWordVector& words) const
    /*throw(eh::Exception)*/
  {
    // for exact triggers arrays words_ and simple_words_ are equal length
    // iterate by both of them and use one with data
    if (!Serialization::exact(trigger_.c_str()) || words.size() != simple_words_.size())
    {
      return false;
    }
    MatchWordVector::const_iterator word_it = words.begin();
    LexemesPtrVector::const_iterator it = words_.begin();
    SubHashVector::const_iterator simple_it = simple_words_.begin();
    for (; word_it != words.end(); ++it, ++word_it, ++simple_it)
    {
      bool match = false;
      if (it < words_.end() && *it)
      {
        for (LexemeData::Forms::const_iterator i = (*it)->forms.begin();
            i != (*it)->forms.end(); ++i)
        {
          const auto form = i->text();
          if (std::string_view(form.data(), form.size()) == word_it->text())
          {
            match = true;
            break;
          }
        }
      }
      else
      {
        const auto word = simple_it->text();
        match = (std::string_view(word.data(), word.size()) == word_it->text());
      }

      if (!match)
      {
        return false;
      }
    }
    return true;
  }

  bool SoftMatcher::match(const MatchWordSet& words, bool soft_match) const
    /*throw(eh::Exception)*/
  {
    if (soft_match)
    {
      return true;
    }

    if (Serialization::exact(trigger_.c_str()))
    {
      return false;
    }
    bool match = true;
    MatchWordSet::const_iterator m_it = words.end();
    for (SubHashVector::const_iterator it = simple_words_.begin(); it != simple_words_.end(); ++it)
    {
      m_it = words.find(Generics::StringViewHashAdapter(it->hash(), std::string_view(*it)));
      match = (m_it != words.end());
      if (!match)
      {
        return false;
      }
    }

    for (LexemesPtrVector::const_iterator it = words_.begin(); it != words_.end(); ++it)
    {
      match = false;
      if (*it)
      {
        for (LexemeData::Forms::const_iterator i = (*it)->forms.begin();
          i != (*it)->forms.end(); ++i)
        {
          m_it = words.find(Generics::StringViewHashAdapter(i->hash(), std::string_view(*i)));
          if (m_it != words.end())
          {
            match = true;
            break;
          }
        }
      }

      if (!match)
      {
        return false;
      }
    }
    return match;
  }

  bool SoftMatcher::find_token(const String::SubString& ftoken, String::SubString& token) noexcept
  {
    if (main_lexem_.in())
    {
      for (auto it = main_lexem_->forms.begin(); it != main_lexem_->forms.end(); ++it)
      {
        if (*it == ftoken)
        {
          token = *it;
          return true;
        }
      }
      return false;
    }
    SubStringVector substrings;
    Serialization::get_parts(trigger_, substrings);
    for (auto it = substrings.begin(); it != substrings.end(); ++it)
    {
      if (*it == ftoken)
      {
        token = *it;
        return true;
      }
    }
    return false;
  }

  SubStringVector&
  SoftMatcher::get_tokens(SubStringVector& container)
    /*throw(eh::Exception)*/
  {
    SubStringVector substrings;
    Serialization::get_parts(trigger_, substrings);
    container.reserve(Commons::DEFAULT_MAX_HARD_WORD_SEQ * substrings.size());
    String::SubString token;
    for (auto it = substrings.begin(); it != substrings.end(); ++it)
    {
      String::StringManip::CharSplitter tokenizer(*it, TRIGGER_WORD_SEPARATORS);
      while (tokenizer.get_token(token))
      {
        container.push_back(token);
      }
    }
    return container;
  }


  void SoftMatcher::get_keyword_parts(std::vector<std::string_view>& parts) const
  {
    SubStringVector serialized_parts;
    Serialization::get_parts(trigger_, serialized_parts);
    for (const auto& part : serialized_parts)
    {
      parts.emplace_back(part.data(), part.size());
    }
    const auto append_forms = [&](const Lexeme_var& lexeme)
    {
      if (lexeme)
      {
        for (const auto& form : lexeme->forms)
        {
          parts.emplace_back(form.text().data(), form.text().size());
        }
      }
    };
    append_forms(main_lexem_);
    for (const auto& lexeme : words_)
    {
      append_forms(lexeme);
    }
    std::sort(parts.begin(), parts.end());
    parts.erase(std::unique(parts.begin(), parts.end()), parts.end());
  }

  SoftMatcher::SoftMatcher(unsigned short lang, std::string& trigger)
    noexcept
    : lang_(lang)
  {
    trigger_.swap(trigger);
  }

  void SoftMatcher::configure_matching_info(
    const SubStringVector& parts,
    LexemesPtrVector& lexemes,
    unsigned int word_num)
    noexcept
  {
    char type = trigger_type();
    if (type == 'D')
    {
      // there isn't any additional information for uid triggers
      return;
    }

    if (type == 'U')
    {
      simple_words_.insert(simple_words_.end(), parts.begin(), parts.end());
      return;
    }

    if (Serialization::exact(trigger_.c_str()))
    {
      simple_words_.insert(simple_words_.end(), parts.begin(), parts.end());
      words_.swap(lexemes);
      if (words_.size() > word_num)
      {
        main_lexem_ = words_[word_num];
      }
      return;
    }

    if (!lexemes.empty() && lexemes[word_num])
    {
      main_lexem_.swap(lexemes[word_num]);
    }
    simple_words_.reserve(parts.size());
    for (size_t i = 0; i < parts.size(); ++i)
    {
      if (i != word_num)
      {
        if (!lexemes.empty() && lexemes[i])
        {
          words_.push_back(lexemes[i]);
        }
        else
        {
          simple_words_.push_back(parts[i]);
        }
      }
    }
  }
}
