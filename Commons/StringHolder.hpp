#ifndef COMMONS_STRINGHOLDER_HPP
#define COMMONS_STRINGHOLDER_HPP

#include <string>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Generics/Hash.hpp>
#include <Generics/Hash.hpp>

#include <Stream/MemoryStream.hpp>

namespace AdServer
{
namespace Commons
{
  /* const std::string holder for decrease
   *   number of std::string copy construction
   */
  class StringHolder: public ReferenceCounting::AtomicImpl
  {
  public:
    template <typename... T>
    explicit
    StringHolder(T... data) /*throw(eh::Exception)*/
      : str_(std::forward<T>(data)...)
    {
    }

    StringHolder(const String::SubString& str)
      : str_(str.data(), str.length())
    {}
    
    const std::string*
    operator ->() const noexcept
    {
      return &str_;
    }

    const std::string&
    str() const noexcept
    {
      return str_;
    }

    bool
    operator ==(const StringHolder& arg) const noexcept
    {
      return this == &arg || str_ == arg.str_;
    }

  private:
    virtual
    ~StringHolder() noexcept = default;

  private:
    std::string str_;
  };
  typedef ReferenceCounting::SmartPtr<
    StringHolder, ReferenceCounting::PolicyAssert>
    StringHolder_var;

  class StringHolderHashAdapter
  {
  public:
    StringHolderHashAdapter() noexcept
      : hash_(0)
    {
    }

    explicit
    StringHolderHashAdapter(StringHolder* str) noexcept
      : value_(ReferenceCounting::add_ref(str))
    {
      Generics::Murmur64Hash hash(hash_);
      hash_add(hash, value_->str());
    }
    
    StringHolder_var
    value() const noexcept
    {
      return value_;
    }

    bool
    operator ==(const StringHolderHashAdapter& right) const noexcept
    {
      return value_->str() == right.value_->str();
    }

    size_t
    hash() const noexcept
    {
      return hash_;
    }

  private:
    StringHolder_var value_;
    size_t hash_;
  };

  /**
   * Wrapper over StringHolder
   */
  class ImmutableString
  {
  public:
    typedef ReferenceCounting::ConstPtr<StringHolder,
      ReferenceCounting::PolicyAssert> CStringHolder_var;

    template <typename... T>
    explicit
    ImmutableString(T... data) /*throw(eh::Exception)*/
      : string_holder_(new StringHolder(std::forward<T>(data)...))
    {
    }

    template <typename T>
    ImmutableString&
    operator =(T&& data) /*throw(eh::Exception)*/
    {
      string_holder_ = new StringHolder(std::forward<T>(data));
      return *this;
    }

    const std::string&
    str() const noexcept
    {
      return string_holder_->str();
    }

    bool
    empty() const noexcept
    {
      return string_holder_->str().empty();
    }

    bool
    operator <(const ImmutableString& arg) const noexcept
    {
      return str() < arg.str();
    }

    bool
    operator ==(const ImmutableString& arg) const noexcept
    {
      return str() == arg.str();
    }

    bool
    operator !=(const ImmutableString& arg) const noexcept
    {
      return str() != arg.str();
    }

  private:
    CStringHolder_var string_holder_;
  };
}
}

inline
std::ostream&
operator <<(std::ostream& os, const AdServer::Commons::ImmutableString& arg)
  /*throw(eh::Exception)*/
{
  os << arg.str();
  return os;
}

namespace Stream::MemoryStream
{
  template<>
  struct ToCharsLenHelper<AdServer::Commons::ImmutableString>
  {
    size_t
    operator()(const AdServer::Commons::ImmutableString&) noexcept
    {
      // TODO
      return 0;
    }
  };

  template<>
  struct ToCharsHelper<AdServer::Commons::ImmutableString>
  {
    std::to_chars_result
    operator()(char*, char* last, const AdServer::Commons::ImmutableString&) noexcept
    {
      // TODO
      return {last, std::errc::value_too_large};
    }
  };

  template<>
  struct ToStringHelper<AdServer::Commons::ImmutableString>
  {
    std::string
    operator()(const AdServer::Commons::ImmutableString&) noexcept
    {
      // TODO
      return "";
    }
  };

  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE>
  struct OutputMemoryStreamHelper<Elem, Traits, Allocator, AllocatorInitializer,
    SIZE, AdServer::Commons::ImmutableString>
  {
    OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
    operator()(OutputMemoryStream<Elem, Traits, Allocator,
      AllocatorInitializer, SIZE>& ostr, const AdServer::Commons::ImmutableString& arg)
    {
      typedef typename AdServer::Commons::ImmutableString ArgT;
      return OutputMemoryStreamHelperImpl(ostr, arg,
        ToCharsLenHelper<ArgT>(), ToCharsHelper<ArgT>(), ToStringHelper<ArgT>());
    }
  };
}

#endif /*COMMONS_STRINGHOLDER_HPP*/
