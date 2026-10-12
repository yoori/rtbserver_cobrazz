#include <chrono>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <grpcpp/grpcpp.h>
#include <Commons/Grpc/Batch.grpc.pb.h>
#include <RequestInfoSvcs/UrlInfoStorage/UrlInfoStorage.grpc.pb.h>

namespace
{
  namespace Proto = adserver::request_info_svcs::url_info_storage;
  using namespace std::chrono_literals;

  void require(bool condition, const std::string& message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  std::string get(Proto::UrlInfoStorage::Stub& stub, const std::string& url)
  {
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + 10s);
    Proto::GetUrlProfileRequest request;
    request.set_url(url);
    Proto::GetUrlProfileResponse response;
    const auto status = stub.get_url_profile(&context, request, &response);
    require(status.ok(), "get_url_profile: " + status.error_message());
    return response.text();
  }

  void set(Proto::UrlInfoStorage::Stub& stub, const std::string& url, const std::string& text)
  {
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + 10s);
    Proto::SetUrlProfileRequest request;
    request.set_url(url);
    request.set_text(text);
    Proto::SetUrlProfileResponse response;
    const auto status = stub.set_url_profile(&context, request, &response);
    require(status.ok(), "set_url_profile: " + status.error_message());
  }

  void test_batch(const std::shared_ptr<grpc::Channel>& channel)
  {
    auto transport = adserver::grpc::BatchTransport::NewStub(channel);
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + 20s);
    auto stream = transport->stream_batches(&context);
    const std::string prefix = std::string("/") + Proto::UrlInfoStorage::service_full_name() + "/";

    // Items can execute concurrently; wait for each write batch before submitting its reads.
    for (unsigned int batch_id = 1; batch_id <= 4; ++batch_id)
    {
      const bool writing = batch_id % 2;
      const std::string text = "batch " + std::to_string((batch_id + 1) / 2);
      adserver::grpc::BatchRequest batch;
      batch.set_batch_id(batch_id);
      for (unsigned int i = 0; i < 160; ++i)
      {
        auto* item = batch.add_items();
        item->set_request_id(i);
        const std::string url = "https://batch.test/" + std::to_string(i);
        if (writing)
        {
          Proto::SetUrlProfileRequest write;
          write.set_url(url);
          write.set_text(text);
          item->set_full_method(prefix + "set_url_profile");
          item->set_payload(write.SerializeAsString());
        }
        else
        {
          Proto::GetUrlProfileRequest read;
          read.set_url(url);
          item->set_full_method(prefix + "get_url_profile");
          item->set_payload(read.SerializeAsString());
        }
      }

      auto* invalid = batch.add_items();
      invalid->set_request_id(160);
      invalid->set_full_method(prefix + "get_url_profile");
      auto* unknown = batch.add_items();
      unknown->set_request_id(161);
      unknown->set_full_method(prefix + "unknown");
      require(stream->Write(batch), "batch write failed");
      std::map<unsigned long, adserver::grpc::BatchResponseItem> responses;
      adserver::grpc::BatchResponse response;
      while (responses.size() < 162 && stream->Read(&response))
      {
        require(response.batch_id() == batch_id, "batch_id mismatch");
        for (const auto& item : response.items())
        {
          require(responses.emplace(item.request_id(), item).second, "duplicate response");
        }
      }

      require(responses.size() == 162, "missing batch responses");
      for (unsigned int i = 0; i < 160; ++i)
      {
        require(responses.at(i).status_code() == 0, "batch item failed");
        if (!writing)
        {
          Proto::GetUrlProfileResponse result;
          require(result.ParseFromString(responses.at(i).payload()), "invalid response payload");
          require(result.text() == text, "batch profile mismatch");
        }
      }

      require(responses.at(160).status_code() == grpc::StatusCode::INVALID_ARGUMENT,
        "empty URL must be rejected");
      require(responses.at(161).status_code() == grpc::StatusCode::UNIMPLEMENTED,
        "unknown method must be rejected");
    }

    stream->WritesDone();
    adserver::grpc::BatchResponse response;
    require(!stream->Read(&response), "unexpected extra batch response");
    require(stream->Finish().ok(), "batch stream failed");
  }
}

int main(int argc, char** argv)
{
  try
  {
    require(argc == 3, "usage: UrlInfoStorageGrpcTest endpoint test|persisted");
    grpc::ChannelArguments arguments;
    arguments.SetInt(GRPC_ARG_ENABLE_HTTP_PROXY, 0);
    auto channel =
      grpc::CreateCustomChannel(argv[1], grpc::InsecureChannelCredentials(), arguments);
    auto stub = Proto::UrlInfoStorage::NewStub(channel);
    if (std::string(argv[2]) == "persisted")
    {
      require(get(*stub, "https://http.test/profile") == "HTTP: Привет\n\"text\"",
        "HTTP profile is not visible through gRPC");
      require(get(*stub, "https://batch.test/159") == "batch 2", "batch profile not persisted");
    }
    else
    {
      require(get(*stub, "https://missing.test/").empty(), "missing profile must be empty");
      const std::string text = "Привет\n\"text\"\\";
      set(*stub, "https://grpc.test/profile", text);
      require(get(*stub, "https://grpc.test/profile") == text, "unary round trip failed");
      set(*stub, "https://grpc.test/empty", "nonempty");
      set(*stub, "https://grpc.test/empty", "");
      require(get(*stub, "https://grpc.test/empty").empty(), "empty overwrite failed");
      test_batch(channel);
    }

    std::cout << "UrlInfoStorage gRPC checks passed\n";
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
