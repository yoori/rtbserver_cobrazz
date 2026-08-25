#pragma once

#include <eh/Exception.hpp>
#include <Generics/Singleton.hpp>

#include <ExpressionMatcherGrpc.grpc-client.hpp>

class Application_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  using Client = AdServer::RequestInfoSvcs::
    ExpressionMatcherGrpcAsyncClient;

  Application_() noexcept;
  virtual ~Application_() noexcept;

  void main(int& argc, char** argv) noexcept;

protected:
  void print(Client& expression_matcher, const char* user_id) noexcept;

  void print_user_trigger_match(
    Client& expression_matcher,
    const char* user_id,
    bool temporary) noexcept;

  void print_request_trigger_match(
    Client& expression_matcher,
    const char* request_id) noexcept;

  void print_household_colo_reach(
    Client& expression_matcher,
    const char* user_id) noexcept;
};

using Application = Generics::Singleton<Application_>;
