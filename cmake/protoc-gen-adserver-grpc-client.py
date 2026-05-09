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
    "#include <functional>",
    "#include <memory>",
    "#include <string>",
    "",
    "#include <grpcpp/support/status.h>",
    "",
    "#include <Commons/Grpc/AsyncBatchingClientBase.hpp>",
    "#include <Commons/Grpc/GrpcClient.hpp>",
    "#include <Commons/Grpc/GrpcExecutor.hpp>",
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
      lines.extend([
        "    using {} = std::function<void(".format(callback_name(method.name)),
        "      const grpc::Status&,",
        "      const {}&)>;".format(cpp_type(method.output_type)),
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
    lines.extend([
      "  {}::~{}() = default;".format(batching_client, batching_client),
      "",
      "  {}::{}(".format(batching_client, batching_client),
      "    const std::string& endpoint,",
      "    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,",
      "    AdServer::Grpc::BatchingOptions options)",
      "    : AdServer::Grpc::AsyncBatchingClientBase(",
      "        endpoint,",
      "        std::move(grpc_executor),",
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
      lines.extend([
        "  void",
        "  {}::{}(".format(batching_client, method.name),
        "    const {}& request,".format(request_type),
        "    {} callback)".format(callback_name(method.name)),
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
