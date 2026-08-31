#include <Commons/UserInfoManip.hpp>

#include <array>
#include <cstddef>

namespace
{
  constexpr char BASE64MOD_ALPHABET[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
  constexpr std::size_t USER_ID_SIZE = 16;
  constexpr std::size_t ENCODED_USER_ID_SIZE = 24;

  std::array<char, ENCODED_USER_ID_SIZE>
  encode_user_id(const AdServer::Commons::UserId& user_id) noexcept
  {
    std::array<char, ENCODED_USER_ID_SIZE> encoded{};
    const auto* input = user_id.begin();
    std::size_t output_position = 0;
    for (std::size_t input_position = 0; input_position + 2 < USER_ID_SIZE; input_position += 3)
    {
      const auto byte1 = input[input_position];
      const auto byte2 = input[input_position + 1];
      const auto byte3 = input[input_position + 2];
      encoded[output_position++] = BASE64MOD_ALPHABET[byte1 >> 2];
      encoded[output_position++] = BASE64MOD_ALPHABET[((byte1 << 4) & 0x30) | (byte2 >> 4)];
      encoded[output_position++] = BASE64MOD_ALPHABET[((byte2 << 2) & 0x3c) | (byte3 >> 6)];
      encoded[output_position++] = BASE64MOD_ALPHABET[byte3 & 0x3f];
    }

    const auto last_byte = input[USER_ID_SIZE - 1];
    encoded[output_position++] = BASE64MOD_ALPHABET[last_byte >> 2];
    encoded[output_position++] = BASE64MOD_ALPHABET[(last_byte << 4) & 0x30];
    encoded[output_position++] = '.';
    encoded[output_position] = '.';
    return encoded;
  }
}

namespace AdServer::Commons
{
  const UserId PROBE_USER_ID("PPPPPPPPPPPPPPPPPPPPPA..");

  std::uint32_t
  user_id_sampling_hash(std::string_view encoded_user_id) noexcept
  {
    return Generics::CRC::reversed(
      0,
      encoded_user_id.data(),
      encoded_user_id.size());
  }

  std::uint32_t
  user_id_sampling_hash(const UserId& user_id) noexcept
  {
    // Keep the hash compatible with ClickHouse CRC32(uid), where uid is the
    // canonical padded base64mod representation stored in RImpression.
    const auto encoded_user_id = encode_user_id(user_id);
    return user_id_sampling_hash(
      std::string_view(encoded_user_id.data(), encoded_user_id.size()));
  }
}
