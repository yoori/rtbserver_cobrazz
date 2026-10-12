#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <google/protobuf/repeated_field.h>

namespace WireAlgs
{
  template<typename ULongSeqType, typename DecimalType>
  void pack_decimal_into_seq(ULongSeqType& seq, const DecimalType& value)
  {
    const unsigned long EL_NUMBER = DecimalType::PACK_SIZE / 4 +
      (DecimalType::PACK_SIZE % 4 ? 1 : 0);
    uint32_t buf[EL_NUMBER];
    ::memset(buf, 0, EL_NUMBER * 4);
    value.pack(buf);

    std::uint32_t pos = seq.size();
    if constexpr (std::is_same_v<ULongSeqType, google::protobuf::RepeatedField<uint32_t>>)
    {
      seq.Resize(pos + EL_NUMBER + 1, 0);
    }
    else
    {
      seq.resize(pos + EL_NUMBER + 1);
    }

    seq[pos] = 0;
    for (std::uint32_t i = 0; i < EL_NUMBER; ++i)
    {
      seq[pos + i + 1] = buf[i];
    }
  }

  template<typename ULongSeqType, typename DecimalType>
  void unpack_decimal_from_seq(DecimalType& value, const ULongSeqType& seq, std::uint32_t pos)
  {
    const unsigned long EL_NUMBER = DecimalType::PACK_SIZE / 4 +
      (DecimalType::PACK_SIZE % 4 ? 1 : 0);
    uint32_t buf[EL_NUMBER];

    assert(seq.size() - pos == EL_NUMBER);

    for (std::uint32_t i = 0; i < EL_NUMBER; ++i)
    {
      buf[i] = seq[pos + i];
    }

    value.unpack(buf);
  }
}
