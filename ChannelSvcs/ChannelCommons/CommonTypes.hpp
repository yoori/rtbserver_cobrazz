#pragma once

#include <vector>
#include <algorithm>
#include <boost/unordered/unordered_flat_set.hpp>
#include <string>
#include <list>
#include <map>
#include <set>

#include <eh/Exception.hpp>
#include <String/SubString.hpp>
#include <Generics/Time.hpp>
#include <Generics/MonoAllocator.hpp>
#include <Generics/Uuid.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Language/BLogic/NormalizeTrigger.hpp>
#include <ChannelSvcs/ChannelCommons/Lexeme.hpp>

namespace AdServer::ChannelSvcs
{
  typedef unsigned int IdType;
  typedef std::vector<IdType> IdVector;
  typedef std::list<IdType> IdList;
  typedef std::set<IdType> IdSet;
  typedef std::vector<std::string> StringVector;
  typedef std::vector<String::SubString> SubStringVector;

  typedef std::string HardWord;
  typedef std::vector<HardWord> HardWords;
  typedef SubStringVector SoftWord;
  typedef std::vector<Lexeme_var> LexemesPtrVector;
  typedef Language::Trigger::Trigger::Part Part;
  typedef Language::Trigger::Trigger::Parts Parts;

  typedef std::set<std::string> StringSet;
  typedef std::set<String::SubString> SubStringSet;
  typedef std::list<std::string> StringList;
  typedef std::list<String::SubString> SubStringList;

  struct Trigger
  {
    IdType channel_trigger_id;
    std::string trigger;
    char type;
    bool negative;
  };

  struct SoftTriggerWord
  {
    SoftTriggerWord() = default;

    SoftTriggerWord(SoftTriggerWord&& init) noexcept = default;

    void swap(SoftTriggerWord& word) noexcept;

    IdType channel_trigger_id = 0;
    std::string trigger;

  private:
    SoftTriggerWord(const SoftTriggerWord&);
  };

  typedef std::list<SoftTriggerWord> SoftTriggerList;

  typedef Trigger UrlWord;

  typedef std::list<Trigger> TriggerList;
  typedef std::vector<Trigger> TriggerVector;
  typedef std::vector<Generics::Uuid> UuidVector;

  typedef TriggerList UrlWords;

  struct MergeAtom
  {
    const MergeAtom& operator=(IdType in) noexcept;
    bool operator==(IdType in) const noexcept;
    IdType operator()() const noexcept;
    void print(std::ostream& ostr) const /*throw(eh::Exception)*/;
    IdType id; // id
    std::string lang;
    SoftTriggerList soft_words; // words should be merged as soft
    std::string uids_trigger;
  };

  using MatchWordSet = Generics::MonoUnorderedSet<Generics::StringViewHashAdapter,
    Generics::HashFunForHashAdapter<Generics::StringViewHashAdapter>>;

  using MatchWordVector = Generics::MonoVector<Generics::StringViewHashAdapter>;

  class PreparedMatchWords
  {
  public:
    explicit PreparedMatchWords(Generics::MonoAllocatorArena* arena) noexcept;

    void reserve(std::size_t capacity);

    void insert(const Generics::StringViewHashAdapter& word);

    template<typename Iterator>
    void insert(Iterator first, Iterator last);

    std::size_t size() const noexcept;

    const MatchWordSet& words() const noexcept;

    const MatchWordVector& candidates() const noexcept;

  private:
    MatchWordSet words_;
    MatchWordVector candidates_;
  };

  struct MatchWords:
    public boost::unordered_flat_set<Generics::StringViewHashAdapter,
      Generics::HashFunForHashAdapter<Generics::StringViewHashAdapter>>
  {
    struct Sequence
    {
      std::vector<std::size_t> positions;
      std::size_t max_length;
    };

    std::string data_holder_;//uses for memory allocation of SubStrings
    std::vector<Sequence> deferred_sequences;
    bool use_keyword_hooks = true;
  };
}

namespace AdServer::ChannelSvcs
{
  inline
  const MergeAtom& MergeAtom::operator=(IdType in) noexcept
  {
    id = in;
    return *this;
  }

  inline
  bool MergeAtom::operator==(IdType in) const noexcept
  {
    return (id == in);
  }

  inline
  IdType MergeAtom::operator()() const noexcept
  {
    return id;
  }

  inline PreparedMatchWords::PreparedMatchWords(Generics::MonoAllocatorArena* arena) noexcept
    : words_(arena), candidates_(arena)
  {}

  inline void PreparedMatchWords::reserve(std::size_t capacity)
  {
    words_.reserve(capacity);
    candidates_.reserve(capacity ? std::max(capacity, std::size_t{8}) : 0);
  }

  inline void PreparedMatchWords::insert(const Generics::StringViewHashAdapter& word)
  {
    const auto inserted = words_.insert(word);
    if (inserted.second)
    {
      candidates_.push_back(word);
    }
  }

  template<typename Iterator>
  inline void PreparedMatchWords::insert(Iterator first, Iterator last)
  {
    for (; first != last; ++first)
    {
      insert(*first);
    }
  }

  inline std::size_t PreparedMatchWords::size() const noexcept
  {
    return candidates_.size();
  }

  inline const MatchWordSet& PreparedMatchWords::words() const noexcept
  {
    return words_;
  }

  inline const MatchWordVector& PreparedMatchWords::candidates() const noexcept
  {
    return candidates_;
  }
}
