#include <Commons/GrpcAlgs.hpp>
#include <Commons/WireAlgs.hpp>

#include <array>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <Generics/Decimal.hpp>
#include <Generics/SimpleDecimal.hpp>

namespace
{
  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  void test_time()
  {
    for (const auto& value : {Generics::Time::ZERO, Generics::Time(1700000000, 123456),
        Generics::Time(-3600, 654321)})
    {
      std::array<unsigned char, Generics::Time::TIME_PACK_LEN> expected{};
      value.pack(expected.data());
      const auto packed = GrpcAlgs::pack_time(value);
      require(packed.size() == expected.size(), "Timestamp width changed");
      require(std::memcmp(packed.data(), expected.data(), expected.size()) == 0,
        "Timestamp bytes changed");
      require(GrpcAlgs::unpack_time(packed) == value, "Timestamp round trip failed");
    }
  }

  template<typename Decimal>
  void test_decimal(const Decimal& value)
  {
    std::array<unsigned char, Decimal::PACK_SIZE> expected{};
    value.pack(expected.data());
    const auto packed = GrpcAlgs::pack_decimal(value);
    require(packed.size() == expected.size(), "Decimal width changed");
    require(std::memcmp(packed.data(), expected.data(), expected.size()) == 0,
      "Decimal bytes changed");
    require(GrpcAlgs::unpack_decimal<Decimal>(packed) == value, "Decimal round trip failed");

    const AdServer::Commons::Optional<Decimal> optional(value);
    const auto packed_optional = GrpcAlgs::pack_optional_decimal(optional);
    require(packed_optional == packed, "Optional decimal bytes changed");
    const auto restored = GrpcAlgs::unpack_optional_decimal<Decimal>(packed_optional);
    require(restored.present() && *restored == value, "Optional decimal round trip failed");
    const AdServer::Commons::Optional<Decimal> absent;
    require(GrpcAlgs::pack_optional_decimal(absent).empty(), "Absent decimal is not empty");
    require(!GrpcAlgs::unpack_optional_decimal<Decimal>({}).present(), "Empty decimal is present");

    std::vector<std::uint32_t> sequence{41, 42};
    WireAlgs::pack_decimal_into_seq(sequence, value);
    require(sequence[0] == 41 && sequence[1] == 42 && sequence[2] == 0,
      "Sequence prefix or decimal separator changed");
    constexpr std::size_t word_count = (Decimal::PACK_SIZE + 3) / 4;
    require(sequence.size() == word_count + 3, "Decimal sequence width changed");
    std::array<std::uint32_t, word_count> expected_words{};
    value.pack(expected_words.data());
    require(std::equal(sequence.begin() + 3, sequence.end(), expected_words.begin()),
      "Decimal sequence bytes changed");
    Decimal restored_from_sequence;
    WireAlgs::unpack_decimal_from_seq(restored_from_sequence, sequence, 3);
    require(restored_from_sequence == value, "Decimal sequence round trip failed");
  }

  void test_ids()
  {
    const AdServer::Commons::UserId id("Z96-Qh5eSh-C9lr6tAuskg..");
    const auto packed = GrpcAlgs::pack_user_id(id);
    require(packed.size() == id.size(), "User ID width changed");
    require(packed == std::string(id.begin(), id.end()), "User ID bytes changed");
    require(GrpcAlgs::unpack_user_id(packed) == id, "User ID round trip failed");
    require(GrpcAlgs::pack_request_id(id) == packed, "Request ID bytes changed");
    require(GrpcAlgs::unpack_request_id(packed) == id, "Request ID round trip failed");
    require(GrpcAlgs::unpack_user_id({}) == AdServer::Commons::UserId(), "Empty user ID changed");
    require(GrpcAlgs::unpack_request_id({}) == AdServer::Commons::RequestId(),
      "Empty request ID changed");
  }
}

int main()
{
  try
  {
    test_time();
    using RevenueDecimal = Generics::SimpleDecimal<std::uint64_t, 18, 8>;
    using ImpRevenueDecimal = Generics::Decimal<std::uint64_t, 36, 8>;
    test_decimal(RevenueDecimal("-123.45678901"));
    test_decimal(RevenueDecimal("0"));
    test_decimal(ImpRevenueDecimal("1234567890123456789.12345678"));
    test_decimal(ImpRevenueDecimal("-1.00000001"));
    test_ids();
    std::cout << "PASS: time, decimal, optional decimal, decimal sequences and IDs\n";
    return 0;
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
