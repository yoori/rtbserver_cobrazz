#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <eh/Exception.hpp>

namespace AdServer::Commons
{
  template<typename StringType = std::string>
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
        StringType&& value,
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

    class ProcessorSet
    {
    public:
      ProcessorSet() = default;

      ProcessorSet(const ProcessorSet&) = delete;
      ProcessorSet& operator=(const ProcessorSet&) = delete;
      ProcessorSet(ProcessorSet&&) = default;
      ProcessorSet& operator=(ProcessorSet&&) = default;

      void
      add_processor(
        std::string_view path,
        std::shared_ptr<ValueProcessor> processor,
        bool as_string = false);

    private:
      friend class FastJsonParser;

      struct Entry
      {
        std::string path;
        std::shared_ptr<ValueProcessor> processor;
        bool as_string = false;
      };

      std::vector<Entry> entries_;
    };

    explicit
    FastJsonParser(bool strict = true);

    FastJsonParser(ProcessorSet&& processors, bool strict = true);

    ~FastJsonParser() noexcept;

    FastJsonParser(const FastJsonParser&) = delete;
    FastJsonParser& operator=(const FastJsonParser&) = delete;
    FastJsonParser(FastJsonParser&&) = delete;
    FastJsonParser& operator=(FastJsonParser&&) = delete;

    void
    parse(std::string_view json, void* context) const;

    template<typename StringCreatorType>
    void
    parse(
      std::string_view json,
      void* context,
      StringCreatorType&& string_creator) const;

  private:
    struct Impl;
    using StringCreator = StringType (*)(void*);

    template<typename StringCreatorType>
    static StringType
    create_string_(void* string_creator);

    void
    parse_(
      std::string_view json,
      void* context,
      StringCreator string_creator,
      void* string_creator_context) const;

    std::unique_ptr<Impl> impl_;
  };

  template<typename StringType>
  template<typename StringCreatorType>
  void
  FastJsonParser<StringType>::parse(
    std::string_view json,
    void* context,
    StringCreatorType&& string_creator) const
  {
    using Creator = std::remove_reference_t<StringCreatorType>;
    parse_(
      json,
      context,
      &create_string_<Creator>,
      &string_creator);
  }

  template<typename StringType>
  template<typename StringCreatorType>
  StringType
  FastJsonParser<StringType>::create_string_(void* string_creator)
  {
    return (*static_cast<StringCreatorType*>(string_creator))();
  }
}
