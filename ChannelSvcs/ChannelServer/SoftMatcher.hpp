#ifndef AD_SERVER_SOFT_MATCHERS_HPP_
#define AD_SERVER_SOFT_MATCHERS_HPP_

#include <iostream>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <String/StringManip.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Commons/Constants.hpp>
#include <ChannelSvcs/ChannelCommons/CommonTypes.hpp>
#include <ChannelSvcs/ChannelCommons/Serialization.hpp>

namespace AdServer
{
namespace ChannelSvcs
{
  namespace
  {
    const String::AsciiStringManip::CharCategory TRIGGER_WORD_SEPARATORS(" \"[]");
  }

  /* SoftMatcher is class for matching any triggers.*/
  class SoftMatcher: public ReferenceCounting::AtomicImpl
  {
  public:
    typedef std::vector<Generics::SubStringHashAdapter> SubHashVector;
    struct InternalWord
    {
      String::SubString word;
      Lexeme_var lexeme;
    };

    typedef std::vector<InternalWord> WordsVector;

    SoftMatcher(
      unsigned short lang,
      std::string& trigger)
      noexcept;

    bool match(const MatchWords& words, bool soft_match) const
        /*throw(eh::Exception)*/;

    bool match_exact(const StringVector& words) const
        /*throw(eh::Exception)*/;

    char trigger_type() const noexcept;

    bool exact() const noexcept;

    bool negative() const noexcept;

    const std::string& get_trigger() const noexcept;

    unsigned short get_lang() const noexcept;

    size_t memory_size() const noexcept;

    void configure_matching_info(
      const SubStringVector& parts,
      LexemesPtrVector& lexemes,
      unsigned int word_num)
      noexcept;

    std::ostream& print(std::ostream& out) const /*throw(eh::Exception)*/;

    /* try to find matched token with same value  in matcher */
    bool find_token(const String::SubString& ftoken, String::SubString& token)
      noexcept;

    /*collect tokens for soft matching */
    SubStringVector&
    get_tokens(SubStringVector& container)
      /*throw(eh::Exception)*/;

    const Generics::SubStringHashAdapter&
    get_url_postfix() const noexcept;

    const Generics::SubStringHashAdapter&
    get_url_prefix() const noexcept;

    const Lexeme_var& matched_lexeme() const noexcept;

  protected:
    virtual
    ~SoftMatcher() noexcept
    {}

  private:
    std::string trigger_;
    SubHashVector simple_words_;//holds words without lexems for matching
    LexemesPtrVector words_;//holds lexemes for matching
    Lexeme_var main_lexem_;//holds unmatched lexem
    //WordsVector words_;
    unsigned short lang_;
  };

  typedef SoftMatcher SoftMatcherType;

  typedef ReferenceCounting::SmartPtr<
    SoftMatcherType,
    ReferenceCounting::PolicyAssert> SoftMatcher_var;

  typedef std::set<SoftMatcher_var> MatcherVarsSet;

}// namespace ChannelSvcs
}// namespace AdServer

namespace AdServer
{
namespace ChannelSvcs
{
  inline
  char SoftMatcher::trigger_type() const
    noexcept
  {
    return Serialization::trigger_type(trigger_.data());
  }

  inline
  bool SoftMatcher::exact() const noexcept
  {
    return Serialization::exact(trigger_.data());
  }

  inline
  bool SoftMatcher::negative() const noexcept
  {
    return Serialization::negative(trigger_.data());
  }

  inline
  unsigned short SoftMatcher::get_lang() const noexcept
  {
    return lang_;
  }

  inline
  const std::string& SoftMatcher::get_trigger() const
    noexcept
  {
    return trigger_;
  }

  inline
  const Generics::SubStringHashAdapter&
  SoftMatcher::get_url_prefix() const noexcept
  {//for url prefix holds in first word, postfix in second
    return simple_words_[0];
  }

  inline
  const Generics::SubStringHashAdapter&
  SoftMatcher::get_url_postfix() const noexcept
  {//for url prefix holds in first word, postfix in second
    return simple_words_[1];
  }

  inline
  size_t SoftMatcher::memory_size() const noexcept
  {
    return sizeof(SoftMatcher) + trigger_.capacity() +
      simple_words_.capacity() * sizeof(String::SubString) +
      (words_.capacity() + 1) * sizeof(Lexeme_var);
  }

  inline
  const Lexeme_var& SoftMatcher::matched_lexeme() const noexcept
  {
    return main_lexem_;
  }

}// namespace ChannelSvcs
}// namespace AdServer

#endif //AD_SERVER_SOFT_MATCHERS_HPP_

