#!/usr/bin/env python3.8

import os
import sys

from google.protobuf.compiler import plugin_pb2


def cpp_type(type_name):
  return type_name.lstrip(".").replace(".", "::")


def cpp_namespace_from_param(parameter):
  for item in parameter.split(","):
    if item.startswith("namespace="):
      return item.split("=", 1)[1].replace(".", "::")
  return ""


def namespace_open(namespace):
  return "namespace {}\n{{\n".format(namespace) if namespace else ""


def namespace_close(namespace):
  return "}\n" if namespace else ""


def callback_name(method_name):
  return "".join(part[:1].upper() + part[1:] for part in method_name.split("_")) + "Callback"


def method_cpp_name(method_name):
  return "".join(part[:1].upper() + part[1:] for part in method_name.split("_"))


def result_name(method_name):
  return method_cpp_name(method_name) + "Result"


def awaiter_name(method_name):
  return method_cpp_name(method_name) + "Awaiter"


def proto_include(proto_name):
  return proto_name[:-len(".proto")] + ".grpc.pb.h"


def service_full_name(package, service_name):
  return "{}.{}".format(package, service_name) if package else service_name


def unary_methods(service):
  return [
    method for method in service.method
    if not method.client_streaming and not method.server_streaming
  ]


def batch_transport_method(file_desc):
  for service in file_desc.service:
    for method in service.method:
      if method.client_streaming and method.server_streaming:
        return "/{}/{}".format(
          service_full_name(file_desc.package, service.name),
          method.name)
  return ""


def generate_hpp(file_desc, namespace):
  lines = [
    "#pragma once",
    "",
      "#include <coroutine>",
    "#include <exception>",
    "#include <functional>",
    "#include <memory>",
    "#include <stdexcept>",
    "#include <string>",
    "",
    "#include <grpcpp/support/status.h>",
    "",
    "#include <Commons/ExecutorPool.hpp>",
    "#include <Commons/Grpc/AsyncBatchingClientBase.hpp>",
    "#include <Commons/Grpc/GrpcClient.hpp>",
    "#include <Commons/Grpc/GrpcExecutor.hpp>",
    "#include <Commons/Grpc/ResponseHolder.hpp>",
    "#include <{}>".format(proto_include(file_desc.name)),
    "",
  ]
  lines.append(namespace_open(namespace).rstrip())

  for service in file_desc.service:
    service_name = service.name
    async_client = "{}AsyncClient".format(service_name)
    batching_client = "{}AsyncBatchingClient".format(service_name)
    methods = unary_methods(service)
    if not methods:
      continue

    lines.extend([
      "  class {} :".format(async_client),
      "    public virtual AdServer::Grpc::Client",
      "  {",
      "  public:",
      "    using Stats = AdServer::Grpc::Stats;",
      "",
    ])
    for method in methods:
      response_type = cpp_type(method.output_type)
      lines.extend([
        "    using {} = std::function<void(".format(callback_name(method.name)),
        "      const grpc::Status&,",
        "      AdServer::Grpc::ResponseHolder<{}>&&)>;".format(response_type),
        "",
      ])
    lines.extend([
      "    virtual ~{}() = default;".format(async_client),
      "",
    ])
    for method in methods:
      lines.extend([
        "    virtual void {}(".format(method.name),
        "      const {}& request,".format(cpp_type(method.input_type)),
        "      {} callback) = 0;".format(callback_name(method.name)),
        "",
      ])
    if methods:
      lines.pop()
    lines.extend([
      "  };",
      "",
    ])

    coro_client = "{}CoroClient".format(service_name)
    lines.extend([
      "  class {} final".format(coro_client),
      "  {",
      "  public:",
      "    {}(".format(coro_client),
      "      std::shared_ptr<{}> client,".format(async_client),
      "      std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool);",
      "",
    ])
    for method in methods:
      response_type = cpp_type(method.output_type)
      request_type = cpp_type(method.input_type)
      res_name = result_name(method.name)
      aw_name = awaiter_name(method.name)
      lines.extend([
        "    struct {}".format(res_name),
        "    {",
        "      grpc::Status status;",
        "      AdServer::Grpc::ResponseHolder<{}> response_holder;".format(response_type),
        "      const {}& response;".format(response_type),
        "    };",
        "",
        "    class {}".format(aw_name),
        "    {",
        "    public:",
        "      {}(".format(aw_name),
        "        std::shared_ptr<{}> client,".format(async_client),
        "        std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,",
        "        const {}& request);".format(request_type),
        "",
        "      bool",
        "      await_ready() const noexcept;",
        "",
      "      void",
      "      await_suspend(std::coroutine_handle<> handle) noexcept;",
        "",
        "      {}".format(res_name),
        "      await_resume();",
        "",
        "    private:",
        "      struct State",
        "      {",
        "        grpc::Status status;",
        "        AdServer::Grpc::ResponseHolder<{}> response_holder;".format(response_type),
        "        std::exception_ptr exception;",
      "      };",
        "",
        "      std::shared_ptr<{}> client_;".format(async_client),
        "      std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool_;",
        "      const {}& request_;".format(request_type),
        "      std::shared_ptr<State> state_;",
        "    };",
        "",
        "    {}".format(aw_name),
        "    {}(const {}& request);".format(method.name, request_type),
        "",
      ])
    if methods:
      lines.pop()
    lines.extend([
      "",
      "  private:",
      "    std::shared_ptr<{}> client_;".format(async_client),
      "    std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool_;",
      "  };",
      "",
      "  class {} final".format(batching_client),
      "    : public AdServer::Grpc::AsyncBatchingClientBase,",
      "      public {}".format(async_client),
      "  {",
      "  public:",
    ])
    for method in methods:
      cb = callback_name(method.name)
      lines.append("    using {} = {}::{};".format(cb, async_client, cb))
    lines.extend([
      "",
      "    ~{}() override;".format(batching_client),
      "",
      "    {}(".format(batching_client),
      "      const std::string& endpoint,",
      "      std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,",
      "      std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>",
      "        coalesce_runner,",
      "      AdServer::Grpc::BatchingOptions options = {});",
      "",
    ])
    for method in methods:
      lines.extend([
        "    void {}(".format(method.name),
        "      const {}& request,".format(cpp_type(method.input_type)),
        "      {} callback) override;".format(callback_name(method.name)),
        "",
      ])
    if methods:
      lines.pop()
    lines.extend([
      "  };",
      "",
    ])

  if file_desc.service:
    lines.pop()
  lines.append(namespace_close(namespace).rstrip())
  lines.append("")
  return "\n".join(lines)


def generate_cpp(file_desc, namespace):
  header_name = os.path.basename(file_desc.name[:-len(".proto")] + ".grpc-client.hpp")
  batch_method = batch_transport_method(file_desc)
  lines = [
    '#include "{}"'.format(header_name),
    "",
    "#include <utility>",
    "",
    "namespace",
    "{",
  ]
  for service in file_desc.service:
    full_service = service_full_name(file_desc.package, service.name)
    for method in unary_methods(service):
      lines.extend([
        "  constexpr const char* {}_full_method =".format(method.name),
        '    "/{}/{}";'.format(full_service, method.name),
        "",
      ])
  if lines[-1] == "":
    lines.pop()
  lines.extend([
    "}",
    "",
  ])
  lines.append(namespace_open(namespace).rstrip())

  for service in file_desc.service:
    if not unary_methods(service):
      continue

    batching_client = "{}AsyncBatchingClient".format(service.name)
    async_client = "{}AsyncClient".format(service.name)
    coro_client = "{}CoroClient".format(service.name)
    lines.extend([
      "  {}::{}(".format(coro_client, coro_client),
      "    std::shared_ptr<{}> client,".format(async_client),
      "    std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool)",
      "    : client_(std::move(client)),",
      "      executor_pool_(std::move(executor_pool))",
      "  {}",
      "",
      "  {}::~{}() = default;".format(batching_client, batching_client),
      "",
      "  {}::{}(".format(batching_client, batching_client),
      "    const std::string& endpoint,",
      "    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,",
      "    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>",
      "      coalesce_runner,",
      "    AdServer::Grpc::BatchingOptions options)",
      "    : AdServer::Grpc::AsyncBatchingClientBase(",
      "        endpoint,",
      "        std::move(grpc_executor),",
      "        std::move(coalesce_runner),",
      "        [] (AdServer::Grpc::BatchingOptions options) {",
    ])
    if batch_method:
      lines.extend([
      "          if (options.batch_stream_full_method.empty())",
      "          {",
      '            options.batch_stream_full_method = "{}";'.format(batch_method),
      "          }",
      ])
    lines.extend([
      "          return options;",
      "        }(std::move(options)))",
      "  {}",
      "",
    ])

    for method in unary_methods(service):
      response_type = cpp_type(method.output_type)
      request_type = cpp_type(method.input_type)
      res_name = result_name(method.name)
      aw_name = awaiter_name(method.name)
      cb = callback_name(method.name)
      default_response_error = "{} response is not initialized".format(method.name)
      lines.extend([
        "  {}::{}::{}(".format(coro_client, aw_name, aw_name),
        "    std::shared_ptr<{}> client,".format(async_client),
        "    std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,",
        "    const {}& request)".format(request_type),
        "    : client_(std::move(client)),",
        "      executor_pool_(std::move(executor_pool)),",
        "      request_(request),",
        "      state_(std::make_shared<State>())",
        "  {}",
        "",
        "  bool",
        "  {}::{}::await_ready() const noexcept".format(coro_client, aw_name),
        "  {",
        "    return false;",
        "  }",
        "",
      "  void",
      "  {}::{}::await_suspend(std::coroutine_handle<> handle) noexcept".format(coro_client, aw_name),
      "  {",
      "    auto state = state_;",
      "    auto client = client_;",
      "    auto executor_pool = executor_pool_;",
      "    try",
      "    {",
      "      auto callback_state = state;",
      "      auto callback_executor_pool = executor_pool;",
      "      client->{}(".format(method.name),
      "        request_,",
      "        [state = std::move(callback_state),",
      "         executor_pool = std::move(callback_executor_pool),",
        "         handle](",
        "          const grpc::Status& status,",
        "          AdServer::Grpc::ResponseHolder<{}>&& response_holder)".format(response_type),
        "        {",
        "          state->status = status;",
        "          state->response_holder = std::move(response_holder);",
        "          executor_pool->post([handle]() mutable { handle.resume(); });",
        "        });",
      "    }",
      "    catch(...)",
      "    {",
      "      state->exception = std::current_exception();",
      "      executor_pool->post([handle]() mutable { handle.resume(); });",
      "    }",
      "  }",
        "",
        "  {}::{}".format(coro_client, res_name),
        "  {}::{}::await_resume()".format(coro_client, aw_name),
        "  {",
        "    if(state_->exception)",
        "    {",
        "      std::rethrow_exception(state_->exception);",
        "    }",
        "",
        "    if(!state_->response_holder)",
        "    {",
        "      if(state_->status.ok())",
        "      {",
        '      throw std::logic_error("{}");'.format(default_response_error),
        "      }",
        "      state_->response_holder =",
        "        AdServer::Grpc::ResponseHolder<{}>::make_value({}());".format(
          response_type, response_type),
        "    }",
        "",
        "    const auto& response = state_->response_holder.get();",
        "    return {",
        "      std::move(state_->status),",
        "      std::move(state_->response_holder),",
        "      response};",
        "  }",
        "",
        "  {}::{}".format(coro_client, aw_name),
        "  {}::{}(const {}& request)".format(coro_client, method.name, request_type),
        "  {",
        "    return {}(client_, executor_pool_, request);".format(aw_name),
        "  }",
        "",
      ])
      lines.extend([
        "  void",
        "  {}::{}(".format(batching_client, method.name),
        "    const {}& request,".format(request_type),
        "    {} callback)".format(cb),
        "  {",
        "    enqueue_request_<{}, {}>(".format(request_type, response_type),
        "      {}_full_method,".format(method.name),
        "      request,",
        "      std::move(callback),",
        '      "Unable to parse {} response");'.format(method.name),
        "  }",
        "",
      ])

  if lines[-1] == "":
    lines.pop()
  lines.append(namespace_close(namespace).rstrip())
  lines.append("")
  return "\n".join(lines)


def main():
  request = plugin_pb2.CodeGeneratorRequest()
  request.ParseFromString(sys.stdin.buffer.read())

  namespace = cpp_namespace_from_param(request.parameter)
  files_to_generate = set(request.file_to_generate)
  response = plugin_pb2.CodeGeneratorResponse()
  response.supported_features = (
    plugin_pb2.CodeGeneratorResponse.FEATURE_PROTO3_OPTIONAL)

  for file_desc in request.proto_file:
    if file_desc.name not in files_to_generate:
      continue
    base = os.path.basename(file_desc.name[:-len(".proto")])
    outputs = [
      ("{}.grpc-client.hpp".format(base), generate_hpp(file_desc, namespace)),
      ("{}.grpc-client.cpp".format(base), generate_cpp(file_desc, namespace)),
    ]
    for name, content in outputs:
      output = response.file.add()
      output.name = name
      output.content = content

  sys.stdout.buffer.write(response.SerializeToString())


if __name__ == "__main__":
  main()
