#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <eh/Exception.hpp>

namespace AdServer::Commons
{
  class FastJsonParser
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(ParseError, Exception);
    DECLARE_EXCEPTION(UnexpectedType, Exception);

    struct ValueProcessor
    {
      virtual
      ~ValueProcessor() noexcept = default;

      virtual void
      object_started(std::string_view path, void* context) const;

      virtual void
      array_started(std::string_view path, void* context) const;

      virtual void
      process_integer(
        int64_t value,
        std::string_view path,
        void* context) const;

      virtual void
      process_float(
        double value,
        std::string_view path,
        void* context) const;

      virtual void
      process_number(
        std::string_view value,
        bool is_float,
        std::string_view path,
        void* context) const;

      virtual void
      process_string(
        std::string_view value,
        std::string_view path,
        void* context) const;

      virtual void
      process_string(
        std::string&& value,
        std::string_view path,
        void* context) const;

      virtual void
      process_bool(
        bool value,
        std::string_view path,
        void* context) const;

      virtual void
      process_null(std::string_view path, void* context) const;
    };

    explicit
    FastJsonParser(bool strict = true);

    ~FastJsonParser() noexcept;

    FastJsonParser(const FastJsonParser&) = delete;
    FastJsonParser& operator=(const FastJsonParser&) = delete;
    FastJsonParser(FastJsonParser&&) = delete;
    FastJsonParser& operator=(FastJsonParser&&) = delete;

    void
    add_processor(
      std::string_view path,
      std::shared_ptr<ValueProcessor> processor,
      bool as_string = false);

    void
    parse(std::string_view json, void* context) const;

  private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
  };
}
