#include <eh/Exception.hpp>
#include <iostream>
#include <Commons/ExternalUserIdUtils.hpp>

int
main(int argc, char* argv[])
{
  if(argc < 2)
  {
    std::cerr << "argument not defined" << std::endl;
    return 1;
  }

  AdServer::Commons::ExternalUserIdArray user_ids;

  AdServer::Commons::dns_decode_external_user_ids(
    user_ids,
    String::SubString(argv[1]));

  for(auto it = user_ids.begin(); it != user_ids.end(); ++it)
  {
    std::cout << *it << std::endl;
  }

  return 0;
}

