#pragma once

#include <boost/unordered/unordered_flat_map.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ObjectPool.hpp>
#include <Generics/HashTableAdapters.hpp>
#include <ChannelSvcs/DictionaryProvider/DictionaryProvider.hpp>

namespace AdServer::ChannelSvcs
{
  class DictionaryMatcher
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    using DictionaryProviderPoolConfig = CORBACommons::ObjectPoolRefConfiguration;

    using DictionaryProviderPool = CORBACommons::ObjectPool<
      AdServer::ChannelSvcs::DictionaryProvider, DictionaryProviderPoolConfig>;
    using DictionaryProviderPoolPtr = std::unique_ptr<DictionaryProviderPool>;

    struct StringHashAdapterHash
    {
      size_t
      operator()(const Generics::StringHashAdapter& key) const noexcept
      {
        return key.hash();
      }
    };

    using LexemeCache = boost::unordered_flat_map<
      Generics::StringHashAdapter, Lexeme_var, StringHashAdapterHash>;

  public:
    DictionaryMatcher(
      const CORBACommons::CorbaClientAdapter& adapter,
      CORBACommons::CorbaObjectRefList& dictionary_server_refs,
      Logging::Logger* logger)
      /*throw(Exception)*/;

    virtual ~DictionaryMatcher() noexcept {};

    void get_lexemes(const char* lang, LexemeCache& lexemes)
      /*throw(Exception)*/;

    virtual bool ready() const noexcept;

    static bool
    is_lexemized(const char* trigger) noexcept;

  private:
    AdServer::ChannelSvcs::DictionaryProvider::LexemeSeq*
    query_dictionary_words_(const char* lang, const CORBACommons::StringSeq& seq_words) const
      /*throw(Exception)*/;

    void trace_result_(const char* lang, const LexemeCache& cache)
      noexcept;

  private:
    static const char* ASPECT;
    const CORBACommons::CorbaClientAdapter& c_adapter_;
    DictionaryProviderPoolPtr dictionary_pool_;
    Logging::Logger* logger_;
  };
}

namespace AdServer::ChannelSvcs
{
  inline
  bool DictionaryMatcher::is_lexemized(const char* trigger) noexcept
  {
    char trigger_type = Serialization::trigger_type(trigger);
    return (trigger_type != 'U' && trigger_type != 'D');
  }
}
