#ifndef CHANNEL_SVCS_DICTIONARY_MATCHER_HPP_
#define CHANNEL_SVCS_DICTIONARY_MATCHER_HPP_

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ObjectPool.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/HashTableAdapters.hpp>
//#include <ChannelSvcs/ChannelCommons/TriggerParser.hpp>
#include <ChannelSvcs/DictionaryProvider/DictionaryProvider.hpp>

namespace AdServer
{
namespace ChannelSvcs
{
  class DictionaryMatcher
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    DictionaryMatcher(
      const CORBACommons::CorbaClientAdapter& adapter,
      CORBACommons::CorbaObjectRefList& dictionary_server_refs,
      Logging::Logger* logger)
      /*throw(Exception)*/;

    virtual ~DictionaryMatcher() noexcept {};

    /*
    virtual size_t add_trigger(MergeAtom& merge_atom);
    */

    typedef Generics::GnuHashTable<Generics::StringHashAdapter, Lexeme_var> LexemeCache;

    void get_lexemes(
      const char* lang,
      LexemeCache& lexemes)
      /*throw(Exception)*/;

    virtual bool ready() const noexcept;

    static bool
    is_lexemized(const char* trigger) noexcept;


    typedef CORBACommons::ObjectPoolRefConfiguration
      DictionaryProviderPoolConfig;

    typedef
      CORBACommons::ObjectPool<
        AdServer::ChannelSvcs::DictionaryProvider,
        DictionaryProviderPoolConfig>
      DictionaryProviderPool;
    typedef std::unique_ptr<DictionaryProviderPool> DictionaryProviderPoolPtr;

  private:
    static const char* ASPECT;

    AdServer::ChannelSvcs::DictionaryProvider::LexemeSeq*
    query_dictionary_words_(
      const char* lang,
      const CORBACommons::StringSeq& seq_words) const
      /*throw(Exception)*/;

    void trace_result_(const char* lang, const LexemeCache& cache)
      noexcept;

  private:
    const CORBACommons::CorbaClientAdapter& c_adapter_;
    DictionaryProviderPoolPtr dictionary_pool_;
    Logging::Logger* logger_;
  };

}
}

namespace AdServer
{
namespace ChannelSvcs
{
    inline
    bool DictionaryMatcher::is_lexemized(const char* trigger) noexcept
    {
      char trigger_type = Serialization::trigger_type(trigger);
      return (trigger_type != 'U' && trigger_type != 'D');
    }
}
}
#endif//CHANNEL_SVCS_DICTIONARY_MATCHER_HPP_
