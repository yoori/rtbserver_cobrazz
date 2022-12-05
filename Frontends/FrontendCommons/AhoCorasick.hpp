 
#ifndef FRONTENDCOMMONS_AHO_CORASICK_HPP
#define FRONTENDCOMMONS_AHO_CORASICK_HPP

#include <list>
#include <queue>
#include <set>
#include <string>
#include <vector>

#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>
#include <String/AsciiStringManip.hpp>

namespace FrontendCommons
{
  /**
   * This is implementation of Aho-Corasick multi-string matching algorithm.
   * See http://en.wikipedia.org/wiki/Aho%E2%80%93Corasick_string_matching_algorithm
   *
   * Optimized for matching static set of patterns.
   *
   * Main idea - to build automata.
   *  State corresponds to the matched part of pattern(s);
   *  Event corresponds to symbol of "search in" string.
   *
   * PatternTag - some data, which associated with pattern
   */
  template<typename PatternTag>
  class AhoCorasik : Generics::Uncopyable
  {
  public:
    struct MatchDetails
    {
      const String::SubString search_in_string;
      const PatternTag tag;
    };

  public:
    AhoCorasik(bool case_sensitive = true)
      /*throw(eh::Exception)*/;

    void
    add_pattern(const char* pattern, PatternTag tag)
      /*throw(eh::Exception)*/;

    template<typename OnMatch>
    void
    match(const String::SubString& str, OnMatch& on_match) const noexcept;

  private:
    typedef std::set<PatternTag> Tags;

    struct Pattern
    {
      std::string text;
      PatternTag tag;
    };

    struct State
    {
      typedef std::vector<State*> Transitions;
      Transitions transitions;

      Tags tags;
    };

    typedef std::vector<Pattern> Patterns;
    typedef std::vector<size_t> EncodeTable;
    typedef std::list<State> States;

    struct StateTag
    {
      State* state;
      State* prev_state;
      std::vector<size_t> matched;
    };

  private:
    Patterns patterns_;
    EncodeTable encode_table_;
    State begin_state_;
    States states_;
    const bool case_sensitive_;

  private:
    void
    compile_() /*throw(eh::Exception)*/;

    size_t
    build_encode_table_() /*throw(eh::Exception)*/;

    size_t
    encode_(char ch) const noexcept;

    void
    build_states_(size_t alphabet_size) /*throw(eh::Exception)*/;

    void
    set_fail_transitions_() /*throw(eh::Exception)*/;
  };

}

namespace FrontendCommons
{

  template<typename PatternTag>
  AhoCorasik<PatternTag>::AhoCorasik(bool case_sensitive)
    /*throw(eh::Exception)*/
    : case_sensitive_(case_sensitive)
  {
    begin_state_.transitions.push_back(&begin_state_);
    build_encode_table_();
  }

  template<typename PatternTag>
  void
  AhoCorasik<PatternTag>::add_pattern(const char* pattern, PatternTag tag)
    /*throw(eh::Exception)*/
  {
    Pattern p = { pattern, tag };
    patterns_.push_back(p);
    compile_();
  }

  template<typename PatternTag>
  template<typename OnMatch>
  void
  AhoCorasik<PatternTag>::match(
    const String::SubString& str, OnMatch& on_match) const
    noexcept
  {
    const State* state = &begin_state_;
    for (const char* curr_pos = str.begin(); curr_pos != str.end();
      curr_pos++)
    {
      state = state->transitions[encode_(*curr_pos)];

      for (typename Tags::const_iterator ci = state->tags.begin();
          ci != state->tags.end(); ++ci)
      {
        MatchDetails details = {str, *ci};

        if (on_match(details))
        {
          // the terminate matching signal was received
          return;
        }
      }
    }
  }

  template<typename PatternTag>
  void
  AhoCorasik<PatternTag>::compile_() /*throw(eh::Exception)*/
  {
    const size_t alphabet_size = build_encode_table_();
    build_states_(alphabet_size);
    set_fail_transitions_();
  }

  template<typename PatternTag>
  size_t
  AhoCorasik<PatternTag>::build_encode_table_() /*throw(eh::Exception)*/
  {
    const size_t MAX_ALPHABET_SIZE = 256;

    encode_table_.clear();
    encode_table_.resize(MAX_ALPHABET_SIZE, 0);
    size_t alphabet_size = 1;

    for (typename Patterns::const_iterator pi = patterns_.begin();
        pi != patterns_.end(); ++pi)
    {
      for (std::string::const_iterator ci = pi->text.begin();
          ci != pi->text.end(); ++ci)
      {
        const size_t inx = static_cast<unsigned char>(*ci);

        if (!encode_table_[inx])
        {
          if (case_sensitive_)
          {
            encode_table_[inx] = alphabet_size;
          }
          else
          {
            encode_table_[static_cast<unsigned char>(
              String::AsciiStringManip::to_lower(*ci))] = alphabet_size;
            encode_table_[static_cast<unsigned char>(
              String::AsciiStringManip::to_upper(*ci))] = alphabet_size;
          }

          ++alphabet_size;
        }
      }
    }

    return alphabet_size;
  }

  template<typename PatternTag>
  size_t
  AhoCorasik<PatternTag>::encode_(char ch) const noexcept
  {
    return encode_table_[static_cast<unsigned char>(ch)];
  }

  template<typename PatternTag>
  void
  AhoCorasik<PatternTag>::build_states_(size_t alphabet_size)
    /*throw(eh::Exception)*/
  {
    states_.clear();
    begin_state_.transitions.clear();
    begin_state_.transitions.resize(alphabet_size, 0);

    for (typename Patterns::const_iterator pi = patterns_.begin();
        pi != patterns_.end(); ++pi)
    {
      State* state = &begin_state_;

      for (size_t i = 0; i < pi->text.length(); ++i)
      {
        const size_t inx = encode_(pi->text[i]);
        State* next_state = state->transitions[inx];

        if (!next_state)
        {
          State new_state;
          new_state.transitions.resize(alphabet_size, 0);
          states_.push_back(new_state);
          next_state = &states_.back();
          state->transitions[inx] = next_state;
        }

        state = next_state;
      }

      state->tags.insert(pi->tag);
    }
  }

  template<typename PatternTag>
  void
  AhoCorasik<PatternTag>::set_fail_transitions_() /*throw(eh::Exception)*/
  {
    std::queue<StateTag> unwatched;
    StateTag begin_tag;
    begin_tag.state = &begin_state_;
    begin_tag.prev_state = 0;
    unwatched.push(begin_tag);

    while (!unwatched.empty())
    {
      StateTag curr_tag = unwatched.front();
      State* curr_state = curr_tag.state;
      unwatched.pop();

      for (size_t i = 0; i < curr_state->transitions.size(); ++i)
      {
        State* next_prev_state =
          curr_tag.prev_state ?
          curr_tag.prev_state->transitions[i] : &begin_state_;

        if (curr_state->transitions[i])
        {
          curr_state->transitions[i]->tags.insert(
            next_prev_state->tags.begin(), next_prev_state->tags.end());
          StateTag tag = { curr_state->transitions[i],
            next_prev_state, curr_tag.matched };
          tag.matched.push_back(i);
          unwatched.push(tag);
        }
        else
        {
          curr_state->transitions[i] = next_prev_state;
        }
      }
    }
  }
}

#endif /*FRONTENDCOMMONS_AHO_CORASICK_HPP*/
