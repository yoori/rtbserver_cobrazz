#pragma once

#if defined(__GNUC__)
#define AD_BUFFER_WRITER_ALWAYS_INLINE inline __attribute__((always_inline))
#else
#define AD_BUFFER_WRITER_ALWAYS_INLINE inline
#endif

namespace AdServer::LogProcessing
{
  template <typename InvariantsChecker, char Separator>
  BufferOutputArchive<InvariantsChecker, Separator>::BufferOutputArchive(
    BufferWriter& writer) noexcept
    : writer_(writer)
  {}

  template <typename InvariantsChecker, char Separator>
  template <typename Field>
  BufferOutputArchive<InvariantsChecker, Separator>&
  BufferOutputArchive<InvariantsChecker, Separator>::operator&(const Field& field)
  {
    writer_ << field << Separator;
    return *this;
  }

  template <typename InvariantsChecker, char Separator>
  template <typename Field>
  BufferOutputArchive<InvariantsChecker, Separator>&
  BufferOutputArchive<InvariantsChecker, Separator>::operator^(const Field& field)
  {
    writer_ << field;
    return *this;
  }

  template <typename InvariantsChecker, char Separator>
  template <typename Data>
  BufferOutputArchive<InvariantsChecker, Separator>&
  BufferOutputArchive<InvariantsChecker, Separator>::operator<<(const Data& data)
  {
    InvariantsChecker::invariant(data);
    const_cast<Data&>(data).serialize(*this);
    return *this;
  }

  inline void
  BufferWriter::flush_if_required_(std::size_t append_size)
  {
    if (file_ && buffer_limit_ != 0 && buffer_.size() + append_size > buffer_limit_)
    {
      flush();
    }
  }

  template<typename Integer>
  AD_BUFFER_WRITER_ALWAYS_INLINE void
  BufferWriter::append_integer_(Integer value)
  {
    char buffer[32];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    const auto append_size = static_cast<std::size_t>(result.ptr - buffer);

    if (file_ && buffer_limit_ != 0 && append_size >= buffer_limit_)
    {
      flush();
      write_direct_(buffer, append_size);
      return;
    }

    flush_if_required_(append_size);
    buffer_.append(buffer, append_size);
  }

  inline void
  BufferWriter::append(char value)
  {
    flush_if_required_(1);
    buffer_.push_back(value);
  }

  AD_BUFFER_WRITER_ALWAYS_INLINE void
  BufferWriter::append_number(unsigned long value)
  {
    append_integer_(value);
  }

  AD_BUFFER_WRITER_ALWAYS_INLINE void
  BufferWriter::append_number(long value)
  {
    append_integer_(value);
  }

  AD_BUFFER_WRITER_ALWAYS_INLINE void
  BufferWriter::append_number(unsigned long long value)
  {
    append_integer_(value);
  }

  AD_BUFFER_WRITER_ALWAYS_INLINE void
  BufferWriter::append_number(long long value)
  {
    append_integer_(value);
  }

  AD_BUFFER_WRITER_ALWAYS_INLINE void
  BufferWriter::append_number(unsigned int value)
  {
    append_integer_(value);
  }

  AD_BUFFER_WRITER_ALWAYS_INLINE void
  BufferWriter::append_number(int value)
  {
    append_integer_(value);
  }

  inline BufferWriter&
  operator<<(BufferWriter& out, char value)
  {
    out.append(value);
    return out;
  }

  inline BufferWriter&
  operator<<(BufferWriter& out, bool value)
  {
    out.append(value ? '1' : '0');
    return out;
  }

  inline BufferWriter&
  operator<<(BufferWriter& out, unsigned long value)
  {
    out.append_number(value);
    return out;
  }

  inline BufferWriter&
  operator<<(BufferWriter& out, long value)
  {
    out.append_number(value);
    return out;
  }

  inline BufferWriter&
  operator<<(BufferWriter& out, unsigned long long value)
  {
    out.append_number(value);
    return out;
  }

  inline BufferWriter&
  operator<<(BufferWriter& out, long long value)
  {
    out.append_number(value);
    return out;
  }

  inline BufferWriter&
  operator<<(BufferWriter& out, unsigned int value)
  {
    out.append_number(value);
    return out;
  }

  inline BufferWriter&
  operator<<(BufferWriter& out, int value)
  {
    out.append_number(value);
    return out;
  }

  inline BufferWriter&
  operator<<(BufferWriter& out, unsigned short value)
  {
    out.append_number(static_cast<unsigned int>(value));
    return out;
  }

  inline BufferWriter&
  operator<<(BufferWriter& out, short value)
  {
    out.append_number(static_cast<int>(value));
    return out;
  }

  template <typename Container>
  BufferWriter&
  write_sequence_(BufferWriter& out, const Container& values, char separator)
  {
    if (values.empty())
    {
      out << '-';
      return out;
    }

    auto it = values.begin();
    out << *it;
    for (++it; it != values.end(); ++it)
    {
      out << separator << *it;
    }
    return out;
  }

  template <typename Convertor, const char STRING_SEPARATOR>
  BufferWriter&
  operator<<(BufferWriter& out, const StringIO<Convertor, STRING_SEPARATOR>& value)
  {
    const char* ptr = value.data();
    const char* const end = ptr + value.size();
    for (; ptr != end; ++ptr)
    {
      if (static_cast<unsigned char>(*ptr) < 128)
      {
        const char* reserved = Convertor::WRITE_RESERVED_CHAR_TABLE[
          static_cast<unsigned char>(*ptr)];
        if (*reserved)
        {
          out.append(Aux_::EscapeChar::ESC_CHAR);
          out.append(std::string_view(reserved, 2));
          continue;
        }
      }
      out.append(*ptr);
    }
    return out;
  }

  template <class PRECISION_TRAITS>
  BufferWriter&
  operator<<(BufferWriter& out, const TimeFacet<PRECISION_TRAITS>& value)
  {
    out << value.time().get_gm_time().format(PRECISION_TRAITS::format());
    return out;
  }

  template <class UUID>
  BufferWriter&
  operator<<(BufferWriter& out, const UuidIoWrapper<UUID>& value)
  {
    if (value.is_null())
    {
      out << UuidIoWrapper<UUID>::NULL_MARKER;
    }
    else
    {
      out << static_cast<const UUID&>(value);
    }
    return out;
  }

  template <typename Base, const unsigned TOTAL, const unsigned FRACTION>
  BufferWriter&
  operator<<(BufferWriter& out, const Generics::SimpleDecimal<Base, TOTAL, FRACTION>& value)
  {
    out << value.str();
    return out;
  }

  template <std::size_t Precision>
  BufferWriter&
  operator<<(BufferWriter& out, const FixedPrecisionDouble<Precision>& value)
  {
    char buffer[Precision + 384];
    const int size = std::snprintf(
      buffer,
      sizeof(buffer),
      "%.*f",
      static_cast<int>(Precision),
      value.value());
    if (size > 0 && static_cast<std::size_t>(size) < sizeof(buffer))
    {
      out.append(std::string_view(buffer, size));
    }
    return out;
  }

  template <typename Object>
  BufferWriter&
  operator<<(BufferWriter& out, const EmptyHolder<Object>& value)
  {
    if (value.get().empty())
    {
      return out << EmptyHolder<Object>::NULL_MARKER;
    }
    return out << value.get();
  }

  template <typename Value, typename OptionalValueTraits>
  BufferWriter&
  operator<<(BufferWriter& out, const OptionalValue<Value, OptionalValueTraits>& value)
  {
    if (value.present() && !OptionalValueTraits::is_empty(value.get()))
    {
      out << OptionalValueTraits::PRESENT_MARKER << value.get();
    }
    else
    {
      out << OptionalValueTraits::NULL_MARKER;
    }
    return out;
  }

  inline BufferWriter&
  operator<<(BufferWriter& out, CampaignSvcs::AuctionType value)
  {
    return out << CampaignSvcs::put_auction_type(value);
  }

  template <typename Value>
  BufferWriter&
  operator<<(BufferWriter& out, const std::list<Value>& values)
  {
    return write_sequence_(out, values, ',');
  }

  template <typename Value>
  BufferWriter&
  operator<<(BufferWriter& out, const std::deque<Value>& values)
  {
    return write_sequence_(out, values, ',');
  }

  template <typename Value>
  BufferWriter&
  operator<<(BufferWriter& out, const std::vector<Value>& values)
  {
    return write_sequence_(out, values, ',');
  }

  template <typename Value>
  BufferWriter&
  operator<<(BufferWriter& out, const Value& value)
  {
    std::ostringstream stream;
    stream << value;
    out.append(stream.str());
    return out;
  }
}

#undef AD_BUFFER_WRITER_ALWAYS_INLINE
