#include<cstdlib>
#include<iostream>
#include<Generics/Network.hpp>

unsigned resolve_index(const char* hostname)
{
  char buf[4096];
  hostent addresses;
  try
  {
    Generics::Network::Resolver::get_host_by_name(
      hostname,
      addresses,
      buf,
      sizeof(buf));
  }
  catch(const Generics::Network::Resolver::Exception& e)
  {
    std::cerr << "can't resolve hostname: " << hostname << ". : " << e.what();
    exit(1);
  }
  if(addresses.h_length>0)
  {
    return *reinterpret_cast<unsigned*>(addresses.h_addr_list[0]);
  }
  else
  {
    std::cerr << "can't get address for hostname: " << hostname;
    exit(1);
  }
}

int main(int argc, char* argv[])
{
  if(argc > 1)
  {
    unsigned  res = resolve_index(argv[1]);
    std::cout << "index = " << res << std::endl;
  }
  return 0;
}
