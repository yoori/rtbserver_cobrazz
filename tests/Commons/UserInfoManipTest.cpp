#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <Commons/UserInfoManip.hpp>

namespace
{
  void
  check_hash(std::string_view encoded_user_id, std::uint32_t expected_hash)
  {
    const std::string encoded_user_id_string(encoded_user_id);
    const AdServer::Commons::UserId user_id(encoded_user_id_string.c_str());
    if (AdServer::Commons::user_id_sampling_hash(encoded_user_id) != expected_hash ||
      AdServer::Commons::user_id_sampling_hash(user_id) != expected_hash)
    {
      throw std::runtime_error("Unexpected user ID sampling hash");
    }
  }
}

int
main()
{
  try
  {
    check_hash("Z96-Qh5eSh-C9lr6tAuskg..", 4094596666U);
    check_hash("EAAAAAAAAAAAAAAAAAAAAA..", 1749498248U);
    check_hash("PPPPPPPPPPPPPPPPPPPPPA..", 3205975032U);

    for (unsigned int i = 0; i < 100; ++i)
    {
      const auto user_id = AdServer::Commons::UserId::create_random_based();
      const std::string encoded_user_id = user_id.to_string();
      if (AdServer::Commons::user_id_sampling_hash(user_id) !=
        AdServer::Commons::user_id_sampling_hash(encoded_user_id))
      {
        throw std::runtime_error("Binary and encoded user ID hashes differ");
      }
    }

    std::cout << "UserInfoManipTest: OK" << std::endl;
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << "UserInfoManipTest: " << ex.what() << std::endl;
    return 1;
  }
}
