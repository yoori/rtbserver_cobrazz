#ifndef AD_SERVER_DICTIOMARY_CONTAINER
#define AD_SERVER_DICTIOMARY_CONTAINER

#include <vector>
#include <set>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <ReferenceCounting/Map.hpp>
#include <eh/Exception.hpp>
#include <Sync/PosixLock.hpp>
#include <String/SubString.hpp>

namespace AdServer
{
  namespace ChannelSvcs
  {

    typedef std::set<String::SubString> Forms;

    class Dictionary : public ReferenceCounting::AtomicImpl
    {
    public:
      Dictionary() /*throw(eh::Exception)*/;

      void
      add_lexeme(const Forms& forms) /*throw(eh::Exception)*/;

      void
      get_forms(const char* word, Forms& forms) const /*throw(eh::Exception)*/;

      void
      get_statistic(std::ostream& ostr) const /*throw(eh::Exception)*/;

      void
      append(const Dictionary& dict) /*throw(eh::Exception)*/;

      void
      clear() noexcept;

      /**
       * MUST be called before using.
       */
      void
      finalize() /*throw(eh::Exception)*/;

      size_t
      get_words_count() const noexcept;

      size_t
      get_lexems_count() const noexcept;

      size_t
      get_buffer_size() const noexcept;

    private:
      static const char LEXEME_SEPARATOR;
      static const char FORM_SEPARATOR;

      typedef std::vector<char> Buffer;
      Buffer buffer_;

      typedef const char* Index;
      typedef std::vector<Index> Indexes;
      Indexes word_indexes_;

      size_t lexems_count_;

    private:
      virtual
      ~Dictionary() noexcept { };

      void
      make_lexeme_(const char* begin, Forms& forms) const /*throw(eh::Exception)*/;

      void
      build_index_() /*throw(eh::Exception)*/;
    };

    typedef ReferenceCounting::QualPtr<Dictionary> Dictionary_var;

    struct LexemeItem
    {
      const char* word;
      Forms forms;
    };

    typedef std::vector<LexemeItem> LexemeItemVector;

    class DictionaryContainer
    {
    public:
      typedef ReferenceCounting::Map<std::string, Dictionary_var>
        Dictionaries;

      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      void
      load_dictionary(const char* lang, std::istream& is) /*throw(eh::Exception)*/;

      void
      find_lexemes(const char* lang, LexemeItemVector& res) const /*throw(eh::Exception)*/;

      void
      get_statistic(std::ostream& ostr) const /*throw(eh::Exception)*/;

      typedef Sync::PosixMutex Mutex_;
      typedef Sync::PosixGuard ReadGuard_;
      typedef Sync::PosixGuard WriteGuard_;

    private:
      void
      fill_lexemes_(const Dictionary& dict,
          LexemeItemVector& res) const /*throw(eh::Exception)*/;

      void
      add_dictionary_(const char* lang) /*throw(eh::Exception)*/;

    private:
      mutable Mutex_ lock_;
      Dictionaries dictionaries_;
    };

  }

}

#endif //AD_SERVER_DICTIOMARY_CONTAINER
