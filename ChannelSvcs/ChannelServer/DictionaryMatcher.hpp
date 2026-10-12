#pragma once

#include <boost/unordered/unordered_flat_map.hpp>
#include <Generics/HashTableAdapters.hpp>
#include <Logger/Logger.hpp>
#include <ChannelSvcs/ChannelCommons/Lexeme.hpp>
#include <ChannelSvcs/ChannelCommons/Serialization.hpp>
#include <ChannelSvcs/DictionaryProviderClient/DictionaryProviderClient.hpp>

namespace AdServer::ChannelSvcs
{
  class DictionaryMatcher
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    struct StringHashAdapterHash
    {
      size_t operator()(const Generics::StringHashAdapter& key) const noexcept;
    };

    using LexemeCache = boost::unordered_flat_map<
      Generics::StringHashAdapter, Lexeme_var, StringHashAdapterHash>;

    DictionaryMatcher(const std::vector<std::string>& endpoints, Logging::Logger* logger);
    virtual ~DictionaryMatcher() noexcept;

    void get_lexemes(const char* lang, LexemeCache& lexemes);
    virtual bool ready() const noexcept;
    static bool is_lexemized(const char* trigger) noexcept;

  private:
    void trace_result_(const char* lang, const LexemeCache& cache) noexcept;

    static const char* ASPECT;
    DictionaryProviderClient dictionary_client_;
    Logging::Logger_var logger_;
  };

  inline size_t
  DictionaryMatcher::StringHashAdapterHash::operator()(const Generics::StringHashAdapter& key)
    const noexcept
  {
    return key.hash();
  }

  inline bool DictionaryMatcher::is_lexemized(const char* trigger) noexcept
  {
    const char type = Serialization::trigger_type(trigger);
    return type != 'U' && type != 'D';
  }
}
