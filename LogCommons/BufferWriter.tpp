#pragma once

namespace AdServer::LogProcessing
{
  inline void
  BufferWriter::append(char value)
  {
    flush_if_required_(1);
    buffer_.push_back(value);
  }

  inline void
  BufferWriter::append_number(unsigned long value)
  {
    append_integer_(value);
  }

  inline void
  BufferWriter::append_number(long value)
  {
    append_integer_(value);
  }

  inline void
  BufferWriter::append_number(unsigned long long value)
  {
    append_integer_(value);
  }

  inline void
  BufferWriter::append_number(long long value)
  {
    append_integer_(value);
  }

  inline void
  BufferWriter::append_number(unsigned int value)
  {
    append_integer_(value);
  }

  inline void
  BufferWriter::append_number(int value)
  {
    append_integer_(value);
  }

  inline void
  BufferWriter::flush_if_required_(std::size_t append_size)
  {
    if (file_ &&
      buffer_limit_ != 0 &&
      buffer_.size() + append_size > buffer_limit_)
    {
      flush();
    }
  }

  template<typename Integer>
  void
  BufferWriter::append_integer_(Integer value)
  {
    char buffer[32];
    const auto result = std::to_chars(
      buffer,
      buffer + sizeof(buffer),
      value);
    append(std::string_view(buffer, result.ptr - buffer));
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
  operator<<(
    BufferWriter& out,
    const StringIO<Convertor, STRING_SEPARATOR>& value)
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
  operator<<(
    BufferWriter& out,
    const Generics::SimpleDecimal<Base, TOTAL, FRACTION>& value)
  {
    out << value.str();
    return out;
  }

  template <typename Value, typename OptionalValueTraits>
  BufferWriter&
  operator<<(
    BufferWriter& out,
    const OptionalValue<Value, OptionalValueTraits>& value)
  {
    if (value.present() && !OptionalValueTraits::is_empty(value.get()))
    {
      out << value.get();
    }
    else
    {
      out << OptionalValueTraits::NULL_MARKER;
    }
    return out;
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
