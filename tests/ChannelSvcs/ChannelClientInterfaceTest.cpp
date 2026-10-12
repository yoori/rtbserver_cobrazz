#include <iostream>
#include <stdexcept>
#include <type_traits>

#include <Logger/StreamLogger.hpp>
#include <ChannelSvcs/ChannelClient/ChannelDistributedGrpcClient.hpp>

int main()
{
  using Client = AdServer::ChannelSvcs::ChannelDistributedGrpcClient;
  static_assert(!std::is_abstract_v<Client>);

  try
  {
    Logging::Logger_var logger = new Logging::OStream::Logger(
      Logging::OStream::Config(std::cerr));
    Client client({{"127.0.0.1:1"}}, {}, nullptr, {}, logger);
    unsigned int calls = 0;
    const auto callback = [&](const grpc::Status& status, auto&&)
    {
      if (status.error_code() != grpc::StatusCode::UNIMPLEMENTED)
      {
        throw std::runtime_error("distributed matching client accepted a ChannelUpdate request");
      }

      ++calls;
    };
    client.check({}, callback);
    client.update_triggers({}, callback);
    client.update_all_ccg({}, callback);
    client.get_count_chunks({}, callback);
    if (calls != 4)
    {
      throw std::runtime_error("missing ChannelUpdate callback");
    }

    std::cout << "ChannelClientInterfaceTest passed\n";
  }
  catch (const std::exception& ex)
  {
    std::cerr << ex.what() << '\n';
    return 1;
  }
}
