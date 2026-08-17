#pragma once

#include <memory>

#include <eh/Exception.hpp>
#include <Generics/Singleton.hpp>

#include <ExpressionMatcherGrpc.grpc.pb.h>

class Application_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  using Stub = adserver::request_info_svcs::expression_matcher::
    ExpressionMatcherGrpc::StubInterface;

  Application_() noexcept;
  virtual ~Application_() noexcept;

  void main(int& argc, char** argv) noexcept;

protected:
  void print(Stub& expression_matcher, const char* user_id) noexcept;

  void print_user_trigger_match(
    Stub& expression_matcher,
    const char* user_id,
    bool temporary) noexcept;

  void print_request_trigger_match(
    Stub& expression_matcher,
    const char* request_id) noexcept;

  void print_household_colo_reach(
    Stub& expression_matcher,
    const char* user_id) noexcept;
};

using Application = Generics::Singleton<Application_>;
