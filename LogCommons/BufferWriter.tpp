#pragma once

namespace AdServer::LogProcessing
{
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
