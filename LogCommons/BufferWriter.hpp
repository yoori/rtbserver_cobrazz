#pragma once

#include <charconv>
#include <cstdio>
#include <cstddef>
#include <iosfwd>
#include <sstream>
#include <string>
#include <string_view>

#include "LogCommons.hpp"

namespace AdServer::LogProcessing
{
  class BufferWriter
  {
  public:
    explicit
    BufferWriter(std::size_t reserve_size = 0);

    BufferWriter(const char* file_name, std::size_t buffer_limit);

    ~BufferWriter();

    BufferWriter(const BufferWriter&) = delete;

    BufferWriter&
    operator=(const BufferWriter&) = delete;

    void
    clear() noexcept;

    bool
    empty() const noexcept;

    std::size_t
    size() const noexcept;

    const char*
    data() const noexcept;

    const std::string&
    str() const noexcept;

    void
    reserve(std::size_t size);

    void
    append(char value);

    void
    append(const char* value);

    void
    append(std::string_view value);

    void
    append(const std::string& value);

    void
    append_number(unsigned long value);

    void
    append_number(long value);

    void
    append_number(unsigned long long value);

    void
    append_number(long long value);

    void
    append_number(unsigned int value);

    void
    append_number(int value);

    void
    write_to(std::ostream& out) const;

    void
    flush();

    void
    close();

    bool
    good() const noexcept;

  private:
    template<typename Integer>
    void
    append_integer_(Integer value);

    void
    flush_if_required_(std::size_t append_size);

    void
    write_direct_(const char* data, std::size_t size);

  private:
    std::string buffer_;
    std::FILE* file_ = nullptr;
    std::size_t buffer_limit_ = 0;
    bool good_ = true;
  };

  template <
    typename InvariantsChecker = Aux_::OwnInvariants,
    char Separator = '\t'>
  class BufferOutputArchive
  {
  public:
    explicit
    BufferOutputArchive(BufferWriter& writer) noexcept;

    template <typename Field>
    BufferOutputArchive&
    operator&(const Field& field);

    template <typename Field>
    BufferOutputArchive&
    operator^(const Field& field);

    template <typename Data>
    BufferOutputArchive&
    operator<<(const Data& data);

  private:
    BufferWriter& writer_;
  };

  using BufferTabOutputArchive = BufferOutputArchive<Aux_::OwnInvariants, '\t'>;
  using SimpleBufferTabOutputArchive = BufferOutputArchive<Aux_::NoInvariants, '\t'>;

  BufferWriter&
  operator<<(BufferWriter& out, char value);

  BufferWriter&
  operator<<(BufferWriter& out, const char* value);

  BufferWriter&
  operator<<(BufferWriter& out, std::string_view value);

  BufferWriter&
  operator<<(BufferWriter& out, const std::string& value);

  BufferWriter&
  operator<<(BufferWriter& out, bool value);

  BufferWriter&
  operator<<(BufferWriter& out, unsigned long value);

  BufferWriter&
  operator<<(BufferWriter& out, long value);

  BufferWriter&
  operator<<(BufferWriter& out, unsigned long long value);

  BufferWriter&
  operator<<(BufferWriter& out, long long value);

  BufferWriter&
  operator<<(BufferWriter& out, unsigned int value);

  BufferWriter&
  operator<<(BufferWriter& out, int value);

  BufferWriter&
  operator<<(BufferWriter& out, unsigned short value);

  BufferWriter&
  operator<<(BufferWriter& out, short value);

  template <typename Convertor, const char STRING_SEPARATOR>
  BufferWriter&
  operator<<(BufferWriter& out, const StringIO<Convertor, STRING_SEPARATOR>& value);

  template <class PRECISION_TRAITS>
  BufferWriter&
  operator<<(BufferWriter& out, const TimeFacet<PRECISION_TRAITS>& value);

  BufferWriter&
  operator<<(BufferWriter& out, const Generics::Uuid& value);

  template <class UUID>
  BufferWriter&
  operator<<(BufferWriter& out, const UuidIoWrapper<UUID>& value);

  template <typename Base, const unsigned TOTAL, const unsigned FRACTION>
  BufferWriter&
  operator<<(BufferWriter& out, const Generics::SimpleDecimal<Base, TOTAL, FRACTION>& value);

  template <typename Value, typename OptionalValueTraits>
  BufferWriter&
  operator<<(BufferWriter& out, const OptionalValue<Value, OptionalValueTraits>& value);

  template <typename Value>
  BufferWriter&
  operator<<(BufferWriter& out, const std::list<Value>& values);

  template <typename Value>
  BufferWriter&
  operator<<(BufferWriter& out, const std::deque<Value>& values);

  template <typename Value>
  BufferWriter&
  operator<<(BufferWriter& out, const std::vector<Value>& values);

  template <typename Value>
  BufferWriter&
  operator<<(BufferWriter& out, const Value& value);
}

#include "BufferWriter.tpp"
